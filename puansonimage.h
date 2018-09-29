#ifndef PUANSONIMAGE_H
#define PUANSONIMAGE_H

#include <QtGlobal>
#include <QString>
#include <QPoint>
#include <QImage>
#include <QtMath>
#include <QSharedPointer>

#include <QVector2D>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#define LIBRAW_NODLL
#include <libraw/libraw.h>

#include "types.h"

#include <QDebug>

#define ETALON_ANGLE_FULL_NUMBER_OF_PARAMETERS 8

using namespace cv;

class IdealContourArc
{
public:
    IdealContourArc():
    center(QPointF(0.0, 0.0)), start_point(QPointF(0.0, 0.0)), r(0.0), alpha(0.0), phi(0.0)
    { }

    IdealContourArc(const QPointF &_center, const QPointF &_start_point, const qreal _R, const qreal _alpha, const qreal _phi):
    center(_center), start_point(_start_point), r(_R), alpha(_alpha), phi(_phi)
    { }

    QPointF &Center()
    {
        return center;
    }

    QPointF &StartPoint()
    {
        return start_point;
    }

    qreal &R()
    {
        return r;
    }

    qreal &Alpha()
    {
        return alpha;
    }

    qreal &Phi()
    {
        return phi;
    }

    QPointF Center() const
    {
        return center;
    }

    QPointF StartPoint() const
    {
        return start_point;
    }

    qreal R() const
    {
        return r;
    }

    qreal Alpha() const
    {
        return alpha;
    }

    qreal Phi() const
    {
        return phi;
    }

private:
    QPointF center; // Центр окружности дуги
    QPointF start_point; // Точка начала дуги
    qreal r; // Радиус дуги
    qreal alpha; // Начальный угол поворота дуги
    qreal phi; // Образующий угол дуги
};

const QPoint start_inner_segment_null_point(INT32_MAX, INT32_MAX);

class IdealInnerSegment
{
public:
    IdealInnerSegment() { }

    IdealInnerSegment(const QPoint &_start_point, const QLine &first_line, const QPoint &_end_point = QPoint(), const bool ultimate = false):
        start_point(_start_point), end_point(_end_point), ultimate_flag(ultimate)
    {
        inner_path.moveTo(first_line.x1(), first_line.y1());
        inner_path.lineTo(first_line.x2(), first_line.y2());
        inner_lines.append(first_line);
    }

    IdealInnerSegment(const QPoint &_start_point, const IdealContourArc &first_arc, const QPoint &_end_point = QPoint(), const bool ultimate = false):
        start_point(_start_point), end_point(_end_point), ultimate_flag(ultimate)
    {
        inner_path.moveTo(_start_point.x(), _start_point.y());
        inner_path.arcTo(QRectF(first_arc.Center().x() - first_arc.R(), first_arc.Center().y() - first_arc.R(), first_arc.R() * 2.0, first_arc.R() * 2.0), first_arc.Alpha(), first_arc.Phi());
        inner_arcs.append(first_arc);
    }

    inline void addInnerLine(const QLine &line)
    {
        inner_path.lineTo(line.x2(), line.y2());
        inner_lines.append(line);
    }

    inline void addInnerArc(const IdealContourArc &arc)
    {
        inner_path.arcTo(QRectF(arc.Center().x() - arc.R(), arc.Center().y() - arc.R(), arc.R() * 2.0, arc.R() * 2.0), arc.Alpha(), arc.Phi());
        inner_arcs.append(arc);
    }

    inline void setStartPoint(const QPoint &point)
    {
        start_point = point;
        ultimate_flag = true;
    }

    inline void setEndPoint(const QPoint &point)
    {
        end_point = point;
        ultimate_flag = true;
    }

    inline QPoint getStartPoint() const
    {
        return start_point;
    }

    inline QPoint getEndPoint() const
    {
        return end_point;
    }

    inline bool isStartPointNull() const
    {
        return start_point.isNull();
    }

    inline bool isEndPointNull() const
    {
        return end_point.isNull();
    }

    inline QVector<QLine> getInnerLines() const
    {
        return inner_lines;
    }

    inline QVector<IdealContourArc> getInnerArcs() const
    {
        return inner_arcs;
    }

    inline QPainterPath getInnerPath() const
    {
        return inner_path;
    }

    inline QPainterPath &InnerPath()
    {
        return inner_path;
    }

    inline bool isUltimate() const
    {
        return ultimate_flag;
    }

    QVector<QLine> getControlLines(const quint8 step, const QRect &analysis_rect = QRect()) const;

private:
    QPoint start_point;
    QPoint end_point;
    QVector<QLine> inner_lines;
    QVector<IdealContourArc> inner_arcs;

    QPainterPath inner_path;

    bool ultimate_flag = true;
};

