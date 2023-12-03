#include <QMenuBar>
#include <QFileDialog>
#include <QKeyCombination>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QListWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include "error.hpp"
#include "controls_settings_window.hpp"
#include "emulator_window.hpp"
#include "settings.hpp"
#include "speed_settings_window.hpp"
#include "file_rw.hpp"

using namespace SuperShuckie64;

void EmulatorWindow::set_up_menu() {
    QMenuBar *bar = new QMenuBar(this);
    this->setMenuBar(bar);

    // Add base menus
    auto *file_menu = bar->addMenu("File");
    this->gameplay_menu = bar->addMenu("Gameplay");
    this->replays_menu = bar->addMenu("Replays");
    auto *settings_menu = bar->addMenu("Settings");

    #define ADD_ACTION_AND_CONNECT_THEN(text, menu, action, ...) { \
        auto *a = menu->addAction(text); \
        connect(a, SIGNAL(triggered()), this, SLOT(action)); \
        __VA_ARGS__; \
    }
    #define ADD_ACTION_AND_CONNECT_WITH_SHORTCUT_THEN(text, menu, action, shortcut, ...) ADD_ACTION_AND_CONNECT_THEN(text, menu, action, a->setShortcut(shortcut), __VA_ARGS__);
    #define ADD_ACTION_AND_CONNECT_WITH_SHORTCUT(text, menu, action, shortcut) ADD_ACTION_AND_CONNECT_WITH_SHORTCUT_THEN(text, menu, action, shortcut, (void)0)
    #define ADD_ACTION_AND_CONNECT(text, menu, action) ADD_ACTION_AND_CONNECT_THEN(text, menu, action, (void)0)

    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("Open ROM...", file_menu, open_rom_dialog(), QKeyCombination(Qt::ControlModifier, Qt::Key_O));
    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("Save game", file_menu, save_sram(), QKeyCombination(Qt::ControlModifier, Qt::Key_S));
    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("Save game as a new game...", file_menu, save_sram_new(), QKeyCombination(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_S));
    file_menu->addSeparator();
    //ADD_ACTION_AND_CONNECT("Quit without saving", file_menu, close());
    ADD_ACTION_AND_CONNECT("Close ROM", file_menu, close_rom());
    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("Quit", file_menu, close(), QKeyCombination(Qt::ControlModifier, Qt::Key_Q));

    // Video settings
    auto *scaling = settings_menu->addMenu("Video scaling");
    auto scaling_setting = this->scaling_setting();
    for(int i = 1; i <= 16; i++) {
        char str[16];
        std::snprintf(str, sizeof(str), "%dx", i);

        auto *action = scaling->addAction(str);
        action->setCheckable(true);
        action->setChecked(i == scaling_setting);

        action->setData(i);
    }
    connect(scaling, SIGNAL(triggered(QAction *)), this, SLOT(set_scaling_settings(QAction *)));

    settings_menu->addSeparator();
    ADD_ACTION_AND_CONNECT("Speed settings...", settings_menu, open_speed_settings_dialog());

    //ADD_ACTION_AND_CONNECT("Remote command settings...", settings_menu, open_gamehook_settings_dialog());
    settings_menu->addSeparator();
    ADD_ACTION_AND_CONNECT("Controls settings...", settings_menu, open_controls_settings_dialog());
    ADD_ACTION_AND_CONNECT("Reload all controllers", settings_menu, reload_all_controllers());

    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("New game...", this->gameplay_menu, new_game(), QKeyCombination(Qt::ControlModifier, Qt::Key_N));
    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("Load game...", this->gameplay_menu, load_game(), QKeyCombination(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_O));
    this->gameplay_menu->addSeparator();
    ADD_ACTION_AND_CONNECT("Reset console", this->gameplay_menu, perform_reset());
    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT_THEN("Pause", this->gameplay_menu, update_manually_paused(), QKeyCombination(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_P), \
        a->setCheckable(true); \
        a->setChecked(false); \
        this->manually_pause_option = a; \
    );

    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("Start recording replay", this->replays_menu, start_replay_recording(), QKeyCombination(Qt::ControlModifier, Qt::Key_R));
    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("Stop recording", this->replays_menu, stop_replay_recording(), QKeyCombination(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_R));
    ADD_ACTION_AND_CONNECT("Resume recording replay", this->replays_menu, continue_replay_recording());
    ADD_ACTION_AND_CONNECT_THEN("Disable speed changes when recording", this->replays_menu, ignore_recording_speed_changes(), \
        a->setCheckable(true); \
        a->setChecked(this->ignore_recording_speed_changes_setting()); \
        this->ignore_recording_speed_changes_option = a; \
    );

    this->replays_menu->addSeparator();
    ADD_ACTION_AND_CONNECT("Load replay...", this->replays_menu, load_replay());
    ADD_ACTION_AND_CONNECT("Stop replay", this->replays_menu, stop_replay());
    ADD_ACTION_AND_CONNECT_THEN("Ignore speed changes from replay", this->replays_menu, ignore_replay_speed_changes(), \
        a->setCheckable(true); \
        a->setChecked(this->ignore_replay_speed_changes_setting()); \
        this->ignore_replay_speed_changes_option = a; \
    );
    ADD_ACTION_AND_CONNECT_THEN("Loop playback", this->replays_menu, loop_playback(), \
        a->setCheckable(true); \
        a->setChecked(this->loop_playback_setting()); \
        this->loop_playback_option = a; \
    );
    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("Skip forward", this->replays_menu, skip_forward(), QKeyCombination(Qt::ControlModifier, Qt::Key_Period));
    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("Skip backward", this->replays_menu, skip_backward(), QKeyCombination(Qt::ControlModifier, Qt::Key_Comma));
}


