#ifndef PUANSONIMAGE_H
#define PUANSONIMAGE_H

#include <QtGlobal>
#include <QString>
#include <QPoint>
#include <QImage>

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
    quint16 externalTolerancePx;
    quint16 internalTolerancePx;

    // Inner skeleton
    QPoint inner_skeleton_top;
    QPoint inner_skeleton_left;
    QPoint inner_skeleton_right;
    QPoint inner_skeleton_bottom;

    void calculateCalibrationRatio()
    {
        if(reference_point_distance_px == 0)
            calibration_ratio = 0;
        else
            calibration_ratio = static_cast<qreal>(reference_point_distance_mkm) / static_cast<qreal>(reference_point_distance_px);
    }

public:
    PuansonImage(const ImageType_e _image_type, const Mat &_image, const libraw_processed_image_t *_raw_image, const QString &_filename);
    PuansonImage();
    ~PuansonImage();

    PuansonImage &operator=(const PuansonImage &img);

    inline bool isEmpty()
    {
        return empty;
    }

    inline ImageType_e getImageType()
    {
        return image_type;
    }

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

    inline bool isInnerSkelenonPointsAreSet()
    {
        return !((inner_skeleton_top == inner_skeleton_right) && (inner_skeleton_bottom == inner_skeleton_left) && (inner_skeleton_top == inner_skeleton_bottom));
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

    inline void setToleranceFields(const quint16 ext_tolerance_px, const quint16 int_tolerance_px)
    {
        externalTolerancePx = ext_tolerance_px;
        internalTolerancePx = int_tolerance_px;
    }

    inline void getToleranceFields(quint16 &ext_tolerance_px, quint16 &int_tolerance_px)
    {        
        ext_tolerance_px = externalTolerancePx;
        int_tolerance_px = internalTolerancePx;
    }

    inline void setInnerSkeleton(const QPoint &top, const QPoint &right, const QPoint &bottom, const QPoint &left)
    {
        inner_skeleton_top = top;
        inner_skeleton_right = right;
        inner_skeleton_bottom = bottom;
        inner_skeleton_left = left;
    }

    inline void getInnerSkeleton(QPoint &top, QPoint &right, QPoint &bottom, QPoint &left)
    {
        top = inner_skeleton_top;
        right = inner_skeleton_right;
        bottom = inner_skeleton_bottom;
        left = inner_skeleton_left;
    }
};

#endif // PUANSONIMAGE_H
