#include "imagewindow.h"
#include <QGraphicsPixmapItem>
#include <QScrollBar>

ImageWindow::ImageWindow():
    scene(new ImageGraphicsScene(this))
{
    image_x = 0;
    image_y = 0;

    calibration_mode = CalibrationMode_e::NO_CALIBRATION;
}

ImageWindow::~ImageWindow()
{
    delete scene;
}

ImageGraphicsScene::ImageGraphicsScene(ImageWindow *owner_window):
    QGraphicsScene(),
    previousX(0), previousY(0), mouse_button_down(false), window(owner_window)
{
}

void ImageGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if(window)
    {
        if(mouse_button_down)
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
    if(mouseEvent->buttons() & Qt::LeftButton && window->getCalibrationMode() != CalibrationMode_e::NO_CALIBRATION)
    {
        window->mousePressEvent(mouseEvent->scenePos().toPoint());
    }
    else
    {
        mouse_button_down = true;
        PuansonChecker::getInstance()->setIgnoreScrollMoveImage(true);
        previousX = mouseEvent->screenPos().x();
        previousY = mouseEvent->screenPos().y();
        window->setImageCursor(Qt::SizeAllCursor);
    }
}

void ImageGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if(mouse_button_down)
    {
        mouse_button_down = false;
        previousX = 0;
        previousY = 0;
    }

    if((mouseEvent->button() == Qt::LeftButton && window->getCalibrationMode() == CalibrationMode_e::NO_CALIBRATION) || mouseEvent->button() == Qt::RightButton)
    {
        PuansonChecker::getInstance()->setIgnoreScrollMoveImage(false);
        window->setImageCursor(Qt::ArrowCursor);
    }
    else
    {
        window->mouseReleaseEvent(mouseEvent->pos().toPoint());
    }
}

void ImageGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if(mouseEvent->buttons() & Qt::LeftButton)
    {
        if(window->getCalibrationMode() != CalibrationMode_e::NO_CALIBRATION)
        {
            window->mouseDoubleClickEvent(mouseEvent->scenePos().toPoint());
        }
    }
}

void ImageGraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *wheelEvent)
{
    if(window->getCalibrationMode() == CalibrationMode_e::NO_CALIBRATION)
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

        window->wheelEvent(wheelEvent->modifiers() & Qt::ControlModifier, delta);
    }
}
