#ifndef SETTINGSEDIALOG_H
#define SETTINGSEDIALOG_H

#include <QDialog>
#include "types.h"
#include "puansonchecker.h"

#include <QGraphicsScene>

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
    void SettingsDialogAccepted();

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSEDIALOG_H