void EmulatorWindow::close_rom() {
    this->save_sram();
    this->load_rom(std::nullopt);
    this->handle_loaded_rom();
    this->set_window_title_element("ROM unloaded!");
}

static std::optional<std::string> ask_for_save_game(const char *title) {
    QDialog ask_for_new_game;

    auto *layout = new QVBoxLayout(&ask_for_new_game);
    layout->addWidget(new QLabel("Enter the name for your save:", &ask_for_new_game));

    auto *save_name = new QLineEdit(&ask_for_new_game);
    layout->addWidget(save_name);
    save_name->setMaxLength(100);

    ask_for_new_game.connect(save_name, SIGNAL(returnPressed()), &ask_for_new_game, SLOT(accept()));
    ask_for_new_game.setWindowTitle(title);
    ask_for_new_game.setFixedSize(ask_for_new_game.sizeHint());

    if(ask_for_new_game.exec() != QDialog::Accepted) {
        return std::nullopt;
    }

    return save_name->text().toStdString();
}

void EmulatorWindow::save_sram() {
    if(!this->current_rom) {
        this->set_window_title_element("Can't save - no ROM loaded!");
        return; // no rom loaded
    }

    if(this->current_save_name.empty()) {
        this->set_window_title_element("Can't save - no save loaded!");
        return;
    }

    if(this->current_save_name == RESERVED_REPLAY_PLAYBACK_SAVE_NAME) {
        if(this->gameboy->is_playing_back() || true) {
            return; // no need to save if playing back
        }

        QMessageBox msg;
        msg.setWindowTitle("Save replay SRAM?");
        msg.setText("The replay finished playback, but there may be additional, unsaved changes to the save data, with no game on disk to save it to.\n\nWould you like to save it as a new game?");
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setDefaultButton(QMessageBox::Yes);
        msg.setIcon(QMessageBox::Question);

        if(msg.exec() == QMessageBox::Yes) {
            this->save_sram_new();
        }

        return;
    }

    auto sram = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_SaveData, this->current_save_name.c_str());
    if(write_file(sram, this->gameboy->get_sram())) {
        this->set_window_title_element("Saved SRAM successfully!");
    }
    else {
        this->set_window_title_element("Failed to write SRAM.");
    }
}

