#include "checkdetailvalidateddialog.h"
#include "ui_checkdetailvalidateddialog.h"

CheckDetailValidatedDialog::CheckDetailValidatedDialog(bool check_result, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CheckDetailValidatedDialog)
{
    ui->setupUi(this);

    if(check_result == true)
    {
        ui->checkResultTextEdit->setText("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
                                             "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">"
                                             "p, li { white-space: pre-wrap; }"
                                             "</style></head><body style=\" font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;\">"
                                             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:22pt; font-weight:600; color:#55aa00;\">Пригодна</span></p></body></html>");
    }
    else
    {
        ui->checkResultTextEdit->setText("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
                                             "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">"
                                             "p, li { white-space: pre-wrap; }"
                                             "</style></head><body style=\" font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;\">"
                                             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:22pt; font-weight:600; color:#aa0000;\">Не пригодна</span></p></body></html>");
    }

    connect(ui->validateButtonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(ui->validateButtonBox, SIGNAL(rejected()), SLOT(reject()));
}

CheckDetailValidatedDialog::~CheckDetailValidatedDialog()
{
    delete ui;
}
