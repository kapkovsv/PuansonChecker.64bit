#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "puansonchecker.h"
#include "detailanglesourcedialog.h"
#include "etalonangleresultconfirmdialog.h"
#include "etalonresearchpropertiesdialog.h"
#include "currentangleresultconfirmdialog.h"
#include "currentresearchresultandprotocoldialog.h"

#include <QtGlobal>
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QMessageBox>
#include <QKeyEvent>

void MainWindow::loadImageFinished(const ImageType_e image_type)
{
    ui->statusBar->showMessage("");

    if(image_type == ImageType_e::ETALON_IMAGE)
    {
        if(!ui->image_work_etalon_shoot_and_load_action->isEnabled())
            ui->image_work_etalon_shoot_and_load_action->setEnabled(true);

        {
            //QImage img;

            QString text;
            if(PuansonChecker::getInstance()->isEtalonResearchLoaded())
            {
                text = QString("Исследование эталонной детали. Ракурс ") + QString::number(PuansonChecker::getInstance()->getActiveEtalonResearchAngle()) + QString(".");
            }
            else
            {
                ui->etalon_angle_menu->setEnabled(false);
                ui->etalon_research_properties_action->setEnabled(false);
                ui->image_work_etalon_save_angle_to_research->setEnabled(false);
                text += QString("Изображение эталонной детали");
            }
            ui->label->setText(text);

            reference_point1 = reference_point2 = QPoint();

            //PuansonChecker::getInstance()->getEtalonImage(img);
            //PuansonChecker::getInstance()->getEtalonContour(img);

            PuansonChecker::getInstance()->drawEtalonImage();
            //drawImage(img);

            if(PuansonChecker::getInstance()->isCurrentImageLoaded())
            {
                if(PuansonChecker::getInstance()->getActiveCurrentResearchAngle() == 0)
                    PuansonChecker::getInstance()->drawContoursImage();

                if(!ui->image_work_save_result_action->isEnabled())
                    ui->image_work_save_result_action->setEnabled(true);
            }

            const PuansonImage &etalon_image = PuansonChecker::getInstance()->getEtalon();
            ui->label_2->setText("Файл " + etalon_image.getFilename());
        }

        // Исследование ЭТАЛОНА
        if(PuansonChecker::getInstance()->getActiveEtalonResearchAngle() != 0/* работа с исследованием ведётся */
                && !PuansonChecker::getInstance()->isEtalonResearchCompleted())
        {
            //  2. Задание позиции областей поиска реперных точек(только для ракурса 1)
            if(PuansonChecker::getInstance()->getActiveEtalonResearchAngle() == 1)
            {
                menuImageWorkSetLeftBottomReferencePointSearchAreaActionTriggered();
            }
            else
            {
                // 2. Автоматический поиск реперных точек и наложение идеального контура(для ракурсов 2-6)
                menuImageWorkEtalonReferencePointsAutoSearchActionTriggered();
                menuImageWorkEtalonImposeIdealContourToEtalonActionTriggered();
            }
        }
        // ------------

        // Исследование ТЕКУЩЕЙ ДЕТАЛИ
        if(PuansonChecker::getInstance()->getActiveCurrentResearchAngle() != 0)
        {
            // 2. Выбор способа получения ракурса
            DetailAngleSourceDialogResult_e angle_source;

            if(PuansonChecker::getInstance()->isCurrentDetailAngleSourcePhotoShootingOnly())
            {
                angle_source = PHOTO_SHOOTING_DIALOG_RESULT;
            }
            else
            {
                // Диалоговое окно с вопросом о выборе источника получения изображения текущей детали
                DetailAngleSourceDialog etalon_angle_source_dialog(ImageType_e::CURRENT_IMAGE, PuansonChecker::getInstance()->getActiveCurrentResearchAngle());
                etalon_angle_source_dialog.setModal(false);
                etalon_angle_source_dialog.setFixedSize(etalon_angle_source_dialog.size());
                etalon_angle_source_dialog.exec();
                angle_source = static_cast<DetailAngleSourceDialogResult_e>(etalon_angle_source_dialog.result());
            }

            switch(angle_source)
            {
            case PHOTO_SHOOTING_DIALOG_RESULT:
                if(PuansonChecker::getInstance()->useMachineForDetailMovement())
                {
                    if(PuansonChecker::getInstance()->getMachine()->moveToAnglePosition(PuansonChecker::getInstance()->getActiveEtalonResearchAngle()))
                    {
                        menuImageWorkCurrentShootAndLoadActionTriggered();
                    }
                    else
                    {
                        qint32 reply;

                        QMessageBox question(this);
                        question.setWindowTitle("Внимание");
                        question.setText("Невозможно поместить деталь в нужную позицию с помощью станка. Отменить исследование или продолжить в ручном режиме?");
                        question.addButton("Отменить", QMessageBox::RejectRole);
                        question.addButton("Продолжить в ручном режиме", QMessageBox::ApplyRole);
                        reply = question.exec();

                        if(reply == QMessageBox::ApplyRole)
                        {
                            QMessageBox::information(this, "Информационное сообщение", "Поместите деталь в нужную позицию и нажмите \"OK\"");
                            menuImageWorkCurrentShootAndLoadActionTriggered();
                        }
                        else
                        {
                            PuansonChecker::getInstance()->cancelCurrentResearch();
                            QMessageBox::information(this, "Исследование завершено", "Результат исследования: \"Отменено пользователем\"");
                        }
                    }
                }

                break;
            case LOADING_FROM_FILE_DIALOG_RESULT:
                menuImageWorkCurrentLoadActionTriggered();
                break;
            case CLOSE_DIALOG_RESULT:
            default:
                // Исследование текущей детали завершено
                PuansonChecker::getInstance()->cancelCurrentResearch();
                QMessageBox::information(this, "Исследование завершено", "Результат исследования: \"Отменено пользователем\"");
                break;
            }
        }
        // ---------------------------
    }
    else if(image_type == ImageType_e::CURRENT_IMAGE)
    {
        if(!ui->image_work_current_shoot_and_load_action->isEnabled())
            ui->image_work_current_shoot_and_load_action->setEnabled(true);

        if(!ui->image_work_current_save_action->isEnabled())
            ui->image_work_current_save_action->setEnabled(true);

        if(PuansonChecker::getInstance()->isEtalonImageLoaded())
            PuansonChecker::getInstance()->getCurrent().setImageTransform(PuansonChecker::getInstance()->getEtalon().getImageTransform());

        PuansonChecker::getInstance()->drawCurrentImage();

        // Исследование ТЕКУЩЕЙ ДЕТАЛИ
        if(PuansonChecker::getInstance()->getActiveCurrentResearchAngle() != 0)
        {
            // 3. Автоматический поиск реперных точек
            menuImageWorkCurrentReferencePointsAutoSearchActionTriggered();

            // 4. Переход к окну контуров, автоматическое совмещение изображений и определение корректности детали
            //PuansonChecker::getInstance()->activateContoursWindow();
            bool current_detail_valid = PuansonChecker::getInstance()->combineImagesByReferencePointsAndDrawBadPoints();

            if(current_detail_valid)
            {
                // 5. Диалоговое окно подтверждения результатов исследования данного ракурса
                CurrentAngleResultConfirmDialog current_angle_result_confirm_dialog(PuansonChecker::getInstance()->getActiveCurrentResearchAngle());

                current_angle_result_confirm_dialog.setModal(false);
                current_angle_result_confirm_dialog.setFixedSize(current_angle_result_confirm_dialog.size());

                current_angle_result_confirm_dialog.exec();

                CurrentAngleResultConfirmDialogResult_e previous_dialog_result = static_cast<CurrentAngleResultConfirmDialogResult_e>(current_angle_result_confirm_dialog.result());

                if(previous_dialog_result == CANCEL_RESEARCH_CURRENT_DIALOG_RESULT)
                {
                    // Исследование текущей детали завершено
                    PuansonChecker::getInstance()->cancelCurrentResearch();
                    QMessageBox::information(this, "Исследование завершено", "Результат исследования: \"Отменено пользователем\"");
                }
                else if(previous_dialog_result == RESEARCH_THIS_ANGLE_AGAIN_CURRENT_DIALOG_RESULT)
                {
                    PuansonChecker::getInstance()->researchCurrentAngle(PuansonChecker::getInstance()->getActiveCurrentResearchAngle());
                }
                else if(previous_dialog_result == CURRENT_DETAIL_MEASUREMENTS_DIALOG_RESULT)
                {
                    // 6. Определение отклонений размеров текущей детали
                    QRect analysis_rect(qRound((ui->graphicsView->scene()->width() - ui->graphicsView->scene()->width() / 4.0) / 2.0),
                            qRound((ui->graphicsView->scene()->height() - ui->graphicsView->scene()->height() / 4.0) / 2.0),
                            qRound(ui->graphicsView->scene()->width() / 4.0),
                            qRound(ui->graphicsView->scene()->height() / 4.0));

                    if(PuansonChecker::getInstance()->findMeasurementContourPoints(analysis_rect))
                    {
                        PuansonChecker::getInstance()->measurementPointsSettingMode();
                        PuansonChecker::getInstance()->activateContoursWindow();
                    }
                    else
                    {
                        currentDetailResearchGotoNextAngle();
                    }
                }
                else if(previous_dialog_result == PASS_TO_NEXT_ANGLE_CURRENT_DIALOG_RESULT)
                {
                    currentDetailResearchGotoNextAngle();
                }
            }
            else
            {
                // Исследование текущей детали завершено
                quint8 active_angle = PuansonChecker::getInstance()->completeCurrentResearch(false);

                if(!PuansonChecker::getInstance()->getCurrentResearchFolderPath().isEmpty())
                {
                    QString angle_directory_path = PuansonChecker::getInstance()->getCurrentResearchFolderPath() + "/angle_" + QString::number(active_angle);
                    QDir().mkdir(angle_directory_path);
                    PuansonChecker::getInstance()->getCurrent().saveEtalonAngle(angle_directory_path);
                }

                QMessageBox::information(this, "Исследование завершено", "Результат исследования: \"Текущая деталь не соответсвует требованиям\"");
            }
            // ------------
        }
        else // Загрузка текущего изображения из файла
        {
            if(PuansonChecker::getInstance()->isEtalonImageLoaded())
            {
                PuansonChecker::getInstance()->drawCurrentImageReferencePoints();
                PuansonChecker::getInstance()->drawContoursImage();

                if(!ui->image_work_save_result_action->isEnabled())
                    ui->image_work_save_result_action->setEnabled(true);
            }
        }

       // ui->statusBar->showMessage("");
    }
}

