#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include "pixel_buffer_view.hpp"
#include "emulator_window.hpp"

using namespace SuperShuckie64;

// Make sure the extension is valid
template<typename T> static std::optional<std::filesystem::path> validate_event(T *event) {
    auto *d = event->mimeData();
    if(d->hasUrls()) {
        auto urls = d->urls();
        if(urls.length() == 1) {
            auto path = std::filesystem::path(urls[0].toLocalFile().toStdString());
            auto extension = path.extension().string();
            if(extension == ".gb" || extension == ".gbc") {
                return path;
            }
        }
    }
    return std::nullopt;
}

PixelBufferView::PixelBufferView(QWidget *parent, EmulatorWindow &window) : QGraphicsView(parent), window(window) {
    this->setAcceptDrops(true);
}

void PixelBufferView::dragEnterEvent(QDragEnterEvent *event) {
    if(validate_event(event)) {
        event->accept();
    }
}

void PixelBufferView::dragMoveEvent(QDragMoveEvent *event) {
    if(validate_event(event)) {
        event->accept();
    }
}

void PixelBufferView::dropEvent(QDropEvent *event) {
    auto path = validate_event(event);
    if(path) {
        window.load_and_start_rom(*path);
    }
}

void PixelBufferView::keyPressEvent(QKeyEvent *event) {
    if(event->isAutoRepeat()) {
        return;
    }

    auto key_maybe = qt_keycode_to_keyboard_button(event->key());
    if(key_maybe != std::nullopt) {
        std::printf("...on\n");
        this->window.handle_keyboard_event(*key_maybe, true);
    }
}

void PixelBufferView::keyReleaseEvent(QKeyEvent *event) {
    if(event->isAutoRepeat()) {
        return;
    }

    auto key_maybe = qt_keycode_to_keyboard_button(event->key());
    if(key_maybe != std::nullopt) {
        std::printf("...off\n");
        this->window.handle_keyboard_event(*key_maybe, false);
    }
}
