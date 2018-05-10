#include "puansonimage.h"

#include <QtMath>
#include <QDebug>

PuansonImage::PuansonImage(const ImageType_e _image_type, const Mat &_image, const libraw_processed_image_t *_raw_image, const QString &_filename):
    image(_image), raw_image(const_cast<libraw_processed_image_t *>(_raw_image)), p_raw_image_reference_counter(new quint16(1)), filename(_filename),
    image_type(_image_type), empty(false), reference_point_distance_mkm(0), reference_point_distance_px(0), calibration_ratio(0.0),
    externalToleranceMkm(0), internalToleranceMkm(0), isIdealContourSetFlag(false)
{
}

PuansonImage::PuansonImage():
    raw_image(NULL), p_raw_image_reference_counter(new quint16(0)), filename(""), image_type(UNDEFINED_IMAGE),
    empty(true), reference_point_distance_mkm(0), reference_point_distance_px(0), calibration_ratio(0.0),
    externalToleranceMkm(0), internalToleranceMkm(0), isIdealContourSetFlag(false)
{
}

PuansonImage::~PuansonImage()
{
    release();
}

PuansonImage &PuansonImage::operator=(const PuansonImage &img)
{
    release();

    image = img.image;
    image_contour = img.image_contour;
    filename = img.filename;
    image_type = img.image_type;
    raw_image = img.raw_image;
    empty = img.empty;

    reference_point1 = img.reference_point1;
    reference_point2 = img.reference_point2;

    reference_point_distance_mkm = img.reference_point_distance_mkm;
    reference_point_distance_px = img.reference_point_distance_px;

    calibration_ratio = img.calibration_ratio;
    externalToleranceMkm = img.externalToleranceMkm;
    internalToleranceMkm = img.internalToleranceMkm;

    idealSkeletonLines = img.idealSkeletonLines;
    innerIdealSkeletonNormalVectors = img.innerIdealSkeletonNormalVectors;
    outerIdealSkeletonNormalVectors = img.outerIdealSkeletonNormalVectors;

    p_raw_image_reference_counter = img.p_raw_image_reference_counter;
    (*p_raw_image_reference_counter)++;

    return *this;
}

void PuansonImage::getArc(const qreal R, const QPointF &point1, const QPointF &point2, const QPointF &point3, QPointF &_point1, QPointF &_point2, QPointF &point0, qreal &phi, qreal &alpha)
{
    qreal L1 = qSqrt((point1.x() - point2.x()) * (point1.x() - point2.x()) + (point1.y() - point2.y()) * (point1.y() - point2.y()));
    qreal L2 = qSqrt((point3.x() - point2.x()) * (point3.x() - point2.x()) + (point3.y() - point2.y()) * (point3.y() - point2.y()));

    phi = qAcos(((point1.x() - point2.x()) * (point3.x() - point2.x()) + ((point1.y() - point2.y()) * (point3.y() - point2.y())))
                   / (L1 * L2) );

    qreal l = R / qTan(phi / 2.0);

    /*qDebug() << qRadiansToDegrees(phi);
    qDebug() << "R " << R << " l " << l << " L1 " << L1 << " L2 " << L2;
    qDebug() << "phi " << phi << " phi / 2.0 " << phi / 2.0 << " qTan(phi / 2.0) " << qTan(phi / 2.0);
    qDebug() << "point1 " << point1;
    qDebug() << "point2 " << point2;
    qDebug() << "point3 " << point3;
    qDebug() << "l / L1 " << l / L1;
    qDebug() << "l / L2 " << l / L2;*/

    // Точка начала скругления
    _point1 = QPointF(point2.x() - (point2.x() - point1.x()) * l / L1, point2.y() - (point2.y() - point1.y()) * l / L1);
    // Точка окончания скругления
    _point2 = QPointF(point2.x() + (point3.x() - point2.x()) * l / L2, point2.y() + (point3.y() - point2.y()) * l / L2);

    // Центр окружности скругления
    qreal y0 = ( _point1.y() * (point2.y() -_point1.y()) * (_point2.x() - point2.x()) + _point2.y() * (_point1.x() - point2.x()) * (_point2.y() - point2.y()) - (_point2.x() - point2.x()) * (_point1.x() - _point2.x()) * (_point1.x() - point2.x()) ) /
            ( (point2.y() - _point1.y()) * (_point2.x() - point2.x()) + (_point1.x() - point2.x()) * (_point2.y() - point2.y()) );
    qreal x0 = ((point2.y() - _point1.y()) * (y0 - _point1.y())) / (_point1.x() - point2.x()) + _point1.x();
    point0 = QPointF(x0, y0);

    // Угол начала дуги скругления
    alpha = qAcos( (_point1.x() - point0.x())  / qSqrt( (_point1.x() - point0.x()) * (_point1.x() - point0.x()) + (_point1.y() - point0.y()) * (_point1.y() - point0.y()) ) );

    /*qDebug() << "_point1 " << _point1;
    qDebug() << "_point2 " << _point2;
    qDebug() << "point0 " << point0;

    qDebug() << "dist 1 " << qSqrt((_point1.x() - point0.x()) * (_point1.x() - point0.x()) + (_point1.y() - point0.y()) * (_point1.y() - point0.y()));
    qDebug() << "dist 2 " << qSqrt((_point2.x() - point0.x()) * (_point2.x() - point0.x()) + (_point2.y() - point0.y()) * (_point2.y() - point0.y()));*/
}

