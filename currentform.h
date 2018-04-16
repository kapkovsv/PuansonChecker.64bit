#ifndef CURRENTFORM_H
#define CURRENTFORM_H

#include "imagewindow.h"
#include <QWidget>
#include <QGraphicsView>
#include <QWheelEvent>
#include <QMenu>
#include <QDebug>

class PuansonChecker;

namespace Ui {
class CurrentForm;
}

enum ImageMoveMode_e
{
    IMAGE_MOVE_VIEWING = 0,
    IMAGE_MOVE_EDITING
};

Q_DECLARE_METATYPE(ImageMoveMode_e)

class CurrentFormGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

    int previousX;
    int previousY;

    bool left_button_down;

    CurrentForm *window;
    QMenu scene_context_menu;

public:
    CurrentFormGraphicsScene(CurrentForm *owner_window);

    void contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent);
    void mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void wheelEvent(QGraphicsSceneWheelEvent *event);

public slots:
    void sceneContextMenuTriggeredSlot(QAction *action);
};

class CurrentForm : public QWidget, public ImageWindow
{
    Q_OBJECT

public:
    explicit CurrentForm(PuansonChecker *checker);
    ~CurrentForm();

    void moveImage(const qreal dx, const qreal dy);
    void drawImage(const QImage &img);
    void removeReferencePoints();
    void drawReferencePoints();
    void setImageCursor(const QCursor &cursor);
    void loadImageFinished();

    void setLabel2Text(const QString &text);

    void mousePressEvent(const QPoint &p);
    void mouseReleaseEvent(const QPoint &p);

    void keyPressEvent(QKeyEvent *event);

    void setCalibrationMode(CalibrationMode_e mode);

    inline void setImageMoveMode(const ImageMoveMode_e mode)
    {
        image_move_mode = mode;
    }

    inline ImageMoveMode_e getImageMoveMode()
    {
        return image_move_mode;
    }

public slots:
//    void calculateContourOnRotationCheckBoxStateChanged(int state);
    void shotAndLoadButtonPressedSlot();
    void setReferencePointsButtonPressedSlot();
    void cameraConnectionStatusChangedSlot(bool connected);

private:
    Ui::CurrentForm *ui;
    ImageMoveMode_e image_move_mode;

    QPoint reference_point1;
    QPoint reference_point2;
};

class CurrentImageGraphicsView : public ImageGraphicsView
{
    Q_OBJECT

    double current_rotate_angle;
public:
    CurrentImageGraphicsView(QWidget *parent = Q_NULLPTR):ImageGraphicsView(parent), current_rotate_angle(0.0) { }
    CurrentImageGraphicsView(QGraphicsScene *scene, QWidget *parent = Q_NULLPTR):ImageGraphicsView(scene, parent) { }
    ~CurrentImageGraphicsView(){ }

    void wheelEvent(QWheelEvent *event)
    {
        CurrentForm *window = dynamic_cast<CurrentForm *>(this->parent());

        if(window && window->getImageMoveMode() == IMAGE_MOVE_EDITING)
        {
            double rotate_angle = -event->angleDelta().ry() / 120.0 * 0.01;
            current_rotate_angle += rotate_angle;

            qDebug() << "Поворот на угол " + QString::number(current_rotate_angle) + QChar(176);
            window->setLabel2Text("Поворот на угол " + QString::number(current_rotate_angle) + QChar(176));
            PuansonChecker::getInstance()->rotateCurrentImage(rotate_angle);
            PuansonChecker::getInstance()->drawCurrentImage();
            PuansonChecker::getInstance()->drawContoursImage();
        }

        Q_UNUSED(event);
    }
};

#endif // CURRENTFORM_H