// Габариты эталона(мкм)
struct EtalonDetailDimensions
{
    qint32 diameter_1_dimension;
    qint32 diameter_2_dimension;        // Только для 658 модели
    qint32 diameter_3_dimension;        // Только для 658 модели
    qint32 diameter_4_dimension;        // Только для 658 модели
    qint32 diameter_5_dimension;
    qint32 diameter_6_dimension;
    qint32 diameter_6_2_dimension;
    qint32 diameter_7_dimension;
    qint32 diameter_7_2_dimension;
    qint32 diameter_8_dimension;
    qint32 diameter_9_dimension;

    qint32 skirt_bottom_part_lenght;
    qint32 top_part_lenght;
    qint32 diameter_2_top_part_lenght;
    qint32 diameter_3_top_part_lenght;
    qint32 diameter_4_top_part_lenght;
    qint32 middle_part_lenght;
    qint32 bottom_part_lenght;

    qint32 groove_width_dimension;
    qint32 groove_top_width_dimension;
    qint32 groove_bottom_width_dimension;

    qint32 groove_depth_dimension;
    qint32 groove_left_depth_dimension;
    qint32 groove_right_depth_dimension;

    quint32 needle_rounding_radius;
    quint32 skirt_rounding_radius;
    quint32 skirt_bottom_rounding_radius;
};
// -------


class PuansonImage
{
    struct DiameterDimension
    {
        // Вертикальная позиция и диаметр детали
        qint32 measurement_y_position;
        quint32 measurement_ideal_diameter;
        quint32 measurement_actual_diameter;
        // Отклонения от эталона (мкм)
        qint32 right_side_deviation;
        qint32 left_side_deviation;
    };

    struct GrooveMeasurements
    {
        struct GrooveDepthMeasurementPoint
        {
            // Позиция точки измерения
            QPoint measurement_ideal_point;
            // Отклонение от эталона (мкм)
            qint32 deviation;
        };

        struct GrooveWidthMeasurement
        {
            // Вертикальная позиция и диаметр детали
            qint32 measurement_y_position;

            quint32 measurement_actual_width;
            // Отклонения от эталона (мкм)
            qint32 right_side_deviation;
            qint32 left_side_deviation;
        };

        // Depth
        GrooveDepthMeasurementPoint top_left_depth_measurement_point;
        GrooveDepthMeasurementPoint top_right_depth_measurement_point;
        GrooveDepthMeasurementPoint bottom_left_depth_measurement_point;
        GrooveDepthMeasurementPoint bottom_right_depth_measurement_point;

        quint32 measurement_actual_left_depth;
        quint32 measurement_actual_right_depth;

        // Радиус поиска точек контуров в окрестности точек измерения глубины
        static const quint16 measurement_depth_radius_mkm;
        ////////

        // Width
        GrooveWidthMeasurement top_width_measurement;
        GrooveWidthMeasurement bottom_width_measurement;
        ////////

        // Ideal measurements
        quint32 measurement_ideal_width;
        quint32 measurement_ideal_depth;
        /////////////////////
    };

    Mat image;

    static void rawImageDelete(libraw_processed_image_t *_raw_image)
    {
        LibRaw::dcraw_clear_mem(_raw_image);
    }

    QSharedPointer<libraw_processed_image_t> raw_image;
    Mat image_contour;
    QString filename;

    ImageType_e image_type;

    bool empty;
    bool cropped;
    qreal crop_scale;

    QPoint reference_point1;
    QPoint reference_point2;

    quint32 reference_point_distance_mkm;
    quint32 reference_point_distance_px;

    qreal calibration_ratio = PuansonImage::default_calibration_ratio;           // D_mkm / D_px

    // Tolerance fields in px
    quint16 externalToleranceMkm;
    quint16 internalToleranceMkm;

    // Ideal contour
    QPointF ideal_contour_point_of_origin;
    qreal ideal_contour_rotation_angle = 0.0;

    QVector<QLine> idealSkeletonLines;
    QVector<IdealContourArc> idealSkeletonArcs;
    QVector<QPoint> innerIdealSkeletonNormalVectors;
    QVector<QPoint> outerIdealSkeletonNormalVectors;

    QVector<IdealInnerSegment> actualIdealInnerSegments;

    QPainterPath idealContourPath;
    QPainterPath idealContourMeasurementsPath;

    bool isIdealContourSetFlag = false;
    bool isCurrentDetailSuitable = false;

    PuansonModel detail_research_puanson_model = PuansonModel::PUANSON_MODEL_660;

    // Габариты эталона(мкм)                                                                             Диаметры                                                   Вертикальные длины                            Размеры паза                 Радиусы
    //                                                                                                                                                                                                                                         скругления
    static constexpr EtalonDetailDimensions puanson_661_dimensions = {   25002, 0, 0, 0, 25700, 14000, 14000, 15000, 15000, 12000, 11000,            3000, 20000, 0, 0, 0, 40000, 16000,               1500, 1500, 1500, 3000, 3000, 3000,   0, 0, 500          }; // "Граната"
    static constexpr EtalonDetailDimensions puanson_660_dimensions = {   1700, 0, 0, 5100, 11000, 8000, 8000, 9000, 9000, 8000, 7000,                3000, 52000, 0, 0, 6000, 40000, 16000,            1500, 1500, 1500, 3000, 3000, 3000,   500, 5000, 500     }; // "Игла"
    static constexpr EtalonDetailDimensions puanson_658_dimensions = {   4000, 4400, 11100, 15500, 23000, 14000, 14000, 15000, 15000, 10000, 9000,   3000, 53000, 42700, 17800, 10800, 40000, 16000,   1500, 1500, 1500, 3000, 3000, 3000,   1000, 94500, 500   }; // "Широкая игла"

