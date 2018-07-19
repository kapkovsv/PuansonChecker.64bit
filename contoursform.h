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

    void moveImage(const qreal dx, const qreal dy) Q_DECL_OVERRIDE;
    void drawIdealContour(const QPainterPath &ideal_path);
    void drawBadPoints(const QVector<QPoint> &bad_points);
    void drawActualBorders();
    void drawImage(const QImage &img);
    void setImageCursor(const QCursor &cursor) Q_DECL_OVERRIDE;

public slots:
    void etalonContourCheckBoxStateChanged(int state);
    void cannyThres1SpinBoxValueChanged();
    void cannyThres2SpinBoxValueChanged();
    void combineImagesByReferencePointsButtonPressed();

private:
    Ui::ContoursForm *ui;
};

#endif // CONTOURSFORM_H
