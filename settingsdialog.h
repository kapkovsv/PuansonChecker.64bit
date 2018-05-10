#ifndef SETTINGSEDIALOG_H
#define SETTINGSEDIALOG_H

#include <QDialog>
#include "puansonchecker.h"

enum ToleranceUnits_e
{
    TOLERANCE_UNITS_MKM = 0,
    TOLERANCE_UNITS_PX = 1
};

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(PuansonChecker *checker);
    ~SettingsDialog();

public slots:
    void cameraConnectButtonPressed();
    void cameraDisconnectButtonPressed();
    void SettingsDialogAccepted();

private:
    PuansonChecker *checker;

    Ui::SettingsDialog *ui;
};

#endif // SETTINGSEDIALOG_H