void MainWindow::currentDetailResearchGotoNextAngle()
{
    if(!this->isActiveWindow())
        this->activateWindow();

    if(PuansonChecker::getInstance()->getActiveCurrentResearchAngle() != PUANSON_IMAGE_MAX_ANGLE)
    {
        if(!PuansonChecker::getInstance()->getCurrentResearchFolderPath().isEmpty())
        {
            QString angle_directory_path = PuansonChecker::getInstance()->getCurrentResearchFolderPath() + "/angle_" + QString::number(PuansonChecker::getInstance()->getActiveCurrentResearchAngle());
            QDir().mkdir(angle_directory_path);
            PuansonChecker::getInstance()->getCurrent().saveEtalonAngle(angle_directory_path);
        }

        // Переход к следующему ракурсу
        //PuansonChecker::getInstance()->incrementActiveEtalonResearchAngle();
        PuansonChecker::getInstance()->researchCurrentAngle(PuansonChecker::getInstance()->incrementActiveCurrentResearchAngle());
    }
    else
    {
        // Исследование текущей детали завершено
        if(!PuansonChecker::getInstance()->getCurrentResearchFolderPath().isEmpty())
        {
            QString angle_directory_path = PuansonChecker::getInstance()->getCurrentResearchFolderPath() + "/angle_" + QString::number(PuansonChecker::getInstance()->getActiveCurrentResearchAngle());
            QDir().mkdir(angle_directory_path);
            PuansonChecker::getInstance()->getCurrent().saveEtalonAngle(angle_directory_path);
        }

        PuansonChecker::getInstance()->completeCurrentResearch(true);

        EtalonDetailDimensions current_detail_dimensions;
        PuansonChecker::getInstance()->getCurrent().getDetailDimensions(current_detail_dimensions);

        EtalonDetailDimensions ideal_detail_dimensions;
        PuansonChecker::getInstance()->getCurrent().getIdealDimensions(ideal_detail_dimensions);

        // Вывод результата и протокола
        CurrentResearchResultAndProtocolDialog current_research_result_and_protocol_dialog(current_detail_dimensions, ideal_detail_dimensions);

        current_research_result_and_protocol_dialog.setModal(false);
        current_research_result_and_protocol_dialog.setFixedSize(current_research_result_and_protocol_dialog.size());

        current_research_result_and_protocol_dialog.exec();

        //QMessageBox::information(this, "Исследование завершено", "Результат исследования: \"Текущая деталь соответсвует требованиям\"");
    }
}

void MainWindow::drawImage(const QImage &img)
{
    if(!scene->items().isEmpty())
    {
        scene->clear();

        reference_point_1_auto_search_area_ideal_item = reference_point_2_auto_search_area_ideal_item = Q_NULLPTR;

        ideal_origin_point = QPoint(1000, 500);
        ideal_rotation_angle = 135.0;
        ideal_item = Q_NULLPTR;
    }

    scene->addPixmap(QPixmap::fromImage(img));
    scene->setSceneRect(0, 0, img.width(), img.height());

    ui->graphicsView->setScene(scene.data());
}

void MainWindow::setWindowStatus(const QString &status)
{
    ui->statusBar->showMessage(status);
}

