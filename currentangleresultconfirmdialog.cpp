#include "currentangleresultconfirmdialog.h"
#include "ui_currentangleresultconfirmdialog.h"
#include "puansonchecker.h"

CurrentAngleResultConfirmDialog::CurrentAngleResultConfirmDialog(quint8 etalon_research_active_angle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CurrentAngleResultConfirmDialog)
{
    ui->setupUi(this);

    connect(ui->researchThisAngleAgainButton, SIGNAL(pressed()), SLOT(researchThisAngleAgainButtonPressed()));
    connect(ui->setPointsForMeasurementsButton, SIGNAL(pressed()), SLOT(setPointsForMeasurementsButtonPressed()));
    connect(ui->passToNextAngleButton, SIGNAL(pressed()), SLOT(passToNextAngleButtonPressed()));
    connect(ui->cancelResearchButton, SIGNAL(pressed()), SLOT(cancelResearchButtonPressed()));

    if(etalon_research_active_angle != PUANSON_IMAGE_MAX_ANGLE)
        ui->passToNextAngleButton->setText("Перейти к " + QString::number(etalon_research_active_angle + 1) + "-му ракурсу");
    else
        ui->passToNextAngleButton->setText("Завершить исследование эталонной детали");
}

CurrentAngleResultConfirmDialog::~CurrentAngleResultConfirmDialog()
{
    delete ui;
}

void CurrentAngleResultConfirmDialog::researchThisAngleAgainButtonPressed()
{
    done(/*CurrentAngleResultConfirmDialogResult_e::*/RESEARCH_THIS_ANGLE_AGAIN_CURRENT_DIALOG_RESULT);
}

void CurrentAngleResultConfirmDialog::setPointsForMeasurementsButtonPressed()
{
    done(/*CurrentAngleResultConfirmDialogResult_e::*/CURRENT_DETAIL_MEASUREMENTS_DIALOG_RESULT);
}

void CurrentAngleResultConfirmDialog::passToNextAngleButtonPressed()
{
    done(/*CurrentAngleResultConfirmDialogResult_e::*/PASS_TO_NEXT_ANGLE_CURRENT_DIALOG_RESULT);
}

void CurrentAngleResultConfirmDialog::cancelResearchButtonPressed()
{
    done(/*CurrentAngleResultConfirmDialogResult_e::*/CANCEL_RESEARCH_CURRENT_DIALOG_RESULT);
}

void CurrentAngleResultConfirmDialog::closeEvent(QCloseEvent *e)
{
    Q_UNUSED(e)

    done(/*CurrentAngleResultConfirmDialogResult_e::*/CANCEL_RESEARCH_CURRENT_DIALOG_RESULT);
}
