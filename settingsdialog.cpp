#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(PuansonChecker *checker) :
    QDialog(NULL),
    checker(checker),
    ui(new Ui::SettingsDialog)
{
    quint16 ext_tolerance_array[NUMBER_OF_ANGLES];
    quint16 int_tolerance_array[NUMBER_OF_ANGLES];

    ui->setupUi(this);

    PuansonChecker::getInstance()->getGeneralSettings()->getToleranceFields(ext_tolerance_array, int_tolerance_array);

    ui->angle1ExtSpinBox->setValue(ext_tolerance_array[0]);
    ui->angle2ExtSpinBox->setValue(ext_tolerance_array[1]);
    ui->angle3ExtSpinBox->setValue(ext_tolerance_array[2]);
    ui->angle4ExtSpinBox->setValue(ext_tolerance_array[3]);
    ui->angle5ExtSpinBox->setValue(ext_tolerance_array[4]);

    ui->angle1IntSpinBox->setValue(int_tolerance_array[0]);
    ui->angle2IntSpinBox->setValue(int_tolerance_array[1]);
    ui->angle3IntSpinBox->setValue(int_tolerance_array[2]);
    ui->angle4IntSpinBox->setValue(int_tolerance_array[3]);
    ui->angle5IntSpinBox->setValue(int_tolerance_array[4]);

    // Расстояние между реперными точками
    quint32 distance_array[NUMBER_OF_ANGLES];
    PuansonChecker::getInstance()->getGeneralSettings()->getReferencePointDistancesMkm(distance_array);

    ui->angle1DistanceSpinBox->setValue(distance_array[0]);
    ui->angle2DistanceSpinBox->setValue(distance_array[1]);
    ui->angle3DistanceSpinBox->setValue(distance_array[2]);
    ui->angle4DistanceSpinBox->setValue(distance_array[3]);
    ui->angle5DistanceSpinBox->setValue(distance_array[4]);
    ////////////////////////////////////

    // Соотношение мкм на px
    qreal ratio;
    QString status_str[NUMBER_OF_ANGLES];

    for(quint8 angle = 1; angle <= NUMBER_OF_ANGLES; angle++)
    {
        ratio = PuansonChecker::getInstance()->getEtalon(angle).getCalibrationRatio();

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
    ////////////////////////////////////

    // Реперные точки
    QPoint reference_point1;
    QPoint reference_point2;

    for(quint8 angle = 1; angle <= NUMBER_OF_ANGLES; angle++)
    {
        if(!PuansonChecker::getInstance()->getEtalon(angle).isReferencePointsAreSet())
        {
            status_str[angle-1] = "Не заданы";
        }
        else
        {
            PuansonChecker::getInstance()->getEtalon(angle).getReferencePoints(reference_point1, reference_point2);
            status_str[angle-1] = "(" + QString::number(reference_point1.x()) + ", " + QString::number(reference_point1.y()) + "), (" + QString::number(reference_point2.x()) + ", " + QString::number(reference_point2.y()) + ")";
        }
    }

    ui->angle1ReferencePointsStatusLabel->setText(status_str[0]);
    ui->angle2ReferencePointsStatusLabel->setText(status_str[1]);
    ui->angle3ReferencePointsStatusLabel->setText(status_str[2]);
    ui->angle4ReferencePointsStatusLabel->setText(status_str[3]);
    ui->angle5ReferencePointsStatusLabel->setText(status_str[4]);
    ////////////////////////////////////

    // Точки внутреннего остова
    QPoint etalon_top;
    QPoint etalon_right;
    QPoint etalon_bottom;
    QPoint etalon_left;

    for(quint8 angle = 1; angle <= NUMBER_OF_ANGLES; angle++)
    {
        if(!PuansonChecker::getInstance()->getEtalon(angle).isInnerSkelenonPointsAreSet())
        {
            status_str[angle-1] = "Не задан";
        }
        else
        {
            PuansonChecker::getInstance()->getEtalon(angle).getInnerSkeleton(etalon_top, etalon_right, etalon_bottom, etalon_left);

            status_str[angle-1] = "(" + QString::number(etalon_top.x()) + ", " + QString::number(etalon_top.y()) + ")" +
                            ", (" + QString::number(etalon_right.x()) + ", " + QString::number(etalon_right.y()) + ")" +
                            ", (" + QString::number(etalon_bottom.x()) + ", " + QString::number(etalon_bottom.y()) + ")" +
                            ", (" + QString::number(etalon_left.x()) + ", " + QString::number(etalon_left.y()) + ")";
        }
    }

    ui->angle1SkeletonPointsStatusLabel->setText(status_str[0]);
    ui->angle2SkeletonPointsStatusLabel->setText(status_str[1]);
    ui->angle3SkeletonPointsStatusLabel->setText(status_str[2]);
    ui->angle4SkeletonPointsStatusLabel->setText(status_str[3]);
    ui->angle5SkeletonPointsStatusLabel->setText(status_str[4]);
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
    quint16 ext_tolerance_array[NUMBER_OF_ANGLES];
    quint16 int_tolerance_array[NUMBER_OF_ANGLES];

    ext_tolerance_array[0] = ui->angle1ComboBox->currentIndex() == TOLERANCE_UNITS_PX ? ui->angle1ExtSpinBox->value() :
                                                     static_cast<int>(ui->angle1ExtSpinBox->value() / PuansonChecker::getInstance()->getEtalon(1).getCalibrationRatio());
    ext_tolerance_array[1] = ui->angle2ComboBox->currentIndex() == TOLERANCE_UNITS_PX ? ui->angle2ExtSpinBox->value() :
                                                     static_cast<int>(ui->angle2ExtSpinBox->value() / PuansonChecker::getInstance()->getEtalon(2).getCalibrationRatio());
    ext_tolerance_array[2] = ui->angle3ComboBox->currentIndex() == TOLERANCE_UNITS_PX ? ui->angle3ExtSpinBox->value() :
                                                     static_cast<int>(ui->angle3ExtSpinBox->value() / PuansonChecker::getInstance()->getEtalon(3).getCalibrationRatio());
    ext_tolerance_array[3] = ui->angle4ComboBox->currentIndex() == TOLERANCE_UNITS_PX ? ui->angle4ExtSpinBox->value() :
                                                     static_cast<int>(ui->angle4ExtSpinBox->value() / PuansonChecker::getInstance()->getEtalon(4).getCalibrationRatio());
    ext_tolerance_array[4] = ui->angle5ComboBox->currentIndex() == TOLERANCE_UNITS_PX ? ui->angle5ExtSpinBox->value() :
                                                     static_cast<int>(ui->angle5ExtSpinBox->value() / PuansonChecker::getInstance()->getEtalon(5).getCalibrationRatio());

    int_tolerance_array[0] = ui->angle1ComboBox->currentIndex() == TOLERANCE_UNITS_PX ? ui->angle1IntSpinBox->value() :
                                                     static_cast<int>(ui->angle1IntSpinBox->value() / PuansonChecker::getInstance()->getEtalon(1).getCalibrationRatio());
    int_tolerance_array[1] = ui->angle2ComboBox->currentIndex() == TOLERANCE_UNITS_PX ? ui->angle2IntSpinBox->value() :
                                                     static_cast<int>(ui->angle2IntSpinBox->value() / PuansonChecker::getInstance()->getEtalon(2).getCalibrationRatio());
    int_tolerance_array[2] = ui->angle3ComboBox->currentIndex() == TOLERANCE_UNITS_PX ? ui->angle3IntSpinBox->value() :
                                                     static_cast<int>(ui->angle3IntSpinBox->value() / PuansonChecker::getInstance()->getEtalon(3).getCalibrationRatio());
    int_tolerance_array[3] = ui->angle4ComboBox->currentIndex() == TOLERANCE_UNITS_PX ? ui->angle4IntSpinBox->value() :
                                                     static_cast<int>(ui->angle4IntSpinBox->value() / PuansonChecker::getInstance()->getEtalon(4).getCalibrationRatio());
    int_tolerance_array[4] = ui->angle5ComboBox->currentIndex() == TOLERANCE_UNITS_PX ? ui->angle5IntSpinBox->value() :
                                                     static_cast<int>(ui->angle5IntSpinBox->value() / PuansonChecker::getInstance()->getEtalon(5).getCalibrationRatio());

    PuansonChecker::getInstance()->getGeneralSettings()->setToleranceFields(ext_tolerance_array, int_tolerance_array);

    for(quint8 angle = 1; angle < NUMBER_OF_ANGLES; angle++)
        PuansonChecker::getInstance()->getEtalon(angle).setToleranceFields(ext_tolerance_array[angle - 1], int_tolerance_array[angle - 1]);

    quint32 distances_array[NUMBER_OF_ANGLES];

    distances_array[0] = ui->angle1DistanceSpinBox->value();
    distances_array[1] = ui->angle2DistanceSpinBox->value();
    distances_array[2] = ui->angle3DistanceSpinBox->value();
    distances_array[3] = ui->angle4DistanceSpinBox->value();
    distances_array[4] = ui->angle5DistanceSpinBox->value();

    PuansonChecker::getInstance()->getGeneralSettings()->setReferencePointDistancesMkm(distances_array);

    for(quint8 angle = 1; angle <= NUMBER_OF_ANGLES; angle++)
    {qDebug() << "!!!! in " << __PRETTY_FUNCTION__ << "angle " << angle << " distance " << distances_array[angle-1];
        PuansonChecker::getInstance()->getEtalon(angle).setReferencePointDistanceMkm(distances_array[angle-1]);
    }

    PuansonChecker::getInstance()->getGeneralSettings()->saveSettingsToConfigFile();

    accept();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
