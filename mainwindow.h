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
    void removeReferencePoints();
    void removeReferencePointsAndIdealContour();

    void drawReferencePointsAndIdealContour();
    void drawReferenceAndInnerSkeletonPoints();
public:
    explicit MainWindow();
    ~MainWindow() Q_DECL_OVERRIDE;

    void drawImage(const QImage &img);
    void drawIdealContour(const QPoint &_ideal_origin_point = QPoint(), const qreal _ideal_rotation_angle = 0.0);
    void drawActualBorders();
    void drawReferencePoints(const QPoint &reference_point1, const QPoint &reference_point2);
    void drawReferencePointAutoSearchArea(const ReferencePointType_e reference_point_type, const QRect &search_area_rect = QRect());

    void moveImage(const qreal dx, const qreal dy) Q_DECL_OVERRIDE;
    void setImageCursor(const QCursor &cursor) Q_DECL_OVERRIDE;
    void setWindowStatus(const QString &status);
    void setCalibrationMode(CalibrationMode_e mode);

    void currentDetailResearchGotoNextAngle();

    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    void mousePressEvent(const QPoint &p) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(const QPoint &p) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    bool wheelEvent(bool control_modifier, int delta) Q_DECL_OVERRIDE;

    bool windowKeyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

    void loadImageFinished(const ImageType_e image_type);

public slots:

    // Меню "Эталон"
    void menuEtalonResearchActionTriggered();
    void menuEtalonLoadResearchActionTriggered();
    void menuEtalonAngleSetActive1ActionTriggered();
    void menuEtalonAngleSetActive2ActionTriggered();
    void menuEtalonAngleSetActive3ActionTriggered();
    void menuEtalonAngleSetActive4ActionTriggered();
    void menuEtalonAngleSetActive5ActionTriggered();
    void menuEtalonAngleSetActive6ActionTriggered();
    void menuEtalonResearchPropertiesActionTriggered();

    // Меню "Текущая деталь"
    void menuCurrentResearchActionTriggered();

    // Меню "Работа с изображением"
    void menuImageWorkSetLeftBottomReferencePointSearchAreaActionTriggered();
    void menuImageWorkSetRightTopReferencePointSearchAreaActionTriggered();
    void menuImageWorkSaveResultTriggered();

    // Меню "Работа с изображением" -> "Эталон"
    void menuImageWorkEtalonLoadActionTriggered();
    void menuImageWorkEtalonShootAndLoadActionTriggered();
    void menuImageWorkEtalonReferencePointsAutoSearchActionTriggered();
    void menuImageWorkEtalonManuallySetReferencePointsActionTriggered();
    void menuImageWorkEtalonImposeIdealContourToEtalonActionTriggered();
    void menuImageWorkEtalonSaveAngleToResearchActionTriggered();

    // Меню "Работа с изображением" -> "Текущая деталь"
    void menuImageWorkCurrentLoadActionTriggered();
    void menuImageWorkCurrentShootAndLoadActionTriggered();
    void menuImageWorkCurrentManuallySetReferencePointsActionTriggered();
    void menuImageWorkCurrentReferencePointsAutoSearchActionTriggered();
    void menuImageWorkCurrentSaveTriggered();

    // Меню "Перейти к"
    void menuGotoCurrentWindowTriggered();
    void menuGotoContoursWindowTriggered();

    // Меню "Настройки"
    void menuSettingsActionTriggered();

    // "Выход"
    void menuExitActionTriggered();

    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    Ui::MainWindow *ui;
    QAction *angle_actions[PUANSON_IMAGE_MAX_ANGLE];

    // Реперные точки
    QPoint reference_point1;
    QPoint reference_point2;
    // --------------

    // Идеальный контур
    bool ideal_impose_mouse_pressed = false;
    QPoint ideal_impose_previous_point;
    QPoint ideal_origin_point;
    qreal ideal_rotation_angle = 0.0;
    QGraphicsPathItem *ideal_item = Q_NULLPTR;
    // ----------------

    // Области автоматического поиска реперных точек
    bool set_reference_point_auto_search_area_mouse_pressed = false;
    QRect reference_point_1_auto_search_area_rect;
    QRect reference_point_2_auto_search_area_rect;
    QSize auto_search_area_reference_point_1_rect_size = QSize(REFERENCE_POINT_AUTO_SEARCH_AREA_DEFAULT_WIDTH, REFERENCE_POINT_AUTO_SEARCH_AREA_DEFAULT_HEIGHT);
    QSize auto_search_area_reference_point_2_rect_size = QSize(REFERENCE_POINT_AUTO_SEARCH_AREA_DEFAULT_WIDTH, REFERENCE_POINT_AUTO_SEARCH_AREA_DEFAULT_HEIGHT);
    QPoint previous_reference_point_auto_search_area_rect_top_left_point;
    QGraphicsRectItem *reference_point_1_auto_search_area_ideal_item = Q_NULLPTR;
    QGraphicsRectItem *reference_point_2_auto_search_area_ideal_item = Q_NULLPTR;
    // ---------------------------------------------
};

#endif // MAINWINDOW_H
