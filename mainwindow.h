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
    void drawImage(const QImage &img);

    void removeReferenceAndInnerSkeletonPoints();
    void drawReferencePoints();
    void drawInnerSkeletonPoints();
    void drawReferenceAndInnerSkeletonPoints();
public:
    explicit MainWindow(PuansonChecker *checker);
    ~MainWindow();

    void moveImage(const qreal dx, const qreal dy);
    void setImageCursor(const QCursor &cursor);
    void setWindowStatus(const QString &status);
    void setCalibrationMode(CalibrationMode_e mode);

    void mousePressEvent(const QPoint &p);
    void mouseReleaseEvent(const QPoint &p);

    void keyPressEvent(QKeyEvent *event);

    void loadImageFinished(const ImageType_e image_type, quint8 etalon_angle);

    void innerSkeletonPointsSetting();

public slots:
    void menuLoadEtalon1ActionTriggered();
    void menuLoadEtalon2ActionTriggered();
    void menuLoadEtalon3ActionTriggered();
    void menuLoadEtalon4ActionTriggered();
    void menuLoadEtalon5ActionTriggered();
    void menuLoadCurrentActionTriggered();
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
    void menuSetSkeletonInnerPointsActionTriggered();
    void menuSaveCurrentTriggered();
    void menuSaveResultTriggered();
    void menuExitActionTriggered();

    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    QAction *angle_actions[NUMBER_OF_ANGLES];
    quint8 etalon_angle;

    QPoint reference_point1;
    QPoint reference_point2;

    QPoint inner_skeleton_top;
    QPoint inner_skeleton_right;
    QPoint inner_skeleton_bottom;
    QPoint inner_skeleton_left;
};

#endif // MAINWINDOW_H
