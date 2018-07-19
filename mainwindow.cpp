#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "puansonchecker.h"

#include <QtGlobal>
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QMessageBox>
#include <QKeyEvent>

void MainWindow::loadImageFinished(const ImageType_e image_type, const quint8 etalon_angle)
{
    if(image_type == ETALON_IMAGE && etalon_angle >= 1 && etalon_angle <= NUMBER_OF_ANGLES)
    {
        if(!ui->shoot_and_load_etalon_action->isEnabled())
            ui->shoot_and_load_etalon_action->setEnabled(true);

        angle_actions[etalon_angle-1]->setEnabled(true);

        if( scene->items().isEmpty() || etalon_angle == this->etalon_angle )
        {
            QImage img;

            if(scene->items().isEmpty())
            {
                this->etalon_angle = etalon_angle;
                PuansonChecker::getInstance()->setEtalonAngle(etalon_angle);

                QString text = QString("Изображение эталонной детали, ракурс ") + QString::number(PuansonChecker::getInstance()->getEtalonAngle());
                ui->label->setText(text);

                for(quint8 angle = 1; angle <= NUMBER_OF_ANGLES; angle++)
                {
                    if(angle == etalon_angle)
                        angle_actions[angle-1]->setChecked(true);
                    else
                        angle_actions[angle-1]->setChecked(false);
                }
            }

            PuansonChecker::getInstance()->getEtalonImage(etalon_angle, img);
            //PuansonChecker::getInstance()->getEtalonContour(img);

            drawImage(img);

            if(PuansonChecker::getInstance()->isCurrentImageLoaded())
                PuansonChecker::getInstance()->drawContoursImage();

            ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon(etalon_angle).getFilename());
        }
    }
    else if(image_type == CURRENT_IMAGE)
    {
        if(!ui->shoot_and_load_current_action->isEnabled())
            ui->shoot_and_load_current_action->setEnabled(true);

        PuansonChecker::getInstance()->drawCurrentImage();
        //PuansonChecker::getInstance()->drawCurrentContour();

        if(PuansonChecker::getInstance()->isEtalonImageLoaded(PuansonChecker::getInstance()->getEtalonAngle()))
            PuansonChecker::getInstance()->drawContoursImage();
    }

    ui->statusBar->showMessage("");
}

void MainWindow::menuLoadEtalon1ActionTriggered()
{
    QString file_to_open = QFileDialog::getOpenFileName(0, "Открыть файл эталона ракурс 1", "", "*.nef *.NEF");

    if(file_to_open.isEmpty())
        return;

    ui->statusBar->showMessage("Загрузка эталонного изображения, ракурс 1 ...");

    PuansonChecker::getInstance()->loadEtalonImage(1, file_to_open);
}

void MainWindow::menuLoadEtalon2ActionTriggered()
{
    QString file_to_open = QFileDialog::getOpenFileName(0, "Открыть файл эталона ракурс 2", "", "*.nef *.NEF");

    ui->statusBar->showMessage("Загрузка эталонного изображения, ракурс 2 ...");

    if(file_to_open.isEmpty())
        return;

    PuansonChecker::getInstance()->loadEtalonImage(2, file_to_open);
}

void MainWindow::menuLoadEtalon3ActionTriggered()
{
    QString file_to_open = QFileDialog::getOpenFileName(0, "Открыть файл эталона ракурс 3", "", "*.nef *.NEF");

    ui->statusBar->showMessage("Загрузка эталонного изображения, ракурс 3 ...");

    if(file_to_open.isEmpty())
        return;

    PuansonChecker::getInstance()->loadEtalonImage(3, file_to_open);
}

void MainWindow::menuLoadEtalon4ActionTriggered()
{
    QString file_to_open = QFileDialog::getOpenFileName(0, "Открыть файл эталона ракурс 4", "", "*.nef *.NEF");

    ui->statusBar->showMessage("Загрузка эталонного изображения, ракурс 4 ...");

    if(file_to_open.isEmpty())
        return;

    PuansonChecker::getInstance()->loadEtalonImage(4, file_to_open);
}

