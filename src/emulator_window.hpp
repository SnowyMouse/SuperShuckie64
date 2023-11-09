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

    class EmulatorWindow : public QMainWindow {
        Q_OBJECT
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

        QMenu *gameplay_menu;
        InputState input_state = {};
        std::unique_ptr<GameboyContext> gameboy;
        QPixmap pixel_buffer_pixmap;
        QGraphicsPixmapItem *pixel_buffer_pixmap_item = nullptr;
        std::vector<std::uint32_t> pixel_buffer;
        PixelBufferView *pixel_buffer_view;
        QGraphicsScene *pixel_buffer_scene = nullptr;

        std::unordered_map<SDL_JoystickID, std::shared_ptr<ControllerInputDevice>> input_devices;

        // Moment when the window was created
        std::chrono::steady_clock::time_point created_time = std::chrono::steady_clock::now();

        // Get how long the window was open
        std::chrono::milliseconds window_alive_time() const noexcept {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - this->created_time);
        }

        std::filesystem::path get_rom_settings_path() const;

        int scaling_setting(int new_setting = 0);

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

    signals:
        void on_device_input(SDL_ControllerButtonEvent &, ControllerInputDevice &);
        void on_device_input(SDL_ControllerAxisEvent &, ControllerInputDevice &);

    };
}

#endif
