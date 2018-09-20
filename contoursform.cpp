#include "contoursform.h"
#include "ui_contoursform.h"
#include <QGraphicsPixmapItem>
#include <QMessageBox>

void ContoursForm::moveImage(const qreal dx, const qreal dy)
{
    QGraphicsPixmapItem *pixmap_item = ui->graphicsView->scene() ? qgraphicsitem_cast<QGraphicsPixmapItem *>(ui->graphicsView->scene()->items().last()) : Q_NULLPTR;

    if(pixmap_item)
    {
        qreal new_x, new_y;

        new_x = image_x - dx;
        new_y = image_y - dy;

        new_x = image_x - dx;
        new_y = image_y - dy;

        if(new_x < 0)
            new_x = 0;
        else if(new_x > pixmap_item->pixmap().width() - ui->graphicsView->width() + 7)
            new_x = pixmap_item->pixmap().width() - ui->graphicsView->width() + 7;

        if(new_y < 0)
            new_y = 0;
        else if(new_y > pixmap_item->pixmap().height() - ui->graphicsView->height() + 7)
            new_y = pixmap_item->pixmap().height() - ui->graphicsView->height() + 7;

        ui->graphicsView->centerOn(new_x + ui->graphicsView->width() / 2 - 3 , new_y + ui->graphicsView->height() / 2 - 3);

        image_x = new_x;
        image_y = new_y;
    }
}

void ContoursForm::mousePressEvent(const QPoint &p)
{
    if(calibration_mode == CalibrationMode_e::MANUAL_SETTING_REFERENCE_POINTS)
    {
        QPoint new_point;

        for(Measurement &measurement : measurements)
        {
            if(!(measurement.current_detail_position_is_set && measurement.outer_tolerance_etalon_detail_position_is_set))
            {
                if(p.x() >= (measurement.measurement_line.x2() > measurement.measurement_line.x1() ? measurement.measurement_line.x2() : measurement.measurement_line.x1()))
                    new_point = measurement.measurement_line.x2() > measurement.measurement_line.x1() ? measurement.measurement_line.p2() : measurement.measurement_line.p1();
                else if(p.x() <= (measurement.measurement_line.x1() < measurement.measurement_line.x2() ? measurement.measurement_line.x1() : measurement.measurement_line.x2()))
                    new_point = measurement.measurement_line.x1() < measurement.measurement_line.x2() ? measurement.measurement_line.p1() : measurement.measurement_line.p2();
                else
                    new_point = QPoint(p.x(), (p.x() - measurement.measurement_line.x1()) * (static_cast<qreal>(measurement.measurement_line.y2() - measurement.measurement_line.y1()) / (measurement.measurement_line.x2() - measurement.measurement_line.x1())) + measurement.measurement_line.y1());

                if(!measurement.current_detail_position_is_set && qSqrt((measurement.current_detail_point - p).x() * (measurement.current_detail_point - p).x() + (measurement.current_detail_point - p).y() * (measurement.current_detail_point - p).y()) <= MEASUREMENT_POINT_CONTOUR_RADIUS)
                {
                    measurement.active_point = &measurement.current_detail_point;

                    ui->graphicsView->scene()->removeItem(measurement.current_detail_point_item);
                    measurement.current_detail_point_item = ui->graphicsView->scene()->addEllipse(new_point.x() - MEASUREMENT_POINT_CONTOUR_RADIUS, new_point.y() - MEASUREMENT_POINT_CONTOUR_RADIUS, MEASUREMENT_POINT_CONTOUR_RADIUS*2, MEASUREMENT_POINT_CONTOUR_RADIUS*2, QPen(Qt::red, 1), QBrush(Qt::red, Qt::Dense5Pattern));

                    break;
                }
                else if(!measurement.outer_tolerance_etalon_detail_position_is_set && qSqrt((measurement.outer_tolerance_etalon_point - p).x() * (measurement.outer_tolerance_etalon_point - p).x() + (measurement.outer_tolerance_etalon_point - p).y() * (measurement.outer_tolerance_etalon_point - p).y()) <= MEASUREMENT_POINT_CONTOUR_RADIUS)
                {
                    measurement.active_point = &measurement.outer_tolerance_etalon_point;

                    ui->graphicsView->scene()->removeItem(measurement.outer_tolerance_etalon_point_item);
                    measurement.outer_tolerance_etalon_point_item = ui->graphicsView->scene()->addEllipse(new_point.x() - MEASUREMENT_POINT_CONTOUR_RADIUS, new_point.y() - MEASUREMENT_POINT_CONTOUR_RADIUS, MEASUREMENT_POINT_CONTOUR_RADIUS*2, MEASUREMENT_POINT_CONTOUR_RADIUS*2, QPen(Qt::green, 1), QBrush(Qt::green, Qt::Dense5Pattern));

                    break;
                }
            }
        }
    }
}

