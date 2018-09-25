#include "puansonimage.h"
#include "puansonchecker.h"

#include <QGenericMatrix>
#include <QVector2D>
#include <QtMath>
#include <QFile>
#include <QtXml>
#include <QFont>
#include <QDebug>

#include <functional>

const qreal PuansonImage::default_calibration_ratio = 4.95613;
const quint16 PuansonImage::GrooveMeasurements::measurement_depth_radius_mkm = 200U;

PuansonImage::PuansonImage(const ImageType_e _image_type, const Mat &_image, const libraw_processed_image_t *_raw_image, const QString &_filename):
    image(_image), raw_image(const_cast<libraw_processed_image_t *>(_raw_image)), p_raw_image_reference_counter(new quint16(1)), filename(_filename),
    image_type(_image_type), empty(false), cropped(false), crop_scale(1.0), reference_point_distance_mkm(0), reference_point_distance_px(0),
    calibration_ratio(0.0), externalToleranceMkm(0), internalToleranceMkm(0), isIdealContourSetFlag(false)
{
    Y_top = qRound((image.rows - image.rows / 4.0) / 2.0);
    Y_bottom = qRound((image.rows + image.rows / 4.0) / 2.0);
    X_left = qRound((image.cols - image.cols / 4.0) / 2.0);
    X_right = qRound((image.cols + image.cols / 4.0) / 2.0);

    setEtalonResearchPuansonModel(detail_research_puanson_model);
}

PuansonImage::PuansonImage():
    raw_image(Q_NULLPTR), p_raw_image_reference_counter(new quint16(0)), filename(""), image_type(ImageType_e::UNDEFINED_IMAGE),
    empty(true), cropped(false), crop_scale(1.0), reference_point_distance_mkm(0), reference_point_distance_px(0),
    calibration_ratio(0.0), externalToleranceMkm(0), internalToleranceMkm(0), isIdealContourSetFlag(false),
    Y_top(0), Y_bottom(0), X_left(0), X_right(0)
{
    setEtalonResearchPuansonModel(detail_research_puanson_model);
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

    copyIdealContour(img);

    Y_top = img.Y_top;
    Y_bottom = img.Y_bottom;
    X_left = img.X_left;
    X_right = img.X_right;

    p_raw_image_reference_counter = img.p_raw_image_reference_counter;
    (*p_raw_image_reference_counter)++;

    return *this;
}

PuansonImage &PuansonImage::operator^=(const PuansonImage &img)
{
    release();

    image = img.image;
    image_contour = img.image_contour;
    filename = img.filename;
    image_type = img.image_type;
    raw_image = img.raw_image;
    empty = img.empty;

    Y_top = img.Y_top;
    Y_bottom = img.Y_bottom;
    X_left = img.X_left;
    X_right = img.X_right;

    p_raw_image_reference_counter = img.p_raw_image_reference_counter;
    (*p_raw_image_reference_counter)++;

    return *this;
}

QVector<QLine> IdealInnerSegment::getControlLines(const quint8 step, const QRect &analysis_rect) const
{
    using namespace std;

    QVector<QLine> control_lines;
    const qreal normal_vector_len = 80.0;

    for(const QLine &inner_line: inner_lines)
    {
        qreal dx = inner_line.x2() - inner_line.x1();
        qreal dy = inner_line.y2() - inner_line.y1();
        qreal inner_line_length = qSqrt(dx * dx + dy * dy);
        QPointF line_ort(dx / inner_line_length, dy / inner_line_length);
        QPointF int_normal_vector;
        QPointF pt;

        function<bool (const QPointF &, const QLine &)> upConditionLambda = [](const QPointF &pt, const QLine &line) -> bool { return pt.x() < (line.x1() > line.x2() ? line.x1() : line.x2()); };
        function<bool (const QPointF &, const QLine &)> downConditionLambda = [](const QPointF &pt, const QLine &line) -> bool { return pt.x() > (line.x1() < line.x2() ? line.x1() : line.x2()); };
        function<bool (const QPointF &, const QLine &)> conditionLambda;

        int_normal_vector.setX(normal_vector_len / qSqrt(1 + dx / dy * dx / dy));
        int_normal_vector.setY(-(dx / dy) * int_normal_vector.x());

        if(line_ort.x() > 0)
        {
            pt = inner_line.x1() < inner_line.x2() ? inner_line.p1() : inner_line.p2();
            conditionLambda = upConditionLambda;
        }
        else
        {
            pt = inner_line.x1() > inner_line.x2() ? inner_line.p1() : inner_line.p2();
            conditionLambda = downConditionLambda;
        }

        do
        {
            QLine new_control_line = QLineF(pt - int_normal_vector, pt + int_normal_vector).toLine();

            if(!analysis_rect.isNull())
            {
                if(new_control_line.y1() > analysis_rect.bottom())
                {
                    new_control_line.setP1((pt - int_normal_vector * (analysis_rect.bottom() - pt.y()) / qAbs(int_normal_vector.y())).toPoint());
                }
                else if(new_control_line.y1() < analysis_rect.top())
                {
                    new_control_line.setP1((pt - int_normal_vector * (pt.y() - analysis_rect.top()) / qAbs(int_normal_vector.y())).toPoint());
                }

                if(new_control_line.x1() > analysis_rect.right())
                {
                    new_control_line.setP1((pt - int_normal_vector * (analysis_rect.right() - pt.x()) / qAbs(int_normal_vector.x())).toPoint());
                }
                else if(new_control_line.x1() < analysis_rect.left())
                {
                    new_control_line.setP1((pt - int_normal_vector * (pt.x() - analysis_rect.left()) / qAbs(int_normal_vector.x())).toPoint());
                }

                if(new_control_line.y2() > analysis_rect.bottom())
                {
                    new_control_line.setP2((pt + int_normal_vector * (analysis_rect.bottom() - pt.y()) / qAbs(int_normal_vector.y())).toPoint());
                }
                else if(new_control_line.y2() < analysis_rect.top())
                {
                    new_control_line.setP2((pt + int_normal_vector * (pt.y() - analysis_rect.top()) / qAbs(int_normal_vector.y())).toPoint());
                }

                if(new_control_line.x2() > analysis_rect.right())
                {
                    new_control_line.setP2((pt + int_normal_vector * (analysis_rect.right() - pt.x()) / qAbs(int_normal_vector.x())).toPoint());
                }
                else if(new_control_line.x2() < analysis_rect.left())
                {
                    new_control_line.setP2((pt + int_normal_vector * (pt.x() - analysis_rect.left()) / qAbs(int_normal_vector.x())).toPoint());
                }
            }

            control_lines.append(new_control_line);
            pt += line_ort * step;
        }
        while(conditionLambda(pt, inner_line));
    }

    for(const IdealContourArc &inner_arc: inner_arcs)
    {
        QPointF pt = inner_arc.StartPoint();
        QPointF int_normal_vector;
        QMatrix m;
        qreal delta_phi = qRadiansToDegrees((inner_arc.Phi() >= 0 ? 1 : -1) * step / inner_arc.R());
        qreal current_phi = 0.0;

        m.rotate(-delta_phi);

        do
        {
            int_normal_vector = (inner_arc.Center() - pt) / inner_arc.R();

            QLine new_control_line = QLineF(pt - int_normal_vector * normal_vector_len, pt + int_normal_vector * normal_vector_len).toLine();

            if(!analysis_rect.isNull())
            {
                if(new_control_line.y1() > analysis_rect.bottom())
                {
                    new_control_line.setP1((pt - int_normal_vector * (analysis_rect.bottom() - pt.y()) / qAbs(int_normal_vector.y())).toPoint());
                }
                else if(new_control_line.y1() < analysis_rect.top())
                {
                    new_control_line.setP1((pt - int_normal_vector * (pt.y() - analysis_rect.top()) / qAbs(int_normal_vector.y())).toPoint());
                }

                if(new_control_line.x1() > analysis_rect.right())
                {
                    new_control_line.setP1((pt - int_normal_vector * (analysis_rect.right() - pt.x()) / qAbs(int_normal_vector.x())).toPoint());
                }
                else if(new_control_line.x1() < analysis_rect.left())
                {
                    new_control_line.setP1((pt - int_normal_vector * (pt.x() - analysis_rect.left()) / qAbs(int_normal_vector.x())).toPoint());
                }

                if(new_control_line.y2() > analysis_rect.bottom())
                {
                    new_control_line.setP2((pt + int_normal_vector * (analysis_rect.bottom() - pt.y()) / qAbs(int_normal_vector.y())).toPoint());
                }
                else if(new_control_line.y2() < analysis_rect.top())
                {
                    new_control_line.setP2((pt + int_normal_vector * (pt.y() - analysis_rect.top()) / qAbs(int_normal_vector.y())).toPoint());
                }

                if(new_control_line.x2() > analysis_rect.right())
                {
                    new_control_line.setP2((pt + int_normal_vector * (analysis_rect.right() - pt.x()) / qAbs(int_normal_vector.x())).toPoint());
                }
                else if(new_control_line.x2() < analysis_rect.left())
                {
                    new_control_line.setP2((pt + int_normal_vector * (pt.x() - analysis_rect.left()) / qAbs(int_normal_vector.x())).toPoint());
                }
            }

            control_lines.append(new_control_line);

            pt -= inner_arc.Center();
            pt = pt.toPoint() * m;
            pt += inner_arc.Center().toPoint();
            current_phi += delta_phi;
        }
        while(qAbs(current_phi) < qAbs(inner_arc.Phi()));
    }

    return control_lines;
}

void PuansonImage::getArc(const qreal R, const QPointF &point1, const QPointF &point2, const QPointF &point3, QPointF &_point1, QPointF &_point2, QPointF &point0, qreal &phi, qreal &alpha)
{
    bool points_was_changed = false;

    qreal L1 = qSqrt((point1.x() - point2.x()) * (point1.x() - point2.x()) + (point1.y() - point2.y()) * (point1.y() - point2.y()));
    qreal L2 = qSqrt((point3.x() - point2.x()) * (point3.x() - point2.x()) + (point3.y() - point2.y()) * (point3.y() - point2.y()));

    phi = qAcos(((point1.x() - point2.x()) * (point3.x() - point2.x()) + ((point1.y() - point2.y()) * (point3.y() - point2.y())))
                   / (L1 * L2) );

    qreal l = R / qTan(phi / 2.0);

    /*qDebug() << qRadiansToDegrees(phi);
    qDebug() << "R " << R << " l " << l << " L1 " << L1 << " L2 " << L2;
    qDebug() << "phi " << phi << " phi / 2.0.0 " << phi / 2.0.0 << " qTan(phi / 2.0.0) " << qTan(phi / 2.0.0);
    qDebug() << "point1 " << point1;
    qDebug() << "point2 " << point2;
    qDebug() << "point3 " << point3;
    qDebug() << "l / L1 " << l / L1;
    qDebug() << "l / L2 " << l / L2;*/

    // Точка начала скругления
    _point1 = QPointF(point2.x() - (point2.x() - point1.x()) * l / L1, point2.y() - (point2.y() - point1.y()) * l / L1);
    // Точка окончания скругления
    _point2 = QPointF(point2.x() + (point3.x() - point2.x()) * l / L2, point2.y() + (point3.y() - point2.y()) * l / L2);

    if(_point1.x() == point2.x())
    {
        qSwap(_point1, _point2);
        points_was_changed = true;
    }

    // Центр окружности скругления
    qreal y0 = ( _point1.y() * (point2.y() -_point1.y()) * (_point2.x() - point2.x()) + _point2.y() * (_point1.x() - point2.x()) * (_point2.y() - point2.y()) - (_point2.x() - point2.x()) * (_point1.x() - _point2.x()) * (_point1.x() - point2.x()) ) /
            ( (point2.y() - _point1.y()) * (_point2.x() - point2.x()) + (_point1.x() - point2.x()) * (_point2.y() - point2.y()) );
    qreal x0 = ((point2.y() - _point1.y()) * (y0 - _point1.y())) / (_point1.x() - point2.x()) + _point1.x();
    point0 = QPointF(x0, y0);

    if(points_was_changed)
        qSwap(_point1, _point2);

    // Угол начала дуги скругления
    alpha = qAcos( (_point1.x() - point0.x())  / qSqrt( (_point1.x() - point0.x()) * (_point1.x() - point0.x()) + (_point1.y() - point0.y()) * (_point1.y() - point0.y()) ) );

    /*qDebug() << "_point1 " << _point1;
    qDebug() << "_point2 " << _point2;
    qDebug() << "point0 " << point0;

    qDebug() << "dist 1 " << qSqrt((_point1.x() - point0.x()) * (_point1.x() - point0.x()) + (_point1.y() - point0.y()) * (_point1.y() - point0.y()));
    qDebug() << "dist 2 " << qSqrt((_point2.x() - point0.x()) * (_point2.x() - point0.x()) + (_point2.y() - point0.y()) * (_point2.y() - point0.y()));*/
}