    EtalonDetailDimensions ideal_etalon_dimensions = puanson_660_dimensions;
    QVector<DiameterDimension> diameter_dimensions;
    GrooveMeasurements groove_dimensions;
    // -------

    // Трансформация обьекта
    QTransform image_transform;
    // Трансформация идеального контура
    QTransform ideal_contour_transform;

    // Actual borders
    int Y_top;
    int Y_bottom;
    int X_left;
    int X_right;

    void calculateCalibrationRatio()
    {
        if(reference_point_distance_px == 0)
            calibration_ratio = default_calibration_ratio;
        else
            calibration_ratio = static_cast<qreal>(reference_point_distance_mkm) / static_cast<qreal>(reference_point_distance_px);
    }

    static quint32 calculateDistance(const QPoint &p1, const QPoint &p2)
    {
        QPoint distance_vector_px = p1 - p2;

        return static_cast<quint32>(qRound(qSqrt(distance_vector_px.x()*distance_vector_px.x() + distance_vector_px.y()*distance_vector_px.y())));
    }

    static void getArc(const qreal R, const QPointF &point1, const QPointF &point2, const QPointF &point3, QPointF &_point1, QPointF &_point2, QPointF &point0, qreal &phi, qreal &alpha);

    bool findP0(const QPointF &center, const qreal R, QPointF &start_point, qreal &alpha, qreal &phi, QPoint &p0) const;
    bool findPN(const QPointF &center, const qreal R, QPointF &start_point, qreal &alpha, qreal &phi, QPoint &pN) const;

public:
    PuansonImage(const ImageType_e _image_type, const Mat &_image, const libraw_processed_image_t *_raw_image, const QString &_filename);
    PuansonImage();
    ~PuansonImage();

    PuansonImage &operator=(const PuansonImage &img);
    // Копирует только графическую информацию
    PuansonImage &operator^=(const PuansonImage &img);

    inline QTransform getImageTransform() const
    {
        return image_transform;
    }

    inline void setImageTransform(const QTransform &t)
    {
        image_transform = t;
    }

    inline bool isEmpty() const
    {
        return empty;
    }

    inline bool isCropped() const
    {
        return cropped;
    }

    inline void setCurrentDetailSuitable(bool is_current_detail_suitable)
    {
        isCurrentDetailSuitable = is_current_detail_suitable;
    }

    inline bool getCurrentDetailSuitable()
    {
        return isCurrentDetailSuitable;
    }

    inline void copyIdealContour(const PuansonImage &img)
    {
        ideal_contour_point_of_origin = img.ideal_contour_point_of_origin;
        ideal_contour_rotation_angle = img.ideal_contour_rotation_angle;

        idealSkeletonLines = img.idealSkeletonLines;
        innerIdealSkeletonNormalVectors = img.innerIdealSkeletonNormalVectors;
        outerIdealSkeletonNormalVectors = img.outerIdealSkeletonNormalVectors;
        idealSkeletonArcs = img.idealSkeletonArcs;
        idealContourPath = img.idealContourPath;
        isIdealContourSetFlag = img.isIdealContourSetFlag;
    }

    inline void getIdealContour(QVector<QLine> &idealLines, QVector<QPoint> &innerNormalVectors, QVector<QPoint> &outerNormalVectors, QPointF &_point_of_origin, qreal &_rotation_angle) const
    {
        idealLines = idealSkeletonLines;
        innerNormalVectors = innerIdealSkeletonNormalVectors;
        outerNormalVectors = outerIdealSkeletonNormalVectors;

        _point_of_origin = ideal_contour_point_of_origin;
        _rotation_angle = ideal_contour_rotation_angle;
    }

    inline void cropImage(const qreal scale = 4.0)
    {
        image = Mat(image, Rect(qRound((image.cols - image.cols / scale) / 2.0),
                                              qRound((image.rows - image.rows / scale) / 2.0),
                                              qRound(image.cols / scale), qRound(image.rows / scale)));

        crop_scale = scale;
        cropped = true;
    }

    inline qreal getCropScale() const
    {
        return crop_scale;
    }

    inline const QPainterPath& getIdealContourPath() const
    {
        return idealContourPath;
    }

    inline QPainterPath& getIdealContourPath()
    {
        return idealContourPath;
    }

    inline const QPainterPath& getIdealContourMeasurementsPath() const
    {
        return idealContourMeasurementsPath;
    }