void MainWindow::menuLoadEtalon5ActionTriggered()
{
    QString file_to_open = QFileDialog::getOpenFileName(0, "Открыть файл эталона ракурс 5", "", "*.nef *.NEF");

    ui->statusBar->showMessage("Загрузка эталонного изображения, ракурс 5 ...");

    if(file_to_open.isEmpty())
        return;

    PuansonChecker::getInstance()->loadEtalonImage(5, file_to_open);
}

void MainWindow::menuShootAndLoadEtalonActionTriggered()
{
    ui->shoot_and_load_etalon_action->setEnabled(false);
    PuansonChecker::getInstance()->shootAndLoadEtalonImage();
}

void MainWindow::menuShootAndLoadCurrentActionTriggered()
{
    if(!PuansonChecker::getInstance()->getEtalon(etalon_angle).isReferencePointsAreSet())
    {
        QMessageBox msgBox;
        msgBox.setText("Для получения изображения текущей детали необходимо сначала задать реперные точки!");
        msgBox.exec();

        return;
    }

    ui->shoot_and_load_current_action->setEnabled(false);
    PuansonChecker::getInstance()->shootAndLoadCurrentImage();
}

void MainWindow::menuLoadCurrentActionTriggered()
{
    if(!PuansonChecker::getInstance()->getEtalon(etalon_angle).isReferencePointsAreSet())
    {
        QMessageBox msgBox;
        msgBox.setText("Для загрузки изображения текущей детали необходимо сначала задать реперные точки!");
        msgBox.exec();

        return;
    }

    QString file_to_open = QFileDialog::getOpenFileName(0, "Открыть файл изображения текущей детали", "", "*.nef *.NEF");

    if(file_to_open.isEmpty())
        return;

    ui->statusBar->showMessage("Загрузка изображения текущей детали ...");
    PuansonChecker::getInstance()->loadCurrentImage(file_to_open);
}

void MainWindow::menuAngle1ActionTriggered()
{
    QImage img;

    if(PuansonChecker::getInstance()->getEtalonImage(1, img))
    {
        this->etalon_angle = 1;

        PuansonChecker::getInstance()->setEtalonAngle(1);
        drawImage(img);

        if(PuansonChecker::getInstance()->isCurrentImageLoaded())
            PuansonChecker::getInstance()->drawContoursImage();

        for(quint8 angle = 1; angle <= NUMBER_OF_ANGLES; angle++)
        {
            if(angle == 1)
                this->angle_actions[angle-1]->setChecked(true);
            else
                this->angle_actions[angle-1]->setChecked(false);
        }

        ui->label->setText("Изображение эталонной детали, ракурс 1");
        ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon(1).getFilename());
    }
    else
    {
        ui->angle_1_action->setChecked(false);

        QMessageBox msgBox;
        msgBox.setText("Изображение эталонной детали, ракурс 1 не загружено!");
        msgBox.exec();
    }
}

void MainWindow::menuAngle2ActionTriggered()
{
    QImage img;

    if(PuansonChecker::getInstance()->getEtalonImage(2, img))
    {
        this->etalon_angle = 2;

        PuansonChecker::getInstance()->setEtalonAngle(2);
        drawImage(img);

        if(PuansonChecker::getInstance()->isCurrentImageLoaded())
            PuansonChecker::getInstance()->drawContoursImage();

        for(quint8 angle = 1; angle <= NUMBER_OF_ANGLES; angle++)
        {
            if(angle == 2)
                this->angle_actions[angle-1]->setChecked(true);
            else
                this->angle_actions[angle-1]->setChecked(false);
        }

        ui->label->setText("Изображение эталонной детали, ракурс 2");
        ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon(2).getFilename());
    }
    else
    {
        ui->angle_2_action->setChecked(false);

        QMessageBox msgBox;
        msgBox.setText("Изображение эталонной детали, ракурс 2 не загружено!");
        msgBox.exec();
    }
}