void MainWindow::drawActualBorders()
{
    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

    QGraphicsTextItem *textItem = ui->graphicsView->scene()->addText("Область анализа корректности детали", QFont("Arial", 10/*, QFont::Bold*/));
    textItem->setDefaultTextColor(Qt::white);
    textItem->setPos(qRound((ui->graphicsView->scene()->width() - ui->graphicsView->scene()->width() / 4.0) / 2.0), qRound((ui->graphicsView->scene()->height() - ui->graphicsView->scene()->height() / 4.0) / 2.0) - 25);

    QRect analysis_rect(qRound((ui->graphicsView->scene()->width() - ui->graphicsView->scene()->width() / 4.0) / 2.0),
            qRound((ui->graphicsView->scene()->height() - ui->graphicsView->scene()->height() / 4.0) / 2.0),
            qRound(ui->graphicsView->scene()->width() / 4.0),
            qRound(ui->graphicsView->scene()->height() / 4.0));

    ui->graphicsView->scene()->addRect(analysis_rect , QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin) );

    QVector<IdealInnerSegment> inner_segments = PuansonChecker::getInstance()->getEtalon().getActualIdealInnerSegments();

    for(IdealInnerSegment segment: inner_segments)
    {
        ui->graphicsView->scene()->addEllipse(segment.getStartPoint().x() - GRAPHICAL_POINT_RADIUS, segment.getStartPoint().y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::red, Qt::Dense5Pattern));
        ui->graphicsView->scene()->addEllipse(segment.getEndPoint().x() - GRAPHICAL_POINT_RADIUS, segment.getEndPoint().y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::black, Qt::Dense5Pattern));

        QPainterPath inner_path = segment.InnerPath();
        ui->graphicsView->scene()->addPath(inner_path, QPen(Qt::darkRed, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        {
            QPainterPath control_path;
            QVector<QLine> control_lines = segment.getControlLines(30, analysis_rect);

            for(const QLine &control_line : control_lines)
            {
                control_path.moveTo(control_line.p1());
                control_path.lineTo(control_line.p2());
            }

            ui->graphicsView->scene()->addPath(control_path, QPen(Qt::darkGreen, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        }
    }
}

void MainWindow::menuEtalonResearchActionTriggered()
{
    PuansonChecker::getInstance()->researchEtalonAngle();
}

void MainWindow::menuEtalonLoadResearchActionTriggered()
{
    QString etalon_research_folder_path = QFileDialog::getExistingDirectory(Q_NULLPTR, "Выберете папку c исследованием эталона", "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if(etalon_research_folder_path.isEmpty())
        return;

    if(PuansonChecker::getInstance()->loadEtalonResearch(etalon_research_folder_path))
    {
        PuansonChecker::getInstance()->setActiveEtalonResearchAngle(1);
        PuansonChecker::getInstance()->getEtalon().loadEtalonAngle(etalon_research_folder_path + "/angle_1");

        angle_actions[0]->setChecked(true);
        ui->etalon_angle_menu->setEnabled(true);
        ui->etalon_research_properties_action->setEnabled(true);
        ui->image_work_etalon_save_angle_to_research->setEnabled(true);

        ui->statusBar->showMessage("Загрузка 1-го ракурса эталонного изображения ...");
    }
    else
    {
    }
}

void MainWindow::menuEtalonAngleSetActive1ActionTriggered()
{
    const quint8 new_active_angle = 1;

    if(new_active_angle != PuansonChecker::getInstance()->getActiveEtalonResearchAngle())
    {
        for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
            angle_actions[angle-1]->setChecked(false);

        PuansonChecker::getInstance()->setActiveEtalonResearchAngle(new_active_angle);
        PuansonChecker::getInstance()->getEtalon().loadEtalonAngle(PuansonChecker::getInstance()->getEtalonResearchFolderPath() + "/angle_1");

        ui->statusBar->showMessage("Загрузка " + QString::number(new_active_angle) + "-го ракурса эталонного изображения ...");

        ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon().getFilename());
    }

    angle_actions[new_active_angle-1]->setChecked(true);
}

void MainWindow::menuEtalonAngleSetActive2ActionTriggered()
{
    const quint8 new_active_angle = 2;

    if(new_active_angle != PuansonChecker::getInstance()->getActiveEtalonResearchAngle())
    {
        for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
            angle_actions[angle-1]->setChecked(false);

        PuansonChecker::getInstance()->setActiveEtalonResearchAngle(new_active_angle);
        PuansonChecker::getInstance()->getEtalon().loadEtalonAngle(PuansonChecker::getInstance()->getEtalonResearchFolderPath() + "/angle_2");

        ui->statusBar->showMessage("Загрузка " + QString::number(new_active_angle) + "-го ракурса эталонного изображения ...");

        ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon().getFilename());
    }

    angle_actions[new_active_angle-1]->setChecked(true);
}

void MainWindow::menuEtalonAngleSetActive3ActionTriggered()
{
    const quint8 new_active_angle = 3;

    if(new_active_angle != PuansonChecker::getInstance()->getActiveEtalonResearchAngle())
    {
        for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
            angle_actions[angle-1]->setChecked(false);

        PuansonChecker::getInstance()->setActiveEtalonResearchAngle(new_active_angle);
        PuansonChecker::getInstance()->getEtalon().loadEtalonAngle(PuansonChecker::getInstance()->getEtalonResearchFolderPath() + "/angle_3");

        ui->statusBar->showMessage("Загрузка " + QString::number(new_active_angle) + "-го ракурса эталонного изображения ...");

        ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon().getFilename());
    }

    angle_actions[new_active_angle-1]->setChecked(true);
}

void MainWindow::menuEtalonAngleSetActive4ActionTriggered()
{
    const quint8 new_active_angle = 4;

    if(new_active_angle != PuansonChecker::getInstance()->getActiveEtalonResearchAngle())
    {
        for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
            angle_actions[angle-1]->setChecked(false);

        PuansonChecker::getInstance()->setActiveEtalonResearchAngle(new_active_angle);
        PuansonChecker::getInstance()->getEtalon().loadEtalonAngle(PuansonChecker::getInstance()->getEtalonResearchFolderPath() + "/angle_4");

        ui->statusBar->showMessage("Загрузка " + QString::number(new_active_angle) + "-го ракурса эталонного изображения ...");

        ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon().getFilename());
    }

    angle_actions[new_active_angle-1]->setChecked(true);
}

void MainWindow::menuEtalonAngleSetActive5ActionTriggered()
{
    const quint8 new_active_angle = 5;

    if(new_active_angle != PuansonChecker::getInstance()->getActiveEtalonResearchAngle())
    {
        for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
            angle_actions[angle-1]->setChecked(false);

        PuansonChecker::getInstance()->setActiveEtalonResearchAngle(new_active_angle);
        PuansonChecker::getInstance()->getEtalon().loadEtalonAngle(PuansonChecker::getInstance()->getEtalonResearchFolderPath() + "/angle_5");

        ui->statusBar->showMessage("Загрузка " + QString::number(new_active_angle) + "-го ракурса эталонного изображения ...");

        ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon().getFilename());
    }

    angle_actions[new_active_angle-1]->setChecked(true);
}

void MainWindow::menuEtalonAngleSetActive6ActionTriggered()
{
    const quint8 new_active_angle = 6;

    if(new_active_angle != PuansonChecker::getInstance()->getActiveEtalonResearchAngle())
    {
        for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
            angle_actions[angle-1]->setChecked(false);

        PuansonChecker::getInstance()->setActiveEtalonResearchAngle(new_active_angle);
        PuansonChecker::getInstance()->getEtalon().loadEtalonAngle(PuansonChecker::getInstance()->getEtalonResearchFolderPath() + "/angle_6");

        ui->statusBar->showMessage("Загрузка " + QString::number(new_active_angle) + "-го ракурса эталонного изображения ...");

        ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon().getFilename());
    }

    angle_actions[new_active_angle-1]->setChecked(true);
}

void MainWindow::menuEtalonResearchPropertiesActionTriggered()
{
    EtalonResearchPropertiesDialog etalon_research_properties_dialog(PuansonChecker::getInstance()->getLoadedResearch());

    etalon_research_properties_dialog.setModal(false);
    etalon_research_properties_dialog.setFixedSize(etalon_research_properties_dialog.size());

    etalon_research_properties_dialog.exec();
}

void MainWindow::menuCurrentResearchActionTriggered()
{
    if(PuansonChecker::getInstance()->isEtalonResearchLoaded())
        PuansonChecker::getInstance()->researchCurrentAngle();
    else
        QMessageBox::warning(this, "Внимание!", "Перед исследованием текущей детали необходимо загрузить исследование эталона");
}

void MainWindow::menuImageWorkSetLeftBottomReferencePointSearchAreaActionTriggered()
{
    if(!PuansonChecker::getInstance()->isEtalonImageLoaded())
    {
        QMessageBox::warning(this, "Внимание", "Изображение эталонной детали не загружено!");
        return;
    }

    if(PuansonChecker::getInstance()->getActiveEtalonResearchAngle() != 0/* работа с исследованием ведётся */
            && !PuansonChecker::getInstance()->isEtalonResearchCompleted())
    {
        image_x = 0;
        image_y = ui->graphicsView->scene()->height() - ui->graphicsView->height() / 2;

        ui->graphicsView->centerOn(image_x + ui->graphicsView->width() / 2 - 3 , image_y + ui->graphicsView->height() / 2 - 3);
    }

    setCalibrationMode(CalibrationMode_e::REFERENCE_POINT_1_AREA);
    drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_1);
}

void MainWindow::menuImageWorkSetRightTopReferencePointSearchAreaActionTriggered()
{
    if(!PuansonChecker::getInstance()->isEtalonImageLoaded())
    {
        QMessageBox::warning(this, "Внимание", "Изображение эталонной детали не загружено!");
        return;
    }

    if(PuansonChecker::getInstance()->getActiveEtalonResearchAngle() != 0/* работа с исследованием ведётся */
            && !PuansonChecker::getInstance()->isEtalonResearchCompleted())
    {
        image_x = ui->graphicsView->scene()->width() - ui->graphicsView->width() / 2;
        image_y = 0;

        ui->graphicsView->centerOn(image_x + ui->graphicsView->width() / 2 - 3 , image_y + ui->graphicsView->height() / 2 - 3);
    }

    setCalibrationMode(CalibrationMode_e::REFERENCE_POINT_2_AREA);
    drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_2);
}