void ContoursForm::mouseReleaseEvent(const QPoint &p)
{
    Q_UNUSED(p)

    if(calibration_mode == CalibrationMode_e::MANUAL_SETTING_REFERENCE_POINTS)
    {
        for(Measurement &measurement : measurements)
        {
            if(measurement.active_point != Q_NULLPTR)
            {
                if(measurement.active_point == &measurement.outer_tolerance_etalon_point)   /* Outer tolerance etalon detail contour point */
                {
                    ui->graphicsView->scene()->removeItem(measurement.outer_tolerance_etalon_point_item);
                    measurement.outer_tolerance_etalon_point_item = ui->graphicsView->scene()->addEllipse(measurement.active_point->x() - MEASUREMENT_POINT_CONTOUR_RADIUS, measurement.active_point->y() - MEASUREMENT_POINT_CONTOUR_RADIUS, MEASUREMENT_POINT_CONTOUR_RADIUS*2, MEASUREMENT_POINT_CONTOUR_RADIUS*2, QPen(Qt::green, 1), QBrush(Qt::green, Qt::Dense5Pattern));
                }
                else                                                                        /* Current detail contour point */
                {
                    ui->graphicsView->scene()->removeItem(measurement.current_detail_point_item);
                    measurement.current_detail_point_item = ui->graphicsView->scene()->addEllipse(measurement.active_point->x() - MEASUREMENT_POINT_CONTOUR_RADIUS, measurement.active_point->y() - MEASUREMENT_POINT_CONTOUR_RADIUS, MEASUREMENT_POINT_CONTOUR_RADIUS*2, MEASUREMENT_POINT_CONTOUR_RADIUS*2, QPen(Qt::red, 1), QBrush(Qt::red, Qt::Dense5Pattern));
                }

                measurement.active_point = Q_NULLPTR;
            }
        }
    }
}