void MainWindow::menuAngle3ActionTriggered()
{
    QImage img;

    if(PuansonChecker::getInstance()->getEtalonImage(3, img))
    {
        this->etalon_angle = 3;

        PuansonChecker::getInstance()->setEtalonAngle(3);
        drawImage(img);

        if(PuansonChecker::getInstance()->isCurrentImageLoaded())
            PuansonChecker::getInstance()->drawContoursImage();

        for(quint8 angle = 1; angle <= NUMBER_OF_ANGLES; angle++)
        {
            if(angle == 3)
                this->angle_actions[angle-1]->setChecked(true);
            else
                this->angle_actions[angle-1]->setChecked(false);
        }

        ui->label->setText("Изображение эталонной детали, ракурс 3");
        ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon(3).getFilename());
    }
    else
    {
        ui->angle_3_action->setChecked(false);

        QMessageBox msgBox;
        msgBox.setText("Изображение эталонной детали, ракурс 3 не загружено!");
        msgBox.exec();
    }
}

void MainWindow::menuAngle4ActionTriggered()
{
    QImage img;

    if(PuansonChecker::getInstance()->getEtalonImage(4, img))
    {
        this->etalon_angle = 4;

        PuansonChecker::getInstance()->setEtalonAngle(4);
        drawImage(img);

        if(PuansonChecker::getInstance()->isCurrentImageLoaded())
            PuansonChecker::getInstance()->drawContoursImage();

        for(quint8 angle = 1; angle <= NUMBER_OF_ANGLES; angle++)
        {
            if(angle == 4)
                this->angle_actions[angle-1]->setChecked(true);
            else
                this->angle_actions[angle-1]->setChecked(false);
        }

        ui->label->setText("Изображение эталонной детали, ракурс 4");
        ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon(4).getFilename());
    }
    else
    {
        ui->angle_4_action->setChecked(false);

        QMessageBox msgBox;
        msgBox.setText("Изображение эталонной детали, ракурс 4 не загружено!");
        msgBox.exec();
    }
}

void MainWindow::menuAngle5ActionTriggered()
{
    QImage img;

    if(PuansonChecker::getInstance()->getEtalonImage(5, img))
    {
        this->etalon_angle = 5;

        PuansonChecker::getInstance()->setEtalonAngle(5);
        drawImage(img);

        if(PuansonChecker::getInstance()->isCurrentImageLoaded())
            PuansonChecker::getInstance()->drawContoursImage();

        for(quint8 angle = 1; angle <= NUMBER_OF_ANGLES; angle++)
        {
            if(angle == 5)
                this->angle_actions[angle-1]->setChecked(true);
            else
                this->angle_actions[angle-1]->setChecked(false);
        }

        ui->label->setText("Изображение эталонной детали, ракурс 5");
        ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon(5).getFilename());
    }
    else
    {
        ui->angle_5_action->setChecked(false);

        QMessageBox msgBox;
        msgBox.setText("Изображение эталонной детали, ракурс 5 не загружено!");
        msgBox.exec();
    }
}

void MainWindow::drawImage(const QImage &img)
{
    if(!scene->items().isEmpty())
    {
        scene->clear();

        ideal_origin_point = QPoint(1000, 500);
        ideal_rotate_angle = 135.0;
        ideal_item = Q_NULLPTR;
    }

    scene->addPixmap(QPixmap::fromImage(img));
    scene->setSceneRect(0, 0, img.width(), img.height());

    ui->graphicsView->setScene(scene);
}

void MainWindow::setWindowStatus(const QString &status)
{
    ui->statusBar->showMessage(status);
}

