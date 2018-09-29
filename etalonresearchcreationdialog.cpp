#include "etalonresearchcreationdialog.h"
#include "ui_etalonresearchcreationdialog.h"

#include "puansonchecker.h"
#include <QFileDialog>
#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <QMessageBox>
#include <QDebug>

EtalonResearchCreationDialog::EtalonResearchCreationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EtalonResearchCreationDialog)
{
    ui->setupUi(this);

    connect(ui->puansonModelComboBox, SIGNAL(currentIndexChanged(const QString &)), SLOT(puansonModelComboBoxIndexChanged(const QString &)));
    connect(ui->chooseSaveFolderPathPushButton, SIGNAL(pressed()), SLOT(chooseSaveFolderPathPushButtonPressed()));

    connect(ui->buttonBox, SIGNAL(accepted()), SLOT(etalonResearchCreationDialogAccepted()));
    connect(ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));

    ui->creationDateTimeEdit->setDateTime(QDateTime::currentDateTime());

    PuansonModel current_etalon_model = PuansonChecker::getInstance()->getEtalon().getDetailPuansonModel();
    ui->puansonModelComboBox->setCurrentText(QString::number(static_cast<int>(current_etalon_model)));

    quint32 diameter_1_dimension, diameter_2_dimension, diameter_3_dimension, diameter_4_dimension, diameter_5_dimension, diameter_6_dimension, diameter_7_dimension, diameter_8_dimension, diameter_9_dimension, groove_width_dimension, groove_depth_dimension;
    quint32 needle_rounding_radius, skirt_rounding_radius, skirt_bottom_rounding_radius;

    PuansonChecker::getInstance()->getEtalon().getIdealDimensions(diameter_1_dimension, diameter_2_dimension, diameter_3_dimension, diameter_4_dimension, diameter_5_dimension, diameter_6_dimension, diameter_7_dimension, diameter_8_dimension, diameter_9_dimension, groove_width_dimension, groove_depth_dimension, needle_rounding_radius, skirt_rounding_radius, skirt_bottom_rounding_radius);

    ui->dimensionDiameter1SpinBox->setValue(static_cast<qint32>(diameter_1_dimension));
    ui->dimensionDiameter2SpinBox->setValue(static_cast<qint32>(diameter_2_dimension));
    ui->dimensionDiameter3SpinBox->setValue(static_cast<qint32>(diameter_3_dimension));
    ui->dimensionDiameter4SpinBox->setValue(static_cast<qint32>(diameter_4_dimension));
    ui->dimensionDiameter5SpinBox->setValue(static_cast<qint32>(diameter_5_dimension));
    ui->dimensionDiameter6SpinBox->setValue(static_cast<qint32>(diameter_6_dimension));
    ui->dimensionDiameter7SpinBox->setValue(static_cast<qint32>(diameter_7_dimension));
    ui->dimensionDiameter8SpinBox->setValue(static_cast<qint32>(diameter_8_dimension));
    ui->dimensionDiameter9SpinBox->setValue(static_cast<qint32>(diameter_9_dimension));
    ui->dimensionGrooveWidthSpinBox->setValue(static_cast<qint32>(groove_width_dimension));
    ui->dimensionGrooveDepthSpinBox->setValue(static_cast<qint32>(groove_depth_dimension));

    ui->useMachineForDetailMovementCheckBox->setChecked(PuansonChecker::getInstance()->useMachineForDetailMovement());
    ui->imageSourcePhotoShootingOnlyCheckBox->setChecked(PuansonChecker::getInstance()->isImageSourcePhotoShootingOnly());

    updateIdealPuansonAndDimensionsGraphicsView(current_etalon_model);
}

EtalonResearchCreationDialog::~EtalonResearchCreationDialog()
{
    delete ui;
}

