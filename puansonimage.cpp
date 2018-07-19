#include "puansonimage.h"

#include <QGenericMatrix>
#include <QVector2D>
#include <QtMath>
#include <QDebug>

#include <functional>

PuansonImage::PuansonImage(const ImageType_e _image_type, const Mat &_image, const libraw_processed_image_t *_raw_image, const QString &_filename):
    image(_image), raw_image(const_cast<libraw_processed_image_t *>(_raw_image)), p_raw_image_reference_counter(new quint16(1)), filename(_filename),
    image_type(_image_type), empty(false), cropped(false), crop_scale(1.0), reference_point_distance_mkm(0), reference_point_distance_px(0),
    calibration_ratio(0.0), externalToleranceMkm(0), internalToleranceMkm(0), isIdealContourSetFlag(false)
{
    Y_top = qRound((image.rows - image.rows / 4.0) / 2.0);
    Y_bottom = qRound((image.rows + image.rows / 4.0) / 2.0);
    X_left = qRound((image.cols - image.cols / 4.0) / 2.0);
    X_right = qRound((image.cols + image.cols / 4.0) / 2.0);
}

PuansonImage::PuansonImage():
    raw_image(Q_NULLPTR), p_raw_image_reference_counter(new quint16(0)), filename(""), image_type(UNDEFINED_IMAGE),
    empty(true), cropped(false), crop_scale(1.0), reference_point_distance_mkm(0), reference_point_distance_px(0),
    calibration_ratio(0.0), externalToleranceMkm(0), internalToleranceMkm(0), isIdealContourSetFlag(false),
    Y_top(0), Y_bottom(0), X_left(0), X_right(0)
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

    copyIdealContour(img);

    Y_top = img.Y_top;
    Y_bottom = img.Y_bottom;
    X_left = img.X_left;
    X_right = img.X_right;

    p_raw_image_reference_counter = img.p_raw_image_reference_counter;
    (*p_raw_image_reference_counter)++;

    return *this;
}