void ContoursForm::mouseMoveEvent(QMouseEvent *event)
{
    if(calibration_mode == CalibrationMode_e::MANUAL_SETTING_REFERENCE_POINTS)
    {
        QPoint new_point;
        QPoint nearest_contour_point;
        QPoint max_distance_from_measurement_point;

        for(Measurement &measurement : measurements)
        {
            if(measurement.active_point != Q_NULLPTR)
            {
                max_distance_from_measurement_point.setX(qRound(measurement.measurement_line.x1() + measurement.measurement_line.dx() / 4.0));
                max_distance_from_measurement_point.setY((max_distance_from_measurement_point.x() - measurement.measurement_line.x1()) * (static_cast<qreal>(measurement.measurement_line.dy()) / (measurement.measurement_line.dx())) + measurement.measurement_line.y1());

                if(event->x() >= (max_distance_from_measurement_point.x() > measurement.measurement_line.x1() ? max_distance_from_measurement_point.x() : measurement.measurement_line.x1()))
                    new_point = max_distance_from_measurement_point.x() > measurement.measurement_line.x1() ? max_distance_from_measurement_point : measurement.measurement_line.p1();
                else if(event->x() <= (measurement.measurement_line.x1() < max_distance_from_measurement_point.x() ? measurement.measurement_line.x1() : max_distance_from_measurement_point.x()))
                    new_point = measurement.measurement_line.x1() < max_distance_from_measurement_point.x() ? measurement.measurement_line.p1() : max_distance_from_measurement_point;
                else
                    new_point = QPoint(event->x(), (event->x() - measurement.measurement_line.x1()) * (static_cast<qreal>(measurement.measurement_line.dy()) / (measurement.measurement_line.dx())) + measurement.measurement_line.y1());

                // Проверка на то, что новая точка не выходит за границы области анализа изображения
                if(new_point.x() < qRound((ui->graphicsView->scene()->width() - ui->graphicsView->scene()->width() / 4.0) / 2.0))
                {
                    new_point.setX(qRound((ui->graphicsView->scene()->width() - ui->graphicsView->scene()->width() / 4.0) / 2.0));
                    new_point.setY((new_point.x() - measurement.measurement_line.x1()) * (static_cast<qreal>(measurement.measurement_line.dy()) / (measurement.measurement_line.dx())) + measurement.measurement_line.y1());
                }
                else if(new_point.x() > qRound((ui->graphicsView->scene()->width() - ui->graphicsView->scene()->width() / 4.0) / 2.0 + ui->graphicsView->scene()->width() / 4.0))
                {
                    new_point.setX(qRound((ui->graphicsView->scene()->width() - ui->graphicsView->scene()->width() / 4.0) / 2.0 + ui->graphicsView->scene()->width() / 4.0));
                    new_point.setY((new_point.x() - measurement.measurement_line.x1()) * (static_cast<qreal>(measurement.measurement_line.dy()) / (measurement.measurement_line.dx())) + measurement.measurement_line.y1());
                }

                if(new_point.y() < qRound((ui->graphicsView->scene()->height() - ui->graphicsView->scene()->height() / 4.0) / 2.0))
                {
                    new_point.setY(qRound((ui->graphicsView->scene()->height() - ui->graphicsView->scene()->height() / 4.0) / 2.0));
                    new_point.setX((new_point.y() - measurement.measurement_line.y1()) * (static_cast<qreal>(measurement.measurement_line.dx()) / (measurement.measurement_line.dy())) + measurement.measurement_line.x1());
                }
                else if(new_point.y() > qRound((ui->graphicsView->scene()->height() - ui->graphicsView->scene()->height() / 4.0) / 2.0 + ui->graphicsView->scene()->height() / 4.0))
                {
                    new_point.setY(qRound((ui->graphicsView->scene()->height() - ui->graphicsView->scene()->height() / 4.0) / 2.0 + ui->graphicsView->scene()->height() / 4.0));
                    new_point.setX((new_point.y() - measurement.measurement_line.y1()) * (static_cast<qreal>(measurement.measurement_line.dx()) / (measurement.measurement_line.dy())) + measurement.measurement_line.x1());
                }
                ////////////////////////////////////////////////////////////////////////////////////

                if(measurement.active_point == &measurement.outer_tolerance_etalon_point)   /* Outer tolerance etalon detail contour point */
                {
                    nearest_contour_point = PuansonChecker::getInstance()->findNearestContourPoint(ContourPoints_e::OUTER_TOLERANCE_ETALON_CONTOUR_POINT, new_point, measurement.measurement_line, 10);
                    if(!nearest_contour_point.isNull())
                        new_point = nearest_contour_point;

                    ui->graphicsView->scene()->removeItem(measurement.outer_tolerance_etalon_point_item);
                    measurement.outer_tolerance_etalon_point_item = ui->graphicsView->scene()->addEllipse(new_point.x() - MEASUREMENT_POINT_CONTOUR_RADIUS, new_point.y() - MEASUREMENT_POINT_CONTOUR_RADIUS, MEASUREMENT_POINT_CONTOUR_RADIUS*2, MEASUREMENT_POINT_CONTOUR_RADIUS*2, QPen(Qt::green, 1), QBrush(Qt::green, Qt::Dense5Pattern));
                }
                else                                                                        /* Current detail contour point */
                {
                    nearest_contour_point = PuansonChecker::getInstance()->findNearestContourPoint(ContourPoints_e::CURRENT_DEATIL_CONTOUR_POINT, new_point, measurement.measurement_line, 10);
                    if(!nearest_contour_point.isNull())
                        new_point = nearest_contour_point;

                    ui->graphicsView->scene()->removeItem(measurement.current_detail_point_item);
                    measurement.current_detail_point_item = ui->graphicsView->scene()->addEllipse(new_point.x() - MEASUREMENT_POINT_CONTOUR_RADIUS, new_point.y() - MEASUREMENT_POINT_CONTOUR_RADIUS, MEASUREMENT_POINT_CONTOUR_RADIUS*2, MEASUREMENT_POINT_CONTOUR_RADIUS*2, QPen(Qt::red, 1), QBrush(Qt::red, Qt::Dense5Pattern));
                }

                if(!nearest_contour_point.isNull())
                    *measurement.active_point = nearest_contour_point;

                break;
            }
        }
    }
}

