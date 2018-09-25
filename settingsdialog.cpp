#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QMessageBox>
#include <QFileDialog>

ReferencePointGraphicsScene::ReferencePointGraphicsScene(SettingsDialog *owner_window):
    QGraphicsScene(),
    window(owner_window)
{
}

void ReferencePointGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_UNUSED(mouseEvent)
}

SettingsDialog::SettingsDialog() :
    QDialog(Q_NULLPTR),
    ui(new Ui::SettingsDialog)
{
    quint16 ext_tolerance_array[PUANSON_IMAGE_MAX_ANGLE];
    quint16 int_tolerance_array[PUANSON_IMAGE_MAX_ANGLE];

    ui->setupUi(this);

    QMap<quint8, QPoint> detail_photo_phooting_positions = PuansonChecker::getInstance()->getGeneralSettings()->getDetailPhotoShootingPositions();

    ui->angle1PosXSpinBox->setValue(detail_photo_phooting_positions[1].x());
    ui->angle1PosYSpinBox->setValue(detail_photo_phooting_positions[1].y());
    ui->angle2PosXSpinBox->setValue(detail_photo_phooting_positions[2].x());
    ui->angle2PosYSpinBox->setValue(detail_photo_phooting_positions[2].y());
    ui->angle3PosXSpinBox->setValue(detail_photo_phooting_positions[3].x());
    ui->angle3PosYSpinBox->setValue(detail_photo_phooting_positions[3].y());
    ui->angle4PosXSpinBox->setValue(detail_photo_phooting_positions[4].x());
    ui->angle4PosYSpinBox->setValue(detail_photo_phooting_positions[4].y());
    ui->angle5PosXSpinBox->setValue(detail_photo_phooting_positions[5].x());
    ui->angle5PosYSpinBox->setValue(detail_photo_phooting_positions[5].y());
    ui->angle6PosXSpinBox->setValue(detail_photo_phooting_positions[6].x());
    ui->angle6PosYSpinBox->setValue(detail_photo_phooting_positions[6].y());

    PuansonChecker::getInstance()->getGeneralSettings()->getToleranceFields(ext_tolerance_array, int_tolerance_array);

    ui->angle1ExtSpinBox->setValue(ext_tolerance_array[0]);
    ui->angle2ExtSpinBox->setValue(ext_tolerance_array[1]);
    ui->angle3ExtSpinBox->setValue(ext_tolerance_array[2]);
    ui->angle4ExtSpinBox->setValue(ext_tolerance_array[3]);
    ui->angle5ExtSpinBox->setValue(ext_tolerance_array[4]);
    ui->angle6ExtSpinBox->setValue(ext_tolerance_array[5]);

    ui->angle1IntSpinBox->setValue(int_tolerance_array[0]);
    ui->angle2IntSpinBox->setValue(int_tolerance_array[1]);
    ui->angle3IntSpinBox->setValue(int_tolerance_array[2]);
    ui->angle4IntSpinBox->setValue(int_tolerance_array[3]);
    ui->angle5IntSpinBox->setValue(int_tolerance_array[4]);
    ui->angle6IntSpinBox->setValue(int_tolerance_array[5]);

    // Расстояние между реперными точками
    quint32 reference_points_distance;
    PuansonChecker::getInstance()->getGeneralSettings()->getReferencePointDistanceMkm(reference_points_distance);

    ui->calibrationReferencePointsDistanceSpinBox->setValue(static_cast<qint32>(reference_points_distance));
    ////////////////////////////////////

    // Соотношение мкм на px
    qreal ratio;
    QString status_str;

    ratio = PuansonChecker::getInstance()->getEtalon().getCalibrationRatio();

    if(ratio == 0.0)
        status_str = "Не задано";
    else
        status_str = QString::number(ratio) + " мкм на px";

    //ui->calibrationRatioStatusLabel->setText(status_str);
    ////////////////////////////////////

    // Реперные точки
    QPoint reference_point1;
    QPoint reference_point2;

    if(!PuansonChecker::getInstance()->getEtalon().isReferencePointsAreSet())
    {
        status_str = "Не заданы";
    }
    else
    {
        PuansonChecker::getInstance()->getEtalon().getReferencePoints(reference_point1, reference_point2);
        status_str = "(" + QString::number(reference_point1.x()) + ", " + QString::number(reference_point1.y()) + "), (" + QString::number(reference_point2.x()) + ", " + QString::number(reference_point2.y()) + ")";
    }

    //ui->calibrationReferencePointsPosStatusLabel->setText(status_str);
    ////////////////////////////////////

    // Фотокамера
    ui->cameraConnectionStatusLabel->setText("<b>Статус:</b>   " + PuansonChecker::getInstance()->getCameraStatus());

    connect(ui->cameraConnectButton, SIGNAL(pressed()), SLOT(cameraConnectButtonPressed()));
    connect(ui->cameraDisconnectButton, SIGNAL(pressed()), SLOT(cameraDisconnectButtonPressed()));
    ////////////////////////////////////

    connect(ui->settingsButtonBox, SIGNAL(accepted()), SLOT(SettingsDialogAccepted()));
    connect(ui->settingsButtonBox, SIGNAL(rejected()), SLOT(reject()));
}

