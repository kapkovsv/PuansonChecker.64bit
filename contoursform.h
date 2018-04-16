#ifndef CONTOURSFORM_H
#define CONTOURSFORM_H

#include "imagewindow.h"
#include <QWidget>

class PuansonChecker;

namespace Ui {
class ContoursForm;
}

class ContoursForm : public QWidget, public ImageWindow
{
    Q_OBJECT

public:
    explicit ContoursForm(PuansonChecker *checker);
    ~ContoursForm();

    void moveImage(const qreal dx, const qreal dy);
    void drawImage(const QImage &img);
    void setImageCursor(const QCursor &cursor);

public slots:
    void etalonContourCheckBoxStateChanged(int state);
    void cannyThres1SpinBoxValueChanged(int value);
    void cannyThres2SpinBoxValueChanged(int value);
    void combineImagesByReferencePointsButtonPressed();

private:
    Ui::ContoursForm *ui;
};

#endif // CONTOURSFORM_H
