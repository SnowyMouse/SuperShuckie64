#ifndef SS64_INPUT_DEVICE_HPP
#define SS64_INPUT_DEVICE_HPP

#include <memory>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <SDL2/SDL.h>

namespace SuperShuckie64 {
    struct InputState {
        std::uint8_t input = 0;
        std::uint8_t input_toggle = 0;
        std::uint8_t input_rapid_fire = 0;

        bool reset_console = false;

        float turbo = 0.0;
        float slow = 0.0;
    };

    // Corresponds to an input type
    enum InputType {
        Controller
    };

    enum ButtonType {
        Normal = 0,
        Toggle = 1,
        RapidFire = 2,
    };

    enum ControlType {
        Button = 0,
        AnalogPositive = 1,
        AnalogNegative = 2
    };

    // Corresponds to a control for the emulator
    enum SettingsValues : std::uint8_t {
        SettingsValues_None = 0,

        SettingsValues_A,
        SettingsValues_B,
        SettingsValues_Start,
        SettingsValues_Select,
        SettingsValues_Left,
        SettingsValues_Right,
        SettingsValues_Up,
        SettingsValues_Down,
        SettingsValues_ButtonStart = SettingsValues::SettingsValues_A,
        SettingsValues_ButtonEnd = 32,

        SettingsValues_Turbo,
        SettingsValues_Slow,

        SettingsValues_ResetConsole
    };

    // Get the name of the setting, returning null if invalid
    const char *settings_value_to_name(SettingsValues value) noexcept;

    // Return true if the settings value in particular has
    bool settings_value_has_turbo_and_toggle(SettingsValues value);

    // Maps input indices to a SettingsValues setting. Note that only emulated buttons are respected for toggles/rapid fire (e.g. you can't rapid fire save state).
    struct Settings {
        std::unordered_map<std::uint8_t, std::uint8_t> input;
        std::unordered_map<std::uint8_t, std::uint8_t> input_toggle;
        std::unordered_map<std::uint8_t, std::uint8_t> input_rapid_fire;

        static Settings deserialize(const std::vector<std::uint8_t> &what);
        std::vector<std::uint8_t> serialize() const;
    };

    class InputDevice {
    public:
        virtual const char *get_name() const = 0;
        virtual const std::string &get_name_settings() const = 0;

        // Settings for all buttons
        Settings settings_button;

        // Settings for all analog inputs (positive)
        Settings settings_analog_positive;

        // Settings for all analog inputs (negative)
        Settings settings_analog_negative;
    };

    class KeyboardInputDevice : public InputDevice {
    public:
        const char *get_name() const noexcept override {
            return this->name_settings.c_str();
        }

        const std::string &get_name_settings() const noexcept override {
            return this->name_settings;
        }

    private:
        std::string name_settings = "keyboard";
    };

    class ControllerInputDevice : public InputDevice {
    public:
        ControllerInputDevice(SDL_GameController *what) noexcept;
        void register_input(InputState &state, const SDL_ControllerButtonEvent &button, bool on) noexcept;
        void register_input(InputState &state, const SDL_ControllerAxisEvent &axis) noexcept;
        void restore_default_settings() noexcept;
        SDL_JoystickID get_joystick_id() const noexcept {
            return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(this->what));
        }

        const char *get_name() const noexcept override {
            const char *n = SDL_GameControllerName(this->what);
            return n ? n : "null-controller";
        }

        const std::string &get_name_settings() const noexcept override {
            return this->name_settings;
        }

        // Lower deadzone (anything less than this is treated as 0)
        float lower_dead_zone = 0.05;

        // Upper deadzone (anything higher than this is treated as 1)
        float upper_dead_zone = 0.95;

        // Threshold where a axis is treated as on for the case of digital input
        float on_threshold = 0.5;
    private:
        // Last axis values for all axis
        float axis_values[256] = {};

        // SDL handle
        SDL_GameController *what;

        // Name of the controller's settings
        std::string name_settings;
    };
}

#endif
