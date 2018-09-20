#include "etalonresearchpropertiesdialog.h"
#include "ui_etalonresearchpropertiesdialog.h"

EtalonResearchPropertiesDialog::EtalonResearchPropertiesDialog(const QString &etalon_research_folder_path, PuansonModel research_puanson_model, const QDateTime &research_date_time_of_creation, const quint8 research_number_of_angles, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EtalonResearchPropertiesDialog)
{
    ui->setupUi(this);

    ui->pathValueLineEdit->setText(etalon_research_folder_path);
    ui->puansonModelValueLabel->setText(QString::number(static_cast<int>(research_puanson_model)));
    ui->creationDateTimeEdit->setDateTime(research_date_time_of_creation);
    ui->numberOfAnglesValueLabel->setText(QString::number(research_number_of_angles));

    connect(ui->okButton, SIGNAL(pressed()), SLOT(okButtonPressed()));
}

EtalonResearchPropertiesDialog::~EtalonResearchPropertiesDialog()
{
    delete ui;
}

void EtalonResearchPropertiesDialog::setEtalonDimensions(const quint16 diameter_1_dimension, const quint16 diameter_2_dimension, const quint16 diameter_3_dimension, const quint16 diameter_4_dimension, const quint16 diameter_5_dimension, const quint16 diameter_6_dimension, const quint16 diameter_7_dimension, const quint16 diameter_8_dimension, const quint16 diameter_9_dimension, const quint16 groove_width_dimension, const quint16 groove_depth_dimension)
{
    ui->dimensionDiameter1SpinBox->setValue(diameter_1_dimension);
    ui->dimensionDiameter2SpinBox->setValue(diameter_2_dimension);
    ui->dimensionDiameter3SpinBox->setValue(diameter_3_dimension);
    ui->dimensionDiameter4SpinBox->setValue(diameter_4_dimension);
    ui->dimensionDiameter5SpinBox->setValue(diameter_5_dimension);
    ui->dimensionDiameter6SpinBox->setValue(diameter_6_dimension);
    ui->dimensionDiameter7SpinBox->setValue(diameter_7_dimension);
    ui->dimensionDiameter8SpinBox->setValue(diameter_8_dimension);
    ui->dimensionDiameter9SpinBox->setValue(diameter_9_dimension);
    ui->dimensionDiameter10SpinBox->setValue(0);
    ui->dimensionGrooveWidthSpinBox->setValue(groove_width_dimension);
    ui->dimensionGrooveDepthSpinBox->setValue(groove_depth_dimension);
}

void EtalonResearchPropertiesDialog::okButtonPressed()
{
    close();
}