bool PuansonImage::autoCkeckDetail()
{
    return false;
}

QVector<QLine> PuansonImage::getMeasurementLines(const QRect &analysis_rect)
{
    QVector<QLine> measurement_lines;
    QPoint measurement_point;
    qreal ratio = 1.0 / calibration_ratio;

    for(const DiameterDimension &deviation: diameter_dimensions)
    {
        if(deviation.measurement_ideal_diameter == 0)
            continue;

        measurement_point = (QPointF(deviation.measurement_ideal_diameter / 2.0, deviation.measurement_y_position) * ratio).toPoint();

        if(analysis_rect.contains(ideal_contour_transform.map(measurement_point + ideal_contour_point_of_origin.toPoint())))
        {
            measurement_lines.append(ideal_contour_transform.map(QLine(measurement_point + ideal_contour_point_of_origin.toPoint(), (QPointF(-static_cast<qint32>(deviation.measurement_ideal_diameter / 2.0), deviation.measurement_y_position) * ratio + ideal_contour_point_of_origin).toPoint())));
        }
        else
        {
            measurement_point = (QPointF(-static_cast<qint32>(deviation.measurement_ideal_diameter / 2.0), deviation.measurement_y_position) * ratio).toPoint();

            if(analysis_rect.contains(ideal_contour_transform.map(measurement_point + ideal_contour_point_of_origin.toPoint())))
            {
                measurement_lines.append(ideal_contour_transform.map(QLine(measurement_point + ideal_contour_point_of_origin.toPoint(), (QPointF(deviation.measurement_ideal_diameter / 2.0, deviation.measurement_y_position) * ratio + ideal_contour_point_of_origin).toPoint())));
            }
        }
    }

    return measurement_lines;
}

bool PuansonImage::setDeviation(const QPoint &measurement_point, const qint32 deviation_value_px)
{
    bool ret_val = false;
    qreal ratio = 1.0 / calibration_ratio;

    for(DiameterDimension &deviation: diameter_dimensions)
    {
        if(measurement_point == ideal_contour_transform.map((ideal_contour_point_of_origin + QPointF(deviation.measurement_ideal_diameter / 2.0, deviation.measurement_y_position) * ratio).toPoint()))
        {
            deviation.right_side_deviation = qRound(deviation_value_px * calibration_ratio);
            ret_val = true;

            break;
        }
        else if(measurement_point == ideal_contour_transform.map((ideal_contour_point_of_origin + QPointF(-static_cast<qint32>(deviation.measurement_ideal_diameter / 2.0), deviation.measurement_y_position) * ratio).toPoint()))
        {
            deviation.left_side_deviation = qRound(deviation_value_px * calibration_ratio);
            ret_val = true;

            break;
        }
    }

    return ret_val;
}

