#ifndef PUANSONIMAGE_H
#define PUANSONIMAGE_H

#include <QtGlobal>
#include <QString>
#include <QPoint>
#include <QImage>
#include <QtMath>

#include <QDebug>

#include <opencv2/core/core.hpp>

#define LIBRAW_NODLL
#include <libraw/libraw.h>

using namespace cv;

enum ImageType_e
{
    ETALON_IMAGE = 0,
    CURRENT_IMAGE = 1,
    UNDEFINED_IMAGE = 2
};

class PuansonImage
{
    class IdealContourArc
    {
    public:
        IdealContourArc():
        center(QPointF(0.0, 0.0)), start_point(QPointF(0.0, 0.0)), r(0.0), phi(0.0)
        { }

        IdealContourArc(const QPointF &_center, const QPointF &_start_point, const qreal _R, const qreal _phi):
        center(_center), start_point(_start_point), r(_R), phi(_phi)
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

        qreal &Phi()
        {
            return phi;
        }

    private:
        QPointF center; // Центр окружности дуги
        QPointF start_point; // Точка начала дуги
        qreal r; // Радиус дуги
        qreal phi; // Образующий угол дуги
    };

    Mat image;
    libraw_processed_image_t *raw_image;
    quint16 *p_raw_image_reference_counter;
    Mat image_contour;
    QString filename;

    ImageType_e image_type;

    bool empty;

    QPoint reference_point1;
    QPoint reference_point2;

    quint32 reference_point_distance_mkm;
    quint32 reference_point_distance_px;
    qreal calibration_ratio;                // D_mkm / D_px

    // Tolerance fields in px
    quint16 externalToleranceMkm;
    quint16 internalToleranceMkm;

    // Ideal contour
    QPointF point_of_origin;
    qreal rotation_angle;

    QVector<QLine> idealSkeletonLines;
    QVector<IdealContourArc> idealSkeletonArcs;
    QVector<QPoint> innerIdealSkeletonNormalVectors;
    QVector<QPoint> outerIdealSkeletonNormalVectors;

    QPainterPath idealContourPath;

    bool isIdealContourSetFlag;

    void calculateCalibrationRatio()
    {
        if(reference_point_distance_px == 0)
            calibration_ratio = 0;
        else
            calibration_ratio = static_cast<qreal>(reference_point_distance_mkm) / static_cast<qreal>(reference_point_distance_px);
    }

    quint32 calculateDistance(const QPoint &p1, const QPoint &p2)
    {
        QPoint distance_vector_px = p1 - p2;

        return qSqrt(distance_vector_px.x()*distance_vector_px.x() + distance_vector_px.y()*distance_vector_px.y());
    }

    void getArc(const qreal R, const QPointF &point1, const QPointF &point2, const QPointF &point3, QPointF &_point1, QPointF &_point2, QPointF &point0, qreal &phi, qreal &alpha);

public:
    PuansonImage(const ImageType_e _image_type, const Mat &_image, const libraw_processed_image_t *_raw_image, const QString &_filename);
    PuansonImage();
    ~PuansonImage();

    PuansonImage &operator=(const PuansonImage &img);

    inline bool isEmpty()
    {
        return empty;
    }

    inline void copyIdealContour(const PuansonImage &img)
    {
        idealSkeletonLines = img.idealSkeletonLines;
        innerIdealSkeletonNormalVectors = img.innerIdealSkeletonNormalVectors;
        outerIdealSkeletonNormalVectors = img.outerIdealSkeletonNormalVectors;

        idealSkeletonArcs = img.idealSkeletonArcs;

        idealContourPath = img.idealContourPath;

        isIdealContourSetFlag = img.isIdealContourSetFlag;
    }

    inline void getIdealContour(QVector<QLine> &idealLines, QVector<QPoint> &innerNormalVectors, QVector<QPoint> &outerNormalVectors, QPointF &_point_of_origin, qreal &_rotation_angle)
    {
        idealLines = idealSkeletonLines;
        innerNormalVectors = innerIdealSkeletonNormalVectors;
        outerNormalVectors = outerIdealSkeletonNormalVectors;

        _point_of_origin = point_of_origin;
        _rotation_angle = rotation_angle;
    }

    inline QPainterPath& getIdealContourPath()
    {
        return idealContourPath;
    }

    inline ImageType_e getImageType()
    {
        return image_type;
    }

    QPainterPath drawIdealContour(const QRect &r, const QPointF &_point_of_origin, qreal _rotation_angle, qreal _scale = 1.0);

    static int pointDistanceToLine(const QPoint &pt, const QLine &line);
    void addIdealSkeletonLine(const QPoint &p1, const QPoint &p2, int normal_vecror_x_sign);
    void addIdealSkeletonArc(const QPointF &center, const QPointF &start_point, const qreal R, const qreal phi);
    bool findNearestIdealLineNormalVector(const QPoint &pt, QLine &last_line, QPoint &internalToleranceNormalVector, QPoint &externalToleranceNormalVector);

    bool getQImage(QImage &img);

    inline Mat& getImage()
    {
        return image;
    }

    inline libraw_processed_image_t *getRawImage()
    {
        return raw_image;
    }

    void release();

    inline QString getFilename()
    {
        return filename;
    }

    inline void setImageContour(const Mat &contour)
    {
        image_contour = contour;
    }

    inline Mat& getImageContour()
    {
        return image_contour;
    }

    inline bool isReferencePointsAreSet()
    {
        return !(reference_point1 == reference_point2);
    }

    void setReferencePoints(const QPoint &p1, const QPoint &p2);

    inline void getReferencePoints(QPoint &reference_point1, QPoint &reference_point2)
    {
        reference_point1 = this->reference_point1;
        reference_point2 = this->reference_point2;
    }

    void setReferencePointDistanceMkm(quint32 d)
    {
        reference_point_distance_mkm = d;

        calculateCalibrationRatio();
    }

    inline quint32 getReferencePointDistanceMkm()
    {
        return reference_point_distance_mkm;
    }

    inline quint32 getReferencePointDistancePx()
    {
        return reference_point_distance_px;
    }

    inline qreal getCalibrationRatio()
    {
        return calibration_ratio;
    }

    inline void setToleranceFields(const quint16 ext_tolerance_mkm, const quint16 int_tolerance_mkm)
    {
        externalToleranceMkm = ext_tolerance_mkm;
        internalToleranceMkm = int_tolerance_mkm;
    }

    inline void getToleranceFields(quint16 &ext_tolerance_mkm, quint16 &int_tolerance_mkm)
    {        
        ext_tolerance_mkm = externalToleranceMkm;
        int_tolerance_mkm = internalToleranceMkm;
    }

    inline bool isIdealContourSet()
    {
        return isIdealContourSetFlag;
    }

    inline void setIdealContourSetFlag(bool value)
    {
        isIdealContourSetFlag = value;
    }
};

#endif // PUANSONIMAGE_H