    inline QPainterPath& getIdealContourMeasurementsPath()
    {
        return idealContourMeasurementsPath;
    }

    inline QVector<IdealInnerSegment> getActualIdealInnerSegments() const
    {
        return actualIdealInnerSegments;
    }

    inline ImageType_e getImageType() const
    {
        return image_type;
    }

    static void drawIdealContour(const PuansonModel detail_research_puanson_model, const QRect &r, const QPointF &_point_of_origin, const qreal _rotation_angle, bool draw_measurements, const qreal _scale, const qreal calibration_ratio, QPainterPath &idealContourPath, QPainterPath &idealContourMeasurementsPath);
    QPainterPath drawIdealContour(const QRect &r, const QPointF &_point_of_origin, const qreal _rotation_angle, bool draw_measurements = false, const qreal _scale = 1.0);

    static qint32 pointDistanceToLine(const QPoint &pt, const QLine &line);
    void addIdealSkeletonLine(const QPoint &p1, const QPoint &p2, const int normal_vecror_x_sign, const bool close_flag = false);
    void addIdealSkeletonArc(const QPointF &center, const QPointF &start_point, const qreal R, const qreal alpha, const qreal phi, const bool close_flag = false);
    bool findNearestIdealLineNormalVector(const QPoint &pt, QLine &last_line, QPoint &internalToleranceNormalVector, QPoint &externalToleranceNormalVector) const;

    QVector<QLine> getMeasurementLines(const QRect &analysis_rect);
    bool setDeviation(const QPoint &measurement_point, const qint32 deviation_value);

    bool autoCkeckDetail();

    bool getQImage(QImage &img) const;

    inline Mat& getImage()
    {
        return image;
    }

    inline const Mat& getImage() const
    {
        return image;
    }

    inline libraw_processed_image_t *getRawImage()
    {
        return raw_image.data();
    }

    void release();

    inline QString getFilename() const
    {
        return filename;
    }

    inline void setImageContour(const Mat &contour)
    {
        contour.copyTo(image_contour);
    }

    inline Mat& imageContour()
    {
        return image_contour;
    }

    inline bool isReferencePointsAreSet() const
    {
        return !(reference_point1 == reference_point2);
    }

    void setReferencePoints(const QPoint &p1, const QPoint &p2);

    inline void getReferencePoints(QPoint &reference_point1, QPoint &reference_point2) const
    {
        reference_point1 = this->reference_point1;
        reference_point2 = this->reference_point2;
    }

    void setReferencePointDistanceMkm(const quint32 d)
    {
        reference_point_distance_mkm = d;
        calculateCalibrationRatio();
    }

    inline quint32 getReferencePointDistanceMkm() const
    {
        return reference_point_distance_mkm;
    }

    inline quint32 getReferencePointDistancePx() const
    {
        return reference_point_distance_px;
    }

    inline qreal getCalibrationRatio() const
    {
        return calibration_ratio;
    }

    inline void setToleranceFields(const quint16 ext_tolerance_mkm, const quint16 int_tolerance_mkm)
    {
        externalToleranceMkm = ext_tolerance_mkm;
        internalToleranceMkm = int_tolerance_mkm;
    }

    inline void getToleranceFields(quint16 &ext_tolerance_mkm, quint16 &int_tolerance_mkm) const
    {
        ext_tolerance_mkm = externalToleranceMkm;
        int_tolerance_mkm = internalToleranceMkm;
    }

    inline quint16 getToleranceExtFieldPx()
    {
        return static_cast<quint16>(qRound(externalToleranceMkm / calibration_ratio));
    }

    inline quint16 getToleranceIntFieldPx()
    {
        return static_cast<quint16>(qRound(internalToleranceMkm / calibration_ratio));
    }

    inline bool isIdealContourSet() const
    {
        return isIdealContourSetFlag;
    }

    inline void setIdealContourSetFlag(const bool value)
    {
        isIdealContourSetFlag = value;
    }

    inline QPointF getIdealContourPointOfOrigin() const
    {
        return ideal_contour_point_of_origin;
    }

    inline void setIdealContourPointOfOrigin(const QPointF &pt)
    {
        ideal_contour_point_of_origin = pt;
    }

    inline qreal getIdealContourRotationAngle() const
    {
        return ideal_contour_rotation_angle;
    }

    inline void setIdealContourRotationAngle(const qreal angle)
    {
        ideal_contour_rotation_angle = angle;
    }