void MainWindow::menuImageWorkSaveResultTriggered()
{
    if(PuansonChecker::getInstance()->getEtalon().isEmpty())
    {
        QMessageBox::warning(this, "Внимание!", "Невозможно сохранить результирующее изображение контуров - не загружено изображение ракурса эталона");
        return;
    }

    if(PuansonChecker::getInstance()->getCurrent().isEmpty())
    {
        QMessageBox::warning(this, "Внимание!", "Невозможно сохранить результирующее изображение контуров - не загружено изображение текущей детали");
        return;
    }

    QString selFilter="";
    QString result_image_path = QFileDialog::getSaveFileName(Q_NULLPTR, "Сохранить результирующее изображение контуров", "", "(*.tiff *.TIFF);; (*.tif *.TIF);; (*.png *.PNG);; (*.jpg *.JPG)", &selFilter);

    if(!(result_image_path.endsWith(".tiff") || result_image_path.endsWith(".TIFF") ||
       result_image_path.endsWith(".tif") || result_image_path.endsWith(".TIF") ||
       result_image_path.endsWith(".png") || result_image_path.endsWith(".PNG") ||
       result_image_path.endsWith(".jpg") || result_image_path.endsWith(".JPG")))
    {
        if(selFilter == "(*.tiff *.TIFF)")
            result_image_path += ".tiff";
        else if(selFilter == "(*.tif *.TIF)")
            result_image_path += ".tif";
        else if(selFilter == "(*.png *.PNG)")
            result_image_path += ".png";
        else if(selFilter == "(*.jpg *.JPG)")
            result_image_path += ".jpg";
    }

    if(result_image_path.isEmpty())
    {
        QMessageBox::warning(this, "Внимание!", "Невозможно сохранить результирующее изображение контуров - неправильный путь");
        return;
    }

    if(PuansonChecker::getInstance()->saveResultImage(result_image_path) == false)
    {
        QMessageBox::warning(this, "Внимание!", "Невозможно сохранить результирующее изображение контуров - попытка сохранения в неправильном формате");
        return;
    }
}

void MainWindow::menuImageWorkEtalonLoadActionTriggered()
{
    QString file_to_open = QFileDialog::getOpenFileName(Q_NULLPTR, "Открыть файл изображения ракурса эталона", "", "*.nef *.NEF");

    if(file_to_open.isEmpty())
    {
        if(PuansonChecker::getInstance()->getActiveEtalonResearchAngle() != 0)
        {
            PuansonChecker::getInstance()->cancelEtalonResearch();

            ui->etalon_angle_menu->setEnabled(false);
            ui->etalon_research_properties_action->setEnabled(false);
            ui->image_work_etalon_save_angle_to_research->setEnabled(false);
            ui->label->setText("Изображение эталонной детали");

            QMessageBox::information(this, "Исследование завершено", "Результат исследования: \"Отменено пользователем\"");
        }

        return;
    }

    ui->statusBar->showMessage("Загрузка ракурса эталонного изображения ...");

    PuansonChecker::getInstance()->loadEtalonImage(1, file_to_open);
}

void MainWindow::menuImageWorkEtalonShootAndLoadActionTriggered()
{
    ui->image_work_etalon_shoot_and_load_action->setEnabled(false);
    PuansonChecker::getInstance()->shootAndLoadEtalonImage();
}

void MainWindow::menuImageWorkEtalonReferencePointsAutoSearchActionTriggered()
{
    if(!PuansonChecker::getInstance()->isEtalonImageLoaded())
    {
        QMessageBox::warning(this, "Внимание", "Изображение эталонной детали не загружено!");
        return;
    }

    if(reference_point_1_auto_search_area_rect.isNull())
    {
        QMessageBox::warning(this, "Внимание", "Не задана область поиска левой нижней реперной точки");
        return;
    }

    if(reference_point_2_auto_search_area_rect.isNull())
    {
        QMessageBox::warning(this, "Внимание", "Не задана область поиска правой верхней реперной точки");
        return;
    }

    QVector<QPointF> reference_points = PuansonChecker::getInstance()->findReferencePoints(PuansonChecker::getInstance()->getEtalon());

    PuansonChecker::getInstance()->getEtalon().setReferencePoints(reference_points[0].toPoint(), reference_points[1].toPoint());

    reference_point1 = reference_points[0].toPoint();
    reference_point2 = reference_points[1].toPoint();

    // Нужно сбросить параметры идеального контура и удалить его, т. к. положение репеных точек поменялось и он не актуален
    if(ui->graphicsView->scene() != Q_NULLPTR && ideal_item && ideal_item->scene() == ui->graphicsView->scene())
        ui->graphicsView->scene()->removeItem(ideal_item);

    ideal_origin_point = QPoint(1000, 500);
    ideal_rotation_angle = 135.0;
    ideal_item = Q_NULLPTR;

    drawReferencePoints(reference_point1, reference_point2);

    qreal ratio = PuansonChecker::getInstance()->getEtalon().getCalibrationRatio();
    QMessageBox::information(this, "Информационное сообщение", "Отношение px / мкм : " + QString::number(ratio));
}

void MainWindow::menuImageWorkEtalonManuallySetReferencePointsActionTriggered()
{
    if(!PuansonChecker::getInstance()->isEtalonImageLoaded())
    {
        QMessageBox::warning(this, "Внимание!", "Изображение текущей детали не загружено!");
        return;
    }

    setCalibrationMode(CalibrationMode_e::REFERENCE_POINT_1);
}

void MainWindow::menuImageWorkEtalonImposeIdealContourToEtalonActionTriggered()
{
    if(PuansonChecker::getInstance()->getActiveEtalonResearchAngle() != 0/* работа с исследованием ведётся */
            && !PuansonChecker::getInstance()->isEtalonResearchCompleted())
    {
        image_x = 0;
        image_y = 0;

        ui->graphicsView->centerOn(image_x + ui->graphicsView->width() / 2 - 3 , image_y + ui->graphicsView->height() / 2 - 3);
    }

    const PuansonImage &etalon_image = PuansonChecker::getInstance()->getEtalon();
    if(etalon_image.isReferencePointsAreSet())
    {
        drawActualBorders(); 

        ideal_origin_point = QPoint(1000, 500);
        ideal_rotation_angle = 135.0;

        if(ideal_item)
            ui->graphicsView->scene()->removeItem(ideal_item);

        setImageCursor(Qt::OpenHandCursor);
        drawIdealContour();

        setCalibrationMode(CalibrationMode_e::IDEAL_IMPOSE);
    }
    else
    {
        QMessageBox::warning(this, "Внимание", "Сначала необходимо задать реперные точки");
    }
}

void MainWindow::menuImageWorkEtalonSaveAngleToResearchActionTriggered()
{
    ui->statusBar->showMessage("Сохранение ракурса эталонного изображения ...");
    QString angle_directory_path = PuansonChecker::getInstance()->getEtalonResearchFolderPath() + "/angle_" + QString::number(PuansonChecker::getInstance()->getActiveEtalonResearchAngle());
    /*QDir().mkdir(angle_directory_path);*/
    PuansonChecker::getInstance()->getEtalon().saveEtalonAngle(angle_directory_path);

    ui->statusBar->showMessage("");
}

