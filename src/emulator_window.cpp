#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <SDL2/SDL.h>
#include <QMenu>
#include <algorithm>
#include <QKeyEvent>

#include "file_rw.hpp"
#include "error.hpp"
#include "settings.hpp"

#ifdef _WIN32
#include <dwmapi.h>
#endif

#include "emulator_window.hpp"
#include "../defaultrom/defaultrom.hpp"
#include "pixel_buffer_view.hpp"

using namespace SuperShuckie64;

static std::vector<std::byte> rom_from_path(const std::optional<std::filesystem::path> &path) noexcept {
    if(!path) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte *>(AUTOGEN_DEFAULTROM_HPP_VAL),
            reinterpret_cast<const std::byte *>(AUTOGEN_DEFAULTROM_HPP_VAL) + sizeof(AUTOGEN_DEFAULTROM_HPP_VAL)
        );
    }

    auto buf = read_file(*path);
    if(!buf) {
        return {};
    }

    return *buf;
}

EmulatorWindow::EmulatorWindow(const std::optional<std::filesystem::path> &default_rom) {
    this->udp_command_server = UDPCommandHandler::try_new();
    if(!this->udp_command_server.get()) {
        DISPLAY_ERROR_DIALOG("Failed to start UDP server", "Can't start a UDP commands server. Another application (RetroArch, another SuperShuckie instance, etc.) might be using port 55355 at the moment.\n\nRestart the emulator and try again if you need this!");
    }

    // If the ROM failed to open, do not continue!
    if(!this->load_rom(default_rom)) {
        return;
    }

    this->reload_speed_settings();
    this->set_up_menu();
    this->pixel_buffer_view = new PixelBufferView(this, *this);
    this->setCentralWidget(this->pixel_buffer_view);
    this->pixel_buffer_view->setFrameStyle(0);
    this->pixel_buffer_view->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    this->pixel_buffer_view->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    this->pixel_buffer_view->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);

    this->refresh_scale();
    this->ticker.setInterval(1);
    this->ticker.callOnTimeout(this, &EmulatorWindow::tick);
    this->ticker.start();

    // Remove rounded corners (Windows)
    #ifdef _WIN32
    DWORD one = 1;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(this->winId()), 33, &one, sizeof(one));
    #endif

    this->valid = true;
    this->add_keyboard();
    this->handle_loaded_rom();

    this->setGeometry(this->window_position());
}

bool EmulatorWindow::load_and_start_rom(const std::optional<std::filesystem::path> &path) {
    auto loaded = this->load_rom(path);
    if(loaded) {
        this->handle_loaded_rom();
    }
    return loaded;
}

bool EmulatorWindow::load_rom(const std::optional<std::filesystem::path> &path) {
    auto rom = rom_from_path(path);
    if(rom.empty()) {
        return false;
    }

    if(path != std::nullopt) {
        auto new_rom_name = path->filename().string();
        bool error;
        init_rom_user_data_path(new_rom_name.c_str(), error);
        if(error) {
            return false;
        }
        this->current_rom = path;
        this->current_rom_name = new_rom_name;
    }
    else {
        this->current_rom_name = std::string();
        this->current_rom = std::nullopt;
    }

    this->current_rom_data = std::move(rom);
    this->current_save_name = "default";
    this->displayed_save_name = std::string();
    this->reload_current_rom_data();
    return true;
}

void EmulatorWindow::reload_current_rom_data() {
    this->gameboy = std::make_unique<GameboyContext>(GB_model_t::GB_MODEL_CGB_B, this->current_rom_data, this->udp_command_server);
    this->currently_playing_back_recording = false;
    this->currently_recording = false;
    this->gameboy->set_ignore_speed_changes_on_replay(this->ignore_replay_speed_changes_setting());
    this->gameboy->set_loop_playback(this->loop_playback_setting());

    if(this->current_rom) {
        auto sram = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_SaveData, this->current_save_name.c_str());
        if(std::filesystem::exists(sram)) {
            auto sram_data = read_file(sram);
            if(sram_data) {
                this->gameboy->load_sram(*sram_data);
            }
        }
        this->update_save_name_in_title_bar();
        this->update_gameboy_speed();
    }
}

