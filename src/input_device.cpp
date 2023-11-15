extern "C" {
    #include <gb.h>
}
#include "input_device.hpp"
#include <algorithm>


using namespace SuperShuckie64;

ControllerInputDevice::ControllerInputDevice(SDL_GameController *what) noexcept : what(what) {
    const char *n = this->get_name();
    this->name_settings = n;
    for(char &c : this->name_settings) {
        if(c == '/' || c == '\\' || c == '.') {
            c = '_';
        }
    }
    this->name_settings = std::string("controller-") + name_settings;
}

void ControllerInputDevice::restore_default_settings() noexcept {
    this->settings_button = {};
    this->settings_analog_positive = {};
    this->settings_analog_negative = {};

    this->settings_button.input[SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A] = SettingsValues::SettingsValues_A;
    this->settings_button.input[SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B] = SettingsValues::SettingsValues_B;
    this->settings_button.input[SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START] = SettingsValues::SettingsValues_Start;
    this->settings_button.input[SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_BACK] = SettingsValues::SettingsValues_Select;
    this->settings_button.input[SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT] = SettingsValues::SettingsValues_Left;
    this->settings_button.input[SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = SettingsValues::SettingsValues_Right;
    this->settings_button.input[SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP] = SettingsValues::SettingsValues_Up;
    this->settings_button.input[SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN] = SettingsValues::SettingsValues_Down;
    this->settings_button.input[SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_GUIDE] = SettingsValues::SettingsValues_ResetConsole;

    this->settings_button.input_rapid_fire[SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = SettingsValues::SettingsValues_B;
    this->settings_button.input_rapid_fire[SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = SettingsValues::SettingsValues_A;

    this->settings_analog_positive.input[SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT] = SettingsValues::SettingsValues_Slow;
    this->settings_analog_positive.input[SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT] = SettingsValues::SettingsValues_Turbo;

    this->lower_dead_zone = 0.05;
    this->upper_dead_zone = 0.95;
    this->on_threshold = 0.5;
}

static std::uint8_t generate_gb_input_bitfield(std::uint8_t what) {
    switch(what) {
        case SettingsValues::SettingsValues_A:
            return GB_key_mask_t::GB_KEY_A_MASK;
        case SettingsValues::SettingsValues_B:
            return GB_key_mask_t::GB_KEY_B_MASK;
        case SettingsValues::SettingsValues_Start:
            return GB_key_mask_t::GB_KEY_START_MASK;
        case SettingsValues::SettingsValues_Select:
            return GB_key_mask_t::GB_KEY_SELECT_MASK;
        case SettingsValues::SettingsValues_Left:
            return GB_key_mask_t::GB_KEY_LEFT_MASK;
        case SettingsValues::SettingsValues_Right:
            return GB_key_mask_t::GB_KEY_RIGHT_MASK;
        case SettingsValues::SettingsValues_Up:
            return GB_key_mask_t::GB_KEY_UP_MASK;
        case SettingsValues::SettingsValues_Down:
            return GB_key_mask_t::GB_KEY_DOWN_MASK;
    }
    return 0;
}

static void apply_axis_logic(InputState &response, std::uint8_t input, float value) {
    switch(input) {
        case SettingsValues::SettingsValues_Turbo:
            response.turbo = value;
            break;
        case SettingsValues::SettingsValues_Slow:
            response.slow = value;
            break;
    }
}

static void apply_button_logic(InputState &response, std::uint8_t input, std::uint8_t input_rapid_fire, std::uint8_t input_toggle, bool on) {
    switch(input) {
        case SettingsValues::SettingsValues_ResetConsole: {
            response.reset_console = on;
            return;
        }
        default: break;
    }

    auto button_input = generate_gb_input_bitfield(input);
    auto button_input_rapid_fire = generate_gb_input_bitfield(input_rapid_fire);
    auto button_input_toggle = generate_gb_input_bitfield(input_toggle);

    if(on) {
        response.input |= button_input;
        response.input_rapid_fire |= button_input_rapid_fire;

        if(response.input_toggle & button_input_toggle) {
            response.input_toggle &= ~button_input_toggle;
        }
        else {
            response.input_toggle |= button_input_toggle;
        }
    }
    else {
        response.input &= ~button_input;
        response.input_rapid_fire &= ~button_input_rapid_fire;
    }
}

static void apply_button_logic_for_axis(InputState &response, float on_threshold, float axis_last, float axis_now, std::uint8_t input, std::uint8_t input_rapid_fire, std::uint8_t input_toggle) noexcept {
    if(axis_last < on_threshold && axis_now >= on_threshold) {
        apply_button_logic(response, input, input_rapid_fire, input_toggle, true);
    }
    else if(axis_last >= on_threshold && axis_now < on_threshold) {
        apply_button_logic(response, input, input_rapid_fire, input_toggle, false);
    }
}

void ControllerInputDevice::register_input(InputState &response, const SDL_ControllerButtonEvent &button, bool on) noexcept {
    auto input = this->settings_button.input[button.button];
    auto input_rapid_fire = this->settings_button.input_rapid_fire[button.button];
    auto input_toggle = this->settings_button.input_toggle[button.button];

    apply_button_logic(response, input, input_rapid_fire, input_toggle, on);
    apply_axis_logic(response, input, on ? 1.0 : 0.0);
}

void ControllerInputDevice::register_input(InputState &response, const SDL_ControllerAxisEvent &axis) noexcept {
    float value = std::clamp(axis.value / 32767.0, -1.0, 1.0);
    float abs = std::fabs(value);
    float prev = this->axis_values[axis.axis];
    float prev_abs = std::fabs(prev);

    if(abs < this->lower_dead_zone) {
        value = 0.0;
        abs = 0.0;
    }
    else {
        abs = static_cast<float>(std::min(1.0, (abs - this->lower_dead_zone) / this->upper_dead_zone / (1.0 - this->lower_dead_zone)));
    }

    this->axis_values[axis.axis] = value;

    float positive_value = value > 0.0 ? abs : 0.0;
    float negative_value = value < 0.0 ? abs : 0.0;
    float positive_value_prev = prev > 0.0 ? prev_abs : 0.0;
    float negative_value_prev = prev < 0.0 ? prev_abs : 0.0;

    auto input_positive = this->settings_analog_positive.input[axis.axis];
    auto input_rapid_fire_positive = this->settings_analog_positive.input_rapid_fire[axis.axis];
    auto input_toggle_positive = this->settings_analog_positive.input_toggle[axis.axis];

    auto input_negative = this->settings_analog_negative.input[axis.axis];
    auto input_rapid_fire_negative = this->settings_analog_negative.input_rapid_fire[axis.axis];
    auto input_toggle_negative = this->settings_analog_negative.input_toggle[axis.axis];

    apply_button_logic_for_axis(response, this->on_threshold, positive_value, positive_value_prev, input_positive, input_rapid_fire_positive, input_toggle_positive);
    apply_button_logic_for_axis(response, this->on_threshold, negative_value, negative_value_prev, input_negative, input_rapid_fire_negative, input_toggle_negative);

    apply_axis_logic(response, input_positive, positive_value);
    apply_axis_logic(response, input_negative, negative_value);
}

const char *SuperShuckie64::settings_value_to_name(SettingsValues value) noexcept {
    switch(value) {
        case SettingsValues_A:
            return "A";
        case SettingsValues_B:
            return "B";
        case SettingsValues_Start:
            return "Start";
        case SettingsValues_Select:
            return "Select";
        case SettingsValues_Left:
            return "Left";
        case SettingsValues_Right:
            return "Right";
        case SettingsValues_Up:
            return "Up";
        case SettingsValues_Down:
            return "Down";

        case SettingsValues_Turbo:
            return "Turbo";
        case SettingsValues_Slow:
            return "Slow";

        case SettingsValues_ResetConsole:
            return "Reset console";
    }

    return nullptr;
}

bool settings_value_has_turbo_and_toggle(SettingsValues value) {
    return value >= SettingsValues_ButtonStart && value <= SettingsValues_ButtonEnd;
}

Settings Settings::deserialize(const std::vector<std::uint8_t> &what) {
    std::vector<std::unordered_map<std::uint8_t, std::uint8_t>> maps;

    if(what.size() % 2 != 0 || what.size() < 2 || what[what.size() - 1] != 0xFF || what[what.size() - 2] != 0xFF) {
        std::fputs("Malformed settings detected; using null settings!", stderr);
        return Settings {};
    }

    auto *start = what.data();
    auto *end = start + what.size();
    auto *map = &maps.emplace_back();

    for(auto *i = start; i < end; i+=2) {
        auto k = i[0];
        auto v = i[1];

        if(k == 0xFF && v == 0xFF) {
            map = &maps.emplace_back();
            continue;
        }

        (*map)[k] = v;
    }

    maps.resize(3);
    return Settings {
        .input = maps[0],
        .input_toggle = maps[1],
        .input_rapid_fire = maps[2],
    };
}

static std::vector<std::uint8_t> serialize_u8u8(const std::unordered_map<std::uint8_t, std::uint8_t> &what) {
    std::vector<std::uint8_t> bytes;

    for(auto &i : what) {
        bytes.emplace_back(i.first);
        bytes.emplace_back(i.second);
    }

    bytes.emplace_back(0xFF);
    bytes.emplace_back(0xFF);

    return bytes;
}

std::vector<std::uint8_t> Settings::serialize() const {
    std::vector<std::uint8_t> what;

    auto input = serialize_u8u8(this->input);
    auto input_toggle = serialize_u8u8(this->input_toggle);
    auto input_rapid_fire = serialize_u8u8(this->input_rapid_fire);

    what.insert(what.end(), input.begin(), input.end());
    what.insert(what.end(), input_toggle.begin(), input_toggle.end());
    what.insert(what.end(), input_rapid_fire.begin(), input_rapid_fire.end());

    return what;
}
