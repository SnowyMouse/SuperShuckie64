#include "speed_settings_window.hpp"
#include <QGridLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

using namespace SuperShuckie64;

static void fix_spin_box(QSpinBox *spinbox) {
    spinbox->setSuffix("%");
    spinbox->setMinimum(0);
    spinbox->setMaximum(9999);
}

SpeedSettingsWindow::SpeedSettingsWindow(double base, double turbo, double slow) : QDialog() {
    this->setWindowTitle("Speed settings");

    auto *layout = new QGridLayout(this);

    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 1);

    layout->addWidget(new QLabel("Base speed", this), 0, 0);
    layout->addWidget(new QLabel("Turbo speed multiplier", this), 1, 0);
    layout->addWidget(new QLabel("Slow speed multiplier", this), 2, 0);

    layout->addWidget(this->base_speed = new QSpinBox(this), 0, 1);
    layout->addWidget(this->turbo_speed = new QSpinBox(this), 1, 1);
    layout->addWidget(this->slow_speed = new QSpinBox(this), 2, 1);

    layout->addWidget(this->base_speed_text = new QLabel("", this), 0, 2);
    layout->addWidget(this->turbo_speed_text = new QLabel("", this), 1, 2);
    layout->addWidget(this->slow_speed_text = new QLabel("", this), 2, 2);

    fix_spin_box(this->base_speed);
    fix_spin_box(this->turbo_speed);
    fix_spin_box(this->slow_speed);

    this->base_speed->setValue(9999);
    this->turbo_speed->setValue(9999);
    this->slow_speed->setValue(9999);

    this->fixup_speed_text();

    this->base_speed->setValue(base * 100.0);
    this->turbo_speed->setValue(turbo * 100.0);
    this->slow_speed->setValue(slow * 100.0);

    auto *save = new QPushButton("OK", this);
    connect(save, SIGNAL(clicked()), this, SLOT(accept()));
    layout->addWidget(save, 3, 0, 1, 3);

    this->setFixedSize(this->sizeHint());

    this->fixup_speed_text();

    connect(this->base_speed, SIGNAL(valueChanged(int)), this, SLOT(fixup_speed_text()));
    connect(this->turbo_speed, SIGNAL(valueChanged(int)), this, SLOT(fixup_speed_text()));
    connect(this->slow_speed, SIGNAL(valueChanged(int)), this, SLOT(fixup_speed_text()));
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

void SpeedSettingsWindow::fixup_speed_text() noexcept {
    double gameboy_speed = (125257647.0) / (2 * 1024 * 1024);
    double base_speed = this->get_base_speed() * gameboy_speed;
    double turbo_speed = this->get_turbo_speed() * base_speed;
    double slow_speed = this->get_slow_speed() * base_speed;

    char buffer[30];
    std::snprintf(buffer, sizeof(buffer), "= %0.01f FPS (%0.02fx)", base_speed, base_speed / gameboy_speed);
    this->base_speed_text->setText(buffer);
    std::snprintf(buffer, sizeof(buffer), "= %0.01f FPS (%0.02fx)", turbo_speed, turbo_speed / gameboy_speed);
    this->turbo_speed_text->setText(buffer);
    std::snprintf(buffer, sizeof(buffer), "= %0.01f FPS (%0.02fx)", slow_speed, slow_speed / gameboy_speed);
    this->slow_speed_text->setText(buffer);
}