void EmulatorWindow::update_save_name_in_title_bar() {
    if(this->current_save_name == "default" || this->current_save_name == RESERVED_REPLAY_PLAYBACK_SAVE_NAME) {
        this->displayed_save_name = std::string();
    }
    else {
        this->displayed_save_name = std::string(" (") + this->current_save_name + ")";
    }
}

void EmulatorWindow::update_gameboy_speed() {
    if(this->currently_recording && this->ignore_recording_speed_changes_option->isChecked()) {
        return;
    }

    double contribution_from_turbo = 1.0 + (this->turbo_speed - 1.0) * this->input_state.turbo;
    double contribution_from_slow = 1.0 + (this->slow_speed - 1.0) * this->input_state.slow;
    double speed = this->base_speed * contribution_from_turbo * contribution_from_slow;
    auto total = static_cast<std::uint16_t>(std::clamp(speed * static_cast<double>(SPEED_MULTIPLIER_FACTOR), 0.0, 65535.0));
    this->gameboy->set_speed(total);
}

void EmulatorWindow::refresh_scale() {
    auto scale = this->scaling_setting();
    scale = std::max(scale, static_cast<decltype(scale)>(1));

    this->setMinimumSize(0,0);
    this->setMaximumSize(65535,65535);

    this->gameboy->get_frame_resolution(this->width, this->height);
    this->pixel_buffer.resize(width * height, 0x12345678);
    this->pixel_buffer_view->setFixedWidth(this->width * scale);
    this->pixel_buffer_view->setFixedHeight(this->height * scale);
    this->pixel_buffer_view->setTransform(QTransform::fromScale(scale, scale));
    this->setFixedSize(this->sizeHint());

    // Update the pixel buffer size
    this->pixel_buffer_pixmap = {};
    auto *new_scene = new QGraphicsScene(this->pixel_buffer_view);
    auto *new_pixmap = new_scene->addPixmap(this->pixel_buffer_pixmap);
    if(this->pixel_buffer_scene) {
        delete this->pixel_buffer_pixmap_item;
        auto items = this->pixel_buffer_scene->items();
        for(auto &i : items) {
            new_scene->addItem(i);
        }
    }
    this->pixel_buffer_pixmap_item = new_pixmap;
    this->pixel_buffer_scene = new_scene;
    this->pixel_buffer_view->setScene(this->pixel_buffer_scene);
}

void EmulatorWindow::tick() {
    this->gameboy->copy_frame_buffer(this->pixel_buffer.data());
    this->pixel_buffer_pixmap.convertFromImage(QImage(reinterpret_cast<const uchar *>(this->pixel_buffer.data()), this->width, this->height, QImage::Format::Format_ARGB32));
    this->pixel_buffer_pixmap_item->setPixmap(this->pixel_buffer_pixmap);

    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            // If we hit ctrl-c, close the window (saves)
            case SDL_EventType::SDL_QUIT:
                this->close();

                // If the window wasn't closed, warn
                if(this->isVisible()) {
                    std::fputs("Can't close the main window. Finish what you're doing, first!\n", stderr);
                }
                break;

            // Controller hotplugging
            case SDL_EventType::SDL_CONTROLLERDEVICEADDED:
                this->add_device(event.cdevice);
                break;

            case SDL_EventType::SDL_CONTROLLERDEVICEREMOVED:
                this->remove_device(event.cdevice);
                break;

            // Input handling
            case SDL_EventType::SDL_CONTROLLERAXISMOTION:
                this->register_axis(event.caxis);
                break;
            case SDL_EventType::SDL_CONTROLLERBUTTONDOWN:
                this->register_button(event.cbutton, true);
                break;
            case SDL_EventType::SDL_CONTROLLERBUTTONUP:
                this->register_button(event.cbutton, false);
                break;

            // Ignored
            case SDL_EventType::SDL_JOYBATTERYUPDATED:
            case SDL_EventType::SDL_JOYDEVICEADDED:
            case SDL_EventType::SDL_JOYDEVICEREMOVED:
            case SDL_EventType::SDL_JOYAXISMOTION:
            case SDL_EventType::SDL_JOYBALLMOTION:
            case SDL_EventType::SDL_JOYHATMOTION:
            case SDL_EventType::SDL_JOYBUTTONDOWN:
            case SDL_EventType::SDL_JOYBUTTONUP:
                break;

            default:
                std::fprintf(stderr, "Unhandled SDL event: %d\n", event.type);
                break;
        }
    }

    // If we're no longer playing back, we should note this to the user
    if(this->currently_playing_back_recording && !this->gameboy->is_playing_back()) {
        this->currently_playing_back_recording = false;
        this->set_window_title_element("Playback ended");
    }

    // Do we change the window title back to normal?
    if(this->revert_window_title_timer != std::nullopt && std::chrono::steady_clock::now() > *this->revert_window_title_timer) {
        this->revert_window_title();
    }

    if(this->currently_recording && (this->temporary_file_time_since_last_save + std::chrono::seconds(15)) < std::chrono::steady_clock::now()) {
        this->update_recording_tmp_file();
    }
}