void EmulatorWindow::new_game() {
    auto new_game = ask_for_save_game("New game");
    if(!new_game) {
        return;
    }
    if(new_game == RESERVED_REPLAY_PLAYBACK_SAVE_NAME) {
        DISPLAY_ERROR_DIALOG("Invalid game name", RESERVED_REPLAY_PLAYBACK_SAVE_NAME " is reserved and can't be used.");
        return;
    }

    auto &result = *new_game;

    auto sram = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_SaveData, result.c_str());
    bool should_save_sram = true;

    if(std::filesystem::exists(sram)) {
        QMessageBox msg;
        msg.setWindowTitle("Overwrite?");
        char buf[512];
        std::snprintf(buf, sizeof(buf), "%s already exists. Would you like to load it instead, or reset the save file?\n\nWarning: Resetting a save is permanent.", result.c_str());
        msg.setText(buf);
        msg.setStandardButtons(QMessageBox::Open | QMessageBox::Reset | QMessageBox::Cancel);
        msg.setDefaultButton(QMessageBox::Open);
        msg.setIcon(QMessageBox::Question);

        switch(msg.exec()) {
            case QMessageBox::Cancel:
                return;
            case QMessageBox::Reset:
                std::error_code ec;
                std::filesystem::remove(sram, ec);
                if(ec) {
                    DISPLAY_ERROR_DIALOG("Failed to delete SRAM", "Couldn't delete %s", sram.string().c_str());
                    return;
                }
                should_save_sram = result != this->current_save_name;
                break;
        }
    }

    if(should_save_sram) {
        this->save_sram();
    }
    this->switch_sram(result);
}

void EmulatorWindow::save_sram_new() {
    auto new_game = ask_for_save_game("Save as new game");
    if(!new_game) {
        return;
    }
    auto &result = *new_game;

    auto sram = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_SaveData, result.c_str());

    if(std::filesystem::exists(sram)) {
        QMessageBox msg;
        msg.setWindowTitle("Overwrite?");
        char buf[512];
        std::snprintf(buf, sizeof(buf), "%s already exists. Would you save over it?", result.c_str());
        msg.setText(buf);
        msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msg.setDefaultButton(QMessageBox::Cancel);
        msg.setIcon(QMessageBox::Question);
        if(msg.exec() == QMessageBox::Cancel) {
            return;
        }
    }

    this->current_save_name = result;
    this->update_save_name_in_title_bar();
    this->save_sram();
}

static std::optional<std::string> choose_from_list(const char *title, const char *prompt, const std::vector<std::string> &options, const char *default_selection) {
    QDialog dialog;
    dialog.setWindowTitle(title);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(prompt, &dialog));

    auto *list = new QListWidget(&dialog);
    layout->addWidget(list);

    for(auto &i : options) {
        list->addItem(i.c_str());
    }

    list->sortItems();

    if(default_selection) {
        auto selected = list->findItems(default_selection, Qt::MatchExactly);
        if(selected.size() > 0) {
            list->setCurrentItem(selected[0]);
        }
        else {
            list->setCurrentRow(0);
        }
    }

    dialog.setFixedWidth(dialog.sizeHint().width());
    dialog.setFixedHeight(std::min(500, dialog.sizeHint().height()));

    list->connect(list, SIGNAL(itemActivated(QListWidgetItem *)), &dialog, SLOT(accept()));

    if(dialog.exec() != QDialog::Accepted || list->selectedItems().size() != 1) {
        return std::nullopt;
    }

    return list->selectedItems()[0]->text().toStdString();
}

void EmulatorWindow::load_game() {
    // Get all current SRAMs
    std::vector<std::string> list;
    auto srams = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_SaveData);
    for(auto i : std::filesystem::directory_iterator(srams)) {
        auto basename = i.path().filename().replace_extension().string();
        auto path = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_SaveData, basename.c_str());
        if(!std::filesystem::exists(path) || basename.empty()) {
            continue;
        }
        list.emplace_back(basename.c_str());
    }

    if(list.empty()) {
        DISPLAY_ERROR_DIALOG("No save files found", "You do not have any save files for %s", this->current_rom_name.c_str());
        return;
    }

    auto selection = choose_from_list("Load save file", "Choose a save file to load:", list, this->current_save_name.c_str());
    if(!selection) {
        return;
    }

    this->save_sram();
    this->switch_sram(*selection);
}

