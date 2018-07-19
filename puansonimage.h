#ifndef PUANSONIMAGE_H
#define PUANSONIMAGE_H

#include <QtGlobal>
#include <QString>
#include <QPoint>
#include <QImage>
#include <QtMath>

#include <QVector2D>

#include <QDebug>

#include <opencv2/core/core.hpp>

#define LIBRAW_NODLL
#include <libraw/libraw.h>

using namespace cv;

enum ImageType_e
{
    ETALON_IMAGE = 0,
    CURRENT_IMAGE = 1,
    REFERENCE_POINT_IMAGE = 2,
    UNDEFINED_IMAGE = 3
};

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

class IdealInnerSegment
{
public:
    IdealInnerSegment() { }

    IdealInnerSegment(const QPoint &_start_point, const QLine &first_line, const QPoint &_end_point = QPoint(0, 0)):
        start_point(_start_point), end_point(_end_point)
    {
        inner_path.moveTo(_start_point.x(), _start_point.y());
        inner_path.lineTo(first_line.x2(), first_line.y2());
        inner_lines.append(first_line);
    }

    IdealInnerSegment(const QPoint &_start_point, const IdealContourArc &first_arc, const QPoint &_end_point = QPoint(0, 0)):
        start_point(_start_point), end_point(_end_point)
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

    inline void setEndPoint(const QPoint &point)
    {
        end_point = point;
    }

    inline QPoint getStartPoint()
    {
        return start_point;
    }

    inline QPoint getEndPoint()
    {
        return end_point;
    }

    inline bool isStartPointNull()
    {
        return start_point.isNull();
    }

    inline bool isEndPointNull()
    {
        return end_point.isNull();
    }

    inline QVector<QLine> getInnerLines()
    {
        return inner_lines;
    }

    inline QVector<IdealContourArc> getInnerArcs()
    {
        return inner_arcs;
    }

    inline QPainterPath getInnerPath()
    {
        return inner_path;
    }

    inline QPainterPath &InnerPath()
    {
        return inner_path;
    }

    QVector<QLine> getControlLines(quint8 step);

private:
    QPoint start_point;
    QPoint end_point;
    QVector<QLine> inner_lines;
    QVector<IdealContourArc> inner_arcs;

    QPainterPath inner_path;
};

class PuansonImage
{
    Mat image;
    libraw_processed_image_t *raw_image;
    quint16 *p_raw_image_reference_counter;
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

    QVector<IdealInnerSegment> actualIdealInnerSegments;

    QPainterPath idealContourPath;

    bool isIdealContourSetFlag;

    // Actual borders
    int Y_top;
    int Y_bottom;
    int X_left;
    int X_right;

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

    bool findP0(const QPointF &center, const qreal R, QPointF &start_point, qreal &alpha, qreal &phi, QPoint &p0);
    bool findPN(const QPointF &center, const qreal R, QPointF &start_point, qreal &alpha, qreal &phi, QPoint &pN);

public:
    PuansonImage(const ImageType_e _image_type, const Mat &_image, const libraw_processed_image_t *_raw_image, const QString &_filename);
    PuansonImage();
    ~PuansonImage();

    PuansonImage &operator=(const PuansonImage &img);

    inline bool isEmpty()
    {
        return empty;
    }

    inline bool isCropped()
    {
        return cropped;
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

    inline void cropImage(qreal scale = 4.0)
    {
        image = Mat(image, Rect(qRound((image.cols - image.cols / scale) / 2.0),
                                              qRound((image.rows - image.rows / scale) / 2.0),
                                              image.cols / scale, image.rows / scale));

        crop_scale = scale;
        cropped = true;
    }

    inline qreal getCropScale()
    {
        return crop_scale;
    }

    inline QPainterPath& getIdealContourPath()
    {
        return idealContourPath;
    }

    inline QVector<IdealInnerSegment> getActualIdealInnerSegments()
    {
        return actualIdealInnerSegments;
    }

    inline ImageType_e getImageType()
    {
        return image_type;
    }

    QPainterPath drawIdealContour(const QRect &r, const QPointF &_point_of_origin, qreal _rotation_angle, qreal _scale = 1.0);

    static int pointDistanceToLine(const QPoint &pt, const QLine &line);
    void addIdealSkeletonLine(const QPoint &p1, const QPoint &p2, int normal_vecror_x_sign);
    void addIdealSkeletonArc(const QPointF &center, const QPointF &start_point, const qreal R, const qreal alpha, const qreal phi);
    bool findNearestIdealLineNormalVector(const QPoint &pt, QLine &last_line, QPoint &internalToleranceNormalVector, QPoint &externalToleranceNormalVector);

    bool autoCkeckDetail();

    bool getQImage(QImage &img);

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
