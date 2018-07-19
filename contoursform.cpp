#include "contoursform.h"
#include "ui_contoursform.h"
#include <QGraphicsPixmapItem>
#include <QMessageBox>

void ContoursForm::moveImage(const qreal dx, const qreal dy)
{
    QGraphicsPixmapItem *pixmap_item = ui->graphicsView->scene() ? qgraphicsitem_cast<QGraphicsPixmapItem *>(ui->graphicsView->scene()->items().last()) : Q_NULLPTR;

    if(pixmap_item)
    {
        qreal new_x, new_y;

        new_x = image_x - dx;
        new_y = image_y - dy;

        new_x = image_x - dx;
        new_y = image_y - dy;

        if(new_x < 0)
            new_x = 0;
        else if(new_x > pixmap_item->pixmap().width() - ui->graphicsView->width() + 7)
            new_x = pixmap_item->pixmap().width() - ui->graphicsView->width() + 7;

        if(new_y < 0)
            new_y = 0;
        else if(new_y > pixmap_item->pixmap().height() - ui->graphicsView->height() + 7)
            new_y = pixmap_item->pixmap().height() - ui->graphicsView->height() + 7;

        ui->graphicsView->centerOn(new_x + ui->graphicsView->width() / 2 - 3 , new_y + ui->graphicsView->height() / 2 - 3);

        image_x = new_x;
        image_y = new_y;
    }
}

void ContoursForm::setImageCursor(const QCursor &cursor)
{
    ui->graphicsView->viewport()->setCursor(cursor);
}

void ContoursForm::drawIdealContour(const QPainterPath &ideal_path)
{
    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
    ui->graphicsView->scene()->addPath(ideal_path, QPen(Qt::yellow, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

void ContoursForm::drawBadPoints(const QVector<QPoint> &bad_points)
{
    for(const QPoint &bad_point: bad_points)
    {
        ui->graphicsView->scene()->addEllipse(bad_point.x() - GRAPHICAL_POINT_RADIUS, bad_point.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::yellow, Qt::Dense5Pattern));
    }
}

void ContoursForm::drawActualBorders()
{
    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

    ui->graphicsView->scene()->addRect( qRound((ui->graphicsView->scene()->width() - ui->graphicsView->scene()->width() / 4.0) / 2.0),
                                        qRound((ui->graphicsView->scene()->height() - ui->graphicsView->scene()->height() / 4.0) / 2.0),
                                        qRound(ui->graphicsView->scene()->width() / 4.0) ,
                                        qRound(ui->graphicsView->scene()->height() / 4.0), QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin) );
}

void ContoursForm::drawImage(const QImage &img)
{
    if(!scene->items().isEmpty())
        scene->clear();

    QPixmap pixmap;
    pixmap = QPixmap::fromImage(img);

    if(pixmap.isNull())
    {
        QString memory_string("pixmap is NULL\n\n");
#if 0
        MEMORYSTATUSEX memory_status;
        ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
        memory_status.dwLength = sizeof(MEMORYSTATUSEX);

        if (GlobalMemoryStatusEx(&memory_status))
        {
            memory_string += "total physical memory: " + QString::number(memory_status.ullTotalPhys / (1024 * 1024)) + " MB\n";
            memory_string += "total virtual memory: " + QString::number(memory_status.ullTotalVirtual / (1024 * 1024)) + " MB\n";

            memory_string += "availible physical memory: " + QString::number(memory_status.ullAvailPhys / (1024 * 1024)) + " MB\n";
            memory_string += "availible virtual memory: " + QString::number(memory_status.ullAvailVirtual / (1024 * 1024)) + " MB\n";
        }
        else
        {
            memory_string += "Unknown RAM\n";
        }
#endif // 0

        QMessageBox msgBox;
        msgBox.setText(memory_string);
        msgBox.exec();
    }

    scene->addPixmap(pixmap);
    scene->setSceneRect(0, 0, img.width(), img.height());
    ui->graphicsView->setScene(scene);
}

void ContoursForm::etalonContourCheckBoxStateChanged(int state)
{
    PuansonChecker::getInstance()->setDrawEtalonContourFlag(state == Qt::Checked);
    PuansonChecker::getInstance()->updateContoursImage();
}

void ContoursForm::cannyThres1SpinBoxValueChanged()
{
    PuansonChecker::getInstance()->setCannyThreshold1(ui->cannyThres1SpinBox->value());
    PuansonChecker::getInstance()->updateContoursImage();
}

void ContoursForm::cannyThres2SpinBoxValueChanged()
{
    PuansonChecker::getInstance()->setCannyThreshold2(ui->cannyThres2SpinBox->value());
    PuansonChecker::getInstance()->updateContoursImage();
}

void ContoursForm::combineImagesByReferencePointsButtonPressed()
{
    PuansonChecker::getInstance()->combineImagesByReferencePoints();
    //PuansonChecker::getInstance()->cropImages();
    //PuansonChecker::getInstance()->drawEtalonImage();
    PuansonChecker::getInstance()->drawCurrentImage(false);
    PuansonChecker::getInstance()->drawContoursImage();
}

ContoursForm::ContoursForm(PuansonChecker *checker) :
    QWidget(Q_NULLPTR),
    ImageWindow(),
    ui(new Ui::ContoursForm)
{
    ui->setupUi(this);

    ui->etalonContourCheckBox->setCheckState(checker->getDrawEtalonContourFlag() ? Qt::Checked : Qt::Unchecked);

    ui->cannyThres1SpinBox->setValue(checker->getCannyThreshold1());
    ui->cannyThres2SpinBox->setValue(checker->getCannyThreshold2());

    connect(ui->etalonContourCheckBox, SIGNAL(stateChanged(int)), SLOT(etalonContourCheckBoxStateChanged(int)));

    connect(ui->cannyThres1SpinBox, SIGNAL(editingFinished()), SLOT(cannyThres1SpinBoxValueChanged()));
    connect(ui->cannyThres2SpinBox, SIGNAL(editingFinished()), SLOT(cannyThres2SpinBoxValueChanged()));

    connect(ui->combineImagesByReferencePointsButton, SIGNAL(pressed()), SLOT(combineImagesByReferencePointsButtonPressed()));
}

ContoursForm::~ContoursForm()
{
    scene->clear();
    delete scene;
    delete ui;
}