void EmulatorWindow::switch_sram(const std::string &new_sram) {
    this->current_save_name = new_sram;
    this->reload_current_rom_data();
    this->refresh_pause_state();
    this->set_window_title_element("SRAM switched successfully!");
}

void EmulatorWindow::open_rom_dialog() {
    QFileDialog rom_opener;
    rom_opener.setFileMode(QFileDialog::FileMode::ExistingFile);
    rom_opener.setNameFilters(QStringList({"GB/GBC/GBA ROM dumps (*.gb *.gbc *.gba)", "Any files (*)"}));
    rom_opener.setWindowTitle("Select a ROM to open");
    rom_opener.exec();

    auto files = rom_opener.selectedFiles();
    if(files.size() != 1) {
        return;
    }

    if(!this->load_rom(files[0].toStdString())) {
        return;
    }

    this->handle_loaded_rom();
}

void EmulatorWindow::reload_all_controllers() {
    this->input_devices.clear();

    this->add_keyboard();
    for(int i = 0; i < SDL_NumJoysticks(); i++) {
        this->add_device(SDL_GameControllerOpen(i));
    }

    this->set_window_title_element("Reloaded all controllers!");
}

void EmulatorWindow::open_speed_settings_dialog() {
    SpeedSettingsWindow window(this->base_speed, this->turbo_speed, this->slow_speed);
    if(window.exec() != QDialog::Accepted) {
        return;
    }

    auto settings = get_settings();
    settings.setValue("speed/base", window.get_base_speed());
    settings.setValue("speed/turbo", window.get_turbo_speed());
    settings.setValue("speed/slow", window.get_slow_speed());

    this->reload_speed_settings();
}

void EmulatorWindow::reload_speed_settings() noexcept {
    auto settings = get_settings();
    this->base_speed = settings.value("speed/base", this->base_speed).toDouble();
    this->turbo_speed = settings.value("speed/turbo", this->turbo_speed).toDouble();
    this->slow_speed = settings.value("speed/slow", this->slow_speed).toDouble();
    this->update_gameboy_speed();
}

bool EmulatorWindow::check_can_start_recording() {
    if(!this->current_rom) {
        this->set_window_title_element("Can't start a replay recording - no ROM loaded!");
        return false;
    }
    if(this->gameboy->is_recording()) {
        this->set_window_title_element("Can't start a replay recording - already recording!");
        return false;
    }
    if(this->gameboy->is_playing_back()) {
        this->set_window_title_element("Can't start a replay recording - playback in process!");
        return false;
    }
    if(!this->make_recording_tmp_file()) {
        return false;
    }
    return true;
}

void EmulatorWindow::start_replay_recording() {
    if(!this->check_can_start_recording()) {
        return;
    }
    this->recording_file = {};
    this->gameboy->start_replay_recording(current_rom_name.c_str());
    this->set_window_title_element("Replay started!");
    this->currently_recording = true;
}

bool EmulatorWindow::make_recording_tmp_file() {
    this->temporary_file_path = this->assign_recording_file_name("tmp-recording");

    auto path_str = this->temporary_file_path.string();
    const char *path_cstr = path_str.c_str();
    this->temporary_file_recording = std::fopen(path_cstr, "wb");
    this->temporary_file_recording_offset = 0;
    this->temporary_file_time_since_last_save = std::chrono::steady_clock::now();
    if(!this->temporary_file_recording) {
        DISPLAY_ERROR_DIALOG("Failed to create replay", "Could not create replay file %s", path_cstr);
        return false;
    }
    return true;
}

void EmulatorWindow::update_recording_tmp_file() {
    if(!this->temporary_file_recording) {
        return;
    }
    auto latest_bytes = this->gameboy->get_current_replay_recording_data(this->temporary_file_recording_offset);
    if(latest_bytes.empty()) {
        return;
    }
    if(std::fwrite(latest_bytes.data(), latest_bytes.size(), 1, this->temporary_file_recording) != 1) {
        DISPLAY_ERROR_DIALOG("Failed to create replay", "Could not write to replay file %s", this->temporary_file_path.string().c_str());
        this->stop_replay_recording();
        this->close_recording_tmp_file();
        return;
    }
    this->temporary_file_recording_offset += latest_bytes.size();
    this->temporary_file_time_since_last_save = std::chrono::steady_clock::now();
}