    inline void setEtalonResearchPuansonModel(const PuansonModel _detail_research_puanson_model)
    {
        detail_research_puanson_model = _detail_research_puanson_model;

        switch(detail_research_puanson_model)
        {
        case PuansonModel::PUANSON_MODEL_658:
            ideal_etalon_dimensions = puanson_658_dimensions;
            break;
        case PuansonModel::PUANSON_MODEL_660:
            ideal_etalon_dimensions = puanson_660_dimensions;
            break;
        case PuansonModel::PUANSON_MODEL_661:
            ideal_etalon_dimensions = puanson_661_dimensions;
            break;
        }

        diameter_dimensions.resize(11);

        diameter_dimensions[0].measurement_ideal_diameter = static_cast<quint32>(ideal_etalon_dimensions.diameter_1_dimension);
        diameter_dimensions[0].measurement_y_position = ideal_etalon_dimensions.top_part_lenght;

        diameter_dimensions[1].measurement_ideal_diameter = static_cast<quint32>(ideal_etalon_dimensions.diameter_2_dimension);
        diameter_dimensions[1].measurement_y_position = ideal_etalon_dimensions.diameter_2_top_part_lenght;

        diameter_dimensions[2].measurement_ideal_diameter = static_cast<quint32>(ideal_etalon_dimensions.diameter_3_dimension);
        diameter_dimensions[2].measurement_y_position = ideal_etalon_dimensions.diameter_3_top_part_lenght;

        diameter_dimensions[3].measurement_ideal_diameter = static_cast<quint32>(detail_research_puanson_model == PuansonModel::PUANSON_MODEL_658 ? ideal_etalon_dimensions.diameter_4_dimension : 0);
        diameter_dimensions[3].measurement_y_position = ideal_etalon_dimensions.diameter_4_top_part_lenght;

        diameter_dimensions[4].measurement_ideal_diameter = static_cast<quint32>(ideal_etalon_dimensions.diameter_5_dimension);
        diameter_dimensions[4].measurement_y_position = 0;

        diameter_dimensions[5].measurement_ideal_diameter = static_cast<quint32>(ideal_etalon_dimensions.diameter_6_dimension);
        diameter_dimensions[5].measurement_y_position = -static_cast<qint32>(ideal_etalon_dimensions.skirt_bottom_rounding_radius + 250);

        diameter_dimensions[6].measurement_ideal_diameter = static_cast<quint32>(ideal_etalon_dimensions.diameter_6_dimension);
        diameter_dimensions[6].measurement_y_position = -(ideal_etalon_dimensions.skirt_bottom_part_lenght - 750);

        diameter_dimensions[7].measurement_ideal_diameter = static_cast<quint32>(ideal_etalon_dimensions.diameter_7_dimension);
        diameter_dimensions[7].measurement_y_position = -(ideal_etalon_dimensions.skirt_bottom_part_lenght + 250);

        diameter_dimensions[8].measurement_ideal_diameter = static_cast<quint32>(ideal_etalon_dimensions.diameter_7_dimension);
        diameter_dimensions[8].measurement_y_position = -(ideal_etalon_dimensions.skirt_bottom_part_lenght + ideal_etalon_dimensions.middle_part_lenght - 1000);

        diameter_dimensions[9].measurement_ideal_diameter = static_cast<quint32>(ideal_etalon_dimensions.diameter_8_dimension);
        diameter_dimensions[9].measurement_y_position = -(ideal_etalon_dimensions.skirt_bottom_part_lenght + ideal_etalon_dimensions.middle_part_lenght + 500);

        diameter_dimensions[10].measurement_ideal_diameter = static_cast<quint32>(ideal_etalon_dimensions.diameter_9_dimension);
        diameter_dimensions[10].measurement_y_position = -(ideal_etalon_dimensions.skirt_bottom_part_lenght + ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght);

        groove_dimensions.measurement_actual_right_depth = ideal_etalon_dimensions.groove_depth_dimension;
        groove_dimensions.measurement_actual_left_depth = ideal_etalon_dimensions.groove_depth_dimension;

        groove_dimensions.top_right_depth_measurement_point.measurement_ideal_point = QPoint(ideal_etalon_dimensions.groove_width_dimension / 2 + 500, -(ideal_etalon_dimensions.skirt_bottom_part_lenght + ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght));
        groove_dimensions.top_left_depth_measurement_point.measurement_ideal_point = QPoint(-(ideal_etalon_dimensions.groove_width_dimension / 2 + 500), -(ideal_etalon_dimensions.skirt_bottom_part_lenght + ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght));

        groove_dimensions.bottom_right_depth_measurement_point.measurement_ideal_point = QPoint(ideal_etalon_dimensions.groove_width_dimension / 2 - 500, -((ideal_etalon_dimensions.skirt_bottom_part_lenght + ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght) - ideal_etalon_dimensions.groove_depth_dimension));
        groove_dimensions.bottom_left_depth_measurement_point.measurement_ideal_point = QPoint(-(ideal_etalon_dimensions.groove_width_dimension / 2 - 500), -((ideal_etalon_dimensions.skirt_bottom_part_lenght + ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght) - ideal_etalon_dimensions.groove_depth_dimension));

        groove_dimensions.measurement_ideal_depth = static_cast<quint32>(ideal_etalon_dimensions.groove_depth_dimension);

        groove_dimensions.top_width_measurement.measurement_actual_width = static_cast<quint32>(ideal_etalon_dimensions.groove_width_dimension);
        groove_dimensions.top_width_measurement.measurement_y_position = -((ideal_etalon_dimensions.skirt_bottom_part_lenght + ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght) - 500);

        groove_dimensions.bottom_width_measurement.measurement_actual_width = static_cast<quint32>(ideal_etalon_dimensions.groove_width_dimension);
        groove_dimensions.bottom_width_measurement.measurement_y_position = -((ideal_etalon_dimensions.skirt_bottom_part_lenght + ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght) - (ideal_etalon_dimensions.groove_depth_dimension - 500));
    }

