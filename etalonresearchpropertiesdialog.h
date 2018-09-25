#ifndef ETALONRESEARCHPROPERTIESDIALOG_H
#define ETALONRESEARCHPROPERTIESDIALOG_H

#include "puansonchecker.h"

#include <QDialog>
#include <QGraphicsScene>
#include <QDateTime>

namespace Ui {
class EtalonResearchPropertiesDialog;
}

class EtalonResearchPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EtalonResearchPropertiesDialog(const PuansonResearch &etalon_research, QWidget *parent = 0);
    ~EtalonResearchPropertiesDialog();

public slots:
    void okButtonPressed();

private:
    Ui::EtalonResearchPropertiesDialog *ui;

    QGraphicsScene scene;

    void drawIdealPuansonAndDimensionsGraphicsView(const PuansonModel puanson_model);
};

#endif // ETALONRESEARCHPROPERTIESDIALOG_H
