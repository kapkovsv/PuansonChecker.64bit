#include "puansonimage.h"

#include <QtMath>
#include <QDebug>

PuansonImage::PuansonImage(const ImageType_e _image_type, const Mat &_image, const libraw_processed_image_t *_raw_image, const QString &_filename):
    empty(false), image_type(_image_type), image(_image), raw_image(const_cast<libraw_processed_image_t *>(_raw_image)), filename(_filename),
    reference_point_distance_mkm(0), reference_point_distance_px(0), calibration_ratio(0.0), externalTolerancePx(0), internalTolerancePx(0),
    p_raw_image_reference_counter(new quint16(1))
{
}

PuansonImage::PuansonImage():
    empty(true), image_type(UNDEFINED_IMAGE), raw_image(NULL), filename(""), reference_point_distance_mkm(0),
    reference_point_distance_px(0), calibration_ratio(0.0), externalTolerancePx(0), internalTolerancePx(0),
    p_raw_image_reference_counter(new quint16(0))
{
}

PuansonImage::~PuansonImage()
{
    release();
}

PuansonImage &PuansonImage::operator=(const PuansonImage &img)
{qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
    release();
qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
    image = img.image;
    image_contour = img.image_contour;
    filename = img.filename;
    image_type = img.image_type;
    raw_image = img.raw_image;
    empty = img.empty;
qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
    reference_point1 = img.reference_point1;
    reference_point2 = img.reference_point2;

    inner_skeleton_top = img.inner_skeleton_top;
    inner_skeleton_right = img.inner_skeleton_right;
    inner_skeleton_bottom = img.inner_skeleton_bottom;
    inner_skeleton_left = img.inner_skeleton_left;

    reference_point_distance_mkm = img.reference_point_distance_mkm;
    reference_point_distance_px = img.reference_point_distance_px;
qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
    calibration_ratio = img.calibration_ratio;
    externalTolerancePx = img.externalTolerancePx;
    internalTolerancePx = img.internalTolerancePx;

    p_raw_image_reference_counter = img.p_raw_image_reference_counter;
    (*p_raw_image_reference_counter)++;qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
}

bool PuansonImage::getQImage(QImage &img)
{
    if(image.empty())
        return false;

    img = QImage((const unsigned char*)(image.data),
                  image.cols, image.rows,
                  static_cast<int>(image.step), QImage::Format_RGB888).rgbSwapped();

    return true;
}

void PuansonImage::release()
{qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
    image.release();
    image_contour.release();
qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
qDebug() << "p_raw_image_reference_counter " << p_raw_image_reference_counter;
    if(*p_raw_image_reference_counter > 0)
        --(*p_raw_image_reference_counter);
qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
    if(*p_raw_image_reference_counter == 0 && raw_image != NULL)
    {qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
        LibRaw::dcraw_clear_mem(raw_image);
        delete p_raw_image_reference_counter;
        p_raw_image_reference_counter = NULL;
qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
        raw_image = NULL;
    }
}

void PuansonImage::setReferencePoints(const QPoint &p1, const QPoint &p2)
{
    reference_point1 = p1;
    reference_point2 = p2;

    QPoint distance_vector_px = reference_point2 - reference_point1;
    reference_point_distance_px = qSqrt(distance_vector_px.x()*distance_vector_px.x() + distance_vector_px.y()*distance_vector_px.y());

    calculateCalibrationRatio();
}