    void calculateDetailDimensionsFromDeviations(const PuansonImage& etalon_image);

    PuansonModel getDetailPuansonModel() const
    {
        return detail_research_puanson_model;
    }

    // Габариты эталона
    inline void setDetailDimensions(const quint32 _diameter_1_dimension, const quint32 _diameter_2_dimension, const quint32 _diameter_3_dimension, const quint32 _diameter_4_dimension, const quint32 _diameter_5_dimension, const quint32 _diameter_6_dimension, const quint32 _diameter_7_dimension, const quint32 _diameter_8_dimension, const quint32 _diameter_9_dimension, const quint32 _groove_width_dimension, const quint32 _groove_depth_dimension, const quint32 _needle_rounding_radius, const quint32 _skirt_rounding_radius, const quint32 _skirt_bottom_rounding_radius)
    {
        diameter_dimensions[0].measurement_actual_diameter = _diameter_1_dimension;
        diameter_dimensions[1].measurement_actual_diameter = _diameter_2_dimension;
        diameter_dimensions[2].measurement_actual_diameter = _diameter_3_dimension;
        diameter_dimensions[3].measurement_actual_diameter = _diameter_4_dimension;
        diameter_dimensions[4].measurement_actual_diameter = _diameter_5_dimension;
        diameter_dimensions[5].measurement_actual_diameter = _diameter_6_dimension;
        diameter_dimensions[6].measurement_actual_diameter = _diameter_6_dimension;
        diameter_dimensions[7].measurement_actual_diameter = _diameter_7_dimension;
        diameter_dimensions[8].measurement_actual_diameter = _diameter_7_dimension;
        diameter_dimensions[9].measurement_actual_diameter = _diameter_8_dimension;
        diameter_dimensions[10].measurement_actual_diameter = _diameter_9_dimension;

        groove_dimensions.measurement_ideal_width = _groove_width_dimension;
        // Добавить рассчёт точек

        groove_dimensions.measurement_ideal_depth = _groove_depth_dimension;
        groove_dimensions.bottom_right_depth_measurement_point.measurement_ideal_point = QPoint(_groove_width_dimension / 2, -((ideal_etalon_dimensions.skirt_bottom_part_lenght + ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght) - static_cast<qint32>(_groove_depth_dimension)));
        groove_dimensions.bottom_left_depth_measurement_point.measurement_ideal_point = QPoint(-static_cast<qint32>(_groove_width_dimension / 2), -(ideal_etalon_dimensions.skirt_bottom_part_lenght + ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght) - static_cast<qint32>(_groove_depth_dimension));

        ideal_etalon_dimensions.needle_rounding_radius = _needle_rounding_radius;
        ideal_etalon_dimensions.skirt_rounding_radius = _skirt_rounding_radius;
        ideal_etalon_dimensions.skirt_bottom_rounding_radius = _skirt_bottom_rounding_radius;
    }

    inline void getDetailDimensions(quint32 &_diameter_1_dimension, quint32 &_diameter_2_dimension, quint32 &_diameter_3_dimension, quint32 &_diameter_4_dimension, quint32 &_diameter_5_dimension, quint32 &_diameter_6_dimension, quint32 &_diameter_7_dimension, quint32 &_diameter_8_dimension, quint32 &_diameter_9_dimension, quint32 &_groove_width_dimension, quint32 &_groove_depth_dimension, quint32 &_needle_rounding_radius, quint32 &_skirt_rounding_radius, quint32 &_skirt_bottom_rounding_radius) const
    {
        _diameter_1_dimension = diameter_dimensions[0].measurement_actual_diameter;
        _diameter_2_dimension = diameter_dimensions[1].measurement_actual_diameter;
        _diameter_3_dimension = diameter_dimensions[2].measurement_actual_diameter;
        _diameter_4_dimension = diameter_dimensions[3].measurement_actual_diameter;
        _diameter_5_dimension = diameter_dimensions[4].measurement_actual_diameter;
        _diameter_6_dimension = diameter_dimensions[5].measurement_actual_diameter;
        _diameter_7_dimension = diameter_dimensions[7].measurement_actual_diameter;
        _diameter_8_dimension = diameter_dimensions[9].measurement_actual_diameter;
        _diameter_9_dimension = diameter_dimensions[10].measurement_actual_diameter;

        _groove_width_dimension = groove_dimensions.measurement_ideal_width;
        _groove_depth_dimension = groove_dimensions.measurement_ideal_depth;

        _needle_rounding_radius = ideal_etalon_dimensions.needle_rounding_radius;
        _skirt_rounding_radius = ideal_etalon_dimensions.skirt_rounding_radius;
        _skirt_bottom_rounding_radius = ideal_etalon_dimensions.skirt_bottom_rounding_radius;
    }