void EtalonResearchCreationDialog::updateIdealPuansonAndDimensionsGraphicsView(const PuansonModel puanson_model)
{
    QPainterPath idealContourPath, idealContourMeasurementsPath;
    PuansonImage::drawIdealContour(static_cast<PuansonModel>(static_cast<PuansonModel>(puanson_model)), QRect(2, 2, ui->idealPuansonAndDimensionsGraphicsView->width() - 2, ui->idealPuansonAndDimensionsGraphicsView->height() - 2), QPoint(ui->idealPuansonAndDimensionsGraphicsView->width() / 2, ui->idealPuansonAndDimensionsGraphicsView->height() / 2), -90, true, 0.025, PuansonImage::default_calibration_ratio, idealContourPath, idealContourMeasurementsPath);

    scene.clear();
    scene.setSceneRect(0, 0, ui->idealPuansonAndDimensionsGraphicsView->width() - 2, ui->idealPuansonAndDimensionsGraphicsView->height() - 2);
    QPixmap gray_pixmap(ui->idealPuansonAndDimensionsGraphicsView->width() - 2, ui->idealPuansonAndDimensionsGraphicsView->height() - 2);
    gray_pixmap.fill(Qt::lightGray);

    scene.addPixmap(gray_pixmap);

    if(ui->idealPuansonAndDimensionsGraphicsView->scene() == Q_NULLPTR)
    {
        ui->idealPuansonAndDimensionsGraphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
        ui->idealPuansonAndDimensionsGraphicsView->setScene(&scene);
    }

    /*ui->idealPuansonAndDimensionsGraphicsView->scene()->addRect(100, 50, 100, 100, QPen(Qt::darkGray, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QGraphicsTextItem *textItem = ui->idealPuansonAndDimensionsGraphicsView->scene()->addText("Ракурс", QFont("Noto Sans", 5, QFont::Bold));
    textItem->setDefaultTextColor(Qt::darkGray);
    textItem->setPos(100, 30);*/
    ui->idealPuansonAndDimensionsGraphicsView->scene()->addPath(idealContourPath, QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    ui->idealPuansonAndDimensionsGraphicsView->scene()->addPath(idealContourMeasurementsPath, QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

void EtalonResearchCreationDialog::etalonResearchCreationDialogAccepted()
{
    QString etalon_research_folder_path = ui->saveFolderPathLineEdit->text();

    if(etalon_research_folder_path.isEmpty())
    {
        QMessageBox::warning(this, "Внимание!", "Не указан путь с папкой для сохранения");
        ui->saveFolderPathLineEdit->setFocus();
        return;
    }

    PuansonModel puanson_model = static_cast<PuansonModel>(ui->puansonModelComboBox->currentText().toInt());
    QDateTime creation_date_time = ui->creationDateTimeEdit->dateTime();

    PuansonChecker::getInstance()->getEtalon().setEtalonResearchPuansonModel(puanson_model);
    PuansonChecker::getInstance()->setEtalonResearchSettings(etalon_research_folder_path, puanson_model, creation_date_time, NUMBER_OF_ETALON_ANGLES);

    EtalonDetailDimensions etalon_dimensions;

    etalon_dimensions.diameter_1_dimension = ui->dimensionDiameter1SpinBox->value();
    etalon_dimensions.diameter_2_dimension = ui->dimensionDiameter2SpinBox->value();
    etalon_dimensions.diameter_3_dimension = ui->dimensionDiameter3SpinBox->value();
    etalon_dimensions.diameter_4_dimension = ui->dimensionDiameter4SpinBox->value();
    etalon_dimensions.diameter_5_dimension = ui->dimensionDiameter5SpinBox->value();
    etalon_dimensions.diameter_6_dimension = ui->dimensionDiameter6SpinBox->value();
    etalon_dimensions.diameter_7_dimension = ui->dimensionDiameter7SpinBox->value();
    etalon_dimensions.diameter_8_dimension = ui->dimensionDiameter8SpinBox->value();
    etalon_dimensions.diameter_9_dimension = ui->dimensionDiameter9SpinBox->value();

    etalon_dimensions.groove_width_dimension = ui->dimensionGrooveWidthSpinBox->value();
    etalon_dimensions.groove_depth_dimension = ui->dimensionGrooveDepthSpinBox->value();

    etalon_dimensions.needle_rounding_radius = static_cast<quint32>(ui->dimensionRadius1SpinBox->value());
    etalon_dimensions.skirt_rounding_radius = static_cast<quint32>(ui->dimensionRadius2SpinBox->value());
    etalon_dimensions.skirt_bottom_rounding_radius = static_cast<quint32>(ui->dimensionRadius3SpinBox->value());

    PuansonChecker::getInstance()->getLoadedResearch().setDetailDimensions(etalon_dimensions);
    PuansonChecker::getInstance()->getEtalon().setDetailDimensions(static_cast<quint32>(etalon_dimensions.diameter_1_dimension), static_cast<quint32>(etalon_dimensions.diameter_2_dimension), static_cast<quint32>(etalon_dimensions.diameter_3_dimension), static_cast<quint32>(etalon_dimensions.diameter_4_dimension), static_cast<quint32>(etalon_dimensions.diameter_5_dimension), static_cast<quint32>(etalon_dimensions.diameter_6_dimension), static_cast<quint32>(etalon_dimensions.diameter_7_dimension), static_cast<quint32>(etalon_dimensions.diameter_8_dimension), static_cast<quint32>(etalon_dimensions.diameter_9_dimension), static_cast<quint32>(etalon_dimensions.groove_width_dimension), static_cast<quint32>(etalon_dimensions.groove_depth_dimension), static_cast<quint32>(etalon_dimensions.needle_rounding_radius), static_cast<quint32>(etalon_dimensions.skirt_rounding_radius), static_cast<quint32>(etalon_dimensions.skirt_bottom_rounding_radius));

    PuansonChecker::getInstance()->setImageSourcePhotoShootingOnly(ui->imageSourcePhotoShootingOnlyCheckBox->isChecked());
    PuansonChecker::getInstance()->setMachineForDetailMovement(ui->useMachineForDetailMovementCheckBox->isChecked());

    accept();
}

void EtalonResearchCreationDialog::puansonModelComboBoxIndexChanged(const QString &text)
{
//    quint32 diameter_1_dimension, diameter_2_dimension, diameter_3_dimension, diameter_4_dimension, diameter_5_dimension, diameter_6_dimension, diameter_7_dimension, diameter_8_dimension, diameter_9_dimension, groove_width_dimension, groove_depth_dimension;
//    quint32 needle_rounding_radius, skirt_rounding_radius, skirt_bottom_rounding_radius;
    EtalonDetailDimensions detail_dimensions;

    memset(&detail_dimensions, 0, sizeof detail_dimensions);

    //PuansonChecker::getInstance()->getEtalon().setEtalonResearchPuansonModel(static_cast<PuansonModel>(text.toInt()));
    PuansonImage::getIdealDimensions(static_cast<PuansonModel>(text.toInt()), detail_dimensions);
    //PuansonChecker::getInstance()->getEtalon().getIdealDimensions(diameter_1_dimension, diameter_2_dimension, diameter_3_dimension, diameter_4_dimension, diameter_5_dimension, diameter_6_dimension, diameter_7_dimension, diameter_8_dimension, diameter_9_dimension, groove_width_dimension, groove_depth_dimension, needle_rounding_radius, skirt_rounding_radius, skirt_bottom_rounding_radius);

    ui->dimensionDiameter1SpinBox->setValue(static_cast<qint32>(detail_dimensions.diameter_1_dimension));
    ui->dimensionDiameter2SpinBox->setValue(static_cast<qint32>(detail_dimensions.diameter_2_dimension));
    ui->dimensionDiameter3SpinBox->setValue(static_cast<qint32>(detail_dimensions.diameter_3_dimension));
    ui->dimensionDiameter4SpinBox->setValue(static_cast<qint32>(detail_dimensions.diameter_4_dimension));
    ui->dimensionDiameter5SpinBox->setValue(static_cast<qint32>(detail_dimensions.diameter_5_dimension));
    ui->dimensionDiameter6SpinBox->setValue(static_cast<qint32>(detail_dimensions.diameter_6_dimension));
    ui->dimensionDiameter7SpinBox->setValue(static_cast<qint32>(detail_dimensions.diameter_7_dimension));
    ui->dimensionDiameter8SpinBox->setValue(static_cast<qint32>(detail_dimensions.diameter_8_dimension));
    ui->dimensionDiameter9SpinBox->setValue(static_cast<qint32>(detail_dimensions.diameter_9_dimension));

    ui->dimensionGrooveWidthSpinBox->setValue(static_cast<qint32>(detail_dimensions.groove_width_dimension));
    ui->dimensionGrooveDepthSpinBox->setValue(static_cast<qint32>(detail_dimensions.groove_depth_dimension));

    ui->dimensionRadius1SpinBox->setValue(static_cast<qint32>(detail_dimensions.needle_rounding_radius));
    ui->dimensionRadius2SpinBox->setValue(static_cast<qint32>(detail_dimensions.skirt_rounding_radius));
    ui->dimensionRadius3SpinBox->setValue(static_cast<qint32>(detail_dimensions.skirt_bottom_rounding_radius));

    updateIdealPuansonAndDimensionsGraphicsView(static_cast<PuansonModel>(text.toInt()));
}

void EtalonResearchCreationDialog::chooseSaveFolderPathPushButtonPressed()
{
    QString etalon_research_folder_path = QFileDialog::getExistingDirectory(Q_NULLPTR, "Выберете папку для сохранения исследования эталона", "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if(etalon_research_folder_path.isEmpty())
        return;

    ui->saveFolderPathLineEdit->setText(etalon_research_folder_path);
}