void MainWindow::drawActualBorders()
{
    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

    ui->graphicsView->scene()->addRect( qRound((ui->graphicsView->scene()->width() - ui->graphicsView->scene()->width() / 4.0) / 2.0),
                                        qRound((ui->graphicsView->scene()->height() - ui->graphicsView->scene()->height() / 4.0) / 2.0),
                                        qRound(ui->graphicsView->scene()->width() / 4.0) ,
                                        qRound(ui->graphicsView->scene()->height() / 4.0), QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin) );

    QVector<IdealInnerSegment> inner_segments = PuansonChecker::getInstance()->getEtalon(etalon_angle).getActualIdealInnerSegments();

    for(IdealInnerSegment segment: inner_segments)
    {
        ui->graphicsView->scene()->addEllipse(segment.getStartPoint().x() - GRAPHICAL_POINT_RADIUS, segment.getStartPoint().y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::red, Qt::Dense5Pattern));
        ui->graphicsView->scene()->addEllipse(segment.getEndPoint().x() - GRAPHICAL_POINT_RADIUS, segment.getEndPoint().y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::black, Qt::Dense5Pattern));

        QPainterPath inner_path = segment.InnerPath();
        ui->graphicsView->scene()->addPath(inner_path, QPen(Qt::darkRed, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        {
            QPainterPath control_path;
            QVector<QLine> control_lines = segment.getControlLines(30);

            for(const QLine &control_line : control_lines)
            {
                control_path.moveTo(control_line.p1());
                control_path.lineTo(control_line.p2());
            }

            ui->graphicsView->scene()->addPath(control_path, QPen(Qt::darkGreen, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        }
    }
}

void MainWindow::menuGotoCurrentWindowTriggered()
{
    PuansonChecker::getInstance()->activateCurrentImageWindow();
}

void MainWindow::menuGotoContoursWindowTriggered()
{
    PuansonChecker::getInstance()->activateContoursWindow();
}

void MainWindow::menuSettingsActionTriggered()
{
    PuansonChecker::getInstance()->showSettingsDialog();
}

void MainWindow::menuSetEtalonReferencePointsActionTriggered()
{
    if(!PuansonChecker::getInstance()->isEtalonImageLoaded(PuansonChecker::getInstance()->getEtalonAngle()))
    {
        QMessageBox msgBox;
        msgBox.setText("Изображение эталонной детали не загружено!");
        msgBox.exec();

        return;
    }

    setCalibrationMode(REFERENCE_POINT_1);
}

void MainWindow::menuSetCurrentReferencePointsActionTriggered()
{
    if(!PuansonChecker::getInstance()->isCurrentImageLoaded())
    {
        QMessageBox msgBox;
        msgBox.setText("Изображение текущей детали не загружено!");
        msgBox.exec();

        return;
    }

    PuansonChecker::getInstance()->startCurrentCalibration();
    PuansonChecker::getInstance()->activateCurrentImageWindow();
}

void MainWindow::drawIdealContour()
{
    QPainterPath ideal_path = PuansonChecker::getInstance()->getEtalon(etalon_angle).drawIdealContour(QRect(2, 2, ui->graphicsView->scene()->width() - 2, ui->graphicsView->scene()->height() - 2), QPointF(ideal_origin_point), ideal_rotate_angle);
    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

    if(ui->graphicsView->scene() != Q_NULLPTR && ideal_item && ideal_item->scene() == ui->graphicsView->scene())
        ui->graphicsView->scene()->removeItem(ideal_item);

    ideal_item = ui->graphicsView->scene()->addPath(ideal_path, QPen(Qt::red, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    //drawActualBorders();
}

void MainWindow::menuImposeIdealContourToEtalonActionTriggered()
{
    if(PuansonChecker::getInstance()->getEtalon(PuansonChecker::getInstance()->getEtalonAngle()).isReferencePointsAreSet())
    {
        ideal_origin_point = QPoint(1000, 500);
        ideal_rotate_angle = 135.0;

        removeReferencePointsAndIdealContour();
        drawReferencePoints();

        setImageCursor(Qt::OpenHandCursor);
        drawIdealContour();

        setCalibrationMode(IDEAL_IMPOSE);
    }
    else
    {
        QMessageBox::warning(this, "Внимание", "Сначала необходимо задать реперные точки");
    }
}

void MainWindow::menuReferencePointsAutoSearchActionTriggered()
{
    QVector<QPointF> reference_points = PuansonChecker::getInstance()->findReferencePoints();
    QString reference_point_caption;
    QGraphicsTextItem *text_item;
    quint8 reference_point_number = 1;

    for(const QPointF &reference_point: reference_points)
    {
        ui->graphicsView->scene()->addEllipse(reference_point.x() - GRAPHICAL_POINT_RADIUS, reference_point.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::green, Qt::Dense5Pattern));
        reference_point_caption = "Реперная точка " + QString::number(reference_point_number++);
        text_item = ui->graphicsView->scene()->addText(reference_point_caption, QFont("Arial", 10, QFont::Bold));
        text_item->setDefaultTextColor(Qt::green);
        text_item->setPos(reference_point.x() + GRAPHICAL_POINT_RADIUS + 3, reference_point.y() - GRAPHICAL_POINT_RADIUS - 3);
    }
}

// Удаление реперных точек и идеального контура
void MainWindow::removeReferencePointsAndIdealContour()
{
    while(ui->graphicsView->scene()->items().size() != 1)
    {
        ui->graphicsView->scene()->removeItem(ui->graphicsView->scene()->items().first());
        ui->graphicsView->scene()->update();
    }
    update();
}

// Рисование реперных точек, если они заданы
void MainWindow::drawReferencePoints()
{
    QGraphicsTextItem *text_item;

    PuansonChecker::getInstance()->getEtalon(etalon_angle).getReferencePoints(reference_point1, reference_point2);

    // Реперные точки
    if(!(reference_point1 == reference_point2))
    {
        // Точка 1
        ui->graphicsView->scene()->addEllipse(reference_point1.x() - GRAPHICAL_POINT_RADIUS, reference_point1.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::green, Qt::Dense5Pattern));
        text_item = ui->graphicsView->scene()->addText("Реперная точка 1", QFont("Arial", 10, QFont::Bold));
        text_item->setDefaultTextColor(Qt::green);
        text_item->setPos(reference_point1.x() + GRAPHICAL_POINT_RADIUS + 3, reference_point1.y() - GRAPHICAL_POINT_RADIUS - 3);

        // Точка 2
        ui->graphicsView->scene()->addEllipse(reference_point2.x() - GRAPHICAL_POINT_RADIUS, reference_point2.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::green, Qt::Dense5Pattern));
        text_item = ui->graphicsView->scene()->addText("Реперная точка 2", QFont("Arial", 10, QFont::Bold));
        text_item->setDefaultTextColor(Qt::green);
        text_item->setPos(reference_point2.x() - GRAPHICAL_POINT_RADIUS - 60, reference_point2.y() + GRAPHICAL_POINT_RADIUS + 3);
    }
}

// Рисование реперных точек и точек внутреннего остова, если они заданы
void MainWindow::drawReferencePointsAndIdealContour()
{
    drawReferencePoints();

    if(PuansonChecker::getInstance()->getEtalon(etalon_angle).isIdealContourSet())
        drawIdealContour();
}

void MainWindow::setCalibrationMode(CalibrationMode_e mode)
{
    calibration_mode = mode;

    switch(calibration_mode)
    {
        case REFERENCE_POINT_1:
            removeReferencePointsAndIdealContour();

            if(PuansonChecker::getInstance()->getEtalon(etalon_angle).isIdealContourSet())
                drawIdealContour();

            ui->label_2->setText("Калибровка. Укажите реперную точку 1.");
            setImageCursor(Qt::CrossCursor);
        break;
        case REFERENCE_POINT_2:
            ui->label_2->setText("Калибровка. Укажите реперную точку 2.");
        break;
        case IDEAL_IMPOSE:
            ui->label_2->setText("Совместите контур идеальной детали с границами эталона и установите его двойным щелчком левой кнопки мыши.");
            ideal_origin_point = QPoint(1000, 500);
            ideal_rotate_angle = 135.0;
            break;
        case NO_CALIBRATION:
        default:
            drawReferencePointsAndIdealContour();

            ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon(PuansonChecker::getInstance()->getEtalonAngle()).getFilename());
            setImageCursor(Qt::ArrowCursor);
        break;
    }
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)

    switch(calibration_mode)
    {
        case IDEAL_IMPOSE:
        {
            PuansonChecker::getInstance()->getEtalon(etalon_angle).setIdealContourSetFlag(true);
            setCalibrationMode(NO_CALIBRATION);
            PuansonChecker::getInstance()->updateContoursImage();
            setImageCursor(Qt::ArrowCursor);

            drawActualBorders();
        }
        break;
        default:
        break;
    }
}