void PuansonImage::addIdealSkeletonLine(const QPoint &p1, const QPoint &p2, const int normal_vecror_x_sign, const bool close_flag)
{
    QLine new_line;
    QPoint int_normal_vector, ext_normal_vector;
    qreal x_0;
    qreal y_0;

    QPoint p0, pN;

    new_line = QLine(p1, p2);

    if(/*p0.isNull() &&*/ (p1.x() <= X_left && p2.x() >= X_left))
    {
        p0 = QPoint(X_left, qRound((X_left - p1.x()) / static_cast<qreal>(p2.x() - p1.x()) * (p2.y() - p1.y()) + p1.y()));

        if(!(p0.y() >= Y_top && p0.y() <= Y_bottom))
            p0 = QPoint();
    }
    if(p0.isNull() && (p1.x() >= X_right && p2.x() <= X_right))
    {
        p0 = QPoint(X_right, qRound((X_right - p1.x()) / static_cast<qreal>(p2.x() - p1.x()) * (p2.y() - p1.y()) + p1.y()));

        if(!(p0.y() >= Y_top && p0.y() <= Y_bottom))
            p0 = QPoint();
    }
    if(p0.isNull() && (p1.y() >= Y_bottom && p2.y() <= Y_bottom))
    {
        p0 = QPoint(qRound((Y_bottom - p1.y()) / static_cast<qreal>(p2.y() - p1.y()) * (p2.x() - p1.x()) + p1.x()), Y_bottom);

        if(!(p0.x() >= X_left && p0.x() <= X_right))
            p0 = QPoint();
    }
    if(p0.isNull() && (p1.y() <= Y_top && p2.y() >= Y_top))
    {
        p0 = QPoint(qRound((Y_top - p1.y()) / static_cast<qreal>(p2.y() - p1.y()) * (p2.x() - p1.x()) + p1.x()), Y_top);

        if(!(p0.x() >= X_left && p0.x() <= X_right))
            p0 = QPoint();
    }

    if(/*pN.isNull() && */(p1.x() >= X_left && p2.x() <= X_left))
    {
        pN = QPoint(X_left, qRound((X_left - p1.x()) / static_cast<qreal>(p2.x() - p1.x()) * (p2.y() - p1.y()) + p1.y()));

        if(!(pN.y() >= Y_top && pN.y() <= Y_bottom) ||
                !(pN.y() >= qMin(p1.y(), p2.y()) && pN.y() <= qMax(p1.y(), p2.y()) &&
                    (pN.x() >= qMin(p1.x(), p2.x()) && pN.x() <= qMax(p1.x(), p2.x()))))
            pN = QPoint();
    }
    if(pN.isNull() && (p1.x() <= X_right && p2.x() >= X_right))
    {
        pN = QPoint(X_right, qRound((X_right - p1.x()) / static_cast<qreal>(p2.x() - p1.x()) * (p2.y() - p1.y()) + p1.y()));

        if(!(pN.y() >= Y_top && pN.y() <= Y_bottom) ||
                !(pN.y() >= qMin(p1.y(), p2.y()) && pN.y() <= qMax(p1.y(), p2.y()) &&
                    (pN.x() >= qMin(p1.x(), p2.x()) && pN.x() <= qMax(p1.x(), p2.x()))))
            pN = QPoint();
    }
    if(pN.isNull() && (p1.y() <= Y_bottom && p2.y() >= Y_bottom))
    {
        pN = QPoint(qRound((Y_bottom - p1.y()) / static_cast<qreal>(p2.y() - p1.y()) * (p2.x() - p1.x()) + p1.x()), Y_bottom);

        if(!(pN.x() >= X_left && pN.x() <= X_right) ||
                !(pN.y() >= qMin(p1.y(), p2.y()) && pN.y() <= qMax(p1.y(), p2.y()) &&
                    (pN.x() >= qMin(p1.x(), p2.x()) && pN.x() <= qMax(p1.x(), p2.x()))))
        {
            pN = QPoint();
        }
    }
    if(pN.isNull() && (p1.y() >= Y_top && p2.y() <= Y_top))
    {
        pN = QPoint(qRound((Y_top - p1.y()) / static_cast<qreal>(p2.y() - p1.y()) * (p2.x() - p1.x()) + p1.x()), Y_top);

        if(!(pN.x() >= X_left && pN.x() <= X_right) ||
                !(pN.y() >= qMin(p1.y(), p2.y()) && pN.y() <= qMax(p1.y(), p2.y()) &&
                    (pN.x() >= qMin(p1.x(), p2.x()) && pN.x() <= qMax(p1.x(), p2.x()))))
            pN = QPoint();
    }

    if(p0.isNull() && (actualIdealInnerSegments.size() == 0 && QRect(QPoint(X_left, Y_top), QPoint(X_right, Y_bottom)).contains(p1)))
        p0 = p1;

    if(pN.isNull() && close_flag)
        pN = p2;

    if(!pN.isNull())
    {
        if(!p0.isNull())
        {// qDebug() << "before actualIdealInnerSegments.append " << __LINE__ << " p0 " << p0 << " pN " << pN;
            actualIdealInnerSegments.append(IdealInnerSegment(p0, QLine(p0, pN), pN, true));
        }
        else if(actualIdealInnerSegments.size() > 0 && !actualIdealInnerSegments.last().isUltimate())
        {// qDebug() << "before actualIdealInnerSegments.last().setEndPoint " << __LINE__ << " pN " << pN;
         // qDebug() << "start point: " << actualIdealInnerSegments.last().getStartPoint();}
            actualIdealInnerSegments.last().addInnerLine(QLine(new_line.p1(), pN));
            actualIdealInnerSegments.last().setEndPoint(pN);
        }
    }
    else
    {
        if(!p0.isNull())
        {// qDebug() << "before actualIdealInnerSegments.append " << __LINE__ << " p0 " << p0;
            actualIdealInnerSegments.append(IdealInnerSegment(p0, QLine(p0, new_line.p2())));
        }
    }

    if(p0.isNull() && actualIdealInnerSegments.size() > 0 && !actualIdealInnerSegments.last().isStartPointNull() && actualIdealInnerSegments.last().isEndPointNull())
    {
        actualIdealInnerSegments.last().addInnerLine(new_line);
    }

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

bool PuansonImage::findP0(const QPointF &center, const qreal R, QPointF &start_point, qreal &alpha, qreal &phi, QPoint &p0) const
{
    qreal pseudoscalar_production_p0, pseudoscalar_production_end;
    qreal ll;
    qreal _phi = 0.0;
    bool pseudoscalar_production_condition;
    int k;
    int sign;
    qreal Phi = 0.0;
    qreal min_phi = 360.0;

    QPoint end_point;
    QMatrix m;

    end_point = start_point.toPoint();
    end_point -= center.toPoint();
    m.rotate(-phi);
    end_point = end_point * m;
    end_point += center.toPoint();

    pseudoscalar_production_end = (start_point.x() - center.x()) * (end_point.y() - center.y()) - (start_point.y() - center.y()) * (end_point.x() - center.x());

    if(/*p0.isNull() &&*/ (((qAbs(X_left - center.x()) <= R) && start_point.x() <= X_left) || start_point.y() >= Y_bottom || start_point.y() <= Y_top))
    {
        k = -1;
        do {
            QPoint temp_p0(X_left, qRound(center.y() + k * qSqrt(R*R - (X_left - center.x())*(X_left - center.x()))));
            ll = qSqrt((temp_p0.x() - center.x()) * (temp_p0.x() - center.x()) + (temp_p0.y() - center.y()) * (temp_p0.y() - center.y()));
            _phi = qRadiansToDegrees(qAcos(((temp_p0.x() - center.x()) * (start_point.x() - center.x()) + (temp_p0.y() - center.y()) * (start_point.y() - center.y())) \
                    / (ll * qSqrt(((start_point.x() - center.x()) * (start_point.x() - center.x()) + (start_point.y() - center.y()) * (start_point.y() - center.y()))))));

            pseudoscalar_production_p0 = (start_point.x() - center.x()) * (temp_p0.y() - center.y()) - (start_point.y() - center.y()) * (temp_p0.x() - center.x());
            pseudoscalar_production_condition = ((pseudoscalar_production_p0 >= 0) && (pseudoscalar_production_end >= 0)) ||
                    ((pseudoscalar_production_p0 < 0) && (pseudoscalar_production_end < 0));

            if(pseudoscalar_production_condition && _phi < qAbs(phi) && (temp_p0.y() <= Y_bottom && temp_p0.y() >= Y_top))
            {
                if(temp_p0 != start_point.toPoint() && _phi < min_phi)
                {
                    min_phi = _phi;
                    p0 = temp_p0;
                }
            }

            k += 2;
        }
        while (k <= 1);

        if(!p0.isNull())
        {
            sign = phi >= 0 ? 1 : -1;
            Phi = sign * (qAbs(phi) - min_phi);
        }
    }

    if(p0.isNull() && (((qAbs(X_right - center.x()) <= R) && start_point.x() >= X_right) || start_point.y() >= Y_bottom || start_point.y() <= Y_top))
    {
        k = -1;
        do {
            QPoint temp_p0(X_right, qRound(center.y() + k * qSqrt(R*R - (X_right - center.x())*(X_right - center.x()))));
            ll = qSqrt((temp_p0.x() - center.x()) * (temp_p0.x() - center.x()) + (temp_p0.y() - center.y()) * (temp_p0.y() - center.y()));
            _phi = qRadiansToDegrees(qAcos(((temp_p0.x() - center.x()) * (start_point.x() - center.x()) + (temp_p0.y() - center.y()) * (start_point.y() - center.y())) \
                    / (ll * qSqrt(((start_point.x() - center.x()) * (start_point.x() - center.x()) + (start_point.y() - center.y()) * (start_point.y() - center.y()))))));

            pseudoscalar_production_p0 = (start_point.x() - center.x()) * (temp_p0.y() - center.y()) - (start_point.y() - center.y()) * (temp_p0.x() - center.x());
            pseudoscalar_production_condition = ((pseudoscalar_production_p0 >= 0) && (pseudoscalar_production_end >= 0)) ||
                    ((pseudoscalar_production_p0 < 0) && (pseudoscalar_production_end < 0));

            if(pseudoscalar_production_condition && _phi < qAbs(phi) && (temp_p0.y() <= Y_bottom && temp_p0.y() >= Y_top))
            {
                if(temp_p0 != start_point.toPoint() && _phi < min_phi)
                {
                    min_phi = _phi;
                    p0 = temp_p0;
                }
            }

            k += 2;
        }
        while (k <= 1);

        if(!p0.isNull())
        {
            sign = phi >= 0 ? 1 : -1;
            Phi = sign * (qAbs(phi) - min_phi);
        }
    }

    if(p0.isNull() && (((qAbs(Y_bottom - center.y()) <= R) && start_point.y() >= Y_bottom) || start_point.x() >= X_right || start_point.x() <= X_left))
    {
        k = -1;
        do {
            QPoint temp_p0(qRound(center.x() + k * qSqrt(R*R - (Y_bottom - center.y())*(Y_bottom - center.y()))), Y_bottom);
            ll = qSqrt((temp_p0.x() - center.x()) * (temp_p0.x() - center.x()) + (temp_p0.y() - center.y()) * (temp_p0.y() - center.y()));
            _phi = qRadiansToDegrees(qAcos(((temp_p0.x() - center.x()) * (start_point.x() - center.x()) + (temp_p0.y() - center.y()) * (start_point.y() - center.y())) \
                    / (ll * qSqrt(((start_point.x() - center.x()) * (start_point.x() - center.x()) + (start_point.y() - center.y()) * (start_point.y() - center.y()))))));

            pseudoscalar_production_p0 = (start_point.x() - center.x()) * (temp_p0.y() - center.y()) - (start_point.y() - center.y()) * (temp_p0.x() - center.x());
            pseudoscalar_production_condition = ((pseudoscalar_production_p0 >= 0) && (pseudoscalar_production_end >= 0)) ||
                    ((pseudoscalar_production_p0 < 0) && (pseudoscalar_production_end < 0));

            if(pseudoscalar_production_condition && _phi < qAbs(phi) && (temp_p0.x() <= X_right && temp_p0.x() >= X_left))
            {
                if(temp_p0 != start_point.toPoint() && _phi < min_phi)
                {
                    min_phi = _phi;
                    p0 = temp_p0;
                }
            }

            k += 2;
        }
        while (k <= 1);

        if(!p0.isNull())
        {
            sign = phi >= 0 ? 1 : -1;
            Phi = sign * (qAbs(phi) - min_phi);
        }
    }

    if(p0.isNull() && (((qAbs(Y_top - center.y()) <= R) && start_point.y() <= Y_top) || start_point.x() >= X_right || start_point.x() <= X_left))
    {
        k = -1;
        do {
            QPoint temp_p0(qRound(center.x() + k * qSqrt(R*R - (Y_top - center.y())*(Y_top - center.y()))), Y_top);
            ll = qSqrt((temp_p0.x() - center.x()) * (temp_p0.x() - center.x()) + (temp_p0.y() - center.y()) * (temp_p0.y() - center.y()));
            _phi = qRadiansToDegrees(qAcos(((temp_p0.x() - center.x()) * (start_point.x() - center.x()) + (temp_p0.y() - center.y()) * (start_point.y() - center.y())) \
                    / (ll * qSqrt(((start_point.x() - center.x()) * (start_point.x() - center.x()) + (start_point.y() - center.y()) * (start_point.y() - center.y()))))));

            pseudoscalar_production_p0 = (start_point.x() - center.x()) * (temp_p0.y() - center.y()) - (start_point.y() - center.y()) * (temp_p0.x() - center.x());
            pseudoscalar_production_condition = ((pseudoscalar_production_p0 >= 0) && (pseudoscalar_production_end >= 0)) ||
                    ((pseudoscalar_production_p0 < 0) && (pseudoscalar_production_end < 0));

            if(pseudoscalar_production_condition && _phi < qAbs(phi) && (temp_p0.x() <= X_right && temp_p0.x() >= X_left))
            {
                if(temp_p0 != start_point.toPoint() && _phi < min_phi)
                {
                    min_phi = _phi;
                    p0 = temp_p0;
                }
            }

            k += 2;
        }
        while (k <= 1);

        if(!p0.isNull())
        {
            sign = phi >= 0 ? 1 : -1;
            Phi = sign * (qAbs(phi) - min_phi);
        }
    }

    if(!p0.isNull())
    {
        if(alpha != 0.0)
            sign = alpha > 0 ? 1 : -1;
        else
            sign = Phi > 0 ? 1 : -1;

        alpha = alpha + (phi - Phi);
        phi = Phi;
        start_point = p0;

        return true;
    }
    else
    {
        return false;
    }
}

bool PuansonImage::findPN(const QPointF &center, const qreal R, QPointF &start_point, qreal &alpha, qreal &phi, QPoint &pN) const
{
    qreal pseudoscalar_production_pN, pseudoscalar_production_end;
    qreal ll;
    qreal _phi = 0.0;
    bool pseudoscalar_production_condition;
    int k;
    int sign;
    qreal min_phi = 360.0;

    QPoint end_point;
    QMatrix m;

    Q_UNUSED(alpha)

    end_point = start_point.toPoint();
    end_point -= center.toPoint();
    m.rotate(-phi);
    end_point = end_point * m;
    end_point += center.toPoint();

    pseudoscalar_production_end = (start_point.x() - center.x()) * (end_point.y() - center.y()) - (start_point.y() - center.y()) * (end_point.x() - center.x());

    if(/*pN.isNull() &&*/ (qAbs(X_left - center.x()) <= R) && start_point.x() >= X_left)
    {
        pseudoscalar_production_condition = false;

        if(center.y() < Y_bottom + R && center.y() > Y_top - R)
        {
            k = -1;
            do {
                QPoint temp_pN(X_left, qRound(center.y() + k * qSqrt(R*R - (X_left - center.x())*(X_left - center.x()))));
                ll = qSqrt((temp_pN.x() - center.x()) * (temp_pN.x() - center.x()) + (temp_pN.y() - center.y()) * (temp_pN.y() - center.y()));
                _phi = qRadiansToDegrees(qAcos(((temp_pN.x() - center.x()) * (start_point.x() - center.x()) + (temp_pN.y() - center.y()) * (start_point.y() - center.y())) \
                        / (ll * qSqrt(((start_point.x() - center.x()) * (start_point.x() - center.x()) + (start_point.y() - center.y()) * (start_point.y() - center.y()))))));
                pseudoscalar_production_pN = (start_point.x() - center.x()) * (temp_pN.y() - center.y()) - (start_point.y() - center.y()) * (temp_pN.x() - center.x());
                pseudoscalar_production_condition = ((pseudoscalar_production_pN >= 0) && (pseudoscalar_production_end >= 0)) ||
                        ((pseudoscalar_production_pN < 0) && (pseudoscalar_production_end < 0));

                if(pseudoscalar_production_condition && _phi < qAbs(phi) && (temp_pN.y() <= Y_bottom && temp_pN.y() >= Y_top))
                {
                    if(temp_pN != start_point.toPoint() && _phi < min_phi)
                    {
                        min_phi = _phi;
                        pN = temp_pN;
                    }
                }

                k += 2;
            }
            while (k <= 1);
        }
    }

    if(pN.isNull() && (qAbs(X_right - center.x()) <= R) && start_point.x() <= X_right)
    {
        pseudoscalar_production_condition = false;

        if(center.y() < Y_bottom + R && center.y() > Y_top - R)
        {
            k = -1;
            do {
                QPoint temp_pN(X_right, qRound(center.y() + k * qSqrt(R*R - (X_right - center.x())*(X_right - center.x()))));
                ll = qSqrt((temp_pN.x() - center.x()) * (temp_pN.x() - center.x()) + (temp_pN.y() - center.y()) * (temp_pN.y() - center.y()));
                _phi = qRadiansToDegrees(qAcos(((temp_pN.x() - center.x()) * (start_point.x() - center.x()) + (temp_pN.y() - center.y()) * (start_point.y() - center.y())) \
                        / (ll * qSqrt(((start_point.x() - center.x()) * (start_point.x() - center.x()) + (start_point.y() - center.y()) * (start_point.y() - center.y()))))));
                pseudoscalar_production_pN = (start_point.x() - center.x()) * (temp_pN.y() - center.y()) - (start_point.y() - center.y()) * (temp_pN.x() - center.x());
                pseudoscalar_production_condition = ((pseudoscalar_production_pN >= 0) && (pseudoscalar_production_end >= 0)) ||
                        ((pseudoscalar_production_pN < 0) && (pseudoscalar_production_end < 0));

                if(pseudoscalar_production_condition && _phi < qAbs(phi) && (temp_pN.y() <= Y_bottom && temp_pN.y() >= Y_top))
                {
                    if(temp_pN != start_point.toPoint() && _phi < min_phi)
                    {
                        min_phi = _phi;
                        pN = temp_pN;
                    }
                }

                k += 2;
            }
            while (k <= 1);
        }
    }

    if(pN.isNull() && (qAbs(Y_bottom - center.y()) <= R) && start_point.y() <= Y_bottom)
    {
        pseudoscalar_production_condition = false;

        if(center.x() < X_right + R && center.x() > X_left - R)
        {
            k = -1;
            do {
                QPoint temp_pN(qRound(center.x() + k * qSqrt(R*R - (Y_bottom - center.y())*(Y_bottom - center.y()))), Y_bottom);
                ll = qSqrt((temp_pN.x() - center.x()) * (temp_pN.x() - center.x()) + (temp_pN.y() - center.y()) * (temp_pN.y() - center.y()));
                _phi = qRadiansToDegrees(qAcos(((temp_pN.x() - center.x()) * (start_point.x() - center.x()) + (temp_pN.y() - center.y()) * (start_point.y() - center.y())) \
                        / (ll * qSqrt(((start_point.x() - center.x()) * (start_point.x() - center.x()) + (start_point.y() - center.y()) * (start_point.y() - center.y()))))));
                pseudoscalar_production_pN = (start_point.x() - center.x()) * (temp_pN.y() - center.y()) - (start_point.y() - center.y()) * (temp_pN.x() - center.x());
                pseudoscalar_production_condition = ((pseudoscalar_production_pN >= 0) && (pseudoscalar_production_end >= 0)) ||
                        ((pseudoscalar_production_pN < 0) && (pseudoscalar_production_end < 0));

                if(pseudoscalar_production_condition && _phi < qAbs(phi) && (temp_pN.x() <= X_right && temp_pN.x() >= X_left))
                {
                    if(temp_pN != start_point.toPoint() && _phi < min_phi)
                    {
                        min_phi = _phi;
                        pN = temp_pN;
                    }
                }

                k += 2;
            }
            while (k <= 1);
        }
    }

    if(pN.isNull() && (qAbs(Y_top - center.y()) <= R) && start_point.y() >= Y_top)
    {
        pseudoscalar_production_condition = false;

        if(center.x() < X_right + R && center.x() > X_left - R)
        {
            k = -1;
            do {
                QPoint temp_pN(qRound(center.x() + k * qSqrt(R*R - (Y_top - center.y())*(Y_top - center.y()))), Y_top);
                ll = qSqrt((temp_pN.x() - center.x()) * (temp_pN.x() - center.x()) + (temp_pN.y() - center.y()) * (temp_pN.y() - center.y()));
                _phi = qRadiansToDegrees(qAcos(((temp_pN.x() - center.x()) * (start_point.x() - center.x()) + (temp_pN.y() - center.y()) * (start_point.y() - center.y())) \
                        / (ll * qSqrt(((start_point.x() - center.x()) * (start_point.x() - center.x()) + (start_point.y() - center.y()) * (start_point.y() - center.y()))))));
                pseudoscalar_production_pN = (start_point.x() - center.x()) * (temp_pN.y() - center.y()) - (start_point.y() - center.y()) * (temp_pN.x() - center.x());
                pseudoscalar_production_condition = ((pseudoscalar_production_pN >= 0) && (pseudoscalar_production_end >= 0)) ||
                        ((pseudoscalar_production_pN < 0) && (pseudoscalar_production_end < 0));

                if(pseudoscalar_production_condition && _phi < qAbs(phi) && (temp_pN.x() <= X_right && temp_pN.x() >= X_left))
                {
                    if(temp_pN != start_point.toPoint() && _phi < min_phi)
                    {
                        min_phi = _phi;
                        pN = temp_pN;
                    }
                }

                k += 2;
            }
            while (k <= 1);
        }
    }

    if(!pN.isNull())
    {
        sign = phi >= 0 ? 1 : -1;
        phi = sign * min_phi;

        start_point = pN;

        return true;
    }
    else
    {
        return false;
    }
}

void PuansonImage::addIdealSkeletonArc(const QPointF &center, const QPointF &start_point, const qreal R, const qreal alpha, const qreal phi, const bool close_flag)
{
    IdealContourArc new_arc(center, start_point, R, alpha, phi);

    QPoint p0, pN;
    QPoint end_point;
    QMatrix m;

    qreal Alpha = alpha; // Начальный угол дуги после определения точки p0
    qreal Phi = phi;     // Угол дуги после определения точки p0

    QPointF _start_point = start_point;

    end_point = start_point.toPoint();
    end_point -= center.toPoint();
    m.rotate(-phi);
    end_point = end_point * m;
    end_point += center.toPoint();

    if(QRect(QPoint(X_left, Y_top), QPoint(X_right, Y_bottom)).contains(_start_point.toPoint()))
    {
        findP0(center, R, _start_point, Alpha, Phi, p0);
        findPN(center, R, _start_point, Alpha, Phi, pN);

        if(pN.isNull() && close_flag)
            pN = end_point;

        if(p0.isNull() && (actualIdealInnerSegments.size() == 0 || actualIdealInnerSegments.last().isUltimate()))
            p0 = _start_point.toPoint();

        if(p0.isNull() && !pN.isNull())
        {
            if(actualIdealInnerSegments.size() > 0 && !actualIdealInnerSegments.last().isUltimate())
            {
                actualIdealInnerSegments.last().addInnerArc(IdealContourArc(center, start_point, R, Alpha - ideal_contour_rotation_angle, Phi));
                actualIdealInnerSegments.last().setEndPoint(pN);
            }

            Alpha = Alpha + Phi;
            Phi = phi - Phi;
        }
        else if(!p0.isNull() && !pN.isNull())
        {
            actualIdealInnerSegments.append(IdealInnerSegment(p0, IdealContourArc(center, p0, R, Alpha - ideal_contour_rotation_angle, Phi), pN, close_flag));
        }
        else if(!p0.isNull() && pN.isNull())
        {
            actualIdealInnerSegments.append(IdealInnerSegment(p0, IdealContourArc(center, p0, R, Alpha - ideal_contour_rotation_angle, Phi)));
        }
    }
    else
    {
        findP0(center, R, _start_point, Alpha, Phi, p0);
        findPN(center, R, _start_point, Alpha, Phi, pN);

        if(p0.isNull() && (actualIdealInnerSegments.size() == 0 && QRect(QPoint(X_left, Y_top), QPoint(X_right, Y_bottom)).contains(_start_point.toPoint())))
            p0 = _start_point.toPoint();

        if(close_flag && !p0.isNull() && pN.isNull())
            pN = end_point;

        if(!p0.isNull() && pN.isNull())
            actualIdealInnerSegments.append(IdealInnerSegment(p0, IdealContourArc(center, p0, R, Alpha - ideal_contour_rotation_angle, Phi)));

        if(!pN.isNull())
        {
            if(!p0.isNull())
            {
                actualIdealInnerSegments.append(IdealInnerSegment(p0, IdealContourArc(center, p0, R, Alpha - ideal_contour_rotation_angle, Phi), pN, true));
            }
            else if(actualIdealInnerSegments.size() > 0 && actualIdealInnerSegments.last().isUltimate())
            {
                actualIdealInnerSegments.last().addInnerArc(IdealContourArc(center, _start_point, R, Alpha - ideal_contour_rotation_angle, Phi));
                actualIdealInnerSegments.last().setEndPoint(pN);
            }
            /*else
                qDebug() << "pN without p0!!!";*/
        }
    }

    if(p0.isNull() && pN.isNull() && actualIdealInnerSegments.size() > 0 && !actualIdealInnerSegments.last().isUltimate())
        actualIdealInnerSegments.last().addInnerArc(IdealContourArc(new_arc.Center(), new_arc.StartPoint(), new_arc.R(), new_arc.Alpha() - ideal_contour_rotation_angle, new_arc.Phi()));

    idealSkeletonArcs.append(new_arc);
}

void PuansonImage::drawIdealContour(const PuansonModel detail_research_puanson_model, const QRect &r, const QPointF &ideal_contour_point_of_origin, const qreal ideal_contour_rotation_angle, bool draw_measurements, const qreal _scale, const qreal calibration_ratio, QPainterPath &idealContourPath, QPainterPath &idealContourMeasurementsPath)
{
    idealContourPath = QPainterPath();
    idealContourMeasurementsPath = QPainterPath();

    QTransform t;
    t.translate(ideal_contour_point_of_origin.x(), ideal_contour_point_of_origin.y());
    t.rotate(ideal_contour_rotation_angle);
    t.translate(-ideal_contour_point_of_origin.x(), -ideal_contour_point_of_origin.y());

    qreal ratio = 1.0 / calibration_ratio;

    QPainterPath border_path;
    QPointF new_point;

    qreal line_1_x = 0.0;
    qreal line_1_y = 0.0;
    qreal line_2_x = 0.0;
    qreal line_2_y = 0.0;
    qreal line_3_x = 0.0;
    qreal line_3_y = 0.0;

    // Радиус скругления
    qreal R;

    border_path.moveTo(r.x(), r.y());
    border_path.lineTo(r.width(), r.y());
    border_path.lineTo(r.width(), r.height());
    border_path.lineTo(r.x(), r.height());
    border_path.closeSubpath();

    QPointF point1;
    QPointF point2;
    QPointF point3;

    QPointF _point1;
    QPointF _point2;
    QPointF point0;
    qreal phi;
    qreal alpha;

    EtalonDetailDimensions ideal_etalon_dimensions;

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
    default:
        ideal_etalon_dimensions = EtalonDetailDimensions();
        break;
    }

    // Верхняя часть
    if(detail_research_puanson_model == PuansonModel::PUANSON_MODEL_658 || detail_research_puanson_model == PuansonModel::PUANSON_MODEL_660) // Пуансоны-иглы
    {
        line_1_x = _scale * ideal_etalon_dimensions.diameter_5_dimension / 2.0;
        line_1_y = _scale * 0;

        if(detail_research_puanson_model == PuansonModel::PUANSON_MODEL_660)
        {
            line_2_x = _scale * ideal_etalon_dimensions.diameter_4_dimension / 2.0;
            line_2_y = _scale * ideal_etalon_dimensions.diameter_4_top_part_lenght;
        }
        else if(detail_research_puanson_model == PuansonModel::PUANSON_MODEL_658)
        {
            qreal a, b, c, d;

            qreal diameter_5_top_part_lenght = 0.0;

            a = (static_cast<qreal>(ideal_etalon_dimensions.diameter_4_top_part_lenght) - diameter_5_top_part_lenght) / (static_cast<qreal>(ideal_etalon_dimensions.diameter_4_dimension / 2.0) - static_cast<qreal>(ideal_etalon_dimensions.diameter_5_dimension / 2.0));
            b = (static_cast<qreal>(ideal_etalon_dimensions.top_part_lenght) - static_cast<qreal>(ideal_etalon_dimensions.diameter_2_top_part_lenght)) / (static_cast<qreal>(ideal_etalon_dimensions.diameter_1_dimension / 2.0) - static_cast<qreal>(ideal_etalon_dimensions.diameter_2_dimension / 2.0));
            c = diameter_5_top_part_lenght - static_cast<qreal>(ideal_etalon_dimensions.diameter_5_dimension / 2.0) * a;
            d = static_cast<qreal>(ideal_etalon_dimensions.diameter_2_top_part_lenght) - static_cast<qreal>(ideal_etalon_dimensions.diameter_2_dimension / 2.0) * b;

            line_2_x = _scale * ((d - c) / (a - b));
            line_2_y = _scale * ((a * d - b * c) / (a - b));
        }

        line_3_x = _scale * ideal_etalon_dimensions.diameter_1_dimension / 2.0;
        line_3_y = _scale * ideal_etalon_dimensions.top_part_lenght;

        // Скругление 1 между юбкой и иглой
        point1 = QPointF(line_1_x, line_1_y) * ratio;
        point2 = QPointF(line_2_x, line_2_y) * ratio;
        point3 = QPointF(line_3_x, line_3_y) * ratio;

        R = _scale * ideal_etalon_dimensions.skirt_rounding_radius * ratio;

        getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);
        // ----------

        new_point = ideal_contour_point_of_origin;
        //new_point = t.map(new_point);
        idealContourPath.moveTo(new_point);

        new_point = ideal_contour_point_of_origin + point1;
        idealContourPath.lineTo(new_point);

        new_point = ideal_contour_point_of_origin + _point1;
        idealContourPath.lineTo(new_point);

        new_point = ideal_contour_point_of_origin + point0;
        idealContourPath.arcTo(QRectF(new_point.x() - R, new_point.y() - R, R * 2.0, R * 2.0), qRadiansToDegrees(alpha), 180.0 - qRadiansToDegrees(phi));

        new_point = ideal_contour_point_of_origin + _point2;
        idealContourPath.lineTo(new_point);

        /*new_point = ideal_contour_point_of_origin + point3;
        idealContourPath.lineTo(new_point);*/

        /*if(draw_measurements)
        {
            idealContourMeasurementsPath.addText(point0 + ideal_contour_point_of_origin - QPointF(_scale * 4000.0, _scale * 0.0) * ratio, QFont("Noto Sans", 8), "R2");
        }*/

        // Скругление 1 конца иглы
        R = _scale * ideal_etalon_dimensions.needle_rounding_radius * ratio;

        point1 = _point2;
        point2 = point3;
        point3 = QPointF(-line_3_x, line_3_y) * ratio;

        getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

        new_point = ideal_contour_point_of_origin + _point1;
        idealContourPath.lineTo(new_point);

        new_point = ideal_contour_point_of_origin + point0;
        idealContourPath.arcTo(QRectF(new_point.x() - R, new_point.y() - R, R * 2.0, R * 2.0), qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));

        /*new_point = ideal_contour_point_of_origin + _point2;
        idealContourPath.lineTo(new_point);*/

        new_point = ideal_contour_point_of_origin + _point2;
        idealContourPath.lineTo(new_point);
        // ----------

        // Скругление 2 конца иглы
        R = _scale * ideal_etalon_dimensions.needle_rounding_radius * ratio;

        point1 = _point2;
        point2 = point3;
        point3 = QPointF(-line_2_x, line_2_y) * ratio;

        getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

        new_point = ideal_contour_point_of_origin + _point1;
        idealContourPath.lineTo(new_point);

        new_point = ideal_contour_point_of_origin + point0;
        idealContourPath.arcTo(QRectF(new_point.x() - R, new_point.y() - R, R * 2.0, R * 2.0), -qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));

        /*new_point = ideal_contour_point_of_origin + _point2;
        idealContourPath.lineTo(new_point);*/
        // ----------

        if(draw_measurements)
        {
            idealContourMeasurementsPath.moveTo(QPointF(line_3_x, (line_3_y - _scale * 500)) * ratio + ideal_contour_point_of_origin);
            idealContourMeasurementsPath.lineTo(QPointF(line_3_x, (line_3_y + _scale * 1000)) * ratio + ideal_contour_point_of_origin);

            idealContourMeasurementsPath.moveTo(QPointF(-line_3_x, (line_3_y - _scale * 500)) * ratio + ideal_contour_point_of_origin);
            idealContourMeasurementsPath.lineTo(QPointF(-line_3_x, (line_3_y + _scale * 1000)) * ratio + ideal_contour_point_of_origin);

            idealContourMeasurementsPath.moveTo(QPointF(line_3_x, (line_3_y + _scale * 1000)) * ratio + ideal_contour_point_of_origin);
            idealContourMeasurementsPath.lineTo(QPointF(-line_3_x, (line_3_y + _scale * 1000)) * ratio + ideal_contour_point_of_origin);
            idealContourMeasurementsPath.addText(QPointF(-_scale * 800, (line_3_y + _scale * 3000)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "d1");

            //idealContourMeasurementsPath.addText(QPointF(-(line_3_x + _scale * 4000), (line_3_y + _scale * 500)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 8), "R1");
            idealContourMeasurementsPath.addText(QPointF((line_3_x + _scale * 500), (line_3_y + _scale * 500)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "R1");
        }

        // Скругление 2 между юбкой и иглой
        point1 = _point2;
        point2 = point3;
        point3 = QPointF(-line_1_x, line_1_y) * ratio;

        R = _scale * ideal_etalon_dimensions.skirt_rounding_radius * ratio;

        getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);
        // ----------

        /*new_point = ideal_contour_point_of_origin + point1;
        idealContourPath.lineTo(new_point);*/

        if(draw_measurements)
        {
            idealContourMeasurementsPath.addText(point0 + ideal_contour_point_of_origin + QPoint(qRound(_scale * 1000), 0) * ratio, QFont("Noto Sans", 5, QFont::Thin), "R2");
        }

        new_point = ideal_contour_point_of_origin + _point1;
        idealContourPath.lineTo(new_point);

        new_point = ideal_contour_point_of_origin + point0;
        idealContourPath.arcTo(QRectF(new_point.x() - R, new_point.y() - R, R * 2.0, R * 2.0), qRadiansToDegrees(alpha), 180.0 - qRadiansToDegrees(phi));

        new_point = ideal_contour_point_of_origin + _point2;
        idealContourPath.lineTo(new_point);

        new_point = ideal_contour_point_of_origin + point3;
        idealContourPath.lineTo(new_point);

        new_point = ideal_contour_point_of_origin;
        idealContourPath.lineTo(new_point);

        if(draw_measurements)
        {
            idealContourMeasurementsPath.moveTo(QPointF(line_1_x, line_1_y) * ratio + ideal_contour_point_of_origin);
            idealContourMeasurementsPath.lineTo(QPointF(line_1_x, (line_1_y + _scale * 2000)) * ratio + ideal_contour_point_of_origin);

            idealContourMeasurementsPath.moveTo(QPointF(-line_1_x, line_1_y) * ratio + ideal_contour_point_of_origin);
            idealContourMeasurementsPath.lineTo(QPointF(-line_1_x, (line_1_y + _scale * 2000)) * ratio + ideal_contour_point_of_origin);

            idealContourMeasurementsPath.moveTo(QPointF(line_1_x, (line_1_y + _scale * 2000)) * ratio + ideal_contour_point_of_origin);
            idealContourMeasurementsPath.lineTo(QPointF(-line_1_x, (line_1_y + _scale * 2000)) * ratio + ideal_contour_point_of_origin);
            idealContourMeasurementsPath.addText(QPointF(-_scale * 800, (line_1_y + _scale * 4000)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "d5");
        }

        if(detail_research_puanson_model == PuansonModel::PUANSON_MODEL_658)
        {
            if(draw_measurements)
            {
                idealContourMeasurementsPath.moveTo(QPointF(_scale * ideal_etalon_dimensions.diameter_4_dimension / 2.0, _scale * ideal_etalon_dimensions.diameter_4_top_part_lenght) * ratio + ideal_contour_point_of_origin);
                idealContourMeasurementsPath.lineTo(QPointF(-_scale * ideal_etalon_dimensions.diameter_4_dimension / 2.0, _scale * ideal_etalon_dimensions.diameter_4_top_part_lenght) * ratio + ideal_contour_point_of_origin);
                idealContourMeasurementsPath.addText(QPointF(-_scale * 800, _scale * (ideal_etalon_dimensions.diameter_4_top_part_lenght + 2000)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "d4");
            }

            if(draw_measurements)
            {
                idealContourMeasurementsPath.moveTo(QPointF(_scale * ideal_etalon_dimensions.diameter_3_dimension / 2.0, _scale * ideal_etalon_dimensions.diameter_3_top_part_lenght) * ratio + ideal_contour_point_of_origin);
                idealContourMeasurementsPath.lineTo(QPointF(-_scale * ideal_etalon_dimensions.diameter_3_dimension / 2.0, _scale * ideal_etalon_dimensions.diameter_3_top_part_lenght) * ratio + ideal_contour_point_of_origin);
                idealContourMeasurementsPath.addText(QPointF(-_scale * 800, _scale * (ideal_etalon_dimensions.diameter_3_top_part_lenght + 2000)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "d3");
            }

            if(draw_measurements)
            {
                idealContourMeasurementsPath.moveTo(QPointF(_scale * ideal_etalon_dimensions.diameter_2_dimension / 2.0, _scale * ideal_etalon_dimensions.diameter_2_top_part_lenght) * ratio + ideal_contour_point_of_origin);
                idealContourMeasurementsPath.lineTo(QPointF(-_scale * ideal_etalon_dimensions.diameter_2_dimension / 2.0, _scale * ideal_etalon_dimensions.diameter_2_top_part_lenght) * ratio + ideal_contour_point_of_origin);
                idealContourMeasurementsPath.addText(QPointF(-_scale * 800, _scale * (ideal_etalon_dimensions.diameter_2_top_part_lenght + 2000)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "d2");
            }

            if(draw_measurements)
            {
                idealContourMeasurementsPath.addText(QPointF(-_scale * (ideal_etalon_dimensions.diameter_4_dimension / 2.0 + 1000), _scale * (ideal_etalon_dimensions.diameter_3_top_part_lenght + 5000)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "R2");
            }
        }

        //idealContourPath.closeSubpath();
    }
    else if(detail_research_puanson_model == PuansonModel::PUANSON_MODEL_661) // Пуансон-граната
    {
        line_1_x = _scale * ideal_etalon_dimensions.diameter_5_dimension / 2.0;
        line_1_y = _scale * 0;
        line_2_x = _scale * ideal_etalon_dimensions.diameter_1_dimension / 2.0;
        line_2_y = _scale * ideal_etalon_dimensions.top_part_lenght;
        line_3_x = _scale * -static_cast<qint32>(ideal_etalon_dimensions.diameter_1_dimension / 2.0);
        line_3_y = _scale * ideal_etalon_dimensions.top_part_lenght;

        point1 = QPointF(line_1_x, line_1_y) * ratio;
        point2 = QPointF(line_2_x, line_2_y) * ratio;
        point3 = QPointF(line_3_x, line_3_y) * ratio;

        new_point = ideal_contour_point_of_origin;
        idealContourPath.moveTo(new_point);

        new_point = ideal_contour_point_of_origin + point1;
        idealContourPath.lineTo(new_point);

        new_point = ideal_contour_point_of_origin + point2;
        idealContourPath.lineTo(new_point);

        new_point = ideal_contour_point_of_origin + point3;
        idealContourPath.lineTo(new_point);

        point1 = QPointF(-line_1_x, line_1_y) * ratio;

        new_point = ideal_contour_point_of_origin + point1;
        idealContourPath.lineTo(new_point);

        new_point = ideal_contour_point_of_origin;
        idealContourPath.lineTo(new_point);

        if(draw_measurements)
        {
            /*idealContourMeasurementsPath.moveTo(QPointF(line_1_x, (line_1_y + _scale * 500)) * ratio + ideal_contour_point_of_origin);
            idealContourMeasurementsPath.lineTo(QPointF(-line_1_x, (line_1_y + _scale * 500)) * ratio + ideal_contour_point_of_origin);*/
            idealContourMeasurementsPath.addText(QPointF(-_scale * 800, (line_1_y + _scale * 1500)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "d5");
            idealContourMeasurementsPath.addText(QPointF(-_scale * 800, (line_3_y + _scale * 1500)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "d1");
        }
    }
#if 1
    // Нижняя часть
    const qreal line_4_x = _scale * ideal_etalon_dimensions.diameter_6_dimension / 2.0;
    const qreal line_4_y = _scale * 0;

    const qreal line_5_x = _scale * ideal_etalon_dimensions.diameter_6_dimension / 2.0;
    const qreal line_5_y = _scale * -static_cast<qreal>(static_cast<quint32>(ideal_etalon_dimensions.skirt_bottom_part_lenght) - ideal_etalon_dimensions.skirt_bottom_rounding_radius);

    point1 = QPointF(line_1_x, line_1_y) * ratio;
    point2 = QPointF(line_4_x, line_4_y) * ratio;
    point3 = QPointF(line_5_x, line_5_y) * ratio;

    new_point = ideal_contour_point_of_origin;
    //new_point = t.map(new_point);

    /*
    idealContourPath.lineTo(new_point + point3);*/

    R = _scale * ideal_etalon_dimensions.skirt_bottom_rounding_radius * ratio;

    getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

    new_point = ideal_contour_point_of_origin + point1;
    //idealContourPath.lineTo(new_point);
    idealContourPath.moveTo(new_point);

    new_point = ideal_contour_point_of_origin + point0;
    idealContourPath.arcTo(QRectF(new_point.x() - R, new_point.y() - R, R * 2.0, R * 2.0), -qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));

    new_point = ideal_contour_point_of_origin + _point2;
    idealContourPath.lineTo(new_point);

    new_point = ideal_contour_point_of_origin + point3;
    idealContourPath.lineTo(new_point);

    if(draw_measurements)
    {
        idealContourMeasurementsPath.moveTo(QPointF(line_4_x, -(line_4_y + _scale * ideal_etalon_dimensions.skirt_bottom_rounding_radius * 1)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(-line_4_x, -(line_4_y + _scale * ideal_etalon_dimensions.skirt_bottom_rounding_radius * 1)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.addText(QPointF(-_scale * 800, -(line_4_y + _scale * (ideal_etalon_dimensions.skirt_bottom_rounding_radius * 1 + 500))) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "d6");

        idealContourMeasurementsPath.moveTo(QPointF(line_4_x, line_5_y) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(-line_4_x, line_5_y) * ratio + ideal_contour_point_of_origin);

        idealContourMeasurementsPath.addText(QPointF(-(line_4_x  + _scale * 2500 ), -(line_4_y + _scale * (ideal_etalon_dimensions.skirt_bottom_rounding_radius * 1 - 0))) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "R3");
    }

    const qreal line_6_x = _scale * ideal_etalon_dimensions.diameter_7_dimension / 2.0;
    const qreal line_6_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.skirt_bottom_part_lenght);

    const qreal line_7_x = _scale * ideal_etalon_dimensions.diameter_7_dimension / 2.0;
    const qreal line_7_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.middle_part_lenght - 500 * 1.5);

    const qreal line_8_x = _scale * ideal_etalon_dimensions.diameter_8_dimension / 2.0;
    const qreal line_8_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.middle_part_lenght);

    point1 = QPointF(line_6_x, line_6_y) * ratio;

    point2 = QPointF(line_7_x, line_7_y) * ratio;
    point3 = QPointF(line_8_x, line_8_y) * ratio;

    new_point = ideal_contour_point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = ideal_contour_point_of_origin + point2;
    idealContourPath.lineTo(new_point);

    new_point = ideal_contour_point_of_origin + point3;
    idealContourPath.lineTo(new_point);

    if(draw_measurements)
    {
        idealContourMeasurementsPath.moveTo(QPointF(line_6_x, (line_6_y - _scale * 500)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(-line_6_x, (line_6_y - _scale * 500)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.addText(QPointF(-_scale * 800, (line_6_y - _scale * 1000)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "d7");

        idealContourMeasurementsPath.moveTo(QPointF(line_6_x, (line_7_y + _scale * 500)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(-line_6_x, (line_7_y + _scale * 500)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.addText(QPointF(-_scale * 800, (line_7_y)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "d7");
    }

    if(draw_measurements)
    {
        idealContourMeasurementsPath.moveTo(QPointF(line_8_x, (line_7_y - _scale * 1500)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(-line_8_x, (line_7_y - _scale * 1500)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.addText(QPointF(-_scale * 800, (line_7_y - _scale * 2000)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "d8");
    }

    const qreal line_9_x = _scale * ideal_etalon_dimensions.diameter_8_dimension / 2.0;
    const qreal line_9_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght - 1000);

    const qreal line_10_x = _scale * ideal_etalon_dimensions.diameter_9_dimension / 2.0;
    const qreal line_10_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght);

    point1 = QPointF(line_9_x, line_9_y) * ratio;
    point2 = QPointF(line_10_x, line_10_y) * ratio;

    new_point = ideal_contour_point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = ideal_contour_point_of_origin + point2;
    idealContourPath.lineTo(new_point);

    if(draw_measurements)
    {
        idealContourMeasurementsPath.moveTo(QPointF(line_10_x, (line_10_y)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(line_10_x, (line_10_y - _scale * 2500)) * ratio + ideal_contour_point_of_origin);

        idealContourMeasurementsPath.moveTo(QPointF(-line_10_x, (line_10_y)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(-line_10_x, (line_10_y - _scale * 2500)) * ratio + ideal_contour_point_of_origin);

        idealContourMeasurementsPath.moveTo(QPointF(line_10_x, (line_10_y - _scale * 2500)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(-line_10_x, (line_10_y - _scale * 2500)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.addText(QPointF(-_scale * 800, (line_10_y - _scale * 3000)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "d9");
    }

    const qreal line_11_x = _scale * ideal_etalon_dimensions.groove_width_dimension / 2.0;
    const qreal line_11_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght);

    const qreal line_12_x = _scale * ideal_etalon_dimensions.groove_width_dimension / 2.0;
    const qreal line_12_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght - ideal_etalon_dimensions.groove_depth_dimension);

    point3 = point2;

    point1 = QPointF(line_11_x, line_11_y) * ratio;
    point2 = QPointF(line_12_x, line_12_y) * ratio;

    new_point = ideal_contour_point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = ideal_contour_point_of_origin + point2;
    idealContourPath.lineTo(new_point);

    if(draw_measurements)
    {
        idealContourMeasurementsPath.moveTo(QPointF(line_11_x, (line_11_y)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(line_11_x, (line_11_y - _scale * 1000)) * ratio + ideal_contour_point_of_origin);

        idealContourMeasurementsPath.moveTo(QPointF(-line_11_x, (line_11_y)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(-line_11_x, (line_11_y - _scale * 1000)) * ratio + ideal_contour_point_of_origin);

        idealContourMeasurementsPath.moveTo(QPointF(line_11_x, (line_11_y - _scale * 1000)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(-line_11_x, (line_11_y - _scale * 1000)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.addText(QPointF(-_scale * 800, (line_11_y - _scale * 1500)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "ширина паза");
    }

    if(draw_measurements)
    {
        idealContourMeasurementsPath.moveTo(QPointF(line_11_x, (line_11_y)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(-line_11_x, (line_11_y)) * ratio + ideal_contour_point_of_origin);

        idealContourMeasurementsPath.moveTo(QPointF(-_scale * 800, (line_11_y)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.lineTo(QPointF(-_scale * 800, (line_12_y)) * ratio + ideal_contour_point_of_origin);
        idealContourMeasurementsPath.addText(QPointF(10, (line_12_y - _scale * 1500)) * ratio + ideal_contour_point_of_origin, QFont("Noto Sans", 5, QFont::Thin), "глубина паза");
    }

    point3 = point2;

    point1 = QPointF(-line_12_x, line_12_y) * ratio;
    point2 = QPointF(-line_11_x, line_11_y) * ratio;

    new_point = ideal_contour_point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = ideal_contour_point_of_origin + point2;
    idealContourPath.lineTo(new_point);

    point3 = point2;

    point1 = QPointF(-line_10_x, line_10_y) * ratio;
    point2 = QPointF(-line_9_x, line_9_y) * ratio;

    new_point = ideal_contour_point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = ideal_contour_point_of_origin + point2;
    idealContourPath.lineTo(new_point);

    point1 = QPointF(-line_8_x, line_8_y) * ratio;

    point2 = QPointF(-line_7_x, line_7_y) * ratio;
    point3 = QPointF(-line_6_x, line_6_y) * ratio;

    new_point = ideal_contour_point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = ideal_contour_point_of_origin + point2;
    idealContourPath.lineTo(new_point);

    new_point = ideal_contour_point_of_origin + point3;
    idealContourPath.lineTo(new_point);

    point1 = QPointF(-line_5_x, line_5_y) * ratio;

    point2 = QPointF(-line_4_x, line_4_y) * ratio;
    point3 = QPointF(-line_1_x, line_1_y) * ratio;

    new_point = ideal_contour_point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    R = _scale * ideal_etalon_dimensions.skirt_bottom_rounding_radius * ratio;

    getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

    new_point = ideal_contour_point_of_origin + _point1;
    idealContourPath.lineTo(new_point);

    new_point = ideal_contour_point_of_origin + point0;
    idealContourPath.arcTo(QRectF(new_point.x() - R, new_point.y() - R, R * 2.0, R * 2.0), qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));

    /*new_point = ideal_contour_point_of_origin + _point2;
    idealContourPath.moveTo(new_point);*/

    /*new_point = ideal_contour_point_of_origin + point3;
    idealContourPath.lineTo(new_point);*/
#endif // 0
    idealContourPath = t.map(idealContourPath);
    idealContourPath = idealContourPath.intersected(border_path);

    idealContourMeasurementsPath = t.map(idealContourMeasurementsPath);
}

QPainterPath PuansonImage::drawIdealContour(const QRect &r, const QPointF &_point_of_origin, const qreal _rotation_angle, bool draw_measurements, const qreal _scale)
{
    idealContourPath = QPainterPath();
    idealContourMeasurementsPath = QPainterPath();

    ideal_contour_point_of_origin = _point_of_origin;
    ideal_contour_rotation_angle = _rotation_angle;

    QTransform t;
    t.translate(ideal_contour_point_of_origin.x(), ideal_contour_point_of_origin.y());
    t.rotate(ideal_contour_rotation_angle);
    t.translate(-ideal_contour_point_of_origin.x(), -ideal_contour_point_of_origin.y());

    ideal_contour_transform = t;

    qreal ratio = 1.0 / calibration_ratio;

    QPainterPath border_path;

    qreal line_1_x = 0.0;
    qreal line_1_y = 0.0;
    qreal line_2_x = 0.0;
    qreal line_2_y = 0.0;
    qreal line_3_x = 0.0;
    qreal line_3_y = 0.0;

    // Радиус скругления
    qreal R;

    border_path.moveTo(r.x(), r.y());
    border_path.lineTo(r.width(), r.y());
    border_path.lineTo(r.width(), r.height());
    border_path.lineTo(r.x(), r.height());
    border_path.closeSubpath();

    QPointF point1;
    QPointF point2;
    QPointF point3;

    QPointF _point1;
    QPointF _point2;
    QPointF point0;
    qreal phi;
    qreal alpha;

    if(!idealSkeletonLines.isEmpty())
    {
        idealSkeletonLines.clear();
        innerIdealSkeletonNormalVectors.clear();
        outerIdealSkeletonNormalVectors.clear();
        idealSkeletonArcs.clear();
    }

    if(!actualIdealInnerSegments.isEmpty())
        actualIdealInnerSegments.clear();

//qDebug() << "X_left " << X_left << " X_right " << X_right << " Y_top " << Y_top << " Y_bottom " << Y_bottom;
    // Верхняя часть
    if(detail_research_puanson_model == PuansonModel::PUANSON_MODEL_658 || detail_research_puanson_model == PuansonModel::PUANSON_MODEL_660) // Пуансоны-иглы
    {
        line_1_x = _scale * ideal_etalon_dimensions.diameter_5_dimension / 2.0;
        line_1_y = _scale * 0;
        if(detail_research_puanson_model == PuansonModel::PUANSON_MODEL_660)
        {
            line_2_x = _scale * ideal_etalon_dimensions.diameter_4_dimension / 2.0;
            line_2_y = _scale * ideal_etalon_dimensions.diameter_4_top_part_lenght;
        }
        else if(detail_research_puanson_model == PuansonModel::PUANSON_MODEL_658)
        {
            qreal a, b, c, d;

            qreal diameter_5_top_part_lenght = 0.0;

            a = (static_cast<qreal>(ideal_etalon_dimensions.diameter_4_top_part_lenght) - diameter_5_top_part_lenght) / (static_cast<qreal>(ideal_etalon_dimensions.diameter_4_dimension / 2.0) - static_cast<qreal>(ideal_etalon_dimensions.diameter_5_dimension / 2.0));
            b = (static_cast<qreal>(ideal_etalon_dimensions.top_part_lenght) - static_cast<qreal>(ideal_etalon_dimensions.diameter_2_top_part_lenght)) / (static_cast<qreal>(ideal_etalon_dimensions.diameter_1_dimension / 2.0) - static_cast<qreal>(ideal_etalon_dimensions.diameter_2_dimension / 2.0));
            c = diameter_5_top_part_lenght - static_cast<qreal>(ideal_etalon_dimensions.diameter_5_dimension / 2.0) * a;
            d = static_cast<qreal>(ideal_etalon_dimensions.diameter_2_top_part_lenght) - static_cast<qreal>(ideal_etalon_dimensions.diameter_2_dimension / 2.0) * b;

            line_2_x = _scale * ((d - c) / (a - b));
            line_2_y = _scale * ((a * d - b * c) / (a - b));
        }
        line_3_x = _scale * ideal_etalon_dimensions.diameter_1_dimension / 2.0;
        line_3_y = _scale * ideal_etalon_dimensions.top_part_lenght;

        // Скругление 1 между юбкой и иглой
        point1 = QPointF(line_1_x, line_1_y) * ratio;
        point2 = QPointF(line_2_x, line_2_y) * ratio;
        point3 = QPointF(line_3_x, line_3_y) * ratio;

        R = _scale * ideal_etalon_dimensions.skirt_rounding_radius * ratio;

        getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);
        addIdealSkeletonLine(t.map((ideal_contour_point_of_origin).toPoint()), t.map((point1 + ideal_contour_point_of_origin).toPoint()), 1);
        addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((_point1/*point2*/ + ideal_contour_point_of_origin).toPoint()), 1);
        addIdealSkeletonArc(t.map((point0 + ideal_contour_point_of_origin).toPoint()), t.map((_point1 + ideal_contour_point_of_origin).toPoint()), R, qRadiansToDegrees(alpha), 180.0 - qRadiansToDegrees(phi));
        // ----------

        // Скругление 1 конца иглы
        R = _scale * ideal_etalon_dimensions.needle_rounding_radius * ratio;

        point1 = _point2;
        point2 = point3;
        point3 = QPointF(-line_3_x, line_3_y) * ratio;

        getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

        addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((_point1 + ideal_contour_point_of_origin).toPoint()), 1);
        addIdealSkeletonArc(t.map((point0 + ideal_contour_point_of_origin).toPoint()), t.map((_point1 + ideal_contour_point_of_origin).toPoint()), R, qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));
        // ----------

        // Скругление 2 конца иглы
        R = _scale * ideal_etalon_dimensions.needle_rounding_radius * ratio;

        point1 = _point2;
        point2 = point3;
        point3 = QPointF(-line_2_x, line_2_y) * ratio;

        getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

        addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((_point1 + ideal_contour_point_of_origin).toPoint()), 1);
        addIdealSkeletonArc(t.map((point0 + ideal_contour_point_of_origin).toPoint()), t.map((_point1 + ideal_contour_point_of_origin).toPoint()), R, -qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));
        // ----------

        // Скругление 2 между юбкой и иглой
        point1 = _point2;
        point2 = point3;
        point3 = QPointF(-line_1_x, line_1_y) * ratio;

        R = _scale * ideal_etalon_dimensions.skirt_rounding_radius * ratio;

        getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

        addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((_point1/*point2*/ + ideal_contour_point_of_origin).toPoint()), -1);
        addIdealSkeletonArc(t.map((point0 + ideal_contour_point_of_origin).toPoint()), t.map((_point1 + ideal_contour_point_of_origin).toPoint()), R, qRadiansToDegrees(alpha), 180.0 - qRadiansToDegrees(phi));
        addIdealSkeletonLine(t.map((_point2/*point2*/ + ideal_contour_point_of_origin).toPoint()), t.map((point3 + ideal_contour_point_of_origin).toPoint()), -1);
        // ----------

        addIdealSkeletonLine(t.map((point3 + ideal_contour_point_of_origin).toPoint()), t.map(ideal_contour_point_of_origin.toPoint()), 0, true);
    }
    else if(detail_research_puanson_model == PuansonModel::PUANSON_MODEL_661) // Пуансон-граната
    {
        line_1_x = _scale * ideal_etalon_dimensions.diameter_5_dimension / 2.0;
        line_1_y = _scale * 0;
        line_2_x = _scale * ideal_etalon_dimensions.diameter_1_dimension / 2.0;
        line_2_y = _scale * ideal_etalon_dimensions.top_part_lenght;
        line_3_x = _scale * -static_cast<qint32>(ideal_etalon_dimensions.diameter_1_dimension / 2.0);
        line_3_y = _scale * ideal_etalon_dimensions.top_part_lenght;

        point1 = QPointF(line_1_x, line_1_y) * ratio;
        point2 = QPointF(line_2_x, line_2_y) * ratio;
        point3 = QPointF(line_3_x, line_3_y) * ratio;

        addIdealSkeletonLine(t.map((ideal_contour_point_of_origin).toPoint()), t.map((point1 + ideal_contour_point_of_origin).toPoint()), -1);
        addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((point2 + ideal_contour_point_of_origin).toPoint()), -1);
        addIdealSkeletonLine(t.map((point2 + ideal_contour_point_of_origin).toPoint()), t.map((point3 + ideal_contour_point_of_origin).toPoint()), -1);

        point1 = QPointF(-line_1_x, line_1_y) * ratio;

        addIdealSkeletonLine(t.map((point3 + ideal_contour_point_of_origin).toPoint()), t.map((point1 + ideal_contour_point_of_origin).toPoint()), -1);
        addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((ideal_contour_point_of_origin).toPoint()), -1, true);
    }
#if 1
    // Нижняя часть
    const qreal line_4_x = _scale * ideal_etalon_dimensions.diameter_6_dimension / 2.0;
    const qreal line_4_y = _scale * 0;

    const qreal line_5_x = _scale * ideal_etalon_dimensions.diameter_6_dimension / 2.0;
    const qreal line_5_y = _scale * -static_cast<qreal>(static_cast<quint32>(ideal_etalon_dimensions.skirt_bottom_part_lenght) - ideal_etalon_dimensions.skirt_bottom_rounding_radius);

    point1 = QPointF(line_1_x, line_1_y) * ratio;
    point2 = QPointF(line_4_x, line_4_y) * ratio;
    point3 = QPointF(line_5_x, line_5_y) * ratio;

    R = _scale * ideal_etalon_dimensions.skirt_bottom_rounding_radius * ratio;

    getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

    //addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((_point1 + ideal_contour_point_of_origin).toPoint()), 1);
    addIdealSkeletonArc(t.map((point0 + ideal_contour_point_of_origin).toPoint()), t.map((_point1 + ideal_contour_point_of_origin).toPoint()), R, -qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)), false);
    addIdealSkeletonLine(t.map((_point2/*point2*/ + ideal_contour_point_of_origin).toPoint()), t.map((point3 + ideal_contour_point_of_origin).toPoint()), 1);

    const qreal line_6_x = _scale * ideal_etalon_dimensions.diameter_7_dimension / 2.0;
    const qreal line_6_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.skirt_bottom_part_lenght);

    const qreal line_7_x = _scale * ideal_etalon_dimensions.diameter_7_dimension / 2.0;
    const qreal line_7_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.middle_part_lenght - 500 * 1.5);

    const qreal line_8_x = _scale * ideal_etalon_dimensions.diameter_8_dimension / 2.0;
    const qreal line_8_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.middle_part_lenght);

    point1 = QPointF(line_6_x, line_6_y) * ratio;

    addIdealSkeletonLine(t.map((point3 + ideal_contour_point_of_origin).toPoint()), t.map((point1 + ideal_contour_point_of_origin).toPoint()), 1);

    point2 = QPointF(line_7_x, line_7_y) * ratio;
    point3 = QPointF(line_8_x, line_8_y) * ratio;

    addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((point2 + ideal_contour_point_of_origin).toPoint()), 1);
    addIdealSkeletonLine(t.map((point2 + ideal_contour_point_of_origin).toPoint()), t.map((point3 + ideal_contour_point_of_origin).toPoint()), 1);

    const qreal line_9_x = _scale * ideal_etalon_dimensions.diameter_8_dimension / 2.0;
    const qreal line_9_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght - 1000);

    const qreal line_10_x = _scale * ideal_etalon_dimensions.diameter_9_dimension / 2.0;
    const qreal line_10_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght);

    point1 = QPointF(line_9_x, line_9_y) * ratio;
    point2 = QPointF(line_10_x, line_10_y) * ratio;

    addIdealSkeletonLine(t.map((point3 + ideal_contour_point_of_origin).toPoint()), t.map((point1 + ideal_contour_point_of_origin).toPoint()), 1);
    addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((point2 + ideal_contour_point_of_origin).toPoint()), 1);

    const qreal line_11_x = _scale * ideal_etalon_dimensions.groove_width_dimension / 2.0;
    const qreal line_11_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght);

    const qreal line_12_x = _scale * ideal_etalon_dimensions.groove_width_dimension / 2.0;
    const qreal line_12_y = _scale * -static_cast<qreal>(ideal_etalon_dimensions.middle_part_lenght + ideal_etalon_dimensions.bottom_part_lenght - ideal_etalon_dimensions.groove_depth_dimension);

    point3 = point2;

    point1 = QPointF(line_11_x, line_11_y) * ratio;
    point2 = QPointF(line_12_x, line_12_y) * ratio;

    addIdealSkeletonLine(t.map((point3 + ideal_contour_point_of_origin).toPoint()), t.map((point1 + ideal_contour_point_of_origin).toPoint()), -1);
    addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((point2 + ideal_contour_point_of_origin).toPoint()), -1);

    point3 = point2;

    point1 = QPointF(-line_12_x, line_12_y) * ratio;
    point2 = QPointF(-line_11_x, line_11_y) * ratio;

    addIdealSkeletonLine(t.map((point3 + ideal_contour_point_of_origin).toPoint()), t.map((point1 + ideal_contour_point_of_origin).toPoint()), -1);
    addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((point2 + ideal_contour_point_of_origin).toPoint()), -1);

    point3 = point2;

    point1 = QPointF(-line_10_x, line_10_y) * ratio;
    point2 = QPointF(-line_9_x, line_9_y) * ratio;

    addIdealSkeletonLine(t.map((point3 + ideal_contour_point_of_origin).toPoint()), t.map((point1 + ideal_contour_point_of_origin).toPoint()), -1);
    addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((point2 + ideal_contour_point_of_origin).toPoint()), -1);

    point1 = QPointF(-line_8_x, line_8_y) * ratio;

    addIdealSkeletonLine(t.map((point2 + ideal_contour_point_of_origin).toPoint()), t.map((point1 + ideal_contour_point_of_origin).toPoint()), -1);

    point2 = QPointF(-line_7_x, line_7_y) * ratio;
    point3 = QPointF(-line_6_x, line_6_y) * ratio;

    addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((point2 + ideal_contour_point_of_origin).toPoint()), -1);
    addIdealSkeletonLine(t.map((point2 + ideal_contour_point_of_origin).toPoint()), t.map((point3 + ideal_contour_point_of_origin).toPoint()), -1);

    point1 = QPointF(-line_5_x, line_5_y) * ratio;

    addIdealSkeletonLine(t.map((point3 + ideal_contour_point_of_origin).toPoint()), t.map((point1 + ideal_contour_point_of_origin).toPoint()), 1);

    point2 = QPointF(-line_4_x, line_4_y) * ratio;
    point3 = QPointF(-line_1_x, line_1_y) * ratio;

    R = _scale * ideal_etalon_dimensions.skirt_bottom_rounding_radius * ratio;

    getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

    addIdealSkeletonLine(t.map((point1 + ideal_contour_point_of_origin).toPoint()), t.map((_point1 + ideal_contour_point_of_origin).toPoint()), -1);
    addIdealSkeletonArc(t.map((point0 + ideal_contour_point_of_origin).toPoint()), t.map((_point1 + ideal_contour_point_of_origin).toPoint()), R, qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)), true);
    //addIdealSkeletonLine(t.map((_point2 + ideal_contour_point_of_origin).toPoint()), t.map((point3 + ideal_contour_point_of_origin).toPoint()), -1);

#endif // 0

    PuansonImage::drawIdealContour(detail_research_puanson_model, r, ideal_contour_point_of_origin, ideal_contour_rotation_angle, draw_measurements, _scale, calibration_ratio, idealContourPath, idealContourMeasurementsPath);

    return idealContourPath;
}

int PuansonImage::pointDistanceToLine(const QPoint &pt, const QLine &line)
{
    int dist  = qRound(std::abs((line.y2() - line.y1()) * pt.x() - (line.x2() - line.x1()) * pt.y() + line.x2() * line.y1() - line.y2() * line.x1())
                   / qSqrt((line.y2() - line.y1()) * (line.y2() - line.y1()) + (line.x2() - line.x1()) * (line.x2() - line.x1())));

    return dist;
}

bool PuansonImage::findNearestIdealLineNormalVector(const QPoint &pt, QLine &last_line, QPoint &internalToleranceNormalVector, QPoint &externalToleranceNormalVector) const
{
#define NEAREST_LINE_MAX_DISTANCE 50

    QLine nearest_line;
    quint16 nearest_line_distance = UINT16_MAX;
    quint16 current_line_distance;

    IdealContourArc nearest_arc;

    qreal L;
    qreal ll;
    qreal _phi;
    qreal pseudoscalar_production;
    bool pseudoscalar_production_condition;

    // Arcs
    for(auto it = idealSkeletonArcs.begin(); it != idealSkeletonArcs.end(); it++)
    {
        L = 100000.0;
        ll = qSqrt((pt.x() - it->Center().x()) * (pt.x() - it->Center().x()) + (pt.y() - it->Center().y()) * (pt.y() - it->Center().y()));

        _phi = qRadiansToDegrees(qAcos(((pt.x() - it->Center().x()) * (it->StartPoint().x() - it->Center().x()) + (pt.y() - it->Center().y()) * (it->StartPoint().y() - it->Center().y())) \
                / (ll * qSqrt(((it->StartPoint().x() - it->Center().x()) * (it->StartPoint().x() - it->Center().x()) + (it->StartPoint().y() - it->Center().y()) * (it->StartPoint().y() - it->Center().y()))))));

        pseudoscalar_production = (it->StartPoint().x() - it->Center().x()) * (pt.y() - it->Center().y()) - (it->StartPoint().y() - it->Center().y()) * (pt.x() - it->Center().x());

        pseudoscalar_production_condition = it->Phi() > 0 ? pseudoscalar_production <= 0.0 : pseudoscalar_production > 0.0;

        if(pseudoscalar_production_condition && _phi < qAbs(it->Phi()))
        {
            L = qAbs(it->R() - ll);

            uint SHIFT = 0;

            if(L < nearest_line_distance)
            {
                nearest_arc = *it;
                nearest_line_distance = static_cast<quint16>(qRound(L));
                internalToleranceNormalVector = QPoint(qRound((pt.x() - it->Center().x()) / ll * (internalToleranceMkm + SHIFT) / calibration_ratio), qRound((pt.y() - it->Center().y()) / ll * (internalToleranceMkm + SHIFT) / calibration_ratio));
                externalToleranceNormalVector = QPoint(-qRound((pt.x() - it->Center().x()) / ll * (externalToleranceMkm + SHIFT) / calibration_ratio), -qRound((pt.y() - it->Center().y()) / ll * (externalToleranceMkm + SHIFT)  / calibration_ratio));
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
                ((current_line_distance = static_cast<quint16>(pointDistanceToLine(pt, last_line))) < nearest_line_distance) &&
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
        current_line_distance = static_cast<quint16>(pointDistanceToLine(pt, *line_it));

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
            last_line = nearest_line;
        else                                    // Nearest is arc
            last_line.setLine(0, 0, 0, 0);

        return true;
    }
    else
    {
        last_line.setLine(0, 0, 0, 0);
        return false;
    }
}

bool PuansonImage::getQImage(QImage &img) const
{
    if(image.empty())
        return false;

    img = QImage(reinterpret_cast<const unsigned char *>(image.data),
                  image.cols, image.rows,
                  static_cast<int>(image.step), QImage::Format_RGB888).rgbSwapped();

    return true;
}

void PuansonImage::release()
{
    image_transform.reset();

    image.release();
    image_contour.release();
    if(p_raw_image_reference_counter != Q_NULLPTR)
    {
        if(*p_raw_image_reference_counter > 0)
            --(*p_raw_image_reference_counter);

        if(*p_raw_image_reference_counter == 0 && raw_image != Q_NULLPTR)
        {
            LibRaw::dcraw_clear_mem(raw_image);
            delete p_raw_image_reference_counter;
            p_raw_image_reference_counter = Q_NULLPTR;

            raw_image = Q_NULLPTR;
        }
    }
}

void PuansonImage::setReferencePoints(const QPoint &p1, const QPoint &p2)
{
    reference_point1 = p1;
    reference_point2 = p2;

    QPoint distance_vector_px = reference_point2 - reference_point1;
    reference_point_distance_px = static_cast<quint32>(qRound(qSqrt(distance_vector_px.x()*distance_vector_px.x() + distance_vector_px.y()*distance_vector_px.y())));

    calculateCalibrationRatio();
}

void PuansonImage::calculateDetailDimensionsFromDeviations(const PuansonImage& etalon_image)
{
    for(quint8 i = 0; i < diameter_dimensions.size(); i++)
    {
        diameter_dimensions[i].measurement_actual_diameter = etalon_image.diameter_dimensions[i].measurement_actual_diameter + static_cast<quint16>(diameter_dimensions[i].right_side_deviation + diameter_dimensions[i].left_side_deviation);
    }
}

bool PuansonImage::loadEtalonAngle(const QString &etalon_dir)
{
    using namespace std;
    using namespace cv;

    const QString xml_config_filename = "etalon_angle_configuration.xml";
    QFile config_file(etalon_dir + "/" + xml_config_filename);

    if(!config_file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Load etalon error: Configuration file opening error";
        return false;
    }

    QXmlStreamReader xml(&config_file);
    QString original_path, contours_path;
    quint8 loaded_parameters = 0;

    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartDocument)
            continue;

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "etalon_angle_settings")
                continue;

            if (xml.name() == "original_image")
            {
                if(xml.attributes().hasAttribute("filename"))
                {
                    original_path = xml.attributes().value("filename").toString();
                    loaded_parameters++;
                }
            }
            else if (xml.name() == "contours_image")
            {
                if(xml.attributes().hasAttribute("filename"))
                {
                    contours_path = xml.attributes().value("filename").toString();
                    loaded_parameters++;
                }
            }
            else if (xml.name() == "left_bottom_reference_point_position")
            {
                if(xml.attributes().hasAttribute("x") && xml.attributes().hasAttribute("y"))
                {
                    reference_point1 = QPoint(xml.attributes().value("x").toString().toInt(), xml.attributes().value("y").toString().toInt());
                    loaded_parameters++;
                }
            }
            else if (xml.name() == "right_top_reference_point_position")
            {
                if(xml.attributes().hasAttribute("x") && xml.attributes().hasAttribute("y"))
                {
                    reference_point2 = QPoint(xml.attributes().value("x").toString().toInt(), xml.attributes().value("y").toString().toInt());
                    loaded_parameters++;
                }
            }
            else if (xml.name() == "left_bottom_reference_point_search_area")
            {
                if(xml.attributes().hasAttribute("x") && xml.attributes().hasAttribute("y") && xml.attributes().hasAttribute("width") && xml.attributes().hasAttribute("height"))
                {
                    PuansonChecker::getInstance()->setReferencePointSearchArea(ReferencePointType_e::REFERENCE_POINT_1, QRect(xml.attributes().value("x").toString().toInt(), xml.attributes().value("y").toString().toInt(), xml.attributes().value("width").toString().toInt(), xml.attributes().value("height").toString().toInt()));
                    loaded_parameters++;
                }
            }
            else if (xml.name() == "right_top_reference_point_search_area")
            {
                if(xml.attributes().hasAttribute("x") && xml.attributes().hasAttribute("y") && xml.attributes().hasAttribute("width") && xml.attributes().hasAttribute("height"))
                {
                    PuansonChecker::getInstance()->setReferencePointSearchArea(ReferencePointType_e::REFERENCE_POINT_2, QRect(xml.attributes().value("x").toString().toInt(), xml.attributes().value("y").toString().toInt(), xml.attributes().value("width").toString().toInt(), xml.attributes().value("height").toString().toInt()));
                    loaded_parameters++;
                }
            }
            else if (xml.name() == "ideal_contour_position")
            {
                xml.readNext();

                while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "ideal_contour_position"))
                {
                    if (xml.tokenType() == QXmlStreamReader::StartElement)
                    {
                        if (xml.name() == "point_of_origin_position")
                        {
                            if(xml.attributes().hasAttribute("x") && xml.attributes().hasAttribute("y"))
                            {
                                ideal_contour_point_of_origin = QPoint(xml.attributes().value("x").toString().toInt(), xml.attributes().value("y").toString().toInt());
                                loaded_parameters++;
                            }
                        }
                        else if (xml.name() == "rotation_angle")
                        {
                            if(xml.attributes().hasAttribute("degrees"))
                            {
                                ideal_contour_rotation_angle = xml.attributes().value("degrees").toString().toDouble();
                                loaded_parameters++;
                            }
                        }
                    }
                    xml.readNext();
                }

                image_type = ImageType_e::ETALON_IMAGE;
                isIdealContourSetFlag = true;
            }
            else if (xml.name() == "detail_check_result")
            {
                if(xml.attributes().hasAttribute("suitability"))
                {
                    // current detail is valid
                    if(xml.attributes().value("suitability") == "yes")
                        isCurrentDetailSuitable = true;
                    else if(xml.attributes().value("suitability") == "no")
                        isCurrentDetailSuitable = false;

                    image_type = ImageType_e::CURRENT_IMAGE;
                }
            }
        }
    }

    config_file.close();

    if(loaded_parameters == ETALON_ANGLE_FULL_NUMBER_OF_PARAMETERS)
    {
        setReferencePoints(reference_point1, reference_point2);

        PuansonChecker::getInstance()->loadEtalonImage(1, etalon_dir + "/" + original_path, etalon_dir + "/" + contours_path, true);
        //PuansonChecker::getInstance()->drawEtalonImage(false);

        return true;
    }
    else
        return false;
}

