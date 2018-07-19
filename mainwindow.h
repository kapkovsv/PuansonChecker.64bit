#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "imagewindow.h"
#include <QMainWindow>
#include <QGraphicsScene>
#include <QImage>

class PuansonChecker;
class ImageWindow;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow, public ImageWindow
{
    Q_OBJECT

    friend class ImageGraphicsScene;
private:
    void removeReferencePointsAndIdealContour();
    void drawReferencePoints();
    void drawReferencePointsAndIdealContour();
    void drawReferenceAndInnerSkeletonPoints();
public:
    explicit MainWindow();
    ~MainWindow();

    void drawImage(const QImage &img);

    void moveImage(const qreal dx, const qreal dy);
    void setImageCursor(const QCursor &cursor) Q_DECL_OVERRIDE;
    void setWindowStatus(const QString &status);
    void setCalibrationMode(CalibrationMode_e mode);

    void drawActualBorders();

    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    void mousePressEvent(const QPoint &p) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(const QPoint &p) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    bool wheelEvent(int delta) Q_DECL_OVERRIDE;

    bool windowKeyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

    void drawIdealContour();

    void loadImageFinished(const ImageType_e image_type, quint8 etalon_angle);

public slots:
    void menuLoadEtalon1ActionTriggered();
    void menuLoadEtalon2ActionTriggered();
    void menuLoadEtalon3ActionTriggered();
    void menuLoadEtalon4ActionTriggered();
    void menuLoadEtalon5ActionTriggered();
    void menuLoadCurrentActionTriggered();

    void menuShootAndLoadEtalonActionTriggered();
    void menuShootAndLoadCurrentActionTriggered();

    void menuAngle1ActionTriggered();
    void menuAngle2ActionTriggered();
    void menuAngle3ActionTriggered();
    void menuAngle4ActionTriggered();
    void menuAngle5ActionTriggered();
    void menuGotoCurrentWindowTriggered();
    void menuGotoContoursWindowTriggered();
    void menuSettingsActionTriggered();
    void menuSetEtalonReferencePointsActionTriggered();
    void menuSetCurrentReferencePointsActionTriggered();
    void menuImposeIdealContourToEtalonActionTriggered();
    void menuReferencePointsAutoSearchActionTriggered();
    void menuSaveCurrentTriggered();
    void menuSaveResultTriggered();
    void menuExitActionTriggered();

    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    Ui::MainWindow *ui;
    QAction *angle_actions[NUMBER_OF_ANGLES];
    quint8 etalon_angle;

    QPoint reference_point1;
    QPoint reference_point2;

    bool ideal_impose_mouse_pressed = false;
    QPoint ideal_impose_previous_point;
    QPoint ideal_origin_point;
    double ideal_rotate_angle = 0.0;
    QGraphicsPathItem *ideal_item = Q_NULLPTR;
};

#endif // MAINWINDOW_H
