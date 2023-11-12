#include <QGridLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include "emulator_window.hpp"
#include "controls_settings_window.hpp"
#include "input_device.hpp"

using namespace SuperShuckie64;

ControlsSettingsWindow::ControlsSettingsWindow(QWidget *parent, EmulatorWindow &emulator_window) : QDialog(parent), emulator_window(emulator_window) {
    auto *layout = new QVBoxLayout(this);

    this->selected_settings = new QComboBox(this);

    for(auto &i : this->emulator_window.input_devices) {
        auto name = i.second->get_name_settings();
        this->selected_settings->addItem(i.second->get_name(), name.c_str());
        this->all_settings[name] = SettingsTuple {
            .all_settings = {i.second->settings_button, i.second->settings_analog_positive, i.second->settings_analog_negative},
            .input_type = InputType::Controller
        };
    }

    layout->addWidget(this->selected_settings);

    this->container_widget = new QWidget(this);
    this->container_layout = new QVBoxLayout(this->container_widget);
    this->container_layout->setContentsMargins(0,0,0,0);
    this->regenerate_controls_container();
    layout->addWidget(this->container_widget);
    auto *save = new QPushButton("OK", this->container_widget);
    connect(save, SIGNAL(pressed()), this, SLOT(accept()));
    layout->addWidget(save);
    this->container_widget->setFixedSize(this->container_widget->sizeHint());
}

static std::string input_name(std::uint8_t input, InputType input_type, ControlType control_type) {
    switch(input_type) {
        case InputType::Controller: {
            const char *name;
            const char *prefix;
            switch(control_type) {
                case Button:
                    name = SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(input));
                    prefix = "";
                    break;
                case AnalogNegative:
                    name = SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(input));
                    prefix = "-";
                    break;
                case AnalogPositive:
                    name = SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(input));
                    prefix = "+";
                    break;
            }
            if(!name) {
                name = "MISSINGNO.";
            }
            return std::string(name) + prefix;
            break;
        }
        default: std::terminate();
    }
}

static std::string concatenate_input_names_of_type(SettingsValues type, SettingsTuple &settings, ButtonType button_type) {
    std::string all_inputs;

    static_assert(sizeof(settings.all_settings) == sizeof(settings.all_settings[0]) * 3);
    for(std::size_t input = 0; input < 3; input++) {
        auto &i = settings.all_settings[input];
        std::unordered_map<std::uint8_t, std::uint8_t> *selection;
        switch(button_type) {
            case ButtonType::Normal: selection = &i.input; break;
            case ButtonType::RapidFire: selection = &i.input_rapid_fire; break;
            case ButtonType::Toggle: selection = &i.input_toggle; break;
            default: std::terminate();
        }
        for(auto &i : *selection) {
            if(i.second == type) {
                if(!all_inputs.empty()) {
                    all_inputs += ", ";
                }
                all_inputs += input_name(i.first, settings.input_type, static_cast<ControlType>(input));
            }
        }
    }

    return all_inputs;
}

ControlSettingsField::ControlSettingsField(QWidget *parent, ControlsSettingsWindow &settings_window, ButtonType button_type, SettingsTuple &setting, SettingsValues value)
    : QLineEdit(parent), settings_window(settings_window), button_type(button_type), value(value) {

    this->setText(concatenate_input_names_of_type(value, setting, button_type).c_str());
    this->setFixedWidth(125);
}

void ControlsSettingsWindow::regenerate_controls_container() {
    this->selected_control_setting_box = nullptr;
    delete this->controls_container;
    this->controls_container = new QWidget(this->container_widget);

    auto &setting = this->all_settings[this->selected_settings->currentData().toString().toStdString()];

    auto *grid = new QGridLayout(this->controls_container);
    grid->setContentsMargins(0,0,0,0);
    this->controls_container->setLayout(grid);

    grid->addWidget(new QLabel("Input", this->controls_container), 0, 1);
    grid->addWidget(new QLabel("Rapid-Fire", this->controls_container), 0, 2);
    grid->addWidget(new QLabel("Toggle", this->controls_container), 0, 3);

    // Maybe have the settings be comma-separated
    for(SettingsValues i = static_cast<SettingsValues>(0); i < 255; i = static_cast<SettingsValues>(i + 1)) {
        auto *name = settings_value_to_name(i);
        if(!name) {
            continue;
        }

        int row = grid->rowCount();
        grid->addWidget(new QLabel(name, this->controls_container), row, 0);

        auto *input = new ControlSettingsField(this->controls_container, *this, ButtonType::Normal, setting, i);
        grid->addWidget(input, row, 1);

        if(i >= SettingsValues::SettingsValues_ButtonStart && i < SettingsValues::SettingsValues_ButtonEnd) {
            auto *rapid_fire = new ControlSettingsField(this->controls_container, *this, ButtonType::RapidFire, setting, i);
            grid->addWidget(rapid_fire, row, 2);

            auto *toggle = new ControlSettingsField(this->controls_container, *this, ButtonType::Toggle, setting, i);
            grid->addWidget(toggle, row, 3);
        }
    }

    this->container_layout->addWidget(this->controls_container);
}

void ControlsSettingsWindow::handle_input(std::uint8_t input, ControlType control_type) {
    if(!this->selected_control_setting_box) {
        return;
    }

    auto selected_item = this->selected_settings->currentData().toString().toStdString();
    auto &tuple = this->all_settings[selected_item];
    auto &i = tuple.all_settings[control_type];

    std::unordered_map<std::uint8_t, std::uint8_t> *selection;
    switch(this->selected_control_setting_box->button_type) {
        case ButtonType::Normal: selection = &i.input; break;
        case ButtonType::RapidFire: selection = &i.input_rapid_fire; break;
        case ButtonType::Toggle: selection = &i.input_toggle; break;
    }

    i.input.erase(input);
    i.input_rapid_fire.erase(input);
    i.input_toggle.erase(input);

    (*selection)[input] = this->selected_control_setting_box->value;

    this->regenerate_controls_container();
}

void ControlsSettingsWindow::on_device_input(SDL_ControllerButtonEvent &event, ControllerInputDevice &device) {
    auto selected_item = this->selected_settings->currentData().toString().toStdString();
    if(selected_item != device.get_name_settings()) {
        return;
    }
    this->handle_input(event.button, ControlType::Button);
}

void ControlsSettingsWindow::on_device_input(SDL_ControllerAxisEvent &event, ControllerInputDevice &device) {
    if(std::abs(event.value) < 16384) {
        return;
    }
    auto selected_item = this->selected_settings->currentData().toString().toStdString();
    if(selected_item != device.get_name_settings()) {
        return;
    }
    this->handle_input(event.axis, event.value > 0 ? ControlType::AnalogPositive : ControlType::AnalogNegative);
}

void ControlSettingsField::mousePressEvent(QMouseEvent *e) {
    QLineEdit::mousePressEvent(e);
    this->settings_window.selected_control_setting_box = this;
}