void MainWindow::mousePressEvent(const QPoint &p)
{
    switch(calibration_mode)
    {
        case REFERENCE_POINT_1:
        {
            reference_point1 = p;

            ui->graphicsView->scene()->addEllipse(p.x() - GRAPHICAL_POINT_RADIUS, p.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::green, Qt::Dense5Pattern));
            QGraphicsTextItem *text_item = ui->graphicsView->scene()->addText("Реперная точка 1", QFont("Arial", 10, QFont::Bold));
            text_item->setDefaultTextColor(Qt::green);
            text_item->setPos(p.x() + GRAPHICAL_POINT_RADIUS + 3, p.y() - GRAPHICAL_POINT_RADIUS - 3);

            setCalibrationMode(REFERENCE_POINT_2);
        }
        break;
        case REFERENCE_POINT_2:
        {
            reference_point2 = p;

            PuansonChecker::getInstance()->getEtalon(etalon_angle).setReferencePoints(reference_point1, reference_point2);

            ui->graphicsView->scene()->addEllipse(p.x() - GRAPHICAL_POINT_RADIUS, p.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::green, Qt::Dense5Pattern));
            QGraphicsTextItem *text_item = ui->graphicsView->scene()->addText("Реперная точка 2", QFont("Arial", 10, QFont::Bold));
            text_item->setDefaultTextColor(Qt::green);
            text_item->setPos(p.x() - GRAPHICAL_POINT_RADIUS - 60, p.y() + GRAPHICAL_POINT_RADIUS + 3);

            qreal ratio = PuansonChecker::getInstance()->getEtalon(etalon_angle).getCalibrationRatio();
            QMessageBox::information(this, "Информационное сообщение", "Отношение px / мкм : " + QString::number(ratio));

            setCalibrationMode(NO_CALIBRATION);

            /*PuansonChecker::getInstance()->cropEtalonImage(etalon_angle);

            QImage img;
            PuansonChecker::getInstance()->getEtalonImage(etalon_angle, img);
            drawImage(img);*/

            PuansonChecker::getInstance()->updateContoursImage();
        }
        break;
        case IDEAL_IMPOSE:
            ideal_impose_mouse_pressed = true;
            ideal_impose_previous_point = p;
            setImageCursor(Qt::ClosedHandCursor);
        break;
        case NO_CALIBRATION:
        default:
        break;
    }
}

