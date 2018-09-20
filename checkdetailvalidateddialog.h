#ifndef CHECKDETAILVALIDATEDDIALOG_H
#define CHECKDETAILVALIDATEDDIALOG_H

#include <QDialog>

namespace Ui {
class CheckDetailValidatedDialog;
}

class CheckDetailValidatedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CheckDetailValidatedDialog(bool check_result, QWidget *parent = 0);
    ~CheckDetailValidatedDialog();

private:
    Ui::CheckDetailValidatedDialog *ui;
};

#endif // CHECKDETAILVALIDATEDDIALOG_H
