#ifndef SETTINGSEDIALOG_H
#define SETTINGSEDIALOG_H

#include <QDialog>
#include "puansonchecker.h"

#include <QGraphicsScene>

enum ToleranceUnits_e
{
    TOLERANCE_UNITS_MKM = 0,
    TOLERANCE_UNITS_PX = 1
};

namespace Ui {
class SettingsDialog;
}

class SettingsDialog;

class ReferencePointGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    ReferencePointGraphicsScene(SettingsDialog *linked_choose_button);

public slots:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE;

private:
    SettingsDialog *window;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog();
    ~SettingsDialog();

    void referencePointGraphicsSceneClicked(ReferencePointGraphicsScene *clicked_scene);

public slots:
    void cameraConnectButtonPressed();
    void cameraDisconnectButtonPressed();
    void rightTopReferencePointButtonPressed();
    void leftBottomReferencePointButtonPressed();
    void SettingsDialogAccepted();

private:
    Ui::SettingsDialog *ui;

    void loadLeftBottomReferencePointEtalonImage(const QString &reference_point_image_filename);
    void loadRightTopReferencePointEtalonImage(const QString &reference_point_image_filename);
};

#endif // SETTINGSEDIALOG_H
