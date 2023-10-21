#include <cstring>
#include <thread>

#include "../defaultrom/cgb_boot/cgb_boot_fast.hpp"
#include "gameboy_context.hpp"

using namespace SuperShuckie64;

#define NULL_GAMEBOY std::unique_ptr<GB_gameboy_t, void(*)(GB_gameboy_t *)>(nullptr, [](auto *){})

static std::unique_ptr<GB_gameboy_t, void (*)(GB_gameboy_t *)> new_gameboy(GB_model_t initial_model) {
    return std::unique_ptr<GB_gameboy_t, void (*)(GB_gameboy_t *)>(GB_init(GB_alloc(), initial_model), GB_dealloc);
}
static GameboyContext &resolve_instance(GB_gameboy_t *gb) {
    return *reinterpret_cast<GameboyContext *>(GB_get_user_data(gb));
}

GameboyContext::GameboyContext(GB_model_t model, const std::vector<std::byte> &rom) : gameboy(NULL_GAMEBOY) {
    this->set_up_gameboy(model, rom);
    this->start_thread();
}

void GameboyContext::start_thread() noexcept {
    this->stop_thread = false;
    this->thread_running = true;
    std::thread(&GameboyContext::run_thread, this).detach();
}

GameboyContext::~GameboyContext() noexcept {
    this->stop_thread = true;
    while(this->thread_running);
}

void GameboyContext::get_frame_resolution(unsigned &width, unsigned &height) noexcept {
    this->acquire_context();
    auto w = GB_get_screen_width(this->gameboy.get());
    auto h = GB_get_screen_height(this->gameboy.get());
    this->unlock_context();
    width = w;
    height = h;
}

void GameboyContext::load_sram(const std::vector<std::byte> &sram) noexcept {
    this->acquire_context();
    GB_load_battery_from_buffer(this->gameboy.get(), reinterpret_cast<const std::uint8_t *>(sram.data()), sram.size());
    this->unlock_context();
}

std::vector<std::byte> GameboyContext::get_sram() noexcept {
    this->acquire_context();
    std::vector<std::byte> rv;
    rv.resize(GB_save_battery_size(this->gameboy.get()));
    GB_save_battery_to_buffer(this->gameboy.get(), reinterpret_cast<std::uint8_t *>(rv.data()), rv.size());
    this->unlock_context();
    return rv;
}

void GameboyContext::set_up_gameboy(GB_model_t model, const std::vector<std::byte> &rom) noexcept {
    this->gameboy = new_gameboy(model);
    this->current_rom_data = rom;
    this->current_boot_rom_data = std::vector<std::byte>(reinterpret_cast<const std::byte *>(AUTOGEN_CGB_BOOT_FAST_HPP_VAL), reinterpret_cast<const std::byte *>(AUTOGEN_CGB_BOOT_FAST_HPP_VAL + sizeof(AUTOGEN_CGB_BOOT_FAST_HPP_VAL)));

    GB_set_user_data(this->gameboy.get(), this);
    GB_set_vblank_callback(this->gameboy.get(), GameboyContext::on_vblank);
    GB_set_rtc_mode(this->gameboy.get(), GB_rtc_mode_t::GB_RTC_MODE_ACCURATE);
    GB_load_boot_rom_from_buffer(this->gameboy.get(), AUTOGEN_CGB_BOOT_FAST_HPP_VAL, sizeof(AUTOGEN_CGB_BOOT_FAST_HPP_VAL));
    GB_load_rom_from_buffer(this->gameboy.get(), reinterpret_cast<const std::uint8_t *>(rom.data()), rom.size());

    auto rgb_encode = [](GB_gameboy_t *gb, std::uint8_t r, std::uint8_t g, std::uint8_t b) -> std::uint32_t {
        return (static_cast<std::uint32_t>(b) | (static_cast<std::uint32_t>(g) << 8) | (static_cast<std::uint32_t>(r) << 16) | 0xFF000000);
    };
    GB_set_rgb_encode_callback(this->gameboy.get(), rgb_encode);

    this->present_framebuffer_lock.lock();
    std::size_t width = GB_get_screen_width(this->gameboy.get());
    std::size_t height = GB_get_screen_height(this->gameboy.get());
    this->pixel_count = width * height;
    for(auto &fb : this->framebuffers) {
        fb.clear();
        fb.resize(this->pixel_count, 0);
    }
    this->present_framebuffer_lock.unlock();
    this->current_input = 0;
    this->pending_input = 0;

    this->swap_framebuffers();
}

