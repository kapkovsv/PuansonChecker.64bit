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

    PuansonChecker::getInstance()->getGeneralSettings()->getToleranceFields(ext_tolerance_array, int_tolerance_array);

    ui->angle1ExtSpinBox->setValue(ext_tolerance_array[0]);
    ui->angle2ExtSpinBox->setValue(ext_tolerance_array[1]);
    ui->angle3ExtSpinBox->setValue(ext_tolerance_array[2]);
    ui->angle4ExtSpinBox->setValue(ext_tolerance_array[3]);
    ui->angle5ExtSpinBox->setValue(ext_tolerance_array[4]);
    //ui->angle6ExtSpinBox->setValue(ext_tolerance_array[5]);

    ui->angle1IntSpinBox->setValue(int_tolerance_array[0]);
    ui->angle2IntSpinBox->setValue(int_tolerance_array[1]);
    ui->angle3IntSpinBox->setValue(int_tolerance_array[2]);
    ui->angle4IntSpinBox->setValue(int_tolerance_array[3]);
    ui->angle5IntSpinBox->setValue(int_tolerance_array[4]);
    //ui->angle6IntSpinBox->setValue(int_tolerance_array[5]);

    // Расстояние между реперными точками
    quint32 distance_array[PUANSON_IMAGE_MAX_ANGLE];
    PuansonChecker::getInstance()->getGeneralSettings()->getReferencePointDistancesMkm(distance_array);

    ui->angle1DistanceSpinBox->setValue(distance_array[0]);
    ui->angle2DistanceSpinBox->setValue(distance_array[1]);
    ui->angle3DistanceSpinBox->setValue(distance_array[2]);
    ui->angle4DistanceSpinBox->setValue(distance_array[3]);
    ui->angle5DistanceSpinBox->setValue(distance_array[4]);
    ui->angle6DistanceSpinBox->setValue(distance_array[5]);
    ////////////////////////////////////

    // Соотношение мкм на px
    qreal ratio;
    QString status_str[PUANSON_IMAGE_MAX_ANGLE];

    for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
    {
        ratio = PuansonChecker::getInstance()->getEtalon().getCalibrationRatio();

        if(ratio == 0.0)
        {
            status_str[angle-1] = "Не задано";
        }
        else
        {
            status_str[angle-1] = QString::number(ratio) + " мкм на px";
        }
    }

    ui->angle1CalibrationRatioStatusLabel->setText(status_str[0]);
    ui->angle2CalibrationRatioStatusLabel->setText(status_str[1]);
    ui->angle3CalibrationRatioStatusLabel->setText(status_str[2]);
    ui->angle4CalibrationRatioStatusLabel->setText(status_str[3]);
    ui->angle5CalibrationRatioStatusLabel->setText(status_str[4]);
    ui->angle6CalibrationRatioStatusLabel->setText(status_str[5]);
    ////////////////////////////////////

    // Реперные точки
    QPoint reference_point1;
    QPoint reference_point2;

    for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
    {
        if(!PuansonChecker::getInstance()->getEtalon().isReferencePointsAreSet())
        {
            status_str[angle-1] = "Не заданы";
        }
        else
        {
            PuansonChecker::getInstance()->getEtalon().getReferencePoints(reference_point1, reference_point2);
            status_str[angle-1] = "(" + QString::number(reference_point1.x()) + ", " + QString::number(reference_point1.y()) + "), (" + QString::number(reference_point2.x()) + ", " + QString::number(reference_point2.y()) + ")";
        }
    }

    ui->angle1ReferencePointsStatusLabel->setText(status_str[0]);
    ui->angle2ReferencePointsStatusLabel->setText(status_str[1]);
    ui->angle3ReferencePointsStatusLabel->setText(status_str[2]);
    ui->angle4ReferencePointsStatusLabel->setText(status_str[3]);
    ui->angle5ReferencePointsStatusLabel->setText(status_str[4]);
    ui->angle6ReferencePointsStatusLabel->setText(status_str[5]);
    ////////////////////////////////////

    for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
    {
        if(!PuansonChecker::getInstance()->getEtalon().isIdealContourSet())
        {
            status_str[angle-1] = "Не задан";
        }
        else
        {
            QVector<QLine> idealLines;
            QVector<QPoint> innerIdealLines;
            QVector<QPoint> outerIdealLines;

            QPointF ideal_contour_point_of_origin;
            qreal ideal_contour_rotation_angle;

            PuansonChecker::getInstance()->getEtalon().getIdealContour(idealLines,  innerIdealLines, outerIdealLines, ideal_contour_point_of_origin, ideal_contour_rotation_angle);

            status_str[angle-1] = "Начало координат: (" + QString::number(ideal_contour_point_of_origin.x()) + ", " + QString::number(ideal_contour_point_of_origin.y()) + "); Поворот: " + QString::number(ideal_contour_rotation_angle) + QChar(176);
        }
    }

    ui->angle1SkeletonPointsStatusLabel->setText(status_str[0]);
    ui->angle2SkeletonPointsStatusLabel->setText(status_str[1]);
    ui->angle3SkeletonPointsStatusLabel->setText(status_str[2]);
    ui->angle4SkeletonPointsStatusLabel->setText(status_str[3]);
    ui->angle5SkeletonPointsStatusLabel->setText(status_str[4]);
    ui->angle6SkeletonPointsStatusLabel->setText(status_str[5]);
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
            int_tolerance_array[0] = qRound(ui->angle1IntSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio());
            ext_tolerance_array[0] = qRound(ui->angle1ExtSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio());
        }
    }
    else // ui->angle1ComboBox->currentIndex() == TOLERANCE_UNITS_MKM
    {
        int_tolerance_array[0] = ui->angle1IntSpinBox->value();
        ext_tolerance_array[0] = ui->angle1ExtSpinBox->value();
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
            int_tolerance_array[1] = qRound(ui->angle2IntSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio());
            ext_tolerance_array[1] = qRound(ui->angle2ExtSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio());
        }
    }
    else // ui->angle2ComboBox->currentIndex() == TOLERANCE_UNITS_MKM
    {
        int_tolerance_array[1] = ui->angle2IntSpinBox->value();
        ext_tolerance_array[1] = ui->angle2ExtSpinBox->value();
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
            int_tolerance_array[2] = qRound(ui->angle3IntSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio());
            ext_tolerance_array[2] = qRound(ui->angle3ExtSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio());
        }
    }
    else // ui->angle3ComboBox->currentIndex() == TOLERANCE_UNITS_MKM
    {
        int_tolerance_array[2] = ui->angle3IntSpinBox->value();
        ext_tolerance_array[2] = ui->angle3ExtSpinBox->value();
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
            int_tolerance_array[3] = qRound(ui->angle4IntSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio());
            ext_tolerance_array[3] = qRound(ui->angle4ExtSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio());
        }
    }
    else // ui->angle4ComboBox->currentIndex() == TOLERANCE_UNITS_MKM
    {
        int_tolerance_array[3] = ui->angle4IntSpinBox->value();
        ext_tolerance_array[3] = ui->angle4ExtSpinBox->value();
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
            int_tolerance_array[5] = int_tolerance_array[4] = qRound(ui->angle5IntSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio());
            ext_tolerance_array[5] = ext_tolerance_array[4] = qRound(ui->angle5ExtSpinBox->value() * PuansonChecker::getInstance()->getEtalon().getCalibrationRatio());
        }
    }
    else // ui->angle5ComboBox->currentIndex() == TOLERANCE_UNITS_MKM
    {
        int_tolerance_array[5] = int_tolerance_array[4] = ui->angle5IntSpinBox->value();
        ext_tolerance_array[5] = ext_tolerance_array[4] = ui->angle5ExtSpinBox->value();
    }

    if(fail_accept)
        return;

    PuansonChecker::getInstance()->getGeneralSettings()->setToleranceFields(ext_tolerance_array, int_tolerance_array);

    for(quint8 angle = 1; angle < PUANSON_IMAGE_MAX_ANGLE; angle++)
        PuansonChecker::getInstance()->getEtalon().setToleranceFields(ext_tolerance_array[angle - 1], int_tolerance_array[angle - 1]);

    quint32 distances_array[PUANSON_IMAGE_MAX_ANGLE];

    distances_array[0] = ui->angle1DistanceSpinBox->value();
    distances_array[1] = ui->angle2DistanceSpinBox->value();
    distances_array[2] = ui->angle3DistanceSpinBox->value();
    distances_array[3] = ui->angle4DistanceSpinBox->value();
    distances_array[4] = ui->angle5DistanceSpinBox->value();
    distances_array[5] = ui->angle6DistanceSpinBox->value();

    PuansonChecker::getInstance()->getGeneralSettings()->setReferencePointDistancesMkm(distances_array);

    for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
        PuansonChecker::getInstance()->getEtalon().setReferencePointDistanceMkm(distances_array[angle-1]);

    PuansonChecker::getInstance()->getGeneralSettings()->saveSettingsToConfigFile();

    accept();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
