#ifndef ETALONRESEARCHCREATIONDIALOG_H
#define ETALONRESEARCHCREATIONDIALOG_H

#include <QDialog>
#include <QGraphicsScene>

#include "types.h"

namespace Ui {
class EtalonResearchCreationDialog;
}

class EtalonResearchCreationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EtalonResearchCreationDialog(QWidget *parent = 0);
    ~EtalonResearchCreationDialog();

public slots:
    void etalonResearchCreationDialogAccepted();
    void chooseSaveFolderPathPushButtonPressed();
    void puansonModelComboBoxIndexChanged(const QString &text);

private:
    Ui::EtalonResearchCreationDialog *ui;

    QGraphicsScene scene;

    void updateIdealPuansonAndDimensionsGraphicsView(const PuansonModel puanson_model);
};

#endif // ETALONRESEARCHCREATIONDIALOG_H