void GameboyContext::swap_framebuffers() noexcept {
    this->present_framebuffer_lock.lock();
    this->last_framebuffer = this->current_framebuffer;
    this->current_framebuffer = this->work_framebuffer;
    this->work_framebuffer = (this->work_framebuffer + 1) % (sizeof(this->framebuffers) / sizeof(this->framebuffers[0]));
    GB_set_pixels_output(this->gameboy.get(), this->framebuffers[this->work_framebuffer].data());
    this->present_framebuffer_lock.unlock();
}

void GameboyContext::copy_frame_buffer(std::uint32_t *buffer) noexcept {
    this->present_framebuffer_lock.lock();
    auto &current_framebuffer = this->framebuffers[this->current_framebuffer];
    std::memcpy(buffer, current_framebuffer.data(), sizeof(*current_framebuffer.data()) * current_framebuffer.size());
    this->present_framebuffer_lock.unlock();
}

void GameboyContext::copy_frame_buffer_blended(std::uint32_t *buffer) noexcept {
    this->present_framebuffer_lock.lock();

    auto *current_framebuffer = this->framebuffers[this->current_framebuffer].data();
    auto *last_framebuffer = this->framebuffers[this->last_framebuffer].data();

    for(std::size_t i = 0; i < this->pixel_count; i++) {
        std::uint32_t last = last_framebuffer[i];
        std::uint32_t current = current_framebuffer[i];
        std::uint32_t blended = 0x00000000;
        for(int p = 0; p < 4; p++) {
            auto channel_average = (((last & 0xFF) + (current & 0xFF)) & (0xFF << 1)) << 23;
            blended = channel_average | (blended >> 8);
            last >>= 8;
            current >>= 8;
        }
        buffer[i] = blended;
    }

    this->present_framebuffer_lock.unlock();
}

void GameboyContext::on_vblank() noexcept {
    this->swap_framebuffers();

    if(this->is_recording_inner()) {
        this->replay_recorder->next_frame();
    }

    if(!this->is_playing_back_inner()) {
        std::uint8_t new_input_value = this->pending_input;
        std::uint8_t rapid_fire_input_value = this->rapid_fire_input;

        // Handle rapid fire duty cycle
        auto new_step_value = (this->rapid_fire_duty_cycle_step + 1) % this->rapid_fire_duty_cycle;
        if (!(this->rapid_fire_duty_cycle_step = new_step_value)) {
            this->rapid_fire_state = !this->rapid_fire_state;
        }

        // On
        if(this->rapid_fire_state) {
            new_input_value |= rapid_fire_input_value;
        }
        // Off
        else {
            new_input_value &= ~rapid_fire_input_value;
        }

        if(this->current_input != new_input_value) {
            this->handle_new_input(new_input_value);
        }
    }

    if(this->is_playing_back_inner()) {
        this->current_playback_delay--;
        this->play_latest_packet();
    }
}


void GameboyContext::on_vblank(GB_gameboy_t *gb, GB_vblank_type_t type) noexcept {
    if(type != GB_vblank_type_t::GB_VBLANK_TYPE_NORMAL_FRAME) {
        return;
    }

    #ifdef DO_FPS_BENCHMARK
    static auto start = std::chrono::steady_clock::now();
    static unsigned long long i = 0;

    i += 1;

    if(i % 100 == 0) {
        auto time_since = i / (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() / 1000.0);
        std::printf("%.04f\n", time_since);
        start = std::chrono::steady_clock::now();
        i = 0;
    }
    #endif

    resolve_instance(gb).on_vblank();
}

void GameboyContext::run_thread() noexcept {
    this->execute_lock.lock();

    while(true) {
        GB_run(this->gameboy.get());

        // Check if something needs something.
        do {
            this->execute_lock.unlock();
            this->execute_wait.lock();
            this->execute_lock.lock();
            this->execute_wait.unlock();
            if(this->stop_thread) {
                goto end;
            }
        }
        while(this->paused);
    }

    end:
    this->execute_lock.unlock();
    this->thread_running = false;
}

void GameboyContext::handle_new_input(std::uint8_t new_input) noexcept {
    GB_set_key_mask(this->gameboy.get(), static_cast<GB_key_mask_t>(new_input));
    if(this->is_recording_inner()) {
        this->replay_recorder->write_SetInput8(new_input);
    }
    this->current_input = new_input;
}

void GameboyContext::acquire_context() noexcept {
    this->execute_wait.lock();
    this->execute_lock.lock();
}

void GameboyContext::unlock_context() noexcept {
    this->execute_lock.unlock();
    this->execute_wait.unlock();
}

void GameboyContext::set_speed(std::uint16_t new_speed) noexcept {
    this->acquire_context();
    this->handle_set_speed(new_speed);
    this->unlock_context();
}

void GameboyContext::handle_set_speed(std::uint16_t new_speed) noexcept {
    if(this->speed_multiplier == new_speed) {
        return;
    }

    this->speed_multiplier = new_speed;
    GB_set_clock_multiplier(this->gameboy.get(), new_speed / 256.0);

    if(this->is_recording_inner()) {
        this->replay_recorder->write_ChangeGameSpeed(this->speed_multiplier);
    }
}