QVector<QLine> IdealInnerSegment::getControlLines(quint8 step)
{
    using namespace std;

    QVector<QLine> control_lines;
    const qreal normal_vector_len = 80.0;

    for(QLine &inner_line: inner_lines)
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
            control_lines.append(QLineF(pt - int_normal_vector, pt + int_normal_vector).toLine());
            pt += line_ort * step;
        }
        while(conditionLambda(pt, inner_line));
    }

    for(IdealContourArc &inner_arc: inner_arcs)
    {
        QPointF pt = inner_arc.StartPoint();
        QMatrix m;
        qreal delta_phi = qRadiansToDegrees((inner_arc.Phi() >= 0 ? 1 : -1) * step / inner_arc.R());
        qreal current_phi = 0.0;

        m.rotate(-delta_phi);

        do
        {
            control_lines.append(QLineF(pt + ((inner_arc.Center() - pt) / inner_arc.R() * normal_vector_len), pt - ((inner_arc.Center() - pt) / inner_arc.R() * normal_vector_len)).toLine());

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

    if(_point1.x() == point2.x())
    {
        std::swap(_point1, _point2);
        points_was_changed = true;
    }

    // Центр окружности скругления
    qreal y0 = ( _point1.y() * (point2.y() -_point1.y()) * (_point2.x() - point2.x()) + _point2.y() * (_point1.x() - point2.x()) * (_point2.y() - point2.y()) - (_point2.x() - point2.x()) * (_point1.x() - _point2.x()) * (_point1.x() - point2.x()) ) /
            ( (point2.y() - _point1.y()) * (_point2.x() - point2.x()) + (_point1.x() - point2.x()) * (_point2.y() - point2.y()) );
    qreal x0 = ((point2.y() - _point1.y()) * (y0 - _point1.y())) / (_point1.x() - point2.x()) + _point1.x();
    point0 = QPointF(x0, y0);

    if(points_was_changed)
        std::swap(_point1, _point2);

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

void PuansonImage::addIdealSkeletonLine(const QPoint &p1, const QPoint &p2, int normal_vecror_x_sign)
{
    QLine new_line;
    QPoint int_normal_vector, ext_normal_vector;
    qreal x_0;
    qreal y_0;

    QPoint p0, pN;

    new_line = QLine(p1, p2);

    if(/*p0.isNull() &&*/ (p1.x() < X_left && p2.x() > X_left))
    {
        p0 = QPoint(X_left, qRound((X_left - p1.x()) / static_cast<qreal>(p2.x() - p1.x()) * (p2.y() - p1.y()) + p1.y()));

        if(!(p0.y() >= Y_top && p0.y() <= Y_bottom))
        {
            p0.setX(0);
            p0.setY(0);
        }
    }
    if(p0.isNull() && (p1.x() > X_right && p2.x() < X_right))
    {
        p0 = QPoint(X_right, qRound((X_right - p1.x()) / static_cast<qreal>(p2.x() - p1.x()) * (p2.y() - p1.y()) + p1.y()));

        if(!(p0.y() >= Y_top && p0.y() <= Y_bottom))
        {
            p0.setX(0);
            p0.setY(0);
        }
    }
    if(p0.isNull() && (p1.y() > Y_bottom && p2.y() < Y_bottom))
    {
        p0 = QPoint(qRound((Y_bottom - p1.y()) / static_cast<qreal>(p2.y() - p1.y()) * (p2.x() - p1.x()) + p1.x()), Y_bottom);

        if(!(p0.x() >= X_left && p0.x() <= X_right))
        {
            p0.setX(0);
            p0.setY(0);
        }
    }
    if(p0.isNull() && (p1.y() < Y_top && p2.y() > Y_top))
    {
        p0 = QPoint(qRound((Y_top - p1.y()) / static_cast<qreal>(p2.y() - p1.y()) * (p2.x() - p1.x()) + p1.x()), Y_top);

        if(!(p0.x() >= X_left && p0.x() <= X_right))
        {
            p0.setX(0);
            p0.setY(0);
        }
    }

    if(/*pN.isNull() && */(p1.x() > X_left && p2.x() < X_left))
    {
        pN = QPoint(X_left, qRound((X_left - p1.x()) / static_cast<qreal>(p2.x() - p1.x()) * (p2.y() - p1.y()) + p1.y()));

        if(!(pN.y() >= Y_top && pN.y() <= Y_bottom) ||
                !(pN.y() >= qMin(p1.y(), p2.y()) && pN.y() <= qMax(p1.y(), p2.y()) &&
                    (pN.x() >= qMin(p1.x(), p2.x()) && pN.x() <= qMax(p1.x(), p2.x()))))
        {
            pN.setX(0);
            pN.setY(0);
        }
    }
    if(pN.isNull() && (p1.x() < X_right && p2.x() > X_right))
    {
        pN = QPoint(X_right, qRound((X_right - p1.x()) / static_cast<qreal>(p2.x() - p1.x()) * (p2.y() - p1.y()) + p1.y()));

        if(!(pN.y() >= Y_top && pN.y() <= Y_bottom) ||
                !(pN.y() >= qMin(p1.y(), p2.y()) && pN.y() <= qMax(p1.y(), p2.y()) &&
                    (pN.x() >= qMin(p1.x(), p2.x()) && pN.x() <= qMax(p1.x(), p2.x()))))
        {
            pN.setX(0);
            pN.setY(0);
        }
    }
    if(pN.isNull() && (p1.y() < Y_bottom && p2.y() > Y_bottom))
    {
        pN = QPoint(qRound((Y_bottom - p1.y()) / static_cast<qreal>(p2.y() - p1.y()) * (p2.x() - p1.x()) + p1.x()), Y_bottom);

        if(!(pN.x() >= X_left && pN.x() <= X_right) ||
                !(pN.y() >= qMin(p1.y(), p2.y()) && pN.y() <= qMax(p1.y(), p2.y()) &&
                    (pN.x() >= qMin(p1.x(), p2.x()) && pN.x() <= qMax(p1.x(), p2.x()))))
        {
            pN.setX(0);
            pN.setY(0);
        }
    }
    if(pN.isNull() && (p1.y() > Y_top && p2.y() < Y_top))
    {
        pN = QPoint(qRound((Y_top - p1.y()) / static_cast<qreal>(p2.y() - p1.y()) * (p2.x() - p1.x()) + p1.x()), Y_top);

        if(!(pN.x() >= X_left && pN.x() <= X_right) ||
                !(pN.y() >= qMin(p1.y(), p2.y()) && pN.y() <= qMax(p1.y(), p2.y()) &&
                    (pN.x() >= qMin(p1.x(), p2.x()) && pN.x() <= qMax(p1.x(), p2.x()))))
        {
            pN.setX(0);
            pN.setY(0);
        }
    }

    if(!pN.isNull())
    {
        if(!p0.isNull())
        {
            actualIdealInnerSegments.append(IdealInnerSegment(p0, QLine(p0, pN), pN));
        }
        else if(actualIdealInnerSegments.size() > 0)
        {
            actualIdealInnerSegments.last().addInnerLine(QLine(new_line.p1(), pN));
            actualIdealInnerSegments.last().setEndPoint(pN);
        }
        /*else
            qDebug() << "pN without p0!!!";*/
    }
    else
    {
        if(!p0.isNull())
        {
            actualIdealInnerSegments.append(IdealInnerSegment(p0, QLine(p0, new_line.p2())));
        }
    }

    if(p0.isNull() && !actualIdealInnerSegments.last().isStartPointNull() && actualIdealInnerSegments.last().isEndPointNull())
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

bool PuansonImage::findP0(const QPointF &center, const qreal R, QPointF &start_point, qreal &alpha, qreal &phi, QPoint &p0)
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
        if(alpha != 0)
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

bool PuansonImage::findPN(const QPointF &center, const qreal R, QPointF &start_point, qreal &alpha, qreal &phi, QPoint &pN)
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

void PuansonImage::addIdealSkeletonArc(const QPointF &center, const QPointF &start_point, const qreal R, const qreal alpha, const qreal phi)
{
    IdealContourArc new_arc(center, start_point, R, alpha, phi);

    QPoint p0, pN;
    QPoint end_point;
    QMatrix m;

    qreal Alpha = alpha; // Начальный угол дуги после определения точки p0
    qreal Phi = phi;   // Угол дуги после определения точки p0

    QPointF _start_point = start_point;

    end_point = start_point.toPoint();
    end_point -= center.toPoint();
    m.rotate(-phi);
    end_point = end_point * m;
    end_point += center.toPoint();

    if(_start_point.x() <= X_right && _start_point.x() >= X_left && _start_point.y() <= Y_bottom && _start_point.y() >= Y_top)
    {
        if(findPN(center, R, _start_point, Alpha, Phi, pN))
        {
            if(actualIdealInnerSegments.size() > 0)
            {
                actualIdealInnerSegments.last().addInnerArc(IdealContourArc(center, start_point, R, Alpha - rotation_angle, Phi));
                actualIdealInnerSegments.last().setEndPoint(pN);
            }

            Alpha = Alpha + Phi;
            Phi = phi - Phi;
        }

        if(findP0(center, R, _start_point, Alpha, Phi, p0))
        {
            actualIdealInnerSegments.append(IdealInnerSegment(p0, IdealContourArc(center, p0, R, Alpha - rotation_angle, Phi)));
        }
    }
    else
    {
        findP0(center, R, _start_point, Alpha, Phi, p0);
        findPN(center, R, _start_point, Alpha, Phi, pN);

        if(!p0.isNull() && pN.isNull())
        {
            actualIdealInnerSegments.append(IdealInnerSegment(p0, IdealContourArc(center, p0, R, Alpha - rotation_angle, Phi)));
        }

        if(!pN.isNull())
        {
            if(!p0.isNull())
            {
                actualIdealInnerSegments.append(IdealInnerSegment(p0, IdealContourArc(center, p0, R, Alpha - rotation_angle, Phi), pN));
            }
            else if(actualIdealInnerSegments.size() > 0)
            {
                actualIdealInnerSegments.last().addInnerArc(IdealContourArc(center, _start_point, R, Alpha - rotation_angle, Phi));
                actualIdealInnerSegments.last().setEndPoint(pN);
            }
            /*else
                qDebug() << "pN without p0!!!";*/
        }
    }

    if(p0.isNull() && pN.isNull() && !actualIdealInnerSegments.last().isStartPointNull() && actualIdealInnerSegments.last().isEndPointNull())
    {
        actualIdealInnerSegments.last().addInnerArc(IdealContourArc(new_arc.Center(), new_arc.StartPoint(), new_arc.R(), new_arc.Alpha() - rotation_angle, new_arc.Phi()));
    }

    idealSkeletonArcs.append(new_arc);
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
    qreal R = _scale * 5000 * ratio;

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

        actualIdealInnerSegments.clear();
    }

#if 1
    // Верхняя часть
    // Скругление 1 между юбкой и иглой
    point1 = QPointF(line_1_x * ratio, line_1_y * ratio);
    point2 = QPointF(line_2_x * ratio, line_2_y * ratio);
    point3 = QPointF(line_3_x * ratio, line_3_y * ratio);

    R = _scale * 5000 * ratio;

    getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((_point1/*point2*/ + point_of_origin).toPoint()), 1);
    addIdealSkeletonArc(t.map((point0 + point_of_origin).toPoint()), t.map((_point1 + point_of_origin).toPoint()), R, qRadiansToDegrees(alpha), 180.0 - qRadiansToDegrees(phi));
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

   /* new_point = point_of_origin + point3;
    idealContourPath.lineTo(new_point);*/

    // Скругление 1 конца иглы
    R = _scale * 500 * ratio;

    point1 = _point2;
    point2 = point3;
    point3 = QPointF(-line_3_x * ratio, line_3_y * ratio);

    getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((_point1 + point_of_origin).toPoint()), 1);
    addIdealSkeletonArc(t.map((point0 + point_of_origin).toPoint()), t.map((_point1 + point_of_origin).toPoint()), R, qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));

    new_point = point_of_origin + _point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point0;
    idealContourPath.arcTo(QRectF(new_point.x() - R, new_point.y() - R, R * 2.0, R * 2.0), qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));

    /*new_point = point_of_origin + _point2;
    idealContourPath.lineTo(new_point);*/

    new_point = point_of_origin + _point2;
    idealContourPath.lineTo(new_point);
    // ----------

    // Скругление 2 конца иглы
    R = _scale * 500 * ratio;

    point1 = _point2;
    point2 = point3;
    point3 = QPointF(-line_2_x * ratio, line_2_y * ratio);

    getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((_point1 + point_of_origin).toPoint()), 1);
    addIdealSkeletonArc(t.map((point0 + point_of_origin).toPoint()), t.map((_point1 + point_of_origin).toPoint()), R, -qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));

    new_point = point_of_origin + _point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point0;
    idealContourPath.arcTo(QRectF(new_point.x() - R, new_point.y() - R, R * 2.0, R * 2.0), -qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));

    /*new_point = point_of_origin + _point2;
    idealContourPath.lineTo(new_point);*/
    // ----------

    // Скругление 2 между юбкой и иглой
    point1 = _point2;
    point2 = point3;
    point3 = QPointF(-line_1_x * ratio, line_1_y * ratio);

    R = _scale * 5000 * ratio;

    getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((_point1/*point2*/ + point_of_origin).toPoint()), -1);
    addIdealSkeletonArc(t.map((point0 + point_of_origin).toPoint()), t.map((_point1 + point_of_origin).toPoint()), R, qRadiansToDegrees(alpha), 180.0 - qRadiansToDegrees(phi));
    addIdealSkeletonLine(t.map((_point2/*point2*/ + point_of_origin).toPoint()), t.map((point3 + point_of_origin).toPoint()), -1);
    // ----------

    /*new_point = point_of_origin + point1;
    idealContourPath.lineTo(new_point);*/

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
#endif // 0
    // Нижняя часть
    const qreal line_4_x = _scale * 4000;
    const qreal line_4_y = _scale * 0;

    const qreal line_5_x = _scale * 4000;
    const qreal line_5_y = _scale * -2500;

    point1 = QPointF(line_1_x * ratio, line_1_y * ratio);
    point2 = QPointF(line_4_x * ratio, line_4_y * ratio);
    point3 = QPointF(line_5_x * ratio, line_5_y * ratio);

    new_point = point_of_origin;
    //new_point = t.map(new_point);

    /*
    idealContourPath.lineTo(new_point + point3);*/

    R = _scale * 500 * ratio;

    getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((_point1 + point_of_origin).toPoint()), 1);
    addIdealSkeletonArc(t.map((point0 + point_of_origin).toPoint()), t.map((_point1 + point_of_origin).toPoint()), R, -qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));
    addIdealSkeletonLine(t.map((_point2/*point2*/ + point_of_origin).toPoint()), t.map((point3 + point_of_origin).toPoint()), 1);

    new_point = point_of_origin + point1;
    //idealContourPath.lineTo(new_point);
    idealContourPath.moveTo(new_point);

    new_point = point_of_origin + point0;
    idealContourPath.arcTo(QRectF(new_point.x() - R, new_point.y() - R, R * 2.0, R * 2.0), -qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));

    new_point = point_of_origin + _point2;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point3;
    idealContourPath.lineTo(new_point);

    const qreal line_6_x = _scale * 4500;
    const qreal line_6_y = _scale * -3000;

    const qreal line_7_x = _scale * 4500;
    const qreal line_7_y = _scale * -(40000 - 500 * 1.5);

    const qreal line_8_x = _scale * 4000;
    const qreal line_8_y = _scale * -40000;

    point1 = QPointF(line_6_x * ratio, line_6_y * ratio);

    addIdealSkeletonLine(t.map((point3 + point_of_origin).toPoint()), t.map((point1 + point_of_origin).toPoint()), 1);

    point2 = QPointF(line_7_x * ratio, line_7_y * ratio);
    point3 = QPointF(line_8_x * ratio, line_8_y * ratio);

    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((point2 + point_of_origin).toPoint()), 1);
    addIdealSkeletonLine(t.map((point2 + point_of_origin).toPoint()), t.map((point3 + point_of_origin).toPoint()), 1);

    new_point = point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point2;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point3;
    idealContourPath.lineTo(new_point);

    const qreal line_9_x = _scale * 4000;
    const qreal line_9_y = _scale * -55000;

    const qreal line_10_x = _scale * 3000;
    const qreal line_10_y = _scale * -56000;

    point1 = QPointF(line_9_x * ratio, line_9_y * ratio);
    point2 = QPointF(line_10_x * ratio, line_10_y * ratio);

    addIdealSkeletonLine(t.map((point3 + point_of_origin).toPoint()), t.map((point1 + point_of_origin).toPoint()), 1);
    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((point2 + point_of_origin).toPoint()), 1);

    new_point = point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point2;
    idealContourPath.lineTo(new_point);

    const qreal line_11_x = _scale * 750;
    const qreal line_11_y = _scale * -56000;

    const qreal line_12_x = _scale * 750;
    const qreal line_12_y = _scale * -53000;

    point3 = point2;

    point1 = QPointF(line_11_x * ratio, line_11_y * ratio);
    point2 = QPointF(line_12_x * ratio, line_12_y * ratio);

    addIdealSkeletonLine(t.map((point3 + point_of_origin).toPoint()), t.map((point1 + point_of_origin).toPoint()), -1);
    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((point2 + point_of_origin).toPoint()), -1);

    new_point = point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point2;
    idealContourPath.lineTo(new_point);

    point3 = point2;

    point1 = QPointF(-line_12_x * ratio, line_12_y * ratio);
    point2 = QPointF(-line_11_x * ratio, line_11_y * ratio);

    addIdealSkeletonLine(t.map((point3 + point_of_origin).toPoint()), t.map((point1 + point_of_origin).toPoint()), -1);
    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((point2 + point_of_origin).toPoint()), -1);

    new_point = point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point2;
    idealContourPath.lineTo(new_point);

    point3 = point2;

    point1 = QPointF(-line_10_x * ratio, line_10_y * ratio);
    point2 = QPointF(-line_9_x * ratio, line_9_y * ratio);

    addIdealSkeletonLine(t.map((point3 + point_of_origin).toPoint()), t.map((point1 + point_of_origin).toPoint()), -1);
    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((point2 + point_of_origin).toPoint()), -1);

    new_point = point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point2;
    idealContourPath.lineTo(new_point);

    point1 = QPointF(-line_8_x * ratio, line_8_y * ratio);

    addIdealSkeletonLine(t.map((point2 + point_of_origin).toPoint()), t.map((point1 + point_of_origin).toPoint()), -1);

    point2 = QPointF(-line_7_x * ratio, line_7_y * ratio);
    point3 = QPointF(-line_6_x * ratio, line_6_y * ratio);

    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((point2 + point_of_origin).toPoint()), -1);
    addIdealSkeletonLine(t.map((point2 + point_of_origin).toPoint()), t.map((point3 + point_of_origin).toPoint()), -1);

    new_point = point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point2;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point3;
    idealContourPath.lineTo(new_point);

    point1 = QPointF(-line_5_x * ratio, line_5_y * ratio);

    addIdealSkeletonLine(t.map((point3 + point_of_origin).toPoint()), t.map((point1 + point_of_origin).toPoint()), 1);

    point2 = QPointF(-line_4_x * ratio, line_4_y * ratio);
    point3 = QPointF(-line_1_x * ratio, line_1_y * ratio);

    new_point = point_of_origin + point1;
    idealContourPath.lineTo(new_point);

    R = _scale * 500 * ratio;

    getArc(R, point1, point2, point3, _point1, _point2, point0, phi, alpha);

    addIdealSkeletonLine(t.map((point1 + point_of_origin).toPoint()), t.map((_point1 + point_of_origin).toPoint()), -1);
    addIdealSkeletonArc(t.map((point0 + point_of_origin).toPoint()), t.map((_point1 + point_of_origin).toPoint()), R, qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));
    addIdealSkeletonLine(t.map((_point2 + point_of_origin).toPoint()), t.map((point3 + point_of_origin).toPoint()), -1);

    new_point = point_of_origin + _point1;
    idealContourPath.lineTo(new_point);

    new_point = point_of_origin + point0;
    idealContourPath.arcTo(QRectF(new_point.x() - R, new_point.y() - R, R * 2.0, R * 2.0), qRadiansToDegrees(alpha), -(180.0 - qRadiansToDegrees(phi)));

    /*new_point = point_of_origin + _point2;
    idealContourPath.moveTo(new_point);*/

    /*new_point = point_of_origin + point3;
    idealContourPath.lineTo(new_point);*/

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
                nearest_line_distance = L;
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
            last_line = nearest_line;
        }
        else                                    // Nearest is arc
        {
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
    reference_point_distance_px = qSqrt(distance_vector_px.x()*distance_vector_px.x() + distance_vector_px.y()*distance_vector_px.y());

    calculateCalibrationRatio();
}