void MainWindow::mouseReleaseEvent(const QPoint &p)
{
    Q_UNUSED(p)

     switch(calibration_mode)
     {
        case IDEAL_IMPOSE:
            ideal_impose_mouse_pressed = false;
            setImageCursor(Qt::OpenHandCursor);
        break;
        default:
        break;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event)

    if(ideal_impose_mouse_pressed)
    {
        ideal_origin_point = ideal_origin_point + event->localPos().toPoint() - ideal_impose_previous_point;
        ideal_impose_previous_point = event->localPos().toPoint();
        drawIdealContour();
    }
}

bool MainWindow::wheelEvent(int delta)
{
    bool retval = false;

    switch(calibration_mode)
    {
       case IDEAL_IMPOSE:
            ideal_rotate_angle += delta / 120.0 / 10.0;

            drawIdealContour();
            retval = true;
       break;
       default:
       break;
    }

    return retval;
}

void MainWindow::menuSaveCurrentTriggered()
{
    QString current_image_path = QFileDialog::getSaveFileName(0, "Сохранить изображение текущей детали", "", "*.tiff *.TIFF *.tif *.TIF *.png *.PNG *.jpg *.JPG");

    if(!current_image_path.isEmpty())
        PuansonChecker::getInstance()->saveCurrentImage(current_image_path);
}

void MainWindow::menuSaveResultTriggered()
{
    QString result_image_path = QFileDialog::getSaveFileName(0, "Сохранить результирующее изображение контуров", "", "*.tiff *.TIFF *.tif *.TIF *.png *.PNG *.jpg *.JPG *.jpeg *.JPEG");

    if(!result_image_path.isEmpty())
        PuansonChecker::getInstance()->saveResultImage(result_image_path);
}

void MainWindow::menuExitActionTriggered()
{
    PuansonChecker::getInstance()->quit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

    PuansonChecker::getInstance()->quit();
}

void MainWindow::setImageCursor(const QCursor &cursor)
{
    ui->graphicsView->viewport()->setCursor(cursor);
}

