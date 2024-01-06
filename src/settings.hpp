#ifndef SS64_SETTINGS_HPP
#define SS64_SETTINGS_HPP

#include <QSettings>
#include <filesystem>

namespace SuperShuckie64 {
    void migrate_old_settings() noexcept;

    QSettings get_settings() noexcept;
    std::filesystem::path get_applocal_path();

    enum RomUserDataType {
        // Replays (recorded runs)
        RomUserDataType_Replays,

        // Save data (may contain multiple saves)
        RomUserDataType_SaveData,

        // Save states directory (quick saves and permanent save states)
        RomUserDataType_SaveStates,

        // Crashes the app... but at what cost?
        RomUserDataType_Count
    };

    // Gets the base dir for a ROM
    std::filesystem::path get_rom_user_data_path(const char *basename);

    // Get the error output log
    std::filesystem::path get_stderr_log_path();

    // Get the settings ini path
    std::filesystem::path get_settings_path();

    // Gets the user data directory for the ROM
    std::filesystem::path get_rom_user_data_path(const char *basename, RomUserDataType type, const char *innerfile = nullptr);

    // Init all directories for the ROM (does not touch already created directories)
    void init_rom_user_data_path(const char *basename, bool &error);

}

#endif