void EmulatorWindow::close_recording_tmp_file() noexcept {
    if(!this->temporary_file_recording) {
        return;
    }
    std::fclose(this->temporary_file_recording);
    this->temporary_file_recording = nullptr;
}

void EmulatorWindow::continue_replay_recording() {
    if(!this->check_can_start_recording()) {
        return;
    }
    auto replay = this->pick_replay();
    if(!replay) {
        return;
    }
    auto data = this->read_replay_file(replay->c_str());
    if(!data) {
        return;
    }

    this->recording_file = replay;
    this->set_up_replay_playback_environment();
    this->gameboy->start_recording_from_end_of_replay(*data);
    this->set_window_title_element("Replay resumed!");
    this->currently_recording = true;
    this->refresh_pause_state();
}

void EmulatorWindow::set_up_replay_playback_environment() {
    // Save what we have
    this->save_sram();

    // Reload the emulator
    this->current_save_name = RESERVED_REPLAY_PLAYBACK_SAVE_NAME;
    this->reload_current_rom_data();
}

std::filesystem::path EmulatorWindow::assign_recording_file_name(const char *prefix) {
    unsigned int c = 0;
    while(true) {
        char fmt[512];
        std::snprintf(fmt, sizeof(fmt), "%s-%u", prefix, c);
        auto path = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_Replays, fmt);
        if(!std::filesystem::exists(path)) {
            return path;
        }
        c++;
    }
}

void EmulatorWindow::stop_replay_recording() {
    if(!this->gameboy->is_recording()) {
        this->set_window_title_element("Can't stop a replay recording - not recording!");
        return;
    }
    this->currently_recording = false;
    this->update_recording_tmp_file();
    this->close_recording_tmp_file();
    auto compressed = this->gameboy->get_current_replay_recording_data_compressed();
    this->gameboy->stop_replay_recording();

    if(this->temporary_file_recording_offset == 0) {
        this->set_window_title_element("No replay data to write.");
        return;
    }

    std::filesystem::path path;
    if(!this->recording_file) {
        path = this->assign_recording_file_name(current_save_name.c_str());
        this->recording_file = path.filename().string().c_str();
    }
    else {
        path = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_Replays, this->recording_file->c_str());
    }

    const char *file = this->recording_file->c_str();

    if(!write_file(path, compressed)) {
        DISPLAY_ERROR_DIALOG("Failed to save replay", "Could not write replay file %s; however there is a temporary file %s", file, this->temporary_file_path.string().c_str());
        return;
    }

    std::error_code error;
    std::filesystem::remove(this->temporary_file_path, error);

    char fmt_result[600];
    std::snprintf(fmt_result, sizeof(fmt_result), "Replay written to %s", file);
    this->set_window_title_element(fmt_result);
}

std::optional<std::string> EmulatorWindow::pick_replay() {
    // Get all current replays
    std::vector<std::string> list;
    auto replays = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_Replays);
    for(auto i : std::filesystem::directory_iterator(replays)) {
        auto basename = i.path().filename().replace_extension().string();
        auto path = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_Replays, basename.c_str());
        if(!std::filesystem::exists(path) || basename.empty()) {
            continue;
        }
        list.emplace_back(basename.c_str());
    }

    if(list.empty()) {
        DISPLAY_ERROR_DIALOG("No replays found", "You do not have any replays for %s", this->current_rom_name.c_str());
        return {};
    }

    return choose_from_list("Load replay", "Choose a replay to load:", list, this->current_save_name.c_str());
}

std::optional<std::vector<std::byte>> EmulatorWindow::read_replay_file(const char *replay) {
    auto path = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_Replays, replay);
    auto file = read_file(path);
    if(!file) {
        DISPLAY_ERROR_DIALOG("Failed to open replay", "Couldn't open %s", replay);
    }
    return file;
}