void EmulatorWindow::revert_window_title() noexcept {
    this->revert_window_title_timer = std::nullopt;

    char fmt[1024];
    std::snprintf(fmt, sizeof(fmt), "Super Shuckie 64 (name TBD): %s%s", current_rom == std::nullopt ? "(no ROM loaded)" : current_rom->filename().string().c_str(), this->displayed_save_name.c_str());

    this->setWindowTitle(fmt);

    if(this->currently_recording) {
        this->setWindowTitle(this->windowTitle() + " [RECORDING REPLAY]");
    }

    if(this->currently_playing_back_recording) {
        this->setWindowTitle(this->windowTitle() + " [PLAYING REPLAY]");
    }

    if(this->manually_paused) {
        this->setWindowTitle(this->windowTitle() + " [PAUSED]");
    }
}

void EmulatorWindow::set_window_title_element(const char *what) noexcept {
    this->revert_window_title();
    if(what == nullptr) {
        return;
    }

    // Don't show messages for the first couple of seconds
    if(this->window_alive_time().count() < 2000) {
        return;
    }

    this->revert_window_title_timer = std::optional(std::chrono::steady_clock::now() + std::chrono::seconds(3));
    this->setWindowTitle(this->windowTitle() + " - " + what);
}

void EmulatorWindow::add_device(SDL_ControllerDeviceEvent &event) noexcept {
    SDL_GameController *device = SDL_GameControllerOpen(event.which);
    this->add_device(device);
}

void EmulatorWindow::remove_device(SDL_ControllerDeviceEvent &event) noexcept {
    auto what = this->input_devices.find(event.which);
    if(what != this->input_devices.end()) {
        char message[256];
        std::snprintf(message, sizeof(message), "Disconnected controller %s...", what->second->get_name());
        this->set_window_title_element(message);
        this->input_devices.erase(what);
    }
}

void EmulatorWindow::register_button(SDL_ControllerButtonEvent &event, bool on) noexcept {
    auto what = this->input_devices.find(event.which);
    if(what == this->input_devices.end()) {
        std::fputs("Tried to register a button for a device that isn't registered.\n", stderr);
        return;
    }

    auto &controller = dynamic_cast<ControllerInputDevice &>(*what->second);

    if(!this->suppress_game_input) {
        controller.register_input(this->input_state, event, on);
        this->update_input_state_on_gameboy();
    }

    emit on_device_input(event, controller);
}

void EmulatorWindow::update_input_state_on_gameboy() noexcept {
    if(this->input_state.pause && !this->is_pausing) {
        this->toggle_pause();
    }
    this->is_pausing = this->input_state.pause;

    if(!this->currently_playing_back_recording) {
        if(this->input_state.reset_console && !this->is_resetting) {
            this->perform_reset();
        }
        this->is_resetting = this->input_state.reset_console;

        this->gameboy->set_input(this->input_state.input | this->input_state.input_toggle);
        this->gameboy->set_rapid_fire_input(this->input_state.input_rapid_fire);
    }

    if(!this->currently_playing_back_recording || ignore_replay_speed_changes_option->isChecked()) {
        this->update_gameboy_speed();
    }
}

