#ifndef SS64_EMU_WINDOW_HPP
#define SS64_EMU_WINDOW_HPP

#include "../rs/udp-command-handler/include/udp_command_handler.h"
#include "gameboy_context.hpp"
#include "input_device.hpp"

#include <QMainWindow>
#include <QTimer>
#include <optional>
#include <filesystem>
#include <memory>
#include <atomic>
#include <chrono>
#include <unordered_map>

#include <QPixmap>
#include <SDL2/SDL.h>

class QGraphicsScene;
class QGraphicsPixmapItem;
class QMenu;

struct SDL_ControllerDeviceEvent;

#define RESERVED_REPLAY_PLAYBACK_SAVE_NAME "replay-playback"

namespace SuperShuckie64 {
    class PixelBufferView;
    class ControlsSettingsWindow;

    class EmulatorWindow : public QMainWindow {
        Q_OBJECT

        friend ControlsSettingsWindow;

    public:
        EmulatorWindow(const std::optional<std::filesystem::path> &default_rom);

        // Return true if EmulatorWindow's constructor succeeded.
        bool is_valid() { return this->valid; }

        // Load the ROM and start it, returning `true` if successful and `false` on failure.
        bool load_and_start_rom(const std::optional<std::filesystem::path> &path);

    private:
        std::vector<std::byte> current_rom_data;
        std::optional<std::filesystem::path> current_rom;
        std::string current_rom_name;
        std::string current_save_name;
        std::string displayed_save_name;
        std::shared_ptr<UDPCommandHandler> udp_command_server;

        // store these here to reduce mutex locking
        bool currently_recording = false;
        bool currently_playing_back_recording = false;

        bool suppress_game_input = false;
        bool valid = false;
        double base_speed = 1.0;
        double turbo_speed = 4.0;
        double slow_speed = 0.25;
        void update_gameboy_speed();

        void refresh_scale();
        void tick();
        QTimer ticker;

        unsigned width, height;

        std::optional<std::chrono::steady_clock::time_point> revert_window_title_timer;

        bool manually_paused = false;
        QAction *manually_pause_option;
        void refresh_pause_state() noexcept;

        QMenu *gameplay_menu;
        QMenu *replays_menu;
        InputState input_state = {};
        std::unique_ptr<GameboyContext> gameboy;
        QPixmap pixel_buffer_pixmap;
        QGraphicsPixmapItem *pixel_buffer_pixmap_item = nullptr;
        std::vector<std::uint32_t> pixel_buffer;
        PixelBufferView *pixel_buffer_view;
        QGraphicsScene *pixel_buffer_scene = nullptr;

        std::filesystem::path temporary_file_path;
        FILE *temporary_file_recording = nullptr;
        std::size_t temporary_file_recording_offset = 0;
        std::chrono::time_point<std::chrono::steady_clock> temporary_file_time_since_last_save;
        bool make_recording_tmp_file();
        void update_recording_tmp_file();
        void close_recording_tmp_file() noexcept;


        std::filesystem::path assign_recording_file_name(const char *prefix);

        // Used to make sure we only do this once when using it from a controller
        bool is_resetting = false;
        bool is_pausing = false;

        std::unordered_map<SDL_JoystickID, std::shared_ptr<ControllerInputDevice>> input_devices;

        // Moment when the window was created
        std::chrono::steady_clock::time_point created_time = std::chrono::steady_clock::now();

        // Get how long the window was open
        std::chrono::milliseconds window_alive_time() const noexcept {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - this->created_time);
        }

        std::filesystem::path get_rom_settings_path() const;

        int scaling_setting(int new_setting = 0);

        bool ignore_replay_speed_changes_setting(int new_setting = -1);
        QAction *ignore_replay_speed_changes_option = nullptr;
        bool ignore_recording_speed_changes_setting(int new_setting = -1);
        QAction *ignore_recording_speed_changes_option = nullptr;
        bool loop_playback_setting(int new_setting = -1);
        QAction *loop_playback_option = nullptr;
        std::optional<std::string> recording_file;

        // WARNING: Does not save SRAM
        void reload_current_rom_data();

        void set_window_title_element(const char *what) noexcept;
        void reload_speed_settings() noexcept;
        bool load_rom(const std::optional<std::filesystem::path> &path);
        void handle_loaded_rom() noexcept;
        void write_settings_for_controller(const ControllerInputDevice &device);
        void load_settings_for_controller(ControllerInputDevice &device);
        void set_up_menu();
        void add_device(SDL_ControllerDeviceEvent &event) noexcept;
        void remove_device(SDL_ControllerDeviceEvent &event) noexcept;
        void register_button(SDL_ControllerButtonEvent &event, bool on) noexcept;
        void register_axis(SDL_ControllerAxisEvent &event) noexcept;
        void update_input_state_on_gameboy() noexcept;
        void revert_window_title() noexcept;
        void add_device(SDL_GameController *controller) noexcept;
        void closeEvent(QCloseEvent *event) override;
        void switch_sram(const std::string &new_sram);
        void update_save_name_in_title_bar();
        bool check_can_start_recording();
        std::optional<std::string> pick_replay();
        std::optional<std::vector<std::byte>> read_replay_file(const char *replay);
        void set_up_replay_playback_environment();
        void toggle_pause();


    private slots:
        void set_scaling_settings(QAction *);
        void open_rom_dialog();
        void open_speed_settings_dialog();
        void reload_all_controllers();
        void close_rom();
        void save_sram();
        void new_game();
        void load_game();
        void save_sram_new();
        void start_replay_recording();
        void stop_replay_recording();
        void load_replay();
        void stop_replay();
        void open_controls_settings_dialog();
        void perform_reset();
        void ignore_replay_speed_changes();
        void ignore_recording_speed_changes();
        void loop_playback();
        void skip_forward();
        void skip_backward();
        void continue_replay_recording();
        void update_manually_paused();

    signals:
        void on_device_input(SDL_ControllerButtonEvent &, ControllerInputDevice &);
        void on_device_input(SDL_ControllerAxisEvent &, ControllerInputDevice &);

    };
}

#endif
