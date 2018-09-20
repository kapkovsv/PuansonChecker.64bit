#ifndef CURRENTRESEARCHRESULTANDPROTOCOLDIALOG_H
#define CURRENTRESEARCHRESULTANDPROTOCOLDIALOG_H

#include "puansonimage.h"

#include <QDialog>
#include <QGraphicsScene>

namespace Ui {
class CurrentResearchResultAndProtocolDialog;
}

class CurrentResearchResultAndProtocolDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CurrentResearchResultAndProtocolDialog(const EtalonDetailDimensions &current_detail_dimensions, const EtalonDetailDimensions &ideal_detail_dimensions, QWidget *parent = 0);
    ~CurrentResearchResultAndProtocolDialog();

public slots:
    void okButtonPressed();

private:
    Ui::CurrentResearchResultAndProtocolDialog *ui;

    QGraphicsScene scene;

    void drawIdealPuansonAndDimensionsGraphicsView(const PuansonModel puanson_model);
};

#endif // CURRENTRESEARCHRESULTANDPROTOCOLDIALOG_H
