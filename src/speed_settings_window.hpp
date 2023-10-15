#ifndef SS64_SPEED_SETTINGS_WINDOW_HPP
#define SS64_SPEED_SETTINGS_WINDOW_HPP

#include <QDialog>

class QSpinBox;

namespace SuperShuckie64 {
    class SpeedSettingsWindow: public QDialog {
    Q_OBJECT
    public:
        SpeedSettingsWindow(double base, double turbo, double slow);

        double get_base_speed() const noexcept;
        double get_turbo_speed() const noexcept;
        double get_slow_speed() const noexcept;

    private:
        QSpinBox *base_speed, *turbo_speed, *slow_speed;
    };
}

#endif