void SettingsDialog::cameraConnectButtonPressed()
{
    PuansonChecker::getInstance()->connectToCamera();
    ui->cameraConnectionStatusLabel->setText("<b>Статус:</b>   " + PuansonChecker::getInstance()->getCameraStatus());
}

void SettingsDialog::cameraDisconnectButtonPressed()
{
    PuansonChecker::getInstance()->disconnectFromCamera();
    ui->cameraConnectionStatusLabel->setText( "<b>Статус:</b>   " + PuansonChecker::getInstance()->getCameraStatus());
}

void SettingsDialog::SettingsDialogAccepted()
{
    // Reference points distance
    quint32 reference_points_distance = static_cast<quint32>(ui->calibrationReferencePointsDistanceSpinBox->value());

    PuansonChecker::getInstance()->getGeneralSettings()->setReferencePointDistancesMkm(reference_points_distance);
    PuansonChecker::getInstance()->getEtalon().setReferencePointDistanceMkm(reference_points_distance);

    // Detail photoshooting positions
    QMap<quint8, QPoint> detail_photo_shooting_positions;

    detail_photo_shooting_positions.insert(1, QPoint(ui->angle1PosXSpinBox->value(), ui->angle1PosYSpinBox->value()));
    detail_photo_shooting_positions.insert(2, QPoint(ui->angle2PosXSpinBox->value(), ui->angle2PosYSpinBox->value()));
    detail_photo_shooting_positions.insert(3, QPoint(ui->angle3PosXSpinBox->value(), ui->angle3PosYSpinBox->value()));
    detail_photo_shooting_positions.insert(4, QPoint(ui->angle4PosXSpinBox->value(), ui->angle4PosYSpinBox->value()));
    detail_photo_shooting_positions.insert(5, QPoint(ui->angle5PosXSpinBox->value(), ui->angle5PosYSpinBox->value()));
    detail_photo_shooting_positions.insert(6, QPoint(ui->angle6PosXSpinBox->value(), ui->angle6PosYSpinBox->value()));

    PuansonChecker::getInstance()->getGeneralSettings()->setDetailPhotoShootingPositions(detail_photo_shooting_positions);

    // Tolerance fields
    quint16 ext_tolerance_array[PUANSON_IMAGE_MAX_ANGLE];
    quint16 int_tolerance_array[PUANSON_IMAGE_MAX_ANGLE];

    bool fail_accept = false;

    if(ui->angle1ComboBox->currentIndex() == TOLERANCE_UNITS_PX)
    {
        if(PuansonChecker::getInstance()->getEtalon().getCalibrationRatio() == 0.0)
        {
            QMessageBox msgBox;
            msgBox.setText("Не задано соотношение px / мкм для ракурса 1!");
            msgBox.exec();

            fail_accept = true;
        }
        else
        {
            int_tolerance_array[0] = static_cast<quint16>(qRound(ui->angle1IntSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio()));
            ext_tolerance_array[0] = static_cast<quint16>(qRound(ui->angle1ExtSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio()));
        }
    }
    else // ui->angle1ComboBox->currentIndex() == TOLERANCE_UNITS_MKM
    {
        int_tolerance_array[0] = static_cast<quint16>(ui->angle1IntSpinBox->value());
        ext_tolerance_array[0] = static_cast<quint16>(ui->angle1ExtSpinBox->value());
    }

    if(ui->angle2ComboBox->currentIndex() == TOLERANCE_UNITS_PX)
    {
        if(PuansonChecker::getInstance()->getEtalon().getCalibrationRatio() == 0.0)
        {
            QMessageBox msgBox;
            msgBox.setText("Не задано соотношение px / мкм для ракурса 2!");
            msgBox.exec();

            fail_accept = true;
        }
        else
        {
            int_tolerance_array[1] = static_cast<quint16>(qRound(ui->angle2IntSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio()));
            ext_tolerance_array[1] = static_cast<quint16>(qRound(ui->angle2ExtSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio()));
        }
    }
    else // ui->angle2ComboBox->currentIndex() == TOLERANCE_UNITS_MKM
    {
        int_tolerance_array[1] = static_cast<quint16>(ui->angle2IntSpinBox->value());
        ext_tolerance_array[1] = static_cast<quint16>(ui->angle2ExtSpinBox->value());
    }

    if(ui->angle3ComboBox->currentIndex() == TOLERANCE_UNITS_PX)
    {
        if(PuansonChecker::getInstance()->getEtalon().getCalibrationRatio() == 0.0)
        {
            QMessageBox msgBox;
            msgBox.setText("Не задано соотношение px / мкм для ракурса 3!");
            msgBox.exec();

            fail_accept = true;
        }
        else
        {
            int_tolerance_array[2] = static_cast<quint16>(qRound(ui->angle3IntSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio()));
            ext_tolerance_array[2] = static_cast<quint16>(qRound(ui->angle3ExtSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio()));
        }
    }
    else // ui->angle3ComboBox->currentIndex() == TOLERANCE_UNITS_MKM
    {
        int_tolerance_array[2] = static_cast<quint16>(ui->angle3IntSpinBox->value());
        ext_tolerance_array[2] = static_cast<quint16>(ui->angle3ExtSpinBox->value());
    }

    if(ui->angle4ComboBox->currentIndex() == TOLERANCE_UNITS_PX)
    {
        if(PuansonChecker::getInstance()->getEtalon().getCalibrationRatio() == 0.0)
        {
            QMessageBox msgBox;
            msgBox.setText("Не задано соотношение px / мкм для ракурса 4!");
            msgBox.exec();

            fail_accept = true;
        }
        else
        {
            int_tolerance_array[3] = static_cast<quint16>(qRound(ui->angle4IntSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio()));
            ext_tolerance_array[3] = static_cast<quint16>(qRound(ui->angle4ExtSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio()));
        }
    }
    else // ui->angle4ComboBox->currentIndex() == TOLERANCE_UNITS_MKM
    {
        int_tolerance_array[3] = static_cast<quint16>(ui->angle4IntSpinBox->value());
        ext_tolerance_array[3] = static_cast<quint16>(ui->angle4ExtSpinBox->value());
    }

    if(ui->angle5ComboBox->currentIndex() == TOLERANCE_UNITS_PX)
    {
        if(PuansonChecker::getInstance()->getEtalon().getCalibrationRatio() == 0.0)
        {
            QMessageBox msgBox;
            msgBox.setText("Не задано соотношение px / мкм для ракурса 5!");
            msgBox.exec();

            fail_accept = true;
        }
        else
        {
            int_tolerance_array[4] = static_cast<quint16>(qRound(ui->angle5IntSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio()));
            ext_tolerance_array[4] = static_cast<quint16>(qRound(ui->angle5ExtSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio()));
        }
    }
    else // ui->angle5ComboBox->currentIndex() == TOLERANCE_UNITS_MKM
    {
        int_tolerance_array[4] = static_cast<quint16>(ui->angle5IntSpinBox->value());
        ext_tolerance_array[4] = static_cast<quint16>(ui->angle5ExtSpinBox->value());
    }

    if(ui->angle6ComboBox->currentIndex() == TOLERANCE_UNITS_PX)
    {
        if(PuansonChecker::getInstance()->getEtalon().getCalibrationRatio() == 0.0)
        {
            QMessageBox msgBox;
            msgBox.setText("Не задано соотношение px / мкм для ракурса 5!");
            msgBox.exec();

            fail_accept = true;
        }
        else
        {
            int_tolerance_array[5] = static_cast<quint16>(qRound(ui->angle5IntSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio()));
            ext_tolerance_array[5] = static_cast<quint16>(qRound(ui->angle5ExtSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio()));
        }
    }
    else // ui->angle5ComboBox->currentIndex() == TOLERANCE_UNITS_MKM
    {
        int_tolerance_array[5] = static_cast<quint16>(ui->angle5IntSpinBox->value());
        ext_tolerance_array[5] = static_cast<quint16>(ui->angle5ExtSpinBox->value());
    }

    if(fail_accept)
        return;

    for(quint8 angle = 1; angle < PUANSON_IMAGE_MAX_ANGLE; angle++)
        PuansonChecker::getInstance()->getEtalon().setToleranceFields(ext_tolerance_array[angle - 1], int_tolerance_array[angle - 1]);

    PuansonChecker::getInstance()->getLoadedResearch().updateCalibrationRatio();

    PuansonChecker::getInstance()->getGeneralSettings()->saveSettingsToConfigFile();

    accept();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
