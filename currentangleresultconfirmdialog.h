#ifndef CURRENTANGLERESULTCONFIRMDIALOG_H
#define CURRENTANGLERESULTCONFIRMDIALOG_H

#include <QDialog>

enum /*class*/ CurrentAngleResultConfirmDialogResult_e
{
    PASS_TO_NEXT_ANGLE_CURRENT_DIALOG_RESULT = 0,
    CURRENT_DETAIL_MEASUREMENTS_DIALOG_RESULT = 1,
    RESEARCH_THIS_ANGLE_AGAIN_CURRENT_DIALOG_RESULT = 2,
    CANCEL_RESEARCH_CURRENT_DIALOG_RESULT = 3
};

namespace Ui {
class CurrentAngleResultConfirmDialog;
}

class CurrentAngleResultConfirmDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CurrentAngleResultConfirmDialog(quint8 etalon_research_active_angle, QWidget *parent = 0);
    ~CurrentAngleResultConfirmDialog();

public slots:
    void researchThisAngleAgainButtonPressed();
    void setPointsForMeasurementsButtonPressed();
    void passToNextAngleButtonPressed();
    void cancelResearchButtonPressed();

private:
    void closeEvent(QCloseEvent *e) Q_DECL_OVERRIDE;

private:
    Ui::CurrentAngleResultConfirmDialog *ui;
};

#endif // CURRENTANGLERESULTCONFIRMDIALOG_H