void GameboyContext::start_replay_recording(const char *rom_name) {
    this->acquire_context();

    this->replay_recorder = std::make_unique<ReplayWriter>(
        "SuperShuckie64 ALPHA 0", rom_name, this->current_rom_data.data(), this->current_rom_data.size(), this->current_boot_rom_data.data(), this->current_boot_rom_data.size()
    );

    // automatically create and load a savestate here!
    auto len = GB_get_save_state_size(this->gameboy.get());
    std::vector<std::uint8_t> save_state;
    save_state.resize(len);
    GB_save_state_to_buffer(this->gameboy.get(), save_state.data());

    this->replay_recorder->write_AddSaveState(0, save_state.data(), save_state.size());
    this->replay_recorder->write_LoadSaveState(0);
    this->replay_recorder->write_ChangeGameSpeed(this->speed_multiplier);

    this->unlock_context();
}

std::vector<std::byte> GameboyContext::get_current_replay_recording_data() {
    if(!this->replay_recorder) {
        std::fputs("Can't get current replay recording data if no recorder is present!", stderr);
        std::terminate();
    }

    return this->replay_recorder->get_stream();
}

void GameboyContext::stop_replay_recording() {
    this->acquire_context();
    this->replay_recorder = {};
    this->unlock_context();
}

bool GameboyContext::is_recording_inner() const noexcept {
    return this->replay_recorder.get() != nullptr;
}

bool GameboyContext::is_playing_back_inner() const noexcept {
    return static_cast<bool>(this->current_playback);
}

bool GameboyContext::is_recording() noexcept {
    this->acquire_context();
    bool is_recording = this->is_recording_inner();
    this->unlock_context();
    return is_recording;
}

bool GameboyContext::is_playing_back() noexcept {
    this->acquire_context();
    bool is_playing_back = this->is_playing_back_inner();
    this->unlock_context();
    return is_playing_back;
}

void GameboyContext::start_replay_playback(const std::vector<std::byte> &replay) {
    auto replay_reader = ReplayReader(replay.data(), replay.size());

    bool error;
    auto collection = replay_reader.collect(error);
    if(!collection || collection->len() == 0) {
        return;
    }

    this->acquire_context();
    this->current_playback = std::move(collection);
    this->current_playback_offset = 0;
    auto first = (*this->current_playback)[0];
    this->current_playback_delay = first.get_delay();
    this->replay_states = {};
    this->play_latest_packet();
    this->unlock_context();
}

void GameboyContext::stop_replay_playback() {
    this->acquire_context();
    this->current_playback = {};
    this->unlock_context();
}

void GameboyContext::play_latest_packet() {
    if(!this->is_playing_back_inner() || this->current_playback_delay > 0) {
        return;
    }

    auto &collection = *this->current_playback;
    auto len = collection.len();
    if(this->current_playback_offset >= len) {
        std::terminate();
    }

    auto latest = collection[this->current_playback_offset];
    while(true) {
        switch(latest.get_packet_type()) {
            case RR_PacketType::RR_ChangeGameSpeed: {
                std::uint16_t new_speed;
                latest.read_ChangeGameSpeed(new_speed);
                this->handle_set_speed(new_speed);
                break;
            }
            case RR_PacketType::RR_AddSaveState: {
                std::span<const std::byte> data;
                std::uint32_t index;
                latest.read_AddSaveState(index, data);
                this->replay_states[index] = std::vector<std::uint8_t>(reinterpret_cast<const std::uint8_t *>(data.data()), reinterpret_cast<const std::uint8_t *>(data.data() + data.size()));
                break;
            }
            case RR_PacketType::RR_LoadSaveState: {
                std::uint32_t index;
                latest.read_LoadSaveState(index);
                auto state_maybe = this->replay_states.find(index);
                if(state_maybe == this->replay_states.end()) {
                    std::fprintf(stderr, "Tried to load a save state of an index %zu but no such state was given yet.", static_cast<std::size_t>(index));
                    break;
                }
                GB_load_state_from_buffer(this->gameboy.get(), state_maybe->second.data(), state_maybe->second.size());
                break;
            }
            case RR_PacketType::RR_SetInput8: {
                std::uint8_t input;
                latest.read_SetInput8(input);
                this->handle_new_input(input);
                break;
            }
        }

        this->current_playback_offset++;
        if(this->current_playback_offset >= len) {
            this->current_playback = {};
            return;
        }
        latest = collection[this->current_playback_offset];
        if((this->current_playback_delay = latest.get_delay()) > 0) {
            return;
        }
    }
}
