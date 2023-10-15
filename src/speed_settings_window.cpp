#include "speed_settings_window.hpp"
#include <QGridLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

using namespace SuperShuckie64;

static void fix_spin_box(QSpinBox *spinbox, double value) {
    spinbox->setSuffix("%");
    spinbox->setMinimum(0);
    spinbox->setMaximum(25500);
    spinbox->setValue(value * 100.0);
}

SpeedSettingsWindow::SpeedSettingsWindow(double base, double turbo, double slow) : QDialog() {
    this->setWindowTitle("Speed settings");

    auto *layout = new QGridLayout(this);
    layout->addWidget(new QLabel("Base speed", this), 0, 0);
    layout->addWidget(new QLabel("Turbo speed modifier", this), 1, 0);
    layout->addWidget(new QLabel("Slow speed modifier", this), 2, 0);

    layout->addWidget(this->base_speed = new QSpinBox(this), 0, 1);
    layout->addWidget(this->turbo_speed = new QSpinBox(this), 1, 1);
    layout->addWidget(this->slow_speed = new QSpinBox(this), 2, 1);

    fix_spin_box(this->base_speed, base);
    fix_spin_box(this->turbo_speed, turbo);
    fix_spin_box(this->slow_speed, slow);

    auto *save = new QPushButton("OK", this);
    connect(save, SIGNAL(clicked()), this, SLOT(accept()));
    layout->addWidget(save, 3, 0, 1, 2);
}

double SpeedSettingsWindow::get_base_speed() const noexcept {
    return static_cast<double>(this->base_speed->value()) / 100.0;
}

double SpeedSettingsWindow::get_turbo_speed() const noexcept {
    return static_cast<double>(this->turbo_speed->value()) / 100.0;
}

double SpeedSettingsWindow::get_slow_speed() const noexcept {
    return static_cast<double>(this->slow_speed->value()) / 100.0;
}