void EmulatorWindow::register_axis(SDL_ControllerAxisEvent &event) noexcept {
    auto what = this->input_devices.find(event.which);
    if(what == this->input_devices.end()) {
        std::fputs("Tried to register a button for a device that isn't registered.\n", stderr);
        return;
    }

    auto &controller = dynamic_cast<ControllerInputDevice &>(*what->second);

    if(!this->suppress_game_input) {
        controller.register_input(this->input_state, event);
        this->update_input_state_on_gameboy();
    }

    emit on_device_input(event, controller);
}

void EmulatorWindow::handle_loaded_rom() noexcept {
    bool rom_loaded = this->current_rom != std::nullopt;
    this->set_window_title_element(rom_loaded ? "ROM loaded successfully!" : "ROM unloaded successfully!");
    this->gameplay_menu->setEnabled(rom_loaded);
    this->replays_menu->setEnabled(rom_loaded);
    this->save_states_menu->setEnabled(rom_loaded);
    this->update_color_on_gb();
    this->refresh_pause_state();
}

void EmulatorWindow::add_device(SDL_GameController *controller) noexcept {
    if(controller == nullptr) {
        std::fputs("Tried to add a null controller.\n", stderr);
        return;
    }

    auto dev = std::make_shared<ControllerInputDevice>(controller);
    this->load_settings_for_device(*dev);
    this->input_devices[dev->get_joystick_id()] = dev;

    char message[256];
    std::snprintf(message, sizeof(message), "Found controller %s...", dev->get_name());
    this->set_window_title_element(message);
}

void EmulatorWindow::write_settings_for_device(const InputDevice &device) {
    auto settings = get_settings();

    auto button_settings = device.get_name_settings() + "/buttons";
    auto analog_positive_settings = device.get_name_settings() + "/analog-positive";
    auto analog_negative_settings = device.get_name_settings() + "/analog-negative";

    auto button = device.settings_button.serialize();
    auto analog_positive = device.settings_analog_positive.serialize();
    auto analog_negative = device.settings_analog_negative.serialize();

    settings.setValue(button_settings,          QByteArray(reinterpret_cast<char *>(button.data()), button.size()));
    settings.setValue(analog_positive_settings, QByteArray(reinterpret_cast<char *>(analog_positive.data()), analog_positive.size()));
    settings.setValue(analog_negative_settings, QByteArray(reinterpret_cast<char *>(analog_negative.data()), analog_negative.size()));

    auto *controller = dynamic_cast<const ControllerInputDevice *>(&device);
    if(controller) {
        settings.setValue(device.get_name_settings() + "/lower-deadzone", controller->lower_dead_zone);
        settings.setValue(device.get_name_settings() + "/upper-deadzone", controller->upper_dead_zone);
        settings.setValue(device.get_name_settings() + "/on-threshold", controller->on_threshold);
    }
}

