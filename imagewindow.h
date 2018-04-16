#ifndef IMAGEWINDOW_H
#define IMAGEWINDOW_H

#include "puansonchecker.h"
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QScrollBar>
#include <QSlider>

#include <QDebug>

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

            if(graphics_view)
                graphics_view->slider_wheel_move_flag = true;

            QScrollBar::wheelEvent(e);
        }

        void keyPressEvent(QKeyEvent *ev)
        {
            qDebug() << "in " << __PRETTY_FUNCTION__;

            ImageGraphicsView *graphics_view = (dynamic_cast<ImageGraphicsView *>(parent()->parent()));

            if(graphics_view)
                graphics_view->slider_wheel_move_flag = true;

            QScrollBar::keyPressEvent(ev);
        }
    };

    Q_OBJECT

public:
    ImageGraphicsView(QWidget *parent = Q_NULLPTR):QGraphicsView(parent)
    {
        slider_pressed_flag = false;
        slider_wheel_move_flag = false;

        setVerticalScrollBar(new ImageGraphicsScrollBar(this));
        setHorizontalScrollBar(new ImageGraphicsScrollBar(this));
    }

    ImageGraphicsView(QGraphicsScene *scene, QWidget *parent = Q_NULLPTR):QGraphicsView(scene, parent)
    {
        slider_pressed_flag = false;
        slider_wheel_move_flag = false;

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
        slider_wheel_move_flag = true;

        QGraphicsView::keyPressEvent(event);
    }

    void scrollContentsBy(int dx, int dy)
    {
        qDebug() << "in " << __PRETTY_FUNCTION__ << " slider_pressed_flag " << slider_pressed_flag;
        qDebug() << "in " << __PRETTY_FUNCTION__ << " slider_wheel_move_flag " << slider_wheel_move_flag;

        if(slider_pressed_flag || slider_wheel_move_flag)
        {
            ImageWindow *window = dynamic_cast<ImageWindow *>(parent());

            if(window == NULL)
                window = dynamic_cast<ImageWindow *>(parent()->parent());

            if(window)
                PuansonChecker::getInstance()->moveImages(dx, dy, window, true);

            slider_wheel_move_flag = false;
        }

        QGraphicsView::scrollContentsBy(dx, dy);
    }

private:
    bool slider_pressed_flag;
    bool slider_wheel_move_flag;
};

#endif // IMAGEWINDOW_H
