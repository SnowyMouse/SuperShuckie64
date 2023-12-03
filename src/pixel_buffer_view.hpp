#ifndef SS64_PIXEL_BUFFER_VIEW_HPP
#define SS64_PIXEL_BUFFER_VIEW_HPP

#include <QGraphicsView>

namespace SuperShuckie64 {
    class EmulatorWindow;

    class PixelBufferView : public QGraphicsView {
    public:
        PixelBufferView(QWidget *parent, EmulatorWindow &window);

        void dragEnterEvent(QDragEnterEvent *event) override;
        void dragMoveEvent(QDragMoveEvent *event) override;
        void dropEvent(QDropEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;
        void keyReleaseEvent(QKeyEvent *event) override;
    private:
        EmulatorWindow &window;
    };
}

#endif
