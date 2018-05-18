#include "contoursform.h"
#include "ui_contoursform.h"
#include <QGraphicsPixmapItem>
#include <QMessageBox>

void ContoursForm::moveImage(const qreal dx, const qreal dy)
{
    QGraphicsPixmapItem *pixmap_item = ui->graphicsView->scene() ? qgraphicsitem_cast<QGraphicsPixmapItem *>(ui->graphicsView->scene()->items().at(0)) : NULL;

    if(pixmap_item)
    {
        qreal new_x, new_y;

        new_x = image_x - dx;
        new_y = image_y - dy;

        if(new_x <= 0)
            new_x = 1;
        else if(new_x >= pixmap_item->pixmap().width() - ui->graphicsView->width())
            new_x = pixmap_item->pixmap().width() - ui->graphicsView->width();

        if(new_y <= 0)
            new_y = 1;
        else if(new_y >= pixmap_item->pixmap().height() - ui->graphicsView->height())
            new_y = pixmap_item->pixmap().height() - ui->graphicsView->height();

        ui->graphicsView->centerOn(new_x + ui->graphicsView->width() / 2, new_y + ui->graphicsView->height() / 2);

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

void ContoursForm::drawImage(const QImage &img)
{
    if(!scene->items().isEmpty())
        scene->clear();

    QPixmap pixmap;
    pixmap = QPixmap::fromImage(img);

    if(pixmap.isNull())
    {
        QString memory_string("pixmap is NULL\n\n");

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

        QMessageBox msgBox;
        msgBox.setText(memory_string);
        msgBox.exec();
    }

    scene->addPixmap(pixmap);
    ui->graphicsView->setScene(scene);
}

void ContoursForm::etalonContourCheckBoxStateChanged(int state)
{
    checker->setDrawEtalonContourFlag(state == Qt::Checked);
    checker->updateContoursImage();
}

void ContoursForm::cannyThres1SpinBoxValueChanged(int value)
{
    checker->setCannyThreshold1(value);
    checker->updateContoursImage();
}

void ContoursForm::cannyThres2SpinBoxValueChanged(int value)
{
    checker->setCannyThreshold2(value);
    checker->updateContoursImage();
}

void ContoursForm::combineImagesByReferencePointsButtonPressed()
{
    checker->combineImagesByReferencePoints();
    checker->drawCurrentImage();
    checker->drawContoursImage();
}

ContoursForm::ContoursForm(PuansonChecker *checker) :
    QWidget(NULL),
    ImageWindow(checker),
    ui(new Ui::ContoursForm)
{
    ui->setupUi(this);

    ui->etalonContourCheckBox->setCheckState(checker->getDrawEtalonContourFlag() ? Qt::Checked : Qt::Unchecked);

    ui->cannyThres1SpinBox->setValue(checker->getCannyThreshold1());
    ui->cannyThres2SpinBox->setValue(checker->getCannyThreshold2());

    connect(ui->etalonContourCheckBox, SIGNAL(stateChanged(int)), SLOT(etalonContourCheckBoxStateChanged(int)));

    connect(ui->cannyThres1SpinBox, SIGNAL(valueChanged(int)), SLOT(cannyThres1SpinBoxValueChanged(int)));
    connect(ui->cannyThres2SpinBox, SIGNAL(valueChanged(int)), SLOT(cannyThres2SpinBoxValueChanged(int)));

    connect(ui->combineImagesByReferencePointsButton, SIGNAL(pressed()), SLOT(combineImagesByReferencePointsButtonPressed()));
}

ContoursForm::~ContoursForm()
{
    scene->clear();
    delete scene;
    delete ui;
}