void PuansonImage::addIdealSkeletonLine(const QPoint &p1, const QPoint &p2, int normal_vecror_x_sign)
{
    QLine new_line;
    QPoint int_normal_vector, ext_normal_vector;
    qreal x_0;
    qreal y_0;

    new_line = QLine(p1, p2);

    idealSkeletonLines.append(new_line);

    // Внутренний допуск
    x_0 = p2.x() - p1.x();
    y_0 = p1.y() - p2.y();

    int int_normal_vector_X = qRound((internalToleranceMkm / calibration_ratio) / qSqrt(1 + (x_0 / y_0) * (x_0 / y_0)));

    int_normal_vector.setX((normal_vecror_x_sign > 0) ? int_normal_vector_X : -int_normal_vector_X);
    int_normal_vector.setY(qRound((x_0 / y_0) * int_normal_vector.x()));

    innerIdealSkeletonNormalVectors.append(int_normal_vector);
    ////////////////////

    // Внешний допуск
    int ext_normal_vector_X = qRound((externalToleranceMkm  / calibration_ratio) / qSqrt(1 + (x_0 / y_0) * (x_0 / y_0)));

    ext_normal_vector.setX((normal_vecror_x_sign > 0) ? -ext_normal_vector_X : ext_normal_vector_X);
    ext_normal_vector.setY(qRound((x_0 / y_0) * ext_normal_vector.x()));

    outerIdealSkeletonNormalVectors.append(ext_normal_vector);
    ////////////////////
}

void PuansonImage::addIdealSkeletonArc(const QPointF &center, const QPointF &start_point, const qreal R, const qreal phi)
{
    idealSkeletonArcs.append(IdealContourArc(center, start_point, R, phi));
}

