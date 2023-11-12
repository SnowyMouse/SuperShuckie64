#ifndef SS64_CONTROLS_SETTINGS_WINDOW_HPP
#define SS64_CONTROLS_SETTINGS_WINDOW_HPP

#include <QDialog>
#include <QLineEdit>
#include "input_device.hpp"

class QComboBox;

namespace SuperShuckie64 {
    class EmulatorWindow;

    struct SettingsTuple { Settings all_settings[3]; InputType input_type; };

    class ControlSettingsField;

    class ControlsSettingsWindow : public QDialog {
        Q_OBJECT

        friend class ControlSettingsField;

    public:
        ControlsSettingsWindow(QWidget *parent, EmulatorWindow &emulator_window);
        std::unordered_map<std::string, SettingsTuple> all_settings;

    public slots:
        void on_device_input(SDL_ControllerButtonEvent &, ControllerInputDevice &);
        void on_device_input(SDL_ControllerAxisEvent &, ControllerInputDevice &);

    private:
        EmulatorWindow &emulator_window;
        QComboBox *selected_settings;

        QWidget *container_widget;
        QLayout *container_layout;
        QWidget *controls_container = nullptr;
        ControlSettingsField *selected_control_setting_box = nullptr;

        void handle_input(std::uint8_t input, ControlType control_type);
        void regenerate_controls_container();
    };

    class ControlSettingsField: public QLineEdit {
        Q_OBJECT
    public:
        ControlSettingsField(QWidget *parent, ControlsSettingsWindow &settings_window, ButtonType button_type, SettingsTuple &setting, SettingsValues value);

        void mousePressEvent(QMouseEvent *e) override;

        const ButtonType button_type;
        const SettingsValues value;
    private:
        ControlsSettingsWindow &settings_window;
    };
}

#endif
