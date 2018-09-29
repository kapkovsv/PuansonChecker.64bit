#include "currentresearchresultandprotocoldialog.h"
#include "ui_currentresearchresultandprotocoldialog.h"

#include "puansonchecker.h"

CurrentResearchResultAndProtocolDialog::CurrentResearchResultAndProtocolDialog(const EtalonDetailDimensions &current_detail_dimensions, const EtalonDetailDimensions &ideal_detail_dimensions, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CurrentResearchResultAndProtocolDialog)
{
    ui->setupUi(this);

    const PuansonModel puanson_model = PuansonChecker::getInstance()->getEtalon().getDetailPuansonModel();

    drawIdealPuansonAndDimensionsGraphicsView(puanson_model);

    ui->idealPuansonAndDimensionsLabel->setText("Идеальный пуансон модели " + QString::number(static_cast<int>(puanson_model)) + " и его размеры");

    QString current_detail_protocol;
    current_detail_protocol += "<b>Диаметр d1</b>: " + QString::number(current_detail_dimensions.diameter_1_dimension) + "<br>";
    if(puanson_model == PuansonModel::PUANSON_MODEL_658)
    {
        current_detail_protocol += "<b>Диаметр d2</b>: " + QString::number(current_detail_dimensions.diameter_2_dimension) + "<br>";
        current_detail_protocol += "<b>Диаметр d3</b>: " + QString::number(current_detail_dimensions.diameter_3_dimension) + "<br>";
        current_detail_protocol += "<b>Диаметр d4</b>: " + QString::number(current_detail_dimensions.diameter_4_dimension) + "<br>";
    }
    current_detail_protocol += "<b>Диаметр d5</b>: " + QString::number(current_detail_dimensions.diameter_5_dimension) + "<br>";
    current_detail_protocol += "<b>Диаметр d6</b>: " + QString::number(current_detail_dimensions.diameter_6_dimension) + "<br>";
    current_detail_protocol += "<b>Диаметр d6(2)</b>: " + QString::number(current_detail_dimensions.diameter_6_2_dimension) + "<br>";
    current_detail_protocol += "<b>Диаметр d7</b>: " + QString::number(current_detail_dimensions.diameter_7_dimension) + "<br>";
    current_detail_protocol += "<b>Диаметр d7(2)</b>: " + QString::number(current_detail_dimensions.diameter_7_2_dimension) + "<br>";
    current_detail_protocol += "<b>Диаметр d8</b>: " + QString::number(current_detail_dimensions.diameter_8_dimension) + "<br>";
    current_detail_protocol += "<b>Диаметр d9</b>: " + QString::number(current_detail_dimensions.diameter_9_dimension) + "<br>";
    current_detail_protocol += "<b>Ширина паза у горла</b>: " + QString::number(current_detail_dimensions.groove_top_width_dimension) + "<br>";
    current_detail_protocol += "<b>Ширина паза у дна</b>: " + QString::number(current_detail_dimensions.groove_bottom_width_dimension) + "<br>";
    current_detail_protocol += "<b>Глубина паза у левой стенки</b>: " + QString::number(current_detail_dimensions.groove_left_depth_dimension) + "<br>";
    current_detail_protocol += "<b>Глубина паза у правой стенки</b>: " + QString::number(current_detail_dimensions.groove_right_depth_dimension) + "<br>";
    ui->currentDetailProtocolLabel->setText(current_detail_protocol);

    QString ideal_detail_protocol;
    ideal_detail_protocol += "<b>Диаметр d1</b>: " + QString::number(ideal_detail_dimensions.diameter_1_dimension) + "<br>";
    if(puanson_model == PuansonModel::PUANSON_MODEL_658)
    {
        ideal_detail_protocol += "<b>Диаметр d2</b>: " + QString::number(ideal_detail_dimensions.diameter_2_dimension) + "<br>";
        ideal_detail_protocol += "<b>Диаметр d3</b>: " + QString::number(ideal_detail_dimensions.diameter_3_dimension) + "<br>";
        ideal_detail_protocol += "<b>Диаметр d4</b>: " + QString::number(ideal_detail_dimensions.diameter_4_dimension) + "<br>";
    }
    ideal_detail_protocol += "<b>Диаметр d5</b>: " + QString::number(ideal_detail_dimensions.diameter_5_dimension) + "<br>";
    ideal_detail_protocol += "<b>Диаметр d6</b>: " + QString::number(ideal_detail_dimensions.diameter_6_dimension) + "<br>";
    ideal_detail_protocol += "<b>Диаметр d6(2)</b>: " + QString::number(ideal_detail_dimensions.diameter_6_2_dimension) + "<br>";
    ideal_detail_protocol += "<b>Диаметр d7</b>: " + QString::number(ideal_detail_dimensions.diameter_7_dimension) + "<br>";
    ideal_detail_protocol += "<b>Диаметр d7(2)</b>: " + QString::number(ideal_detail_dimensions.diameter_7_2_dimension) + "<br>";
    ideal_detail_protocol += "<b>Диаметр d8</b>: " + QString::number(ideal_detail_dimensions.diameter_8_dimension) + "<br>";
    ideal_detail_protocol += "<b>Диаметр d9</b>: " + QString::number(ideal_detail_dimensions.diameter_9_dimension) + "<br>";
    ideal_detail_protocol += "<b>Ширина паза</b>: " + QString::number(ideal_detail_dimensions.groove_width_dimension) + "<br>";
    ideal_detail_protocol += "<b>Глубина паза</b>: " + QString::number(ideal_detail_dimensions.groove_depth_dimension) + "<br>";
    ui->idealDetailProtocolLabel->setText(ideal_detail_protocol);

    connect(ui->okButton, SIGNAL(pressed()), SLOT(okButtonPressed()));
}

