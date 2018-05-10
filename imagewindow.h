#ifndef IMAGEWINDOW_H
#define IMAGEWINDOW_H

#include "puansonchecker.h"
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QScrollBar>
#include <QSlider>
#include <QKeyEvent>

#define GRAPHICAL_POINT_RADIUS 12

class PuansonChecker;

class ImageWindow
{
public:
    explicit ImageWindow(PuansonChecker *checker);
    ~ImageWindow();

    virtual void moveImage(const qreal dx, const qreal dy) = 0;
    virtual void setImageCursor(const QCursor &cursor) = 0;

    virtual void mousePressEvent(const QPoint &p){ Q_UNUSED(p); }
    virtual void mouseReleaseEvent(const QPoint &p){ Q_UNUSED(p); }
    virtual void mouseMoveEvent(QMouseEvent *event){ Q_UNUSED(event); }

    virtual bool wheelEvent(int delta){ Q_UNUSED(delta); return false; }

    virtual bool windowKeyPressEvent(QKeyEvent *event)
    {
        Q_UNUSED(event);

        return false;
    }

    inline void shiftImageCoords(const qreal dx, const qreal dy)
    {
        image_x -= dx;
        image_y -= dy;
    }

    inline CalibrationMode_e getCalibrationMode()
    {
        return calibration_mode;
    }

protected:
    PuansonChecker *checker;
    QGraphicsScene *scene;
    QGraphicsView *graphicsView;
    qreal image_x, image_y;
    // Только для окон эталона и текущей детали
    CalibrationMode_e calibration_mode;
private:
    int previousX;
    int previousY;
};

class ImageGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

    int previousX;
    int previousY;

    bool left_button_down;

    ImageWindow *window;

public:
    ImageGraphicsScene(ImageWindow *owner_window);

public slots:
    void mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void wheelEvent(QGraphicsSceneWheelEvent *wheelEvent);
};

class ImageGraphicsView : public QGraphicsView
{
    class ImageGraphicsScrollBar : public QScrollBar
    {
    public:
        ImageGraphicsScrollBar(QWidget *parent = Q_NULLPTR):QScrollBar(parent) { }
        ImageGraphicsScrollBar(Qt::Orientation orientation, QWidget *parent = Q_NULLPTR): QScrollBar(orientation, parent) { }

        void mousePressEvent(QMouseEvent *e)
        {
            ImageGraphicsView *graphics_view = (dynamic_cast<ImageGraphicsView *>(parent()->parent()));

            if(graphics_view)
                graphics_view->slider_pressed_flag = true;

            QScrollBar::mousePressEvent(e);
        }

        void mouseReleaseEvent(QMouseEvent *e)
        {
            ImageGraphicsView *graphics_view = (dynamic_cast<ImageGraphicsView *>(parent()->parent()));

            if(graphics_view)
                graphics_view->slider_pressed_flag = false;

            QScrollBar::mouseReleaseEvent(e);
        }

        void wheelEvent(QWheelEvent *e)
        {
            ImageGraphicsView *graphics_view = (dynamic_cast<ImageGraphicsView *>(parent()->parent()));
            ImageWindow *window = dynamic_cast<ImageWindow *>(graphics_view->parent());
            bool window_event_processed = false;

            if(window == NULL)
                window = dynamic_cast<ImageWindow *>(graphics_view->parent()->parent());

            if(window)
                window_event_processed = window->wheelEvent(e->delta());

            if(!window_event_processed)
            {
                if(graphics_view)
                    graphics_view->mouse_wheel_or_key_pressed_flag = true;

                QScrollBar::wheelEvent(e);
            }
        }

        void keyPressEvent(QKeyEvent *ev)
        {
            ImageGraphicsView *graphics_view = (dynamic_cast<ImageGraphicsView *>(parent()->parent()));

            graphics_view->scene();

            if(graphics_view)
                graphics_view->mouse_wheel_or_key_pressed_flag = true;

            QScrollBar::keyPressEvent(ev);
        }
    };

    Q_OBJECT

public:
    ImageGraphicsView(QWidget *parent = Q_NULLPTR):QGraphicsView(parent)
    {
        slider_pressed_flag = false;
        mouse_wheel_or_key_pressed_flag = false;

        setVerticalScrollBar(new ImageGraphicsScrollBar(this));
        setHorizontalScrollBar(new ImageGraphicsScrollBar(this));
    }

    ImageGraphicsView(QGraphicsScene *scene, QWidget *parent = Q_NULLPTR):QGraphicsView(scene, parent)
    {
        slider_pressed_flag = false;
        mouse_wheel_or_key_pressed_flag = false;

        setVerticalScrollBar(new ImageGraphicsScrollBar(this));
        setHorizontalScrollBar(new ImageGraphicsScrollBar(this));
    }

    ~ImageGraphicsView()
    {
        delete verticalScrollBar();
        delete horizontalScrollBar();
    }

    void keyPressEvent(QKeyEvent *event)
    {
        ImageWindow *window = dynamic_cast<ImageWindow *>(parent());
        bool window_key_press_event_processed = false;

        if(window == NULL)
            window = dynamic_cast<ImageWindow *>(parent()->parent());

        if(window)
            window_key_press_event_processed = window->windowKeyPressEvent(event);

        if(!window_key_press_event_processed)
        {
            if(
                event->key() == Qt::Key_Up ||
                event->key() == Qt::Key_Down ||
                event->key() == Qt::Key_Right ||
                event->key() == Qt::Key_Left ||
                event->key() == Qt::Key_PageUp ||
                event->key() == Qt::Key_PageDown
            )
                mouse_wheel_or_key_pressed_flag = true;

            QGraphicsView::keyPressEvent(event);
        }
    }

    void scrollContentsBy(int dx, int dy)
    {
        if(slider_pressed_flag || mouse_wheel_or_key_pressed_flag)
        {
            ImageWindow *window = dynamic_cast<ImageWindow *>(parent());

            if(window == NULL)
                window = dynamic_cast<ImageWindow *>(parent()->parent());

            if(window)
                PuansonChecker::getInstance()->moveImages(dx, dy, window, true);

            mouse_wheel_or_key_pressed_flag = false;
        }

        QGraphicsView::scrollContentsBy(dx, dy);
    }

private:
    bool slider_pressed_flag;
    bool mouse_wheel_or_key_pressed_flag;
};

#endif // IMAGEWINDOW_H
