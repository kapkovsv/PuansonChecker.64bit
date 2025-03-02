#ifndef CURRENTFORM_H
#define CURRENTFORM_H

#include "types.h"
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

Q_DECLARE_METATYPE(ImageMoveMode_e)

class CurrentFormGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

    int previousX;
    int previousY;

    CurrentForm *window;
    QMenu scene_context_menu;

    bool left_button_down;
public:
    CurrentFormGraphicsScene(CurrentForm *owner_window);

    void contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent) Q_DECL_OVERRIDE;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE;
    void wheelEvent(QGraphicsSceneWheelEvent *event) Q_DECL_OVERRIDE;

public slots:
    void sceneContextMenuTriggeredSlot(QAction *action);
};

class CurrentForm : public QWidget, public ImageWindow
{
    Q_OBJECT

public:
    explicit CurrentForm(PuansonChecker *checker);
    ~CurrentForm() Q_DECL_OVERRIDE;

    void moveImage(const qreal dx, const qreal dy) Q_DECL_OVERRIDE;
    void drawImage(const QImage &img);
    void removeReferencePoints();
    void drawReferencePoints();
    void drawReferencePointAutoSearchArea(const ReferencePointType_e reference_point_type, const QRect &search_area);
    void setImageCursor(const QCursor &cursor) Q_DECL_OVERRIDE;
    void loadImageFinished();

    void setLabel2Text(const QString &text);

    void drawActualBorders();

    void mousePressEvent(const QPoint &p) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(const QPoint &p) Q_DECL_OVERRIDE;

    bool windowKeyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

    void setCalibrationMode(CalibrationMode_e mode);

    inline void setImageMoveMode(const ImageMoveMode_e mode)
    {
        image_move_mode = mode;
    }

    inline ImageMoveMode_e getImageMoveMode() const
    {
        return image_move_mode;
    }

public slots:
//    void calculateContourOnRotationCheckBoxStateChanged(int state);
    void shotAndLoadButtonPressedSlot();
    void setReferencePointsManuallyButtonPressedSlot();
    void setReferencePointsAutomaticButtonPressedSlot();
    void cameraConnectionStatusChangedSlot(bool connected);

private:
    Ui::CurrentForm *ui;
    ImageMoveMode_e image_move_mode;

    QPoint reference_point1;
    QPoint reference_point2;

    QGraphicsRectItem *reference_point_1_auto_search_area_ideal_item = Q_NULLPTR;
    QGraphicsRectItem *reference_point_2_auto_search_area_ideal_item = Q_NULLPTR;

    QGraphicsPixmapItem *pixmap_item = Q_NULLPTR;
};

class CurrentImageGraphicsView : public ImageGraphicsView
{
    Q_OBJECT

    double current_rotate_angle;
public:
    CurrentImageGraphicsView(QWidget *parent = Q_NULLPTR):ImageGraphicsView(parent), current_rotate_angle(0.0) { }
    CurrentImageGraphicsView(QGraphicsScene *scene, QWidget *parent = Q_NULLPTR):ImageGraphicsView(scene, parent) { }
    ~CurrentImageGraphicsView() Q_DECL_OVERRIDE { }

    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE
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

        QGraphicsView::wheelEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE
    {
        ImageGraphicsView::keyPressEvent(event);
    }
};

#endif // CURRENTFORM_H
