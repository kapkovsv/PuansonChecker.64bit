#ifndef ETALONANGLERESULTCONFIRMDIALOG_H
#define ETALONANGLERESULTCONFIRMDIALOG_H

#include <QDialog>

enum /*class*/ EtalonAngleResultConfirmDialogResult_e
{
    PASS_TO_NEXT_ANGLE_ETALON_DIALOG_RESULT = 0,
    RETURN_TO_SETTING_REFERENCE_POINTS_SEARCH_AREAS_ETALON_DIALOG_RESULT = 1,
    RETURN_TO_IDEAL_CONTOUR_IMPOSE_ETALON_DIALOG_RESULT = 2,
    CANCEL_RESEARCH_ETALON_DIALOG_RESULT = 3
};

namespace Ui {
class EtalonAngleResultConfirmDialog;
}

class EtalonAngleResultConfirmDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EtalonAngleResultConfirmDialog(quint8 etalon_research_active_angle, QWidget *parent = 0);
    ~EtalonAngleResultConfirmDialog();

public slots:
    void returnToSettingReferencePointsSearchAreasButtonPressed();
    void returnToIdealContourImposeButtonPressed();
    void passToNextAngleButtonPressed();
    void cancelResearchButtonPressed();

private:
    void closeEvent(QCloseEvent *e) Q_DECL_OVERRIDE;

    Ui::EtalonAngleResultConfirmDialog *ui;
};

#endif // ETALONANGLERESULTCONFIRMDIALOG_H