void ContoursForm::mouseDoubleClickEvent(const QPoint &p)
{
    if(calibration_mode == CalibrationMode_e::MANUAL_SETTING_REFERENCE_POINTS)
    {
        for(Measurement &measurement : measurements)
        {
            if(!measurement.current_detail_position_is_set)
            {

                if(qSqrt((measurement.current_detail_point - p).x() * (measurement.current_detail_point - p).x() + (measurement.current_detail_point - p).y() * (measurement.current_detail_point - p).y()) <= MEASUREMENT_POINT_CONTOUR_RADIUS)
                {
                    ui->graphicsView->scene()->removeItem(measurement.current_detail_point_item);
                    measurement.current_detail_point_item = ui->graphicsView->scene()->addEllipse(measurement.current_detail_point.x() - MEASUREMENT_POINT_CONTOUR_RADIUS, measurement.current_detail_point.y() - MEASUREMENT_POINT_CONTOUR_RADIUS, MEASUREMENT_POINT_CONTOUR_RADIUS*2, MEASUREMENT_POINT_CONTOUR_RADIUS*2, QPen(Qt::darkRed, 1), QBrush(Qt::darkRed, Qt::Dense5Pattern));
                    measurement.current_detail_position_is_set = true;

                    break;
                }
            }

            if(!measurement.outer_tolerance_etalon_detail_position_is_set)
            {
                if(qSqrt((measurement.outer_tolerance_etalon_point - p).x() * (measurement.outer_tolerance_etalon_point - p).x() + (measurement.outer_tolerance_etalon_point - p).y() * (measurement.outer_tolerance_etalon_point - p).y()) <= MEASUREMENT_POINT_CONTOUR_RADIUS)
                {
                    ui->graphicsView->scene()->removeItem(measurement.outer_tolerance_etalon_point_item);
                    measurement.outer_tolerance_etalon_point_item = ui->graphicsView->scene()->addEllipse(measurement.outer_tolerance_etalon_point.x() - MEASUREMENT_POINT_CONTOUR_RADIUS, measurement.outer_tolerance_etalon_point.y() - MEASUREMENT_POINT_CONTOUR_RADIUS, MEASUREMENT_POINT_CONTOUR_RADIUS*2, MEASUREMENT_POINT_CONTOUR_RADIUS*2, QPen(Qt::darkGreen, 1), QBrush(Qt::darkGreen, Qt::Dense5Pattern));
                    measurement.outer_tolerance_etalon_detail_position_is_set = true;

                    break;
                }
            }
        }

        if(std::find_if(measurements.begin(), measurements.end(), [](const Measurement& item){return !(item.current_detail_position_is_set && item.outer_tolerance_etalon_detail_position_is_set);}) == std::end(measurements))
        {
            qint16 current_detail_point_deviation_px = 0;
            QPointF etalon_detail_point;

            for(const Measurement &measurement : measurements)
            {
                etalon_detail_point = QPointF(measurement.measurement_line.dx(), measurement.measurement_line.dy()) * (PuansonChecker::getInstance()->getEtalon().getToleranceExtFieldPx() / QLineF(measurement.measurement_line).length()) + measurement.outer_tolerance_etalon_point;
                current_detail_point_deviation_px = qRound(QLineF(etalon_detail_point, measurement.current_detail_point).length());

                // Если расстояние от точки контура текущей детали до противоположного конца линии меньше,чем такое же расстояние от точки эталона, то отклонение отрицательное
                if(QLineF(measurement.measurement_line.p2(), measurement.current_detail_point).length() < QLineF(measurement.measurement_line.p2(), etalon_detail_point).length())
                    current_detail_point_deviation_px = -current_detail_point_deviation_px;

                //qDebug() << "etalon_detail_point: " << etalon_detail_point << "outer_tolerance_etalon_point: " << measurement.outer_tolerance_etalon_point << " current_detail_point: " << measurement.current_detail_point;
                //qDebug() << "current_detail_point_deviation_px " << current_detail_point_deviation_px << "\n\n";

                PuansonChecker::getInstance()->getCurrent().setDeviation(measurement.measurement_line.p1(), current_detail_point_deviation_px);
            }

            calibration_mode = CalibrationMode_e::NO_CALIBRATION;

            PuansonChecker::getInstance()->currentDetailResearchGotoNextAngle();
        }
    }
}

void ContoursForm::setImageCursor(const QCursor &cursor)
{
    ui->graphicsView->viewport()->setCursor(cursor);
}

