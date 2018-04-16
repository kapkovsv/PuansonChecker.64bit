#include "contoursform.h"
#include "ui_contoursform.h"
#include <QGraphicsPixmapItem>
#include <QMessageBox>

#include <new>
#include <stdexcept>

void ContoursForm::moveImage(const qreal dx, const qreal dy)
{qDebug() << "in " << __PRETTY_FUNCTION__ << "main window line " << __LINE__;
    qDebug() << "ui->graphicsView->scene() " << ui->graphicsView->scene();
    QGraphicsPixmapItem *pixmap_item = ui->graphicsView->scene() ? qgraphicsitem_cast<QGraphicsPixmapItem *>(ui->graphicsView->scene()->items().at(0)) : NULL;
qDebug() << "in " << __PRETTY_FUNCTION__ << "main window line " << __LINE__;
    if(pixmap_item)
    {
        qreal new_x, new_y;
qDebug() << "in " << __PRETTY_FUNCTION__ << "main window line " << __LINE__;
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
qDebug() << "in " << __PRETTY_FUNCTION__ << "main window line " << __LINE__;
        ui->graphicsView->centerOn(new_x + ui->graphicsView->width() / 2, new_y + ui->graphicsView->height() / 2);
qDebug() << "in " << __PRETTY_FUNCTION__ << "main window line " << __LINE__;
        image_x = new_x;
        image_y = new_y;
    }
}

void ContoursForm::setImageCursor(const QCursor &cursor)
{
    ui->graphicsView->setCursor(cursor);
}

void ContoursForm::drawImage(const QImage &img)
{qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
    if(!scene->items().isEmpty())
        scene->clear();
    qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
    QPixmap pixmap;
    MEMORYSTATUSEX memory_status;
    try
    {
        qDebug() << "before QPixmap::fromImage";

        ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
        memory_status.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memory_status)) {
            qDebug() << "!!!total physical memory: " << memory_status.ullTotalPhys / (1024 * 1024) << " MB";
            qDebug() << "total virtual memory: " << memory_status.ullTotalVirtual / (1024 * 1024) << " MB";

            qDebug() << "availible physical memory: " << memory_status.ullAvailPhys / (1024 * 1024) << " MB";
            qDebug() << "availible virtual memory: " << memory_status.ullAvailVirtual / (1024 * 1024) << " MB!!!";
        } else {
            qDebug() << "Unknown RAM";
          }

        pixmap = QPixmap::fromImage(img);
        if(pixmap.isNull())
            throw std::runtime_error("pixmap is NULL!");

        qDebug() << "after QPixmap::fromImage";

        ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
        memory_status.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memory_status)) {
            qDebug() << "!!!total physical memory: " << memory_status.ullTotalPhys / (1024 * 1024) << " MB";
            qDebug() << "total virtual memory: " << memory_status.ullTotalVirtual / (1024 * 1024) << " MB";

            qDebug() << "availible physical memory: " << memory_status.ullAvailPhys / (1024 * 1024) << " MB";
            qDebug() << "availible virtual memory: " << memory_status.ullAvailVirtual / (1024 * 1024) << " MB!!!";
        } else {
            qDebug() << "Unknown RAM";
          }
    }
   // catch(std::bad_alloc &ex)
    catch (std::exception const& ex)
    {
        QString memory_string("");

        ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
        memory_status.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memory_status)) {

          memory_string += "total physical memory: " + QString::number(memory_status.ullTotalPhys / (1024 * 1024)) + " MB\n";
          memory_string += "total virtual memory: " + QString::number(memory_status.ullTotalVirtual / (1024 * 1024)) + " MB\n";

          memory_string += "availible physical memory: " + QString::number(memory_status.ullAvailPhys / (1024 * 1024)) + " MB\n";
          memory_string += "availible virtual memory: " + QString::number(memory_status.ullAvailVirtual / (1024 * 1024)) + " MB\n";
        } else {
          memory_string += "Unknown RAM\n";
        }

        qDebug() << "in " << __PRETTY_FUNCTION__ << " ex.what() " << ex.what();
        if(pixmap.isNull())
        {
            QMessageBox msgBox;
            msgBox.setText(memory_string);
            msgBox.exec();
        }
    }
    scene->addPixmap(pixmap);qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
    ui->graphicsView->setScene(scene);qDebug() << "in " << __PRETTY_FUNCTION__ << " line " << __LINE__;
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
    qDebug() << "combine etalon and current images function";

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