void MainWindow::menuImageWorkCurrentLoadActionTriggered()
{
#if 0
    PuansonImage *etalon_image = PuansonChecker::getInstance()->getEtalon(PuansonChecker::getInstance()->getActiveEtalonResearchAngle());

    if(etalon_image = Q_NULLPTR)
    {
        QMessageBox::warning(this, "Внимание!", "Для получения изображения текущей детали необходимо сначала необходимо загрузить эталон!");
        return;
    }

    if(!etalon_image.isReferencePointsAreSet())
    {
        QMessageBox::warning(this, "Внимание!", "Для получения изображения текущей детали необходимо сначала задать реперные точки!");
        return;
    }
#endif // 0

    QString file_to_open = QFileDialog::getOpenFileName(Q_NULLPTR, "Открыть файл изображения текущей детали", "", "*.nef *.NEF");

    if(file_to_open.isEmpty())
    {
        if(PuansonChecker::getInstance()->getActiveCurrentResearchAngle() != 0)
        {
            PuansonChecker::getInstance()->cancelCurrentResearch();
            QMessageBox::information(this, "Исследование завершено", "Результат исследования: \"Отменено пользователем\"");
        }

        return;
    }

    ui->statusBar->showMessage("Загрузка изображения текущей детали ...");
    PuansonChecker::getInstance()->loadCurrentImage(file_to_open);
}

void MainWindow::menuImageWorkCurrentShootAndLoadActionTriggered()
{
#if 0
    PuansonImage *etalon_image = PuansonChecker::getInstance()->getEtalon(PuansonChecker::getInstance()->getActiveEtalonResearchAngle());

    if(etalon_image = Q_NULLPTR)
    {
        QMessageBox::warning(this, "Внимание!", "Для получения изображения текущей детали необходимо сначала необходимо загрузить эталон!");
        return;
    }

    if(!etalon_image.isReferencePointsAreSet())
    {
        QMessageBox::warning(this, "Внимание!", "Для получения изображения текущей детали необходимо сначала задать реперные точки!");
        return;
    }
#endif // 0

    ui->image_work_current_shoot_and_load_action->setEnabled(false);
    PuansonChecker::getInstance()->shootAndLoadCurrentImage();
}

void MainWindow::menuImageWorkCurrentManuallySetReferencePointsActionTriggered()
{
    if(!PuansonChecker::getInstance()->isCurrentImageLoaded())
    {
        QMessageBox::warning(this, "Внимание!", "Изображение текущей детали не загружено!");
        return;
    }

    PuansonChecker::getInstance()->startCurrentCalibration();
    PuansonChecker::getInstance()->activateCurrentImageWindow();
}

void MainWindow::menuImageWorkCurrentReferencePointsAutoSearchActionTriggered()
{
    if(!PuansonChecker::getInstance()->isCurrentImageLoaded())
    {
        QMessageBox::warning(this, "Внимание!", "Изображение текущей детали не загружено!");
        return;
    }

    if(reference_point_1_auto_search_area_rect.isNull())
    {
        QMessageBox::warning(this, "Внимание!", "Не задана область поиска левой нижней реперной точки");
        return;
    }

    if(reference_point_2_auto_search_area_rect.isNull())
    {
        QMessageBox::warning(this, "Внимание!", "Не задана область поиска правой верхней реперной точки");
        return;
    }

    QVector<QPointF> reference_points = PuansonChecker::getInstance()->findReferencePoints(PuansonChecker::getInstance()->getCurrent());

    PuansonChecker::getInstance()->getCurrent().setReferencePoints(reference_points[0].toPoint(), reference_points[1].toPoint());
    PuansonChecker::getInstance()->drawCurrentImageReferencePoints();

    if(PuansonChecker::getInstance()->getActiveCurrentResearchAngle() == 0)/* Обновление контуров только если не проводится исследование */
        PuansonChecker::getInstance()->updateContoursImage();
}