void ContoursForm::drawIdealContour(const QPainterPath &ideal_path, const QPainterPath &ideal_measurements_path)
{
    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
    ui->graphicsView->scene()->addPath(ideal_path, QPen(Qt::yellow, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    ui->graphicsView->scene()->addPath(ideal_measurements_path, QPen(Qt::yellow, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

void ContoursForm::drawBadPoints(const QVector<QPoint> &bad_points)
{
    const int d = GRAPHICAL_POINT_RADIUS*2;
    /*QGraphicsTextItem *text_item;
    int num = 1;*/

    for(const QPoint &bad_point: bad_points)
    {
        /*text_item = ui->graphicsView->scene()->addText(QString::number(num++));
        text_item->setDefaultTextColor(Qt::red);
        text_item->setPos(bad_point.x() - d / 2 - 5, bad_point.y() + d / 2 + 3);*/

        ui->graphicsView->scene()->addEllipse(bad_point.x() - d / 2, bad_point.y() - d / 2, d, d, QPen(Qt::black, 1), QBrush(Qt::yellow, Qt::Dense5Pattern));
    }
}

void ContoursForm::drawActualBorders()
{
    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

    ui->graphicsView->scene()->addRect( qRound((ui->graphicsView->scene()->width() - ui->graphicsView->scene()->width() / 4.0) / 2.0),
                                        qRound((ui->graphicsView->scene()->height() - ui->graphicsView->scene()->height() / 4.0) / 2.0),
                                        qRound(ui->graphicsView->scene()->width() / 4.0),
                                        qRound(ui->graphicsView->scene()->height() / 4.0), QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin) );
}

void ContoursForm::drawMeasurementLineAndPoints(const QLine &measurement_line, const QPoint &outer_point, const QPoint &current_detail_point)
{
    Measurement measurement(measurement_line, outer_point, current_detail_point);

    if(measurement.line_item)
        ui->graphicsView->scene()->removeItem(measurement.line_item);

    if(measurement.outer_tolerance_etalon_point_item)
        ui->graphicsView->scene()->removeItem(measurement.outer_tolerance_etalon_point_item);

    if(measurement.current_detail_point_item)
        ui->graphicsView->scene()->removeItem(measurement.current_detail_point_item);

    measurement.line_item = ui->graphicsView->scene()->addLine(measurement_line, QPen(Qt::yellow, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    measurement.outer_tolerance_etalon_point_item = ui->graphicsView->scene()->addEllipse(outer_point.x() - MEASUREMENT_POINT_CONTOUR_RADIUS, outer_point.y() - MEASUREMENT_POINT_CONTOUR_RADIUS, MEASUREMENT_POINT_CONTOUR_RADIUS*2, MEASUREMENT_POINT_CONTOUR_RADIUS*2, QPen(Qt::green, 1), QBrush(Qt::green, Qt::Dense5Pattern));
    measurement.current_detail_point_item = ui->graphicsView->scene()->addEllipse(current_detail_point.x() - MEASUREMENT_POINT_CONTOUR_RADIUS, current_detail_point.y() - MEASUREMENT_POINT_CONTOUR_RADIUS, MEASUREMENT_POINT_CONTOUR_RADIUS*2, MEASUREMENT_POINT_CONTOUR_RADIUS*2, QPen(Qt::red, 1), QBrush(Qt::red, Qt::Dense5Pattern));

    measurements.append(measurement);
}

void ContoursForm::drawImage(const QImage &img)
{
    if(!scene->items().isEmpty())
        scene->clear();

    QPixmap pixmap;
    pixmap = QPixmap::fromImage(img);

    if(pixmap.isNull())
    {
        QString memory_string("pixmap is NULL\n\n");

#if defined(Q_OS_WIN)
        MEMORYSTATUSEX memory_status;
        ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
        memory_status.dwLength = sizeof(MEMORYSTATUSEX);

        if (GlobalMemoryStatusEx(&memory_status))
        {
            memory_string += "total physical memory: " + QString::number(memory_status.ullTotalPhys / (1024 * 1024)) + " MB\n";
            memory_string += "total virtual memory: " + QString::number(memory_status.ullTotalVirtual / (1024 * 1024)) + " MB\n";

            memory_string += "availible physical memory: " + QString::number(memory_status.ullAvailPhys / (1024 * 1024)) + " MB\n";
            memory_string += "availible virtual memory: " + QString::number(memory_status.ullAvailVirtual / (1024 * 1024)) + " MB\n";
        }
        else
        {
            memory_string += "Unknown RAM\n";
        }
#endif // Q_OS_WIN

        QMessageBox msgBox;
        msgBox.setText(memory_string);
        msgBox.exec();
    }

    scene->addPixmap(pixmap);
    scene->setSceneRect(0, 0, img.width(), img.height());
    ui->graphicsView->setScene(scene);
}

void ContoursForm::etalonContourCheckBoxStateChanged(int state)
{
    PuansonChecker::getInstance()->setDrawEtalonContourFlag(state == Qt::Checked);
    PuansonChecker::getInstance()->updateContoursImage();
}

void ContoursForm::cannyThres1SpinBoxValueChanged()
{
    PuansonChecker::getInstance()->setCannyThreshold1(ui->cannyThres1SpinBox->value());
    PuansonChecker::getInstance()->updateContoursImage();
}

void ContoursForm::cannyThres2SpinBoxValueChanged()
{
    PuansonChecker::getInstance()->setCannyThreshold2(ui->cannyThres2SpinBox->value());
    PuansonChecker::getInstance()->updateContoursImage();
}

bool ContoursForm::combineImagesByReferencePointsButtonPressed()
{
    if(!PuansonChecker::getInstance()->isEtalonImageLoaded())
    {
        QMessageBox msgBox;
        msgBox.setText("Изображение эталонной детали не загружено!");
        msgBox.exec();

        return false;
    }

    if(!PuansonChecker::getInstance()->isCurrentImageLoaded())
    {
        QMessageBox msgBox;
        msgBox.setText("Изображение текущей детали не загружено!");
        msgBox.exec();

        return false;
    }

    PuansonChecker::getInstance()->combineImagesByReferencePoints();
    //PuansonChecker::getInstance()->cropImages();
    //PuansonChecker::getInstance()->drawEtalonImage();
    PuansonChecker::getInstance()->drawCurrentImage(true);
    PuansonChecker::getInstance()->drawContoursImage();

    QVector<QPoint> bad_points;
    bool is_detail_valid = PuansonChecker::getInstance()->checkDetail(bad_points);

    PuansonChecker::getInstance()->activateContoursWindow();

    QGraphicsPixmapItem *pixmap_item = ui->graphicsView->scene() ? qgraphicsitem_cast<QGraphicsPixmapItem *>(ui->graphicsView->scene()->items().last()) : Q_NULLPTR;

    image_x = pixmap_item->pixmap().width() / 2 - ui->graphicsView->width() / 2;
    image_y = pixmap_item->pixmap().height() / 2 - ui->graphicsView->height() / 2;

    ui->graphicsView->centerOn(image_x + ui->graphicsView->width() / 2 - 3 , image_y + ui->graphicsView->height() / 2 - 3);

    is_detail_valid = PuansonChecker::getInstance()->showConfirmCheckResultCorrectnessDialog(is_detail_valid) ? is_detail_valid : !is_detail_valid;

    return is_detail_valid;
}

ContoursForm::ContoursForm(PuansonChecker *checker) :
    QWidget(Q_NULLPTR),
    ImageWindow(),
    ui(new Ui::ContoursForm)
{
    ui->setupUi(this);

    ui->etalonContourCheckBox->setCheckState(checker->getDrawEtalonContourFlag() ? Qt::Checked : Qt::Unchecked);

    ui->cannyThres1SpinBox->setValue(checker->getCannyThreshold1());
    ui->cannyThres2SpinBox->setValue(checker->getCannyThreshold2());

    connect(ui->etalonContourCheckBox, SIGNAL(stateChanged(int)), SLOT(etalonContourCheckBoxStateChanged(int)));

    connect(ui->cannyThres1SpinBox, SIGNAL(editingFinished()), SLOT(cannyThres1SpinBoxValueChanged()));
    connect(ui->cannyThres2SpinBox, SIGNAL(editingFinished()), SLOT(cannyThres2SpinBoxValueChanged()));

    connect(ui->combineImagesByReferencePointsButton, SIGNAL(pressed()), SLOT(combineImagesByReferencePointsButtonPressed()));
}

ContoursForm::~ContoursForm()
{
    scene->clear();

    delete scene;
    delete ui;
}