void EmulatorWindow::load_settings_for_device(InputDevice &device) {
    auto settings = get_settings();

    auto restore_defaults_key = device.get_name_settings() + "/restore-defaults";
    auto should_restore_defaults = settings.value(restore_defaults_key, true).toBool();

    if(should_restore_defaults) {
        settings.setValue(restore_defaults_key, false);
        device.restore_default_settings();
        this->write_settings_for_device(device);
        return;
    }

    auto button_settings = device.get_name_settings() + "/buttons";
    auto analog_positive_settings = device.get_name_settings() + "/analog-positive";
    auto analog_negative_settings = device.get_name_settings() + "/analog-negative";

    auto byte_array_to_uint8array = [](const QByteArray &byte_array) {
        return std::vector<std::uint8_t>(reinterpret_cast<const std::uint8_t *>(byte_array.data()), reinterpret_cast<const std::uint8_t *>(byte_array.data() + byte_array.size()));
    };

    device.settings_button = Settings::deserialize(byte_array_to_uint8array(settings.value(button_settings).toByteArray()));
    device.settings_analog_positive = Settings::deserialize(byte_array_to_uint8array(settings.value(analog_positive_settings).toByteArray()));
    device.settings_analog_negative = Settings::deserialize(byte_array_to_uint8array(settings.value(analog_negative_settings).toByteArray()));

    auto *controller = dynamic_cast<ControllerInputDevice *>(&device);
    if(controller) {
        controller->lower_dead_zone = settings.value(device.get_name_settings() + "/lower-deadzone", controller->lower_dead_zone).toDouble();
        controller->upper_dead_zone = settings.value(device.get_name_settings() + "/upper-deadzone", controller->upper_dead_zone).toDouble();
        controller->on_threshold = settings.value(device.get_name_settings() + "/on-threshold", controller->on_threshold).toDouble();
    }

}

bool EmulatorWindow::save_and_close() {
    if(!this->save_sram()) {
        QMessageBox msg;
        msg.setWindowTitle("Save failed");
        msg.setText("Couldn't save your save data to disk. Do you want to close it anyway?");
        msg.setStandardButtons(QMessageBox::Close | QMessageBox::Cancel);
        msg.setDefaultButton(QMessageBox::Cancel);
        msg.setIcon(QMessageBox::Question);
        return msg.exec() == QMessageBox::Close;
    }
    return true;
}

void EmulatorWindow::closeEvent(QCloseEvent *event) {
    if(!this->save_and_close()) {
        return;
    }
    this->window_position(this->geometry());
    QMainWindow::closeEvent(event);
}

int EmulatorWindow::scaling_setting(int new_setting) {
    auto setting = get_settings();
    if(new_setting) {
        setting.setValue("window/scale", new_setting);
    }

    return setting.value("window/scale", 6).toInt();
}

bool EmulatorWindow::ignore_replay_speed_changes_setting(int new_setting) {
    auto setting = get_settings();
    if(new_setting == 0 || new_setting == 1) {
        setting.setValue("replay/ignore_speed_changes", static_cast<bool>(new_setting));
    }
    return setting.value("replay/ignore_speed_changes", 0).toBool();
}

bool EmulatorWindow::ignore_recording_speed_changes_setting(int new_setting) {
    auto setting = get_settings();
    if(new_setting == 0 || new_setting == 1) {
        setting.setValue("replay/disable_speed_changes_when_recording", static_cast<bool>(new_setting));
    }
    return setting.value("replay/disable_speed_changes_when_recording", 0).toBool();
}

QRect EmulatorWindow::window_position(std::optional<QRect> new_position) {
    auto setting = get_settings();
    if(new_position) {
        setting.setValue("window/position", *new_position);
    }
    return setting.value("window/position", this->geometry()).toRect();
}

bool EmulatorWindow::loop_playback_setting(int new_setting) {
    auto setting = get_settings();
    if(new_setting == 0 || new_setting == 1) {
        setting.setValue("replay/loop", static_cast<bool>(new_setting));
    }
    return setting.value("replay/loop", 0).toBool();
}

void EmulatorWindow::refresh_pause_state() noexcept {
    this->gameboy->set_paused(this->manually_paused);
}

void EmulatorWindow::toggle_pause() {
    this->manually_pause_option->setChecked(!this->manually_paused);
    this->update_manually_paused();
}

#define KEYBOARD_INDEX (-262626)

void EmulatorWindow::add_keyboard() {
    auto dev = std::make_shared<KeyboardInputDevice>();
    this->load_settings_for_device(*dev);
    this->input_devices[KEYBOARD_INDEX] = dev;
}

void EmulatorWindow::handle_keyboard_event(std::uint8_t key, bool on) {
    if(!this->suppress_game_input) {
        dynamic_cast<KeyboardInputDevice &>(*this->input_devices[KEYBOARD_INDEX]).register_input(this->input_state, key, on);
        this->update_input_state_on_gameboy();
    }
}
