#ifndef SS64_SPEED_SETTINGS_WINDOW_HPP
#define SS64_SPEED_SETTINGS_WINDOW_HPP

#include <QDialog>
#include <QLabel>

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
        QLabel *base_speed_text, *turbo_speed_text, *slow_speed_text;

    private slots:
        void fixup_speed_text() noexcept;
    };
}

#endif
