#include "imagewindow.h"
#include <QGraphicsPixmapItem>
#include <QScrollBar>

ImageWindow::ImageWindow(PuansonChecker *checker):
    checker(checker),
    scene(new ImageGraphicsScene(this))
{
    image_x = 0;
    image_y = 0;

    calibration_mode = NO_CALIBRATION;
}

ImageWindow::~ImageWindow()
{
    delete scene;
}

ImageGraphicsScene::ImageGraphicsScene(ImageWindow *owner_window):
    previousX(0), previousY(0), left_button_down(false), window(owner_window)
{
}

void ImageGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if(window)
    {
        if(left_button_down)
        {
            static int x = 0;
            static int y = 0;

            if(y != mouseEvent->screenPos().y() || x != mouseEvent->screenPos().x())
            {
                if(x != mouseEvent->screenPos().x())
                    x = mouseEvent->screenPos().x();
                if(y != mouseEvent->screenPos().y())
                    y = mouseEvent->screenPos().y();

                qreal dx = x - previousX;
                qreal dy = y - previousY;

                PuansonChecker::getInstance()->moveImages(dx, dy);
                previousX = x;
                previousY = y;
            }
        }
        else
        {
            QMouseEvent event(mouseEvent->type(), mouseEvent->scenePos(), mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers());
            window->mouseMoveEvent(&event);
        }
    }
}

void ImageGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if(mouseEvent->buttons() & Qt::LeftButton)
    {
        if(window->getCalibrationMode() != NO_CALIBRATION)
        {
            window->mousePressEvent(mouseEvent->scenePos().toPoint());
        }
        else
        {
            left_button_down = true;
            PuansonChecker::getInstance()->setIgnoreScrollMoveImage(true);
            previousX = mouseEvent->screenPos().x();
            previousY = mouseEvent->screenPos().y();
            window->setImageCursor(Qt::SizeAllCursor);
        }
    }
}

void ImageGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    left_button_down = false;
    previousX = 0;
    previousY = 0;

    if(window->getCalibrationMode() == NO_CALIBRATION)
    {
        PuansonChecker::getInstance()->setIgnoreScrollMoveImage(false);
        window->setImageCursor(Qt::ArrowCursor);
    }
    else
    {
        window->mouseReleaseEvent(mouseEvent->pos().toPoint());
    }
}

void ImageGraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *wheelEvent)
{
    if(window->getCalibrationMode() == NO_CALIBRATION)
    {
    }
    else
    {
        int delta = wheelEvent->delta();

        if(wheelEvent->modifiers() & Qt::ControlModifier)
            delta = qRound(delta / 10.0);

        if(wheelEvent->modifiers() & Qt::ShiftModifier)
            delta *= 100;

        if(wheelEvent->buttons() & Qt::RightButton)
            delta *= 10;

        window->wheelEvent(delta);
    }
}
