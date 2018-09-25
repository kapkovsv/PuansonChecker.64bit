#ifndef CONTOURSFORM_H
#define CONTOURSFORM_H

#include "imagewindow.h"
#include <QWidget>

class PuansonChecker;

namespace Ui {
class ContoursForm;
}

#define MEASUREMENT_POINT_CONTOUR_RADIUS 10

class ContoursForm : public QWidget, public ImageWindow
{
    struct Measurement
    {
        Measurement()
        {
        }

        Measurement(const QLine &_measurement_line, const QPoint &_outer_point, const QPoint &_current_detail_point)
        {
            measurement_line = _measurement_line;

            current_detail_point = _current_detail_point;
            outer_tolerance_etalon_point = _outer_point;
        }

        QLine measurement_line;
        QGraphicsLineItem *line_item = Q_NULLPTR;

        QPoint current_detail_point;
        QGraphicsEllipseItem *current_detail_point_item = Q_NULLPTR;
        QPoint outer_tolerance_etalon_point;
        QGraphicsEllipseItem *outer_tolerance_etalon_point_item = Q_NULLPTR;

        QPoint *active_point = Q_NULLPTR;

        bool current_detail_position_is_set = false;
        bool outer_tolerance_etalon_detail_position_is_set = false;
    };

    Q_OBJECT

public:
    explicit ContoursForm(PuansonChecker *checker);
    ~ContoursForm() Q_DECL_OVERRIDE;

    void moveImage(const qreal dx, const qreal dy) Q_DECL_OVERRIDE;
    void drawIdealContour(const QPainterPath &ideal_path, const QPainterPath &ideal_measurements_path);
    void drawBadPoints(const QVector<QPoint> &bad_points);
    void drawActualBorders();
    void drawMeasurementLineAndPoints(const QLine &measurement_line, const QPoint &outer_point, const QPoint &current_detail_point);
    void drawImage(const QImage &img);
    void setImageCursor(const QCursor &cursor) Q_DECL_OVERRIDE;

    inline void measurementPointsSettingMode()
    {
        calibration_mode = CalibrationMode_e::MANUAL_SETTING_REFERENCE_POINTS;
    }

    void mousePressEvent(const QPoint &p) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(const QPoint &p) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(const QPoint &p) Q_DECL_OVERRIDE;

public slots:
    void etalonContourCheckBoxStateChanged(int state);
    void cannyThres1SpinBoxValueChanged();
    void cannyThres2SpinBoxValueChanged();
    bool combineImagesByReferencePointsButtonPressed();

private:
    Ui::ContoursForm *ui;

    QVector<Measurement> measurements;
};

#endif // CONTOURSFORM_H
