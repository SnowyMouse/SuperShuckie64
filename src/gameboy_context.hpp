#ifndef SS64_GAMEBOY_CONTEXT_HPP
#define SS64_GAMEBOY_CONTEXT_HPP

extern "C" {
#include <gb.h>
}

#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <optional>

#include "../rs/replay-recorder-c/include/replay_recorder.hpp"

namespace SuperShuckie64 {
    /**
     * This is the context for the emulator core, itself.
     *
     * The emulation is run on a separate thread, and most functions have to be synchronized (blocking).
     */
    class GameboyContext {
    public:
        GameboyContext(GB_model_t model, const std::vector<std::byte> &rom);
        ~GameboyContext();

        /**
         * Load the SRAM buffer
         */
        void load_sram(const std::vector<std::byte> &sram) noexcept;

        /**
         * Get the SRAM buffer
         */
        std::vector<std::byte> get_sram() noexcept;

        /**
         * Get the screen resolution
         */
        void get_frame_resolution(unsigned &width, unsigned &height) noexcept;

        /**
         * Copy the last completed frame buffer
         */
        void copy_frame_buffer(std::uint32_t *buffer) noexcept;

        /**
         * Copy the frame buffer with interframe blending from the previous framebuffer
         */
        void copy_frame_buffer_blended(std::uint32_t *buffer) noexcept;

        /**
         * Set the input mask (uses GB_key_mask_t bits)
         *
         * This is non-blocking.
         */
        void set_input(std::uint8_t new_input) noexcept {
            this->pending_input = new_input;
        }

        /**
         * Set the rapid fire input mask (uses GB_key_mask_t bits)
         *
         * This is non-blocking.
         */
        void set_rapid_fire_input(std::uint8_t new_input) noexcept {
            this->rapid_fire_input = new_input;
        }

        /**
         * Set whether or not the emulator should be paused. By default, it is.
         *
         * This is non-blocking.
         */
        void set_paused(bool paused) noexcept {
            this->paused = paused;
        }

        /**
         * Set the new speed
         */
        void set_speed(std::uint16_t new_speed) noexcept;

        bool is_recording() noexcept;
        void start_replay_recording(const char *rom_name);
        std::vector<std::byte> get_current_replay_recording_data();
        void stop_replay_recording();

        bool is_playing_back() noexcept;
        void start_replay_playback(const std::vector<std::byte> &replay);
        void stop_replay_playback();



    private:
        void start_thread() noexcept;
        void set_up_gameboy(GB_model_t model, const std::vector<std::byte> &rom) noexcept;

        void acquire_context() noexcept;
        void unlock_context() noexcept;
        std::mutex execute_lock;
        std::mutex execute_wait;

        std::atomic_uint8_t pending_input = 0;
        std::atomic_uint8_t rapid_fire_input = 0;
        std::atomic_bool paused = true;

        std::uint8_t current_input = 0;

        std::uint8_t rapid_fire_duty_cycle = 3;
        std::uint8_t rapid_fire_duty_cycle_step = 0;
        bool rapid_fire_state = false;

        std::vector<std::uint32_t> framebuffers[3];
        std::size_t last_framebuffer = 0;
        std::size_t current_framebuffer = 1;
        std::size_t work_framebuffer = 2;
        std::size_t pixel_count = 0;

        // 8.8
        std::uint16_t speed_multiplier = 1 << 8;

        void swap_framebuffers() noexcept;
        std::mutex present_framebuffer_lock; // mutex for framebuffers (besides the work framebuffer); used to present the current status of the framebuffers!

        void on_vblank() noexcept;
        static void on_vblank(GB_gameboy_t *, GB_vblank_type_t) noexcept;

        bool is_recording_inner() const noexcept;
        bool is_playing_back_inner() const noexcept;

        void handle_new_input(std::uint8_t new_input) noexcept;
        void handle_set_speed(std::uint16_t new_speed) noexcept;

        std::unique_ptr<GB_gameboy_t, void (*)(GB_gameboy_t *)> gameboy;

        std::unique_ptr<ReplayWriter> replay_recorder;
        std::optional<ReplayReaderItemCollection> current_playback;
        std::unordered_map<std::uint32_t, std::vector<std::uint8_t>> replay_states;
        std::size_t current_playback_offset;
        std::size_t current_playback_delay;
        void play_latest_packet();

        std::atomic_bool thread_running;
        std::atomic_bool stop_thread;

        std::vector<std::byte> current_rom_data, current_boot_rom_data;


        void run_thread() noexcept;
    };
}

#endif
