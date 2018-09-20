#include "etalonangleresultconfirmdialog.h"
#include "ui_etalonangleresultconfirmdialog.h"
#include "puansonchecker.h"

EtalonAngleResultConfirmDialog::EtalonAngleResultConfirmDialog(quint8 etalon_research_active_angle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EtalonAngleResultConfirmDialog)
{
    ui->setupUi(this);

    if(etalon_research_active_angle == 1)
    {
        ui->returnToSettingReferencePointsSearchAreasButton->setEnabled(true);
        connect(ui->returnToSettingReferencePointsSearchAreasButton, SIGNAL(pressed()), SLOT(returnToSettingReferencePointsSearchAreasButtonPressed()));
    }

    connect(ui->returnToIdealContourImposeButton, SIGNAL(pressed()), SLOT(returnToIdealContourImposeButtonPressed()));
    connect(ui->passToNextAngleButton, SIGNAL(pressed()), SLOT(passToNextAngleButtonPressed()));
    connect(ui->cancelResearchButton, SIGNAL(pressed()), SLOT(cancelResearchButtonPressed()));

    if(etalon_research_active_angle != PUANSON_IMAGE_MAX_ANGLE)
        ui->passToNextAngleButton->setText("Перейти к " + QString::number(etalon_research_active_angle + 1) + "-му ракурсу");
    else
        ui->passToNextAngleButton->setText("Завершить исследование эталонной детали");
}

EtalonAngleResultConfirmDialog::~EtalonAngleResultConfirmDialog()
{
    delete ui;
}

void EtalonAngleResultConfirmDialog::returnToSettingReferencePointsSearchAreasButtonPressed()
{
    done(/*EtalonAngleResultConfirmDialogResult_e::*/RETURN_TO_SETTING_REFERENCE_POINTS_SEARCH_AREAS_ETALON_DIALOG_RESULT);
}

void EtalonAngleResultConfirmDialog::returnToIdealContourImposeButtonPressed()
{
    done(/*EtalonAngleResultConfirmDialogResult_e::*/RETURN_TO_IDEAL_CONTOUR_IMPOSE_ETALON_DIALOG_RESULT);
}

void EtalonAngleResultConfirmDialog::passToNextAngleButtonPressed()
{
    done(/*EtalonAngleResultConfirmDialogResult_e::*/PASS_TO_NEXT_ANGLE_ETALON_DIALOG_RESULT);
}

void EtalonAngleResultConfirmDialog::cancelResearchButtonPressed()
{
    done(/*EtalonAngleResultConfirmDialogResult_e::*/CANCEL_RESEARCH_ETALON_DIALOG_RESULT);
}

void EtalonAngleResultConfirmDialog::closeEvent(QCloseEvent *e)
{
    Q_UNUSED(e)

    done(/*EtalonAngleResultConfirmDialogResult_e::*/CANCEL_RESEARCH_ETALON_DIALOG_RESULT);
}