QPainterPath PuansonImage::drawIdealContour(const QRect &r, const QPointF &_point_of_origin, qreal _rotation_angle, qreal _scale)
{
    idealContourPath = QPainterPath();

    point_of_origin = _point_of_origin;
    rotation_angle = _rotation_angle;

    QTransform  t;
    t.translate(point_of_origin.x(), point_of_origin.y());
    t.rotate(rotation_angle);
    t.translate(-point_of_origin.x(), -point_of_origin.y());

    qreal ratio = 1.0 / calibration_ratio;

    QPainterPath border_path;
    QPointF new_point;

    const qreal line_1_x = _scale * 5500;
    const qreal line_1_y = _scale * 0;
    const qreal line_2_x = _scale * 2550;
    const qreal line_2_y = _scale * 6000;
    const qreal line_3_x = _scale * 850;
    const qreal line_3_y = _scale * 52000;

    // Радиус скругления
    const qreal R = _scale * 5000 * ratio;

    border_path.moveTo(0, 0);
    border_path.lineTo(r.width() - 3, 0);
    border_path.lineTo(r.width() - 3, r.height() - 3);
    border_path.lineTo(0, r.height() - 3);
    border_path.closeSubpath();

    QPointF point1;
    QPointF point2;
    QPointF point3;

    QPointF _point1;
    QPointF _point2;
    QPointF point0;
    qreal phi;
    qreal alpha;

    // Скругление 1
    point1 = QPointF(line_1_x * ratio, line_1_y * ratio);
    point2 = QPointF(line_2_x * ratio, line_2_y * ratio);
    point3 = QPointF(line_3_x * ratio, line_3_y * ratio);

    if(!idealSkeletonLines.isEmpty())
    {
        idealSkeletonLines.clear();
        innerIdealSkeletonNormalVectors.clear();
        outerIdealSkeletonNormalVectors.clear();
        idealSkeletonArcs.clear();
    }

    getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((_point1/*point2*/ + point_of_origin).toPoint()), 1);
    addIdealSkeletonLine(t.map((_point2/*point2*/ + point_of_origin).toPoint()), t.map((point3 + point_of_origin).toPoint()), 1);

    addIdealSkeletonArc(t.map((point0 + point_of_origin).toPoint()), t.map((_point1 + point_of_origin).toPoint()), R, 180.0 - qRadiansToDegrees(phi));
    // ----------

    new_point = point_of_origin;
    //new_point = t.map(new_point);
    idealContourPath.moveTo(new_point);

    new_point = point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + _point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point0;
    idealContourPath.arcTo(QRectF(new_point.x() - R, new_point.y() - R, R * 2.0, R * 2.0), qRadiansToDegrees(alpha), 180.0 - qRadiansToDegrees(phi));

    new_point = point_of_origin + _point2;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point3;
    idealContourPath.lineTo(new_point);

    // Скругление 2
    point1 = QPointF(-line_3_x * ratio, line_3_y * ratio);
    point2 = QPointF(-line_2_x * ratio, line_2_y * ratio);
    point3 = QPointF(-line_1_x * ratio, line_1_y * ratio);

    getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((_point1/*point2*/ + point_of_origin).toPoint()), -1);
    addIdealSkeletonLine(t.map((_point2/*point2*/ + point_of_origin).toPoint()), t.map((point3 + point_of_origin).toPoint()), -1);

    addIdealSkeletonArc(t.map((point0 + point_of_origin).toPoint()), t.map((_point1 + point_of_origin).toPoint()), R, 180.0 - qRadiansToDegrees(phi));
    // ----------

    new_point = point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + _point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point0;
    idealContourPath.arcTo(QRectF(new_point.x() - R, new_point.y() - R, R * 2.0, R * 2.0), qRadiansToDegrees(alpha), 180.0 - qRadiansToDegrees(phi));

    new_point = point_of_origin + _point2;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point3;
    idealContourPath.lineTo(new_point);

    addIdealSkeletonLine(t.map((point3 + point_of_origin).toPoint()), t.map(QPoint(line_1_x * ratio, line_1_y * ratio) + point_of_origin.toPoint()), 0);

    idealContourPath.closeSubpath();

    idealContourPath = t.map(idealContourPath);
    idealContourPath = idealContourPath.intersected(border_path);

    return idealContourPath;
}

int PuansonImage::pointDistanceToLine(const QPoint &pt, const QLine &line)
{
    int dist  = qRound(std::abs((line.y2() - line.y1()) * pt.x() - (line.x2() - line.x1()) * pt.y() + line.x2() * line.y1() - line.y2() * line.x1())
                   / qSqrt((line.y2() - line.y1()) * (line.y2() - line.y1()) + (line.x2() - line.x1()) * (line.x2() - line.x1())));

    return dist;
}

