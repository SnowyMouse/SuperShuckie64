#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include "error.hpp"
#include "settings.hpp"

using namespace SuperShuckie64;

static std::filesystem::path get_settings_directory() {
    // TODO: Figure this out for later
    #define PORTABLE
    #ifdef PORTABLE
    return std::filesystem::path(QCoreApplication::applicationDirPath().toStdString());
    #else
    return std::filesystem::path(QDir::currentPath().toStdString());
    #endif
}

std::filesystem::path SuperShuckie64::get_settings_path() {
    return get_applocal_path() / "config.ini";
}

void SuperShuckie64::migrate_old_settings() noexcept {
    auto old_settings = get_settings_directory() / "SuperShuckie64.ini";
    auto new_settings = get_settings_path();
    if(std::filesystem::exists(old_settings) && !std::filesystem::exists(new_settings)) {
        std::filesystem::rename(old_settings, new_settings);
    }

    auto old_logs = get_settings_directory() / "SuperShuckie64-logs.txt";
    auto new_logs = get_stderr_log_path();

    if(std::filesystem::exists(old_logs) && !std::filesystem::exists(new_logs)) {
        std::filesystem::rename(old_logs, new_logs);
    }
}

QSettings SuperShuckie64::get_settings() noexcept {
    return QSettings(get_settings_path().string().c_str(), QSettings::Format::IniFormat);
}

std::filesystem::path SuperShuckie64::get_applocal_path() {
    return get_settings_directory() / "UserData";
}

std::filesystem::path SuperShuckie64::get_rom_user_data_path(const char *basename) {
    char full_name_data[512];
    std::snprintf(full_name_data, sizeof(full_name_data), "%s-userdata", basename);
    return get_applocal_path() / full_name_data;
}

std::filesystem::path SuperShuckie64::get_stderr_log_path() {
    return get_applocal_path() / "logs.txt";
}

std::filesystem::path SuperShuckie64::get_rom_user_data_path(const char *basename, RomUserDataType type, const char *innerfile) {
    auto path = get_rom_user_data_path(basename);
    switch(type) {
        case RomUserDataType_Replays:
            path /= "Replays";
            break;
        case RomUserDataType_SaveData:
            path /= "Saves";
            break;
        case RomUserDataType_SaveStates:
            path /= "Save States";
            break;
        default:
            std::terminate(); // you activated my trap card!
    }
    if(innerfile) {
        char full_file_name[512];
        switch(type) {
            case RomUserDataType_Replays:
                std::snprintf(full_file_name, sizeof(full_file_name), "%s.replay", innerfile);
                break;
            case RomUserDataType_SaveData:
                std::snprintf(full_file_name, sizeof(full_file_name), "%s.sav", innerfile);
                break;
            case RomUserDataType_SaveStates:
                std::snprintf(full_file_name, sizeof(full_file_name), "%s.savestate", innerfile);
                break;
            default:
                std::terminate(); // oops
        }
        path /= full_file_name;
    }
    return path;
}

void SuperShuckie64::init_rom_user_data_path(const char *basename, bool &error) {
    std::error_code ec;
    error = false;
    for(auto i = static_cast<RomUserDataType>(0); i < RomUserDataType::RomUserDataType_Count; i = static_cast<RomUserDataType>(i + 1)) {
        auto dir_to_make = get_rom_user_data_path(basename, i, nullptr);
        std::filesystem::create_directories(dir_to_make, ec);
        if(ec) {
            DISPLAY_ERROR_DIALOG("Failed to initialize user data for %s\n\nUnable to create %s\n\nThe error was: %s", basename, dir_to_make.string().c_str(), ec.message().c_str());
            error = true;
        }
    }
}
