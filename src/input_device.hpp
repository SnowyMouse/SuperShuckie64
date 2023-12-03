#ifndef SS64_INPUT_DEVICE_HPP
#define SS64_INPUT_DEVICE_HPP

#include <memory>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <SDL2/SDL.h>
#include <optional>

namespace SuperShuckie64 {
    struct InputState {
        std::uint8_t input = 0;
        std::uint8_t input_toggle = 0;
        std::uint8_t input_rapid_fire = 0;

        bool reset_console = false;
        bool pause = false;

        float turbo = 0.0;
        float slow = 0.0;
    };

    // Corresponds to an input type
    enum InputType {
        Controller,
        Keyboard
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
        SettingsValues_ResetConsole,
        SettingsValues_Pause,
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
        virtual void restore_default_settings() noexcept {};

        // Settings for all buttons
        Settings settings_button;

        // Settings for all analog inputs (positive)
        Settings settings_analog_positive;

        // Settings for all analog inputs (negative)
        Settings settings_analog_negative;
    };

    class ControllerInputDevice : public InputDevice {
    public:
        ControllerInputDevice(SDL_GameController *what);
        void register_input(InputState &state, const SDL_ControllerButtonEvent &button, bool on) noexcept;
        void register_input(InputState &state, const SDL_ControllerAxisEvent &axis) noexcept;
        void restore_default_settings() noexcept override;
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

    class KeyboardInputDevice: public InputDevice {
    public:
        KeyboardInputDevice();
        void register_input(InputState &response, std::uint8_t button, bool on) noexcept;
        const char *get_name() const noexcept override {
            return "Keyboard";
        }
        const std::string &get_name_settings() const noexcept override {
            return this->name_settings;
        }
    private:
        std::string name_settings;
    };

    enum KeyboardButton : std::uint8_t {
        Keyboard_Button_A,
        Keyboard_Button_B,
        Keyboard_Button_C,
        Keyboard_Button_D,
        Keyboard_Button_E,
        Keyboard_Button_F,
        Keyboard_Button_G,
        Keyboard_Button_H,
        Keyboard_Button_I,
        Keyboard_Button_J,
        Keyboard_Button_K,
        Keyboard_Button_L,
        Keyboard_Button_M,
        Keyboard_Button_N,
        Keyboard_Button_O,
        Keyboard_Button_P,
        Keyboard_Button_Q,
        Keyboard_Button_R,
        Keyboard_Button_S,
        Keyboard_Button_T,
        Keyboard_Button_U,
        Keyboard_Button_V,
        Keyboard_Button_W,
        Keyboard_Button_X,
        Keyboard_Button_Y,
        Keyboard_Button_Z,

        Keyboard_Button_0,
        Keyboard_Button_1,
        Keyboard_Button_2,
        Keyboard_Button_3,
        Keyboard_Button_4,
        Keyboard_Button_5,
        Keyboard_Button_6,
        Keyboard_Button_7,
        Keyboard_Button_8,
        Keyboard_Button_9,

        Keyboard_Button_Numpad_0,
        Keyboard_Button_Numpad_1,
        Keyboard_Button_Numpad_2,
        Keyboard_Button_Numpad_3,
        Keyboard_Button_Numpad_4,
        Keyboard_Button_Numpad_5,
        Keyboard_Button_Numpad_6,
        Keyboard_Button_Numpad_7,
        Keyboard_Button_Numpad_8,
        Keyboard_Button_Numpad_9,

        Keyboard_Button_Space,
        Keyboard_Button_Shift,
        Keyboard_Button_Return,

        Keyboard_Button_Left,
        Keyboard_Button_Down,
        Keyboard_Button_Up,
        Keyboard_Button_Right,
    };

    const char *keyboard_button_name(KeyboardButton button) noexcept;
    std::optional<KeyboardButton> qt_keycode_to_keyboard_button(int key) noexcept;
}

#endif