void MainWindow::moveImage(const qreal dx, const qreal dy)
{
    QGraphicsScene *scene = ui->graphicsView->scene();
    QGraphicsPixmapItem *pixmap_item = (scene == Q_NULLPTR) ? Q_NULLPTR : qgraphicsitem_cast<QGraphicsPixmapItem *>(scene->items().at(scene->items().count()-1));

    if(pixmap_item)
    {
        qreal new_x, new_y;

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

bool MainWindow::windowKeyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        if(calibration_mode != NO_CALIBRATION)
            setCalibrationMode(NO_CALIBRATION);

        return true;
    }

    return false;
}

MainWindow::MainWindow() :
    QMainWindow(Q_NULLPTR),
    ImageWindow(),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    angle_actions[0] = ui->angle_1_action;
    angle_actions[1] = ui->angle_2_action;
    angle_actions[2] = ui->angle_3_action;
    angle_actions[3] = ui->angle_4_action;
    angle_actions[4] = ui->angle_5_action;

    etalon_angle = 1;

    ideal_origin_point = QPoint(1000, 500);
    ideal_rotate_angle = 135.0;
    ideal_item = Q_NULLPTR;

    connect(ui->angle_1_action, SIGNAL(triggered()), SLOT(menuAngle1ActionTriggered()));
    connect(ui->angle_2_action, SIGNAL(triggered()), SLOT(menuAngle2ActionTriggered()));
    connect(ui->angle_3_action, SIGNAL(triggered()), SLOT(menuAngle3ActionTriggered()));
    connect(ui->angle_4_action, SIGNAL(triggered()), SLOT(menuAngle4ActionTriggered()));
    connect(ui->angle_5_action, SIGNAL(triggered()), SLOT(menuAngle5ActionTriggered()));
    connect(ui->load_etalon_1_action, SIGNAL(triggered()), SLOT(menuLoadEtalon1ActionTriggered()));
    connect(ui->load_etalon_2_action, SIGNAL(triggered()), SLOT(menuLoadEtalon2ActionTriggered()));
    connect(ui->load_etalon_3_action, SIGNAL(triggered()), SLOT(menuLoadEtalon3ActionTriggered()));
    connect(ui->load_etalon_4_action, SIGNAL(triggered()), SLOT(menuLoadEtalon4ActionTriggered()));
    connect(ui->load_etalon_5_action, SIGNAL(triggered()), SLOT(menuLoadEtalon5ActionTriggered()));

    connect(ui->shoot_and_load_etalon_action, SIGNAL(triggered()), SLOT(menuShootAndLoadEtalonActionTriggered()));
    connect(ui->shoot_and_load_current_action, SIGNAL(triggered()), SLOT(menuShootAndLoadCurrentActionTriggered()));

    connect(ui->load_current_action, SIGNAL(triggered()), SLOT(menuLoadCurrentActionTriggered()));
    connect(ui->goto_current_window_action, SIGNAL(triggered()), SLOT(menuGotoCurrentWindowTriggered()));
    connect(ui->goto_counturs_window_action, SIGNAL(triggered()), SLOT(menuGotoContoursWindowTriggered()));
    connect(ui->settings_action, SIGNAL(triggered()), SLOT(menuSettingsActionTriggered()));
    connect(ui->set_etalon_reference_points_action, SIGNAL(triggered()), SLOT(menuSetEtalonReferencePointsActionTriggered()));
    connect(ui->set_current_reference_points_action, SIGNAL(triggered()), SLOT(menuSetCurrentReferencePointsActionTriggered()));
    connect(ui->impose_ideal_contour_to_etalon_action, SIGNAL(triggered()), SLOT(menuImposeIdealContourToEtalonActionTriggered()));
    connect(ui->reference_points_auto_search_action, SIGNAL(triggered()), SLOT(menuReferencePointsAutoSearchActionTriggered()));
    connect(ui->save_current_action, SIGNAL(triggered()), SLOT(menuSaveCurrentTriggered()));
    connect(ui->save_result_action, SIGNAL(triggered()), SLOT(menuSaveResultTriggered()));
    connect(ui->exit_action, SIGNAL(triggered()), SLOT(menuExitActionTriggered()));
}

MainWindow::~MainWindow()
{
    delete scene;
    delete ui;
}