void EmulatorWindow::load_replay() {
    auto selection = this->pick_replay();
    if(!selection) {
        return;
    }
    auto file = this->read_replay_file(selection->c_str());
    if(!file) {
        return;
    }

    this->set_up_replay_playback_environment();
    this->gameboy->start_replay_playback(*file);
    this->currently_playing_back_recording = true;

    char fmt[600];
    std::snprintf(fmt, sizeof(fmt), "Loaded replay %s", selection->c_str());
    this->set_window_title_element(fmt);
    this->refresh_pause_state();
}

void EmulatorWindow::stop_replay() {
    if(!this->gameboy->is_playing_back()) {
        this->set_window_title_element("Can't stop a replay playback - not playing back!");
        return;
    }
    this->currently_playing_back_recording = false;
    this->gameboy->stop_replay_playback();
    this->revert_window_title();
}

void EmulatorWindow::set_scaling_settings(QAction *trigger) {
    auto *menu = dynamic_cast<QWidget *>(trigger->parent());
    auto actions = menu->actions();

    for(auto *i : menu->actions()) {
        i->setChecked(i == trigger);
    }

    this->scaling_setting(trigger->data().toInt());
    this->refresh_scale();
}

void EmulatorWindow::open_controls_settings_dialog() {
    this->input_state = {};
    this->update_input_state_on_gameboy();

    ControlsSettingsWindow window(this, *this);

    window.connect(this, SIGNAL(on_device_input(SDL_ControllerButtonEvent &, ControllerInputDevice &)), &window, SLOT(on_device_input(SDL_ControllerButtonEvent &, ControllerInputDevice &)));
    window.connect(this, SIGNAL(on_device_input(SDL_ControllerAxisEvent &, ControllerInputDevice &)), &window, SLOT(on_device_input(SDL_ControllerAxisEvent &, ControllerInputDevice &)));

    this->suppress_game_input = true;
    auto result = window.exec();
    this->suppress_game_input = false;

    if(result != QDialog::Accepted) {
        return;
    }

    for(auto &i : window.all_settings) {
        for(auto &c : this->input_devices) {
            auto &device = *c.second;
            if(device.get_name_settings() != i.first) {
                continue;
            }

            device.settings_button = i.second.all_settings[0];
            device.settings_analog_positive = i.second.all_settings[1];
            device.settings_analog_negative = i.second.all_settings[2];

            this->write_settings_for_device(device);

            break;
        }
    }
}

void EmulatorWindow::perform_reset() {
    this->gameboy->reset();
}

void EmulatorWindow::ignore_replay_speed_changes() {
    bool ignored = this->ignore_replay_speed_changes_option->isChecked();
    this->ignore_replay_speed_changes_setting(ignored);
    this->gameboy->set_ignore_speed_changes_on_replay(ignored);
    this->update_gameboy_speed();
}

void EmulatorWindow::ignore_recording_speed_changes() {
    bool ignored = this->ignore_recording_speed_changes_option->isChecked();
    this->ignore_recording_speed_changes_setting(ignored);
    this->gameboy->set_ignore_speed_changes_on_replay(ignored);
    this->update_gameboy_speed();
}

void EmulatorWindow::skip_forward() {
    if(!this->currently_playing_back_recording) {
        return;
    }
    this->gameboy->skip_to_frame(this->gameboy->get_current_frame_index() + 600);
}

void EmulatorWindow::skip_backward() {
    if(!this->currently_playing_back_recording) {
        return;
    }
    auto frame = this->gameboy->get_current_frame_index();
    this->gameboy->skip_to_frame(frame - std::min(frame, static_cast<std::size_t>(600)));
}

void EmulatorWindow::update_manually_paused() {
    this->manually_paused = this->manually_pause_option->isChecked();
    this->revert_window_title();
    this->refresh_pause_state();
}

void EmulatorWindow::loop_playback() {
    bool looping = this->loop_playback_option->isChecked();
    this->loop_playback_setting(looping);
    this->gameboy->set_loop_playback(looping);
}