void MainWindow::menuImageWorkCurrentSaveTriggered()
{
    if(PuansonChecker::getInstance()->getCurrent().isEmpty())
    {
        QMessageBox::warning(this, "Внимание!", "Невозможно сохранить изображение текущей детали - оно не загружено");
        return;
    }

    QString selFilter="";
    QString current_image_path = QFileDialog::getSaveFileName(Q_NULLPTR, "Сохранить изображение текущей детали", "", "(*.tiff *.TIFF);; (*.tif *.TIF);; (*.png *.PNG);; (*.jpg *.JPG)", &selFilter);

    if(!(current_image_path.endsWith(".tiff") || current_image_path.endsWith(".TIFF") ||
       current_image_path.endsWith(".tif") || current_image_path.endsWith(".TIF") ||
       current_image_path.endsWith(".png") || current_image_path.endsWith(".PNG") ||
       current_image_path.endsWith(".jpg") || current_image_path.endsWith(".JPG")))
    {
        if(selFilter == "(*.tiff *.TIFF)")
            current_image_path += ".tiff";
        else if(selFilter == "(*.tif *.TIF)")
            current_image_path += ".tif";
        else if(selFilter == "(*.png *.PNG)")
            current_image_path += ".png";
        else if(selFilter == "(*.jpg *.JPG)")
            current_image_path += ".jpg";
    }

    if(current_image_path.isEmpty())
    {
        QMessageBox::warning(this, "Внимание!", "Невозможно сохранить изображение текущей детали - неправильный путь");
        return;
    }

    if(PuansonChecker::getInstance()->saveCurrentImage(current_image_path) == false)
    {
        QMessageBox::warning(this, "Внимание!", "Невозможно сохранить изображение текущей детали - попытка сохранения в неправильном формате");
        return;
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

void MainWindow::menuExitActionTriggered()
{
    PuansonChecker::getInstance()->quit();
}

void MainWindow::drawIdealContour(const QPoint &_ideal_origin_point, const qreal _ideal_rotation_angle)
{
    if(!_ideal_origin_point.isNull())
    {
        ideal_origin_point = _ideal_origin_point;
        ideal_rotation_angle = _ideal_rotation_angle;
    }

    QPainterPath ideal_path = PuansonChecker::getInstance()->getEtalon().drawIdealContour(QRect(2, 2, qRound(ui->graphicsView->scene()->width() - 2), qRound(ui->graphicsView->scene()->height() - 2)), ideal_origin_point, ideal_rotation_angle, true);

    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

    if(ui->graphicsView->scene() != Q_NULLPTR && ideal_item && ideal_item->scene() == ui->graphicsView->scene())
        ui->graphicsView->scene()->removeItem(ideal_item);

    ideal_item = ui->graphicsView->scene()->addPath(ideal_path, QPen(Qt::red, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    QPainterPath ideal_measurements_path = PuansonChecker::getInstance()->getEtalon().getIdealContourMeasurementsPath();
    ideal_item = ui->graphicsView->scene()->addPath(ideal_measurements_path, QPen(Qt::green, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

void MainWindow::drawReferencePointAutoSearchArea(const ReferencePointType_e reference_point_type, const QRect &search_area_rect)
{
    QRect reference_point_auto_search_area_rect;
    QGraphicsRectItem **reference_point_auto_search_area_ideal_item = Q_NULLPTR;

    switch(reference_point_type)
    {
    case ReferencePointType_e::REFERENCE_POINT_1:
    default:
        if(!search_area_rect.isEmpty())
            reference_point_1_auto_search_area_rect = search_area_rect;

        reference_point_auto_search_area_rect = reference_point_1_auto_search_area_rect;
        reference_point_auto_search_area_ideal_item = &reference_point_1_auto_search_area_ideal_item;
        break;
    case ReferencePointType_e::REFERENCE_POINT_2:
        if(!search_area_rect.isEmpty())
            reference_point_2_auto_search_area_rect = search_area_rect;

        reference_point_auto_search_area_rect = reference_point_2_auto_search_area_rect;
        reference_point_auto_search_area_ideal_item = &reference_point_2_auto_search_area_ideal_item;
        break;
    }

    if(ui->graphicsView->scene() != Q_NULLPTR && reference_point_auto_search_area_ideal_item && *reference_point_auto_search_area_ideal_item && (*reference_point_auto_search_area_ideal_item)->scene() == ui->graphicsView->scene())
    {
        ui->graphicsView->scene()->removeItem(*reference_point_auto_search_area_ideal_item);
        ui->graphicsView->scene()->update();
    }

    if(ui->graphicsView->scene())
        *reference_point_auto_search_area_ideal_item = ui->graphicsView->scene()->addRect(reference_point_auto_search_area_rect, QPen(Qt::green, 3));
}

// Удаление реперных точек
void MainWindow::removeReferencePoints()
{
    qint32 i = 0;
    QGraphicsItem *current_item;

    while(i < ui->graphicsView->scene()->items().size() - 1)
    {
        current_item = ui->graphicsView->scene()->items()[i];
        if(current_item && current_item != ideal_item && current_item != reference_point_1_auto_search_area_ideal_item && current_item != reference_point_2_auto_search_area_ideal_item)
        {
            ui->graphicsView->scene()->removeItem(current_item);
            continue;
        }

        i++;
    }

    ui->graphicsView->scene()->update();
}

// Удаление реперных точек и идеального контура
void MainWindow::removeReferencePointsAndIdealContour()
{
    qint32 i = 0;
    QGraphicsItem *current_item;

    while(i < ui->graphicsView->scene()->items().size() - 1)
    {
        current_item = ui->graphicsView->scene()->items()[i];
        if(current_item && current_item != reference_point_1_auto_search_area_ideal_item && current_item != reference_point_2_auto_search_area_ideal_item)
        {
            ui->graphicsView->scene()->removeItem(current_item);
            continue;
        }

        i++;
    }

    ui->graphicsView->scene()->update();
}

// Рисование реперных точек, если они заданы
void MainWindow::drawReferencePoints(const QPoint &reference_point1, const QPoint &reference_point2)
{
    QGraphicsTextItem *text_item;

    removeReferencePoints();

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
    drawReferencePoints(reference_point1, reference_point2);

    if(PuansonChecker::getInstance()->getEtalon().isIdealContourSet())
        drawIdealContour();
}

void MainWindow::setCalibrationMode(CalibrationMode_e mode)
{
    calibration_mode = mode;

    switch(calibration_mode)
    {
    case CalibrationMode_e::REFERENCE_POINT_1:
        removeReferencePoints();

        ui->label_2->setText("Калибровка. Укажите реперную точку 1.");
        setImageCursor(Qt::CrossCursor);
        break;
    case CalibrationMode_e::REFERENCE_POINT_2:
        ui->label_2->setText("Калибровка. Укажите реперную точку 2.");
        break;
    case CalibrationMode_e::REFERENCE_POINT_1_AREA:
        {
            if(reference_point_1_auto_search_area_ideal_item != Q_NULLPTR)
                ui->graphicsView->scene()->removeItem(reference_point_1_auto_search_area_ideal_item);
            ui->graphicsView->scene()->update();
            update();

            QPolygonF polygon = ui->graphicsView->mapToScene( QRect( 0, 0,  ui->graphicsView->viewport()->width(),  ui->graphicsView->viewport()->height() ) );
            reference_point_1_auto_search_area_rect.setTopLeft(polygon.at(0).toPoint());
            reference_point_1_auto_search_area_rect.setSize(auto_search_area_reference_point_1_rect_size);

            drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_1);

            ui->label_2->setText("Укажите область автоматического поиска левой нижней реперной точки.");
        }
        break;
    case CalibrationMode_e::REFERENCE_POINT_2_AREA:
        {
            if(reference_point_2_auto_search_area_ideal_item != Q_NULLPTR)
                ui->graphicsView->scene()->removeItem(reference_point_2_auto_search_area_ideal_item);
            ui->graphicsView->scene()->update();
            update();

            QPolygonF polygon = ui->graphicsView->mapToScene( QRect( 0, 0,  ui->graphicsView->viewport()->width(),  ui->graphicsView->viewport()->height() ) );
            reference_point_2_auto_search_area_rect.setTopLeft(polygon.at(0).toPoint());
            reference_point_2_auto_search_area_rect.setSize(auto_search_area_reference_point_2_rect_size);

            drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_2);

            ui->label_2->setText("Укажите область автоматического поиска правой верней реперной точки.");
        }
        break;
    case CalibrationMode_e::IDEAL_IMPOSE:
        ui->label_2->setText("Совместите контур идеальной детали с границами эталона и установите его двойным щелчком левой кнопки мыши.");
        ideal_origin_point = QPoint(1000, 500);
        ideal_rotation_angle = 135.0;
        break;
    case CalibrationMode_e::NO_CALIBRATION:
    default:
        drawReferencePoints(reference_point1, reference_point2);

        ui->label_2->setText("Файл " + PuansonChecker::getInstance()->getEtalon().getFilename());
        setImageCursor(Qt::ArrowCursor);
        break;
    }
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)

    switch(calibration_mode)
    {
    case CalibrationMode_e::IDEAL_IMPOSE:
        {
            PuansonChecker::getInstance()->getEtalon().setIdealContourPointOfOrigin(ideal_origin_point);
            PuansonChecker::getInstance()->getEtalon().setIdealContourRotationAngle(ideal_rotation_angle);
            PuansonChecker::getInstance()->getEtalon().setIdealContourSetFlag(true);
            PuansonChecker::getInstance()->updateContoursImage();

            setCalibrationMode(CalibrationMode_e::NO_CALIBRATION);
            setImageCursor(Qt::ArrowCursor);

            drawActualBorders();

            if(PuansonChecker::getInstance()->getActiveEtalonResearchAngle() != 0/* работа с исследованием ведётся */
                    && !PuansonChecker::getInstance()->isEtalonResearchCompleted())
            {
                // 3(4). Диалоговое окно для подтверждения результатов исследования данного ракурса
                EtalonAngleResultConfirmDialog etalon_angle_result_confirm_dialog(PuansonChecker::getInstance()->getActiveEtalonResearchAngle());

                etalon_angle_result_confirm_dialog.setModal(false);
                etalon_angle_result_confirm_dialog.setFixedSize(etalon_angle_result_confirm_dialog.size());

                etalon_angle_result_confirm_dialog.exec();

                EtalonAngleResultConfirmDialogResult_e previous_dialog_result = static_cast<EtalonAngleResultConfirmDialogResult_e>(etalon_angle_result_confirm_dialog.result());

                if(previous_dialog_result == CANCEL_RESEARCH_ETALON_DIALOG_RESULT)
                {
                    PuansonChecker::getInstance()->cancelEtalonResearch();

                    ui->etalon_angle_menu->setEnabled(false);
                    ui->etalon_research_properties_action->setEnabled(false);
                    ui->image_work_etalon_save_angle_to_research->setEnabled(false);
                    ui->label->setText("Изображение эталонной детали");

                    QMessageBox::information(this, "Исследование завершено", "Результат исследования: \"Отменено пользователем\"");
                }
                else if(previous_dialog_result == RETURN_TO_SETTING_REFERENCE_POINTS_SEARCH_AREAS_ETALON_DIALOG_RESULT)
                {
                    // Задание позиции областей поиска реперных точек(только для ракурса 1)
                    if(PuansonChecker::getInstance()->getActiveEtalonResearchAngle() == 1)
                        menuImageWorkSetLeftBottomReferencePointSearchAreaActionTriggered();
                }
                else if(previous_dialog_result == RETURN_TO_IDEAL_CONTOUR_IMPOSE_ETALON_DIALOG_RESULT)
                {
                    // Автоматический поиск реперных точек и наложение идеального контура
                    menuImageWorkEtalonReferencePointsAutoSearchActionTriggered();
                    menuImageWorkEtalonImposeIdealContourToEtalonActionTriggered();
                }
                else if(previous_dialog_result == PASS_TO_NEXT_ANGLE_ETALON_DIALOG_RESULT)
                {
                    QString angle_directory_path = PuansonChecker::getInstance()->getEtalonResearchFolderPath() + "/angle_" + QString::number(PuansonChecker::getInstance()->getActiveEtalonResearchAngle());
                    QDir().mkdir(angle_directory_path);
                    PuansonChecker::getInstance()->getEtalon().saveEtalonAngle(angle_directory_path);

                    if(PuansonChecker::getInstance()->getActiveEtalonResearchAngle() != PUANSON_IMAGE_MAX_ANGLE)
                    {
                        PuansonChecker::getInstance()->researchEtalonAngle(PuansonChecker::getInstance()->incrementActiveEtalonResearchAngle());
                    }
                    else
                    {
                        // Исследование эталона завершено
                        PuansonChecker::getInstance()->completeEtalonResearch();

                        ui->etalon_angle_menu->setEnabled(true);
                        ui->etalon_research_properties_action->setEnabled(true);
                        ui->image_work_etalon_save_angle_to_research->setEnabled(true);

                        angle_actions[5]->setChecked(true);

                        QMessageBox::information(this, "Исследование завершено", "Результат исследования: \"Все ракурсы исследованы\"");
                    }
                }
            }
        }
        break;
    case CalibrationMode_e::REFERENCE_POINT_1_AREA:
        {
            PuansonChecker::getInstance()->setReferencePointSearchArea(ReferencePointType_e::REFERENCE_POINT_1, reference_point_1_auto_search_area_rect);
            setCalibrationMode(CalibrationMode_e::NO_CALIBRATION);
            setImageCursor(Qt::ArrowCursor);

            drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_1);
            PuansonChecker::getInstance()->drawCurrentImageReferencePoints();

            if(PuansonChecker::getInstance()->getActiveEtalonResearchAngle() != 0/* работа с исследованием ведётся */
                    && !PuansonChecker::getInstance()->isEtalonResearchCompleted())
                menuImageWorkSetRightTopReferencePointSearchAreaActionTriggered();
        }
        break;
    case CalibrationMode_e::REFERENCE_POINT_2_AREA:
        {
            PuansonChecker::getInstance()->setReferencePointSearchArea(ReferencePointType_e::REFERENCE_POINT_2, reference_point_2_auto_search_area_rect);
            setCalibrationMode(CalibrationMode_e::NO_CALIBRATION);
            setImageCursor(Qt::ArrowCursor);

            drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_2);
            PuansonChecker::getInstance()->drawCurrentImageReferencePoints();

            if(PuansonChecker::getInstance()->getActiveEtalonResearchAngle() != 0/* работа с исследованием ведётся */
                    && !PuansonChecker::getInstance()->isEtalonResearchCompleted())
            {
                // 3. Автоматический поиск реперных точек и наложение идеального контура(для ракурса 1)
                menuImageWorkEtalonReferencePointsAutoSearchActionTriggered();
                menuImageWorkEtalonImposeIdealContourToEtalonActionTriggered();
            }
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
    case CalibrationMode_e::REFERENCE_POINT_1:
        {
            reference_point1 = p;

            ui->graphicsView->scene()->addEllipse(p.x() - GRAPHICAL_POINT_RADIUS, p.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::green, Qt::Dense5Pattern));
            QGraphicsTextItem *text_item = ui->graphicsView->scene()->addText("Реперная точка 1", QFont("Arial", 10, QFont::Bold));
            text_item->setDefaultTextColor(Qt::green);
            text_item->setPos(p.x() + GRAPHICAL_POINT_RADIUS + 3, p.y() - GRAPHICAL_POINT_RADIUS - 3);

            setCalibrationMode(CalibrationMode_e::REFERENCE_POINT_2);
        }
        break;
    case CalibrationMode_e::REFERENCE_POINT_2:
        {
            reference_point2 = p;

            PuansonChecker::getInstance()->getEtalon().setReferencePoints(reference_point1, reference_point2);

            ui->graphicsView->scene()->addEllipse(p.x() - GRAPHICAL_POINT_RADIUS, p.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::green, Qt::Dense5Pattern));
            QGraphicsTextItem *text_item = ui->graphicsView->scene()->addText("Реперная точка 2", QFont("Arial", 10, QFont::Bold));
            text_item->setDefaultTextColor(Qt::green);
            text_item->setPos(p.x() - GRAPHICAL_POINT_RADIUS - 60, p.y() + GRAPHICAL_POINT_RADIUS + 3);

            qreal ratio = PuansonChecker::getInstance()->getEtalon().getCalibrationRatio();
            QMessageBox::information(this, "Информационное сообщение", "Отношение px / мкм : " + QString::number(ratio));

            // Нужно сбросить параметры идеального контура и удалить его, т. к. положение репеных точек поменялось и он не актуален
            if(ui->graphicsView->scene() != Q_NULLPTR && ideal_item && ideal_item->scene() == ui->graphicsView->scene())
                ui->graphicsView->scene()->removeItem(ideal_item);

            ideal_origin_point = QPoint(1000, 500);
            ideal_rotation_angle = 135.0;
            ideal_item = Q_NULLPTR;

            setCalibrationMode(CalibrationMode_e::NO_CALIBRATION);

            /*PuansonChecker::getInstance()->cropEtalonImage(PuansonChecker::getInstance()->getActiveEtalonResearchAngle());

            QImage img;
            PuansonChecker::getInstance()->getEtalonImage(PuansonChecker::getInstance()->getActiveEtalonResearchAngle(), img);
            drawImage(img);*/

            PuansonChecker::getInstance()->updateContoursImage();
        }
        break;
    case CalibrationMode_e::IDEAL_IMPOSE:
        {
            ideal_impose_mouse_pressed = true;
            ideal_impose_previous_point = p;
            setImageCursor(Qt::ClosedHandCursor);
        }
        break;
    case CalibrationMode_e::REFERENCE_POINT_1_AREA:
    case CalibrationMode_e::REFERENCE_POINT_2_AREA:
        {
            set_reference_point_auto_search_area_mouse_pressed = true;
            previous_reference_point_auto_search_area_rect_top_left_point = p;
            setImageCursor(Qt::SizeAllCursor);
        }
        break;
    case CalibrationMode_e::NO_CALIBRATION:
    default:
        break;
    }
}

void MainWindow::mouseReleaseEvent(const QPoint &p)
{
    Q_UNUSED(p)

    switch(calibration_mode)
    {
    case CalibrationMode_e::IDEAL_IMPOSE:
        ideal_impose_mouse_pressed = false;
        setImageCursor(Qt::OpenHandCursor);
        break;
    case CalibrationMode_e::REFERENCE_POINT_1_AREA:
    case CalibrationMode_e::REFERENCE_POINT_2_AREA:
        set_reference_point_auto_search_area_mouse_pressed = false;
        setImageCursor(Qt::ArrowCursor);
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

    if(set_reference_point_auto_search_area_mouse_pressed)
    {
        ReferencePointType_e reference_point_type;

        switch (calibration_mode)
        {
        case CalibrationMode_e::REFERENCE_POINT_1_AREA:
        default:
            reference_point_type = ReferencePointType_e::REFERENCE_POINT_1;

            reference_point_1_auto_search_area_rect.setTopLeft(reference_point_1_auto_search_area_rect.topLeft() + event->localPos().toPoint() - previous_reference_point_auto_search_area_rect_top_left_point);
            reference_point_1_auto_search_area_rect.setSize(auto_search_area_reference_point_1_rect_size);

            previous_reference_point_auto_search_area_rect_top_left_point = event->localPos().toPoint();
            break;
        case CalibrationMode_e::REFERENCE_POINT_2_AREA:
            reference_point_type = ReferencePointType_e::REFERENCE_POINT_2;

            reference_point_2_auto_search_area_rect.setTopLeft(reference_point_2_auto_search_area_rect.topLeft() + event->localPos().toPoint() - previous_reference_point_auto_search_area_rect_top_left_point);
            reference_point_2_auto_search_area_rect.setSize(auto_search_area_reference_point_2_rect_size);
            previous_reference_point_auto_search_area_rect_top_left_point = event->localPos().toPoint();
            break;
        }

        drawReferencePointAutoSearchArea(reference_point_type);
    }
}

bool MainWindow::wheelEvent(bool control_modifier, int delta)
{
    bool retval = false;

    switch(calibration_mode)
    {
       case CalibrationMode_e::IDEAL_IMPOSE:
            ideal_rotation_angle += delta / 120.0 / 10.0;

            drawIdealContour();
            retval = true;
       break;
       case CalibrationMode_e::REFERENCE_POINT_1_AREA:
            if(control_modifier)
            {
                auto_search_area_reference_point_1_rect_size.setWidth(qRound(auto_search_area_reference_point_1_rect_size.width() + delta / 120.0 / 1.0));
                auto_search_area_reference_point_1_rect_size.setHeight(qRound(auto_search_area_reference_point_1_rect_size.height() + delta / 120.0 / 1.0));

                ui->graphicsView->scene()->removeItem(reference_point_1_auto_search_area_ideal_item);
                ui->graphicsView->scene()->update();
                update();

                reference_point_1_auto_search_area_rect.setSize(auto_search_area_reference_point_1_rect_size);
                drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_1);

                retval = true;
            }
       break;
       case CalibrationMode_e::REFERENCE_POINT_2_AREA:
            if(control_modifier)
            {
                auto_search_area_reference_point_2_rect_size.setWidth(qRound(auto_search_area_reference_point_2_rect_size.width() + delta / 120.0 / 1.0));
                auto_search_area_reference_point_2_rect_size.setHeight(qRound(auto_search_area_reference_point_2_rect_size.height() + delta / 120.0 / 1.0));

                ui->graphicsView->scene()->removeItem(reference_point_2_auto_search_area_ideal_item);
                ui->graphicsView->scene()->update();
                update();

                reference_point_2_auto_search_area_rect.setSize(auto_search_area_reference_point_2_rect_size);
                drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_2);

                retval = true;
            }
       break;
       default:
       break;
    }

    return retval;
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
        if(calibration_mode != CalibrationMode_e::NO_CALIBRATION)
        {
            switch(calibration_mode)
            {
            case CalibrationMode_e::IDEAL_IMPOSE:
                if(ui->graphicsView->scene() != Q_NULLPTR && ideal_item && ideal_item->scene() == ui->graphicsView->scene())
                    ui->graphicsView->scene()->removeItem(ideal_item);

                ideal_origin_point = QPoint(1000, 500);
                ideal_rotation_angle = 135.0;
                ideal_item = Q_NULLPTR;
                break;
            case CalibrationMode_e::REFERENCE_POINT_1_AREA:
                reference_point_1_auto_search_area_rect = PuansonChecker::getInstance()->getReferencePointSearchArea(ImageType_e::ETALON_IMAGE, ReferencePointType_e::REFERENCE_POINT_1);
                drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_1);
                break;
            case CalibrationMode_e::REFERENCE_POINT_2_AREA:
                reference_point_2_auto_search_area_rect = PuansonChecker::getInstance()->getReferencePointSearchArea(ImageType_e::ETALON_IMAGE, ReferencePointType_e::REFERENCE_POINT_2);
                drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_2);
                break;
            default:
                break;
            }

            setCalibrationMode(CalibrationMode_e::NO_CALIBRATION);
        }

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

    angle_actions[0] = ui->etalon_angle_set_active_1_action;
    angle_actions[1] = ui->etalon_angle_set_active_2_action;
    angle_actions[2] = ui->etalon_angle_set_active_3_action;
    angle_actions[3] = ui->etalon_angle_set_active_4_action;
    angle_actions[4] = ui->etalon_angle_set_active_5_action;
    angle_actions[5] = ui->etalon_angle_set_active_6_action;

    //PuansonChecker::getInstance()->setActiveEtalonResearchAngle(0);

    ideal_origin_point = QPoint(1000, 500);
    ideal_rotation_angle = 135.0;
    ideal_item = Q_NULLPTR;

    // Меню "Эталон"
    connect(ui->etalon_research_action, SIGNAL(triggered()), SLOT(menuEtalonResearchActionTriggered()));
    connect(ui->etalon_load_research_action, SIGNAL(triggered()), SLOT(menuEtalonLoadResearchActionTriggered()));
    connect(ui->etalon_angle_set_active_1_action, SIGNAL(triggered()), SLOT(menuEtalonAngleSetActive1ActionTriggered()));
    connect(ui->etalon_angle_set_active_2_action, SIGNAL(triggered()), SLOT(menuEtalonAngleSetActive2ActionTriggered()));
    connect(ui->etalon_angle_set_active_3_action, SIGNAL(triggered()), SLOT(menuEtalonAngleSetActive3ActionTriggered()));
    connect(ui->etalon_angle_set_active_4_action, SIGNAL(triggered()), SLOT(menuEtalonAngleSetActive4ActionTriggered()));
    connect(ui->etalon_angle_set_active_5_action, SIGNAL(triggered()), SLOT(menuEtalonAngleSetActive5ActionTriggered()));
    connect(ui->etalon_angle_set_active_6_action, SIGNAL(triggered()), SLOT(menuEtalonAngleSetActive6ActionTriggered()));
    connect(ui->etalon_research_properties_action, SIGNAL(triggered()), SLOT(menuEtalonResearchPropertiesActionTriggered()));

    // Меню "Текущая деталь"
    connect(ui->current_research_action, SIGNAL(triggered()), SLOT(menuCurrentResearchActionTriggered()));

    // Меню "Работа с изображением"
    connect(ui->image_work_set_left_bottom_reference_point_search_area, SIGNAL(triggered()), SLOT(menuImageWorkSetLeftBottomReferencePointSearchAreaActionTriggered()));
    connect(ui->image_work_set_right_top_reference_point_search_area, SIGNAL(triggered()), SLOT(menuImageWorkSetRightTopReferencePointSearchAreaActionTriggered()));
    connect(ui->image_work_save_result_action, SIGNAL(triggered()), SLOT(menuImageWorkSaveResultTriggered()));

    // Меню "Работа с изображением" -> "Эталон"
    connect(ui->image_work_etalon_load_image_action, SIGNAL(triggered()), SLOT(menuImageWorkEtalonLoadActionTriggered()));
    connect(ui->image_work_etalon_shoot_and_load_action, SIGNAL(triggered()), SLOT(menuImageWorkEtalonShootAndLoadActionTriggered()));
    connect(ui->image_work_etalon_reference_points_auto_search_action, SIGNAL(triggered()), SLOT(menuImageWorkEtalonReferencePointsAutoSearchActionTriggered()));
    connect(ui->image_work_etalon_manually_set_reference_points_action, SIGNAL(triggered()), SLOT(menuImageWorkEtalonManuallySetReferencePointsActionTriggered()));
    connect(ui->image_work_etalon_impose_ideal_contour_action, SIGNAL(triggered()), SLOT(menuImageWorkEtalonImposeIdealContourToEtalonActionTriggered()));
    connect(ui->image_work_etalon_save_angle_to_research, SIGNAL(triggered()), SLOT(menuImageWorkEtalonSaveAngleToResearchActionTriggered()));

    // Меню "Работа с изображением" -> "Текущая деталь"
    connect(ui->image_work_current_load_image_action, SIGNAL(triggered()), SLOT(menuImageWorkCurrentLoadActionTriggered()));
    connect(ui->image_work_current_shoot_and_load_action, SIGNAL(triggered()), SLOT(menuImageWorkCurrentShootAndLoadActionTriggered()));
    connect(ui->image_work_current_manually_set_reference_points_action, SIGNAL(triggered()), SLOT(menuImageWorkCurrentManuallySetReferencePointsActionTriggered()));
    connect(ui->image_work_current_reference_points_auto_search_action, SIGNAL(triggered()), SLOT(menuImageWorkCurrentReferencePointsAutoSearchActionTriggered()));
    connect(ui->image_work_current_save_action, SIGNAL(triggered()), SLOT(menuImageWorkCurrentSaveTriggered()));

    // Меню "Перейти к"
    connect(ui->goto_current_window_action, SIGNAL(triggered()), SLOT(menuGotoCurrentWindowTriggered()));
    connect(ui->goto_counturs_window_action, SIGNAL(triggered()), SLOT(menuGotoContoursWindowTriggered()));

    // Меню "Настройки"
    connect(ui->settings_action, SIGNAL(triggered()), SLOT(menuSettingsActionTriggered()));

    // "Выход"
    connect(ui->exit_action, SIGNAL(triggered()), SLOT(menuExitActionTriggered()));
}

MainWindow::~MainWindow()
{
    delete ui;
}