    inline void getDetailDimensions(quint32 &_diameter_1_dimension, quint32 &_diameter_2_dimension, quint32 &_diameter_3_dimension, quint32 &_diameter_4_dimension, quint32 &_diameter_5_dimension, quint32 &_diameter_6_dimension, quint32 &_diameter_6_2_dimension, quint32 &_diameter_7_dimension, quint32 &_diameter_7_2_dimension, quint32 &_diameter_8_dimension, quint32 &_diameter_9_dimension, quint32 &_groove_width_dimension, quint32 &_groove_depth_dimension, quint32 &_needle_rounding_radius, quint32 &_skirt_rounding_radius, quint32 &_skirt_bottom_rounding_radius) const
    {
        _diameter_1_dimension = diameter_dimensions[0].measurement_actual_diameter;
        _diameter_2_dimension = diameter_dimensions[1].measurement_actual_diameter;
        _diameter_3_dimension = diameter_dimensions[2].measurement_actual_diameter;
        _diameter_4_dimension = diameter_dimensions[3].measurement_actual_diameter;
        _diameter_5_dimension = diameter_dimensions[4].measurement_actual_diameter;
        _diameter_6_dimension = diameter_dimensions[5].measurement_actual_diameter;
        _diameter_6_2_dimension = diameter_dimensions[6].measurement_actual_diameter;
        _diameter_7_dimension = diameter_dimensions[7].measurement_actual_diameter;
        _diameter_7_2_dimension = diameter_dimensions[8].measurement_actual_diameter;
        _diameter_8_dimension = diameter_dimensions[9].measurement_actual_diameter;
        _diameter_9_dimension = diameter_dimensions[10].measurement_actual_diameter;

        _groove_width_dimension = groove_dimensions.top_width_measurement.measurement_actual_width;
        _groove_depth_dimension = groove_dimensions.measurement_actual_right_depth;

        _needle_rounding_radius = ideal_etalon_dimensions.needle_rounding_radius;
        _skirt_rounding_radius = ideal_etalon_dimensions.skirt_rounding_radius;
        _skirt_bottom_rounding_radius = ideal_etalon_dimensions.skirt_bottom_rounding_radius;
    }

    inline void getDetailDimensions(EtalonDetailDimensions &detail_dimensions)
    {
        detail_dimensions.diameter_1_dimension = static_cast<qint32>(diameter_dimensions[0].measurement_actual_diameter);
        detail_dimensions.diameter_2_dimension = static_cast<qint32>(diameter_dimensions[1].measurement_actual_diameter);
        detail_dimensions.diameter_3_dimension = static_cast<qint32>(diameter_dimensions[2].measurement_actual_diameter);
        detail_dimensions.diameter_4_dimension = static_cast<qint32>(diameter_dimensions[3].measurement_actual_diameter);
        detail_dimensions.diameter_5_dimension = static_cast<qint32>(diameter_dimensions[4].measurement_actual_diameter);
        detail_dimensions.diameter_6_dimension = static_cast<qint32>(diameter_dimensions[5].measurement_actual_diameter);
        detail_dimensions.diameter_6_2_dimension = static_cast<qint32>(diameter_dimensions[6].measurement_actual_diameter);
        detail_dimensions.diameter_7_dimension = static_cast<qint32>(diameter_dimensions[7].measurement_actual_diameter);
        detail_dimensions.diameter_7_2_dimension = static_cast<qint32>(diameter_dimensions[8].measurement_actual_diameter);
        detail_dimensions.diameter_8_dimension = static_cast<qint32>(diameter_dimensions[9].measurement_actual_diameter);
        detail_dimensions.diameter_9_dimension = static_cast<qint32>(diameter_dimensions[10].measurement_actual_diameter);

        detail_dimensions.groove_width_dimension = static_cast<qint32>(groove_dimensions.measurement_ideal_width);
        detail_dimensions.groove_top_width_dimension = static_cast<qint32>(groove_dimensions.top_width_measurement.measurement_actual_width);
        detail_dimensions.groove_bottom_width_dimension = static_cast<qint32>(groove_dimensions.bottom_width_measurement.measurement_actual_width);

        detail_dimensions.groove_depth_dimension = static_cast<qint32>(groove_dimensions.measurement_ideal_depth);
        detail_dimensions.groove_right_depth_dimension = static_cast<qint32>(groove_dimensions.measurement_actual_right_depth);
        detail_dimensions.groove_left_depth_dimension = static_cast<qint32>(groove_dimensions.measurement_actual_left_depth);

        detail_dimensions.needle_rounding_radius = ideal_etalon_dimensions.needle_rounding_radius;
        detail_dimensions.skirt_rounding_radius = ideal_etalon_dimensions.skirt_rounding_radius;
        detail_dimensions.skirt_bottom_rounding_radius = ideal_etalon_dimensions.skirt_bottom_rounding_radius;
    }