CurrentResearchResultAndProtocolDialog::~CurrentResearchResultAndProtocolDialog()
{
    delete ui;
}

void CurrentResearchResultAndProtocolDialog::drawIdealPuansonAndDimensionsGraphicsView(const PuansonModel puanson_model)
{
    QPainterPath idealContourPath, idealContourMeasurementsPath;
    PuansonImage::drawIdealContour(static_cast<PuansonModel>(static_cast<PuansonModel>(puanson_model)), QRect(2, 2, ui->idealPuansonAndDimensionsGraphicsView->width() - 2, ui->idealPuansonAndDimensionsGraphicsView->height() - 2), QPoint(ui->idealPuansonAndDimensionsGraphicsView->width() / 2, ui->idealPuansonAndDimensionsGraphicsView->height() / 2), -90, true, 0.025, PuansonImage::default_calibration_ratio, idealContourPath, idealContourMeasurementsPath);

    scene.clear();
    scene.setSceneRect(0, 0, ui->idealPuansonAndDimensionsGraphicsView->width() - 2, ui->idealPuansonAndDimensionsGraphicsView->height() - 2);
    QPixmap gray_pixmap(ui->idealPuansonAndDimensionsGraphicsView->width() - 2, ui->idealPuansonAndDimensionsGraphicsView->height() - 2);
    gray_pixmap.fill(Qt::lightGray);

    scene.addPixmap(gray_pixmap);

    if(ui->idealPuansonAndDimensionsGraphicsView->scene() == Q_NULLPTR)
    {
        ui->idealPuansonAndDimensionsGraphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
        ui->idealPuansonAndDimensionsGraphicsView->setScene(&scene);
    }

    /*ui->idealPuansonAndDimensionsGraphicsView->scene()->addRect(100, 50, 100, 100, QPen(Qt::darkGray, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QGraphicsTextItem *textItem = ui->idealPuansonAndDimensionsGraphicsView->scene()->addText("Ракурс", QFont("Noto Sans", 5, QFont::Bold));
    textItem->setDefaultTextColor(Qt::darkGray);
    textItem->setPos(100, 30);*/
    ui->idealPuansonAndDimensionsGraphicsView->scene()->addPath(idealContourPath, QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    ui->idealPuansonAndDimensionsGraphicsView->scene()->addPath(idealContourMeasurementsPath, QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

void CurrentResearchResultAndProtocolDialog::okButtonPressed()
{
    close();
}