bool PuansonImage::findNearestIdealLineNormalVector(const QPoint &pt, QLine &last_line, QPoint &internalToleranceNormalVector, QPoint &externalToleranceNormalVector)
{
#define NEAREST_LINE_MAX_DISTANCE 20

    QLine nearest_line;
    quint16 nearest_line_distance = UINT16_MAX;
    quint16 current_line_distance;

    IdealContourArc nearest_arc;

    // Arcs
    for(auto it = idealSkeletonArcs.begin(); it != idealSkeletonArcs.end(); it++)
    {
        qreal L = 100000.0;
        qreal ll = qSqrt((pt.x() - it->Center().x()) * (pt.x() - it->Center().x()) + (pt.y() - it->Center().y()) * (pt.y() - it->Center().y()));
        qreal _phi = qRadiansToDegrees(qAcos(((pt.x() - it->Center().x()) * (it->StartPoint().x() - it->Center().x()) + (pt.y() - it->Center().y()) * (it->StartPoint().y() - it->Center().y())) \
                / (ll * qSqrt(((it->StartPoint().x() - it->Center().x()) * (it->StartPoint().x() - it->Center().x()) + (it->StartPoint().y() - it->Center().y()) * (it->StartPoint().y() - it->Center().y()))))));

        qreal pseudoscalar_production = (it->StartPoint().x() - it->Center().x()) * (pt.y() - it->Center().y()) - (it->StartPoint().y() - it->Center().y()) * (pt.x() - it->Center().x());

        if(pseudoscalar_production < 0.0 && _phi < it->Phi())
        {
            L = std::abs(it->R() - ll);

            if(L < nearest_line_distance)
            {
                nearest_arc = *it;
                nearest_line_distance = L;
                internalToleranceNormalVector = QPoint(qRound((pt.x() - it->Center().x()) / ll * internalToleranceMkm / calibration_ratio), qRound((pt.y() - it->Center().y()) / ll * internalToleranceMkm / calibration_ratio));
                externalToleranceNormalVector = QPoint(-qRound((pt.x() - it->Center().x()) / ll * externalToleranceMkm / calibration_ratio), -qRound((pt.y() - it->Center().y()) / ll * externalToleranceMkm  / calibration_ratio));
            }
        }
    }

    // Lines
    auto line_it = idealSkeletonLines.begin();
    auto inner_it = innerIdealSkeletonNormalVectors.begin();
    auto outer_it = outerIdealSkeletonNormalVectors.begin();
int D = 10;
    if(!last_line.isNull() &&
            (
                ((current_line_distance = pointDistanceToLine(pt, last_line)) < nearest_line_distance) &&
                (current_line_distance < NEAREST_LINE_MAX_DISTANCE) &&
                ((pt.x() >= qMin(last_line.x1(), last_line.x2()) - D) && (pt.x() <= qMax(last_line.x1(), last_line.x2()) + D) &&
                 (pt.y() >= qMin(last_line.y1(), last_line.y2()) - D) && (pt.y() <= qMax(last_line.y1(), last_line.y2()) + D))
            )
    )
    {
        for(; *line_it != last_line && line_it != idealSkeletonLines.end(); line_it++, inner_it++, outer_it++) { }

        if(line_it != idealSkeletonLines.end())
        {
            internalToleranceNormalVector = *inner_it;
            externalToleranceNormalVector = *outer_it;

            return true;
        }
        else
        {
            last_line.setLine(0, 0, 0, 0);
            return false;
        }
    }

    for(line_it = idealSkeletonLines.begin(); line_it != idealSkeletonLines.end(); line_it++, inner_it++, outer_it++)
    {
        current_line_distance = pointDistanceToLine(pt, *line_it);

        if(current_line_distance < nearest_line_distance && ((pt.x() >= qMin(line_it->x1(), line_it->x2()) - D) && (pt.x() <= qMax(line_it->x1(), line_it->x2()) + D) &&
                                                             (pt.y() >= qMin(line_it->y1(), line_it->y2()) - D) && (pt.y() <= qMax(line_it->y1(), line_it->y2()) + D)))
        {
            if(!nearest_arc.Center().isNull())
                nearest_arc = IdealContourArc();

            nearest_line_distance = current_line_distance;
            nearest_line = *line_it;
            internalToleranceNormalVector = *inner_it;
            externalToleranceNormalVector = *outer_it;
        }
    }

    if(nearest_line_distance <= NEAREST_LINE_MAX_DISTANCE)
    {
        if(nearest_arc.Center().isNull())       // Nearest is line
        {
//            qDebug() << "line nearest_line_distance " << nearest_line_distance;
            last_line = nearest_line;
        }
        else                                    // Nearest is arc
        {
//            qDebug() << "arc nearest_line_distance " << nearest_line_distance;
            last_line.setLine(0, 0, 0, 0);
        }

        return true;
    }
    else
    {
        last_line.setLine(0, 0, 0, 0);
        return false;
    }
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
{
    image.release();
    image_contour.release();
    if(p_raw_image_reference_counter != NULL)
    {
        if(*p_raw_image_reference_counter > 0)
            --(*p_raw_image_reference_counter);

        if(*p_raw_image_reference_counter == 0 && raw_image != NULL)
        {
            LibRaw::dcraw_clear_mem(raw_image);
            delete p_raw_image_reference_counter;
            p_raw_image_reference_counter = NULL;

            raw_image = NULL;
        }
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