bool PuansonImage::saveEtalonAngle(const QString &etalon_dir) const
{
    using namespace std;
    using namespace cv;

    const QString original_filename = "original_image.nef";
    const QString contours_filename = "contours_image.tiff";
    const QString xml_config_filename = "etalon_angle_configuration.xml";

    if(filename.isEmpty() || image_contour.empty() || reference_point1.isNull() || reference_point2.isNull() || ideal_contour_point_of_origin.isNull())
        return false;

    QFile::copy(filename, etalon_dir + "/" + original_filename);

    vector<int> compression_params;
    compression_params.push_back(IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(3);

    imwrite((etalon_dir + "/" + contours_filename).toStdString(), image_contour/*, compression_params*/);

    QFile config_file(etalon_dir + "/" + xml_config_filename);

    if(!config_file.open(QIODevice::WriteOnly))
    {
        config_file.close();
        return false;
    }

    QXmlStreamWriter stream(&config_file);
    stream.setAutoFormatting(true);

    QRect reference_point_1_auto_search_area_rect = PuansonChecker::getInstance()->getReferencePointSearchArea(ImageType_e::ETALON_IMAGE, ReferencePointType_e::REFERENCE_POINT_1);
    QRect reference_point_2_auto_search_area_rect = PuansonChecker::getInstance()->getReferencePointSearchArea(ImageType_e::ETALON_IMAGE, ReferencePointType_e::REFERENCE_POINT_2);

    stream.writeStartDocument();
    stream.writeStartElement("etalon_angle_settings");
        stream.writeStartElement("original_image");
            stream.writeAttribute("filename", original_filename);
        stream.writeEndElement(); // original_image
        stream.writeStartElement("contours_image");
            stream.writeAttribute("filename", contours_filename);
        stream.writeEndElement(); // contours_image
        stream.writeStartElement("left_bottom_reference_point_position");
            stream.writeAttribute("x", QString::number(reference_point1.x()));
            stream.writeAttribute("y", QString::number(reference_point1.y()));
        stream.writeEndElement(); // left_bottom_reference_point_position
        stream.writeStartElement("right_top_reference_point_position");
            stream.writeAttribute("x", QString::number(reference_point2.x()));
            stream.writeAttribute("y", QString::number(reference_point2.y()));
        stream.writeEndElement(); // right_top_reference_point_position
        stream.writeStartElement("left_bottom_reference_point_search_area");
            stream.writeAttribute("x", QString::number(reference_point_1_auto_search_area_rect.x()));
            stream.writeAttribute("y", QString::number(reference_point_1_auto_search_area_rect.y()));
            stream.writeAttribute("width", QString::number(reference_point_1_auto_search_area_rect.width()));
            stream.writeAttribute("height", QString::number(reference_point_1_auto_search_area_rect.height()));
        stream.writeEndElement(); // left_bottom_reference_point_search_area
        stream.writeStartElement("right_top_reference_point_search_area");
            stream.writeAttribute("x", QString::number(reference_point_2_auto_search_area_rect.x()));
            stream.writeAttribute("y", QString::number(reference_point_2_auto_search_area_rect.y()));
            stream.writeAttribute("width", QString::number(reference_point_2_auto_search_area_rect.width()));
            stream.writeAttribute("height", QString::number(reference_point_2_auto_search_area_rect.height()));
        stream.writeEndElement(); // right_top_reference_point_search_area

        if(image_type == ImageType_e::ETALON_IMAGE)
        {
            stream.writeStartElement("ideal_contour_position");
                stream.writeStartElement("point_of_origin_position");
                    stream.writeAttribute("x", QString::number(ideal_contour_point_of_origin.x()));
                    stream.writeAttribute("y", QString::number(ideal_contour_point_of_origin.y()));
                stream.writeEndElement(); // point_of_origin_position
                stream.writeStartElement("rotation_angle");
                    stream.writeAttribute("degrees", QString::number(ideal_contour_rotation_angle));
                stream.writeEndElement(); // rotation_angle
            stream.writeEndElement(); // ideal_contour_position
        }
        else if(image_type == ImageType_e::CURRENT_IMAGE)
        {
            stream.writeStartElement("detail_check_result");
                stream.writeAttribute("suitability", isCurrentDetailSuitable ? "yes" : "no");
            stream.writeEndElement(); // detail_check_result
        }

    stream.writeEndElement(); // etalon_angle_settings
    stream.writeEndDocument();

    config_file.close();

    return true;
}
