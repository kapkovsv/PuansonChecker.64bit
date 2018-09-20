#include "currentresearchcreationdialog.h"
#include "ui_currentresearchcreationdialog.h"

#include "puansonchecker.h"
#include <QFileDialog>
#include <QMessageBox>

CurrentResearchCreationDialog::CurrentResearchCreationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CurrentResearchCreationDialog)
{
    ui->setupUi(this);

    connect(ui->chooseSaveFolderPathPushButton, SIGNAL(pressed()), SLOT(chooseSaveFolderPathPushButtonPressed()));
    connect(ui->saveResearchToDiskCheckBox, SIGNAL(stateChanged(int)), SLOT(saveResearchToDiskCheckBoxStateChanged(int)));

    connect(ui->buttonBox, SIGNAL(accepted()), SLOT(currentResearchCreationDialogAccepted()));
    connect(ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));

    QString current_research_folder_path;
    QDateTime current_research_date_time_of_creation;
    bool current_detail_angle_source_photo_shooting_only;

    PuansonChecker::getInstance()->getCurrentResearchSettings(current_research_folder_path, current_research_date_time_of_creation, current_detail_angle_source_photo_shooting_only);

    ui->saveFolderPathLineEdit->setText(current_research_folder_path);
    ui->creationDateTimeEdit->setDateTime(current_research_date_time_of_creation);
    ui->imageSourcePhotoShootingOnlyCheckBox->setChecked(current_detail_angle_source_photo_shooting_only);

    ui->creationDateTimeEdit->setDateTime(QDateTime::currentDateTime());
}

CurrentResearchCreationDialog::~CurrentResearchCreationDialog()
{
    delete ui;
}

void CurrentResearchCreationDialog::saveResearchToDiskCheckBoxStateChanged(int state)
{
    if(state == Qt::Checked)
    {
        ui->creationDateTimeLabel->setEnabled(true);
        ui->creationDateTimeEdit->setEnabled(true);
        ui->saveFolderPathLabel->setEnabled(true);
        ui->saveFolderPathLineEdit->setEnabled(true);
        ui->chooseSaveFolderPathPushButton->setEnabled(true);
    }
    else if(state == Qt::Unchecked)
    {
        ui->creationDateTimeLabel->setEnabled(false);
        ui->creationDateTimeEdit->setEnabled(false);
        ui->saveFolderPathLabel->setEnabled(false);
        ui->saveFolderPathLineEdit->setEnabled(false);
        ui->chooseSaveFolderPathPushButton->setEnabled(false);
    }
}

void CurrentResearchCreationDialog::currentResearchCreationDialogAccepted()
{
    QString current_research_folder_path = ui->saveFolderPathLineEdit->text();
    QDateTime creation_date_time = ui->creationDateTimeEdit->dateTime();
    bool current_detail_angle_source_photo_shooting_only = ui->imageSourcePhotoShootingOnlyCheckBox->checkState() == Qt::Checked;

    if(!(ui->saveResearchToDiskCheckBox->checkState() == Qt::Checked))
        current_research_folder_path = "";

    PuansonChecker::getInstance()->setCurrentResearchSettings(current_research_folder_path, creation_date_time, current_detail_angle_source_photo_shooting_only);

    accept();
}

void CurrentResearchCreationDialog::chooseSaveFolderPathPushButtonPressed()
{
    QString current_research_folder_path = QFileDialog::getExistingDirectory(Q_NULLPTR, "Выберете папку для сохранения исследования эталона", "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if(current_research_folder_path.isEmpty())
        return;

    ui->saveFolderPathLineEdit->setText(current_research_folder_path);
}