    inline void getIdealDimensions(quint32 &_diameter_1_dimension, quint32 &_diameter_2_dimension, quint32 &_diameter_3_dimension, quint32 &_diameter_4_dimension, quint32 &_diameter_5_dimension, quint32 &_diameter_6_dimension, quint32 &_diameter_7_dimension, quint32 &_diameter_8_dimension, quint32 &_diameter_9_dimension, quint32 &_groove_width_dimension, quint32 &_groove_depth_dimension, quint32 &_needle_rounding_radius, quint32 &_skirt_rounding_radius, quint32 &_skirt_bottom_rounding_radius) const
    {
        _diameter_1_dimension = static_cast<quint32>(ideal_etalon_dimensions.diameter_1_dimension);
        _diameter_2_dimension = static_cast<quint32>(ideal_etalon_dimensions.diameter_2_dimension);
        _diameter_3_dimension = static_cast<quint32>(ideal_etalon_dimensions.diameter_3_dimension);
        _diameter_4_dimension = static_cast<quint32>(ideal_etalon_dimensions.diameter_4_dimension);
        _diameter_5_dimension = static_cast<quint32>(ideal_etalon_dimensions.diameter_5_dimension);
        _diameter_6_dimension = static_cast<quint32>(ideal_etalon_dimensions.diameter_6_dimension);
        _diameter_7_dimension = static_cast<quint32>(ideal_etalon_dimensions.diameter_7_dimension);
        _diameter_8_dimension = static_cast<quint32>(ideal_etalon_dimensions.diameter_8_dimension);
        _diameter_9_dimension = static_cast<quint32>(ideal_etalon_dimensions.diameter_9_dimension);

        _groove_width_dimension = static_cast<quint32>(ideal_etalon_dimensions.groove_width_dimension);
        _groove_depth_dimension = static_cast<quint32>(ideal_etalon_dimensions.groove_depth_dimension);

        _needle_rounding_radius = ideal_etalon_dimensions.needle_rounding_radius;
        _skirt_rounding_radius = ideal_etalon_dimensions.skirt_rounding_radius;
        _skirt_bottom_rounding_radius = ideal_etalon_dimensions.skirt_bottom_rounding_radius;
    }

    inline static void getIdealDimensions(const PuansonModel detail_research_puanson_model, EtalonDetailDimensions &detail_dimensions)
    {
        switch(detail_research_puanson_model)
        {
        case PuansonModel::PUANSON_MODEL_658:
            detail_dimensions = puanson_658_dimensions;
            break;
        case PuansonModel::PUANSON_MODEL_660:
            detail_dimensions = puanson_660_dimensions;
            break;
        case PuansonModel::PUANSON_MODEL_661:
            detail_dimensions = puanson_661_dimensions;
            break;
        }
    }

    inline void getIdealDimensions(EtalonDetailDimensions &detail_dimensions)
    {
        detail_dimensions.diameter_1_dimension = ideal_etalon_dimensions.diameter_1_dimension;
        detail_dimensions.diameter_2_dimension = ideal_etalon_dimensions.diameter_2_dimension;
        detail_dimensions.diameter_3_dimension = ideal_etalon_dimensions.diameter_3_dimension;
        detail_dimensions.diameter_4_dimension = ideal_etalon_dimensions.diameter_4_dimension;
        detail_dimensions.diameter_5_dimension = ideal_etalon_dimensions.diameter_5_dimension;
        detail_dimensions.diameter_6_dimension = ideal_etalon_dimensions.diameter_6_dimension;
        detail_dimensions.diameter_6_2_dimension = ideal_etalon_dimensions.diameter_6_dimension;
        detail_dimensions.diameter_7_dimension = ideal_etalon_dimensions.diameter_7_dimension;
        detail_dimensions.diameter_7_2_dimension = ideal_etalon_dimensions.diameter_7_dimension;
        detail_dimensions.diameter_8_dimension = ideal_etalon_dimensions.diameter_8_dimension;
        detail_dimensions.diameter_9_dimension = ideal_etalon_dimensions.diameter_9_dimension;

        detail_dimensions.groove_width_dimension = ideal_etalon_dimensions.groove_width_dimension;
        detail_dimensions.groove_depth_dimension = ideal_etalon_dimensions.groove_depth_dimension;

        detail_dimensions.needle_rounding_radius = ideal_etalon_dimensions.needle_rounding_radius;
        detail_dimensions.skirt_rounding_radius = ideal_etalon_dimensions.skirt_rounding_radius;
        detail_dimensions.skirt_bottom_rounding_radius = ideal_etalon_dimensions.skirt_bottom_rounding_radius;
    }
    // ------

    bool loadEtalonAngle(const QString &etalon_dir);
    bool saveEtalonAngle(const QString &etalon_dir) const;

    static const qreal default_calibration_ratio;
};

#endif // PUANSONIMAGE_H
