#include <QGridLayout>
#include <QMenuBar>
#include <QFileDialog>
#include <QKeyCombination>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QListWidget>
#include "error.hpp"
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

    ADD_ACTION_AND_CONNECT("Speed settings...", settings_menu, open_speed_settings_dialog());
    //ADD_ACTION_AND_CONNECT("Remote command settings...", settings_menu, open_gamehook_settings_dialog());
    settings_menu->addSeparator();
    //ADD_ACTION_AND_CONNECT("Controls settings...", settings_menu, open_controls_settings_dialog());
    ADD_ACTION_AND_CONNECT("Reload all controllers", settings_menu, reload_all_controllers());


    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("New game...", this->gameplay_menu, new_game(), QKeyCombination(Qt::ControlModifier, Qt::Key_N));
    ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("Load game...", this->gameplay_menu, load_game(), QKeyCombination(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_O));
    file_menu->addSeparator();
    //ADD_ACTION_AND_CONNECT_WITH_SHORTCUT("Record replay...", this->gameplay_menu, start_replay(), QKeyCombination(Qt::ControlModifier, Qt::Key_R));
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

void EmulatorWindow::new_game() {
    auto new_game = ask_for_save_game("New game");
    if(!new_game) {
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

void EmulatorWindow::load_game() {
    QDialog ask_for_load_game;
    ask_for_load_game.setWindowTitle("Load game");

    auto *layout = new QVBoxLayout(&ask_for_load_game);
    layout->addWidget(new QLabel("Select a save file to load:", &ask_for_load_game));

    auto *list = new QListWidget(&ask_for_load_game);
    layout->addWidget(list);

    // Get all current SRAMs
    auto srams = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_SaveData);
    for(auto i : std::filesystem::directory_iterator(srams)) {
        auto basename = i.path().filename().replace_extension().string();
        auto path = get_rom_user_data_path(this->current_rom_name.c_str(), RomUserDataType::RomUserDataType_SaveData, basename.c_str());
        if(!std::filesystem::exists(path) || basename.empty()) {
            continue;
        }
        list->addItem(basename.c_str());
    }

    if(list->count() == 0) {
        DISPLAY_ERROR_DIALOG("No save files found", "You do not have any save files for %s", this->current_rom_name.c_str());
        return;
    }

    list->sortItems();

    auto current_sram_maybe = list->findItems(this->current_save_name.c_str(), Qt::MatchExactly);
    if(current_sram_maybe.size() > 0) {
        list->setCurrentItem(current_sram_maybe[0]);
    }
    else {
        list->setCurrentRow(0);
    }

    ask_for_load_game.setFixedWidth(ask_for_load_game.sizeHint().width());
    ask_for_load_game.setFixedHeight(std::min(500, ask_for_load_game.sizeHint().height()));

    connect(list, SIGNAL(itemActivated(QListWidgetItem *)), &ask_for_load_game, SLOT(accept()));

    if(ask_for_load_game.exec() != QDialog::Accepted || list->selectedItems().size() != 1) {
        return;
    }

    this->save_sram();
    this->switch_sram(list->selectedItems()[0]->text().toStdString());
}

void EmulatorWindow::switch_sram(const std::string &new_sram) {
    this->current_save_name = new_sram;
    this->reload_current_rom_data();
    this->gameboy->set_paused(false);
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

    this->handle_loaded_rom();
}

void EmulatorWindow::reload_all_controllers() {
    this->input_devices.clear();

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
