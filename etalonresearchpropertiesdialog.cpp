#include "etalonresearchpropertiesdialog.h"
#include "ui_etalonresearchpropertiesdialog.h"

EtalonResearchPropertiesDialog::EtalonResearchPropertiesDialog(const PuansonResearch &etalon_research, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EtalonResearchPropertiesDialog)
{
    ui->setupUi(this);

    PuansonModel puanson_model = etalon_research.getDetailPuansonModel();

    ui->pathValueLineEdit->setText(etalon_research.getFolderPath());
    ui->puansonModelValueLabel->setText(QString::number(static_cast<int>(puanson_model)));
    ui->creationDateTimeEdit->setDateTime(etalon_research.getDateTimeOfCreation());
    ui->numberOfAnglesValueLabel->setText(QString::number(etalon_research.getNumberOfAngles()));

    // Размеры детали
    EtalonDetailDimensions detail_dimensions = etalon_research.getDetailDimensions();

    ui->dimensionDiameter1SpinBox->setValue(detail_dimensions.diameter_1_dimension);
    ui->dimensionDiameter2SpinBox->setValue(detail_dimensions.diameter_2_dimension);
    ui->dimensionDiameter3SpinBox->setValue(detail_dimensions.diameter_3_dimension);
    ui->dimensionDiameter4SpinBox->setValue(detail_dimensions.diameter_4_dimension);
    ui->dimensionDiameter5SpinBox->setValue(detail_dimensions.diameter_5_dimension);
    ui->dimensionDiameter6SpinBox->setValue(detail_dimensions.diameter_6_dimension);
    ui->dimensionDiameter7SpinBox->setValue(detail_dimensions.diameter_7_dimension);
    ui->dimensionDiameter8SpinBox->setValue(detail_dimensions.diameter_8_dimension);
    ui->dimensionDiameter9SpinBox->setValue(detail_dimensions.diameter_9_dimension);
    ui->dimensionDiameter10SpinBox->setValue(0);
    ui->dimensionGrooveWidthSpinBox->setValue(detail_dimensions.groove_width_dimension);
    ui->dimensionGrooveDepthSpinBox->setValue(detail_dimensions.groove_depth_dimension);
    ////////////////////////////////////

    QString ideal_contour_origin_status_str[PUANSON_IMAGE_MAX_ANGLE];
    QString ideal_contour_rotation_angle_status_str[PUANSON_IMAGE_MAX_ANGLE];

    QString calibration_reference_points_position_status_str[PUANSON_IMAGE_MAX_ANGLE];
    QString calibration_ratio_status_str[PUANSON_IMAGE_MAX_ANGLE];

    QPoint ideal_contour_point_of_origin;
    qreal ideal_contour_rotation_angle;

    QPair<QPoint, QPoint> calibration_reference_points_position;
    qreal calibration_ratio;

    for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
    {
        if(etalon_research.getAngleParameters(angle, ideal_contour_point_of_origin, ideal_contour_rotation_angle, calibration_reference_points_position, calibration_ratio))
        {
            ideal_contour_origin_status_str[angle-1] = "(" + QString::number(ideal_contour_point_of_origin.x()) + ", " + QString::number(ideal_contour_point_of_origin.y()) + ")";
            ideal_contour_rotation_angle_status_str[angle-1] = QString::number(ideal_contour_rotation_angle) + QChar(176);

            calibration_reference_points_position_status_str[angle-1] = "(" + QString::number(calibration_reference_points_position.first.x()) + ", " + QString::number(calibration_reference_points_position.first.y()) + "), " +
                    "(" + QString::number(calibration_reference_points_position.second.x()) + ", " + QString::number(calibration_reference_points_position.second.y()) + ")";
            calibration_ratio_status_str[angle-1] = QString::number(calibration_ratio) + " px/мкм";
        }
        else
        {
            ideal_contour_origin_status_str[angle-1] = "Не задано";
            ideal_contour_rotation_angle_status_str[angle-1] = "Не задано";

            calibration_reference_points_position_status_str[angle-1] = "Не задано";
            calibration_ratio_status_str[angle-1] = "Не задано";
        }
    }

    // Позиции идеальных контуров для ракурсов
    ui->idealContourPositionAngle1OriginStatusLabel->setText(ideal_contour_origin_status_str[0]);
    ui->idealContourPositionAngle1RotationAngleStatusLabel->setText(ideal_contour_rotation_angle_status_str[0]);
    ui->idealContourPositionAngle2OriginStatusLabel->setText(ideal_contour_origin_status_str[1]);
    ui->idealContourPositionAngle2RotationAngleStatusLabel->setText(ideal_contour_rotation_angle_status_str[1]);
    ui->idealContourPositionAngle3OriginStatusLabel->setText(ideal_contour_origin_status_str[2]);
    ui->idealContourPositionAngle3RotationAngleStatusLabel->setText(ideal_contour_rotation_angle_status_str[2]);
    ui->idealContourPositionAngle4OriginStatusLabel->setText(ideal_contour_origin_status_str[3]);
    ui->idealContourPositionAngle4RotationAngleStatusLabel->setText(ideal_contour_rotation_angle_status_str[3]);
    ui->idealContourPositionAngle5OriginStatusLabel->setText(ideal_contour_origin_status_str[4]);
    ui->idealContourPositionAngle5RotationAngleStatusLabel->setText(ideal_contour_rotation_angle_status_str[4]);
    ui->idealContourPositionAngle6OriginStatusLabel->setText(ideal_contour_origin_status_str[5]);
    ui->idealContourPositionAngle6RotationAngleStatusLabel->setText(ideal_contour_rotation_angle_status_str[5]);
    ui->idealContourPositionAngle7OriginStatusLabel->setText("Не задано");
    ui->idealContourPositionAngle7RotationAngleStatusLabel->setText("Не задано");
    ui->idealContourPositionAngle8OriginStatusLabel->setText("Не задано");
    ui->idealContourPositionAngle8RotationAngleStatusLabel->setText("Не задано");
    ui->idealContourPositionAngle9OriginStatusLabel->setText("Не задано");
    ui->idealContourPositionAngle9RotationAngleStatusLabel->setText("Не задано");
    ui->idealContourPositionAngle10OriginStatusLabel->setText("Не задано");
    ui->idealContourPositionAngle10RotationAngleStatusLabel->setText("Не задано");
    ui->idealContourPositionAngle11OriginStatusLabel->setText("Не задано");
    ui->idealContourPositionAngle11RotationAngleStatusLabel->setText("Не задано");
    ui->idealContourPositionAngle12OriginStatusLabel->setText("Не задано");
    ui->idealContourPositionAngle12RotationAngleStatusLabel->setText("Не задано");
    ////////////////////////////////////

    // Позиции реперных точек и соотношение пикс / мкм для ракурсов
    ui->referencePointsAngle1RatioStatusLabel->setText(calibration_ratio_status_str[0]);
    ui->referencePointsAngle1PositionsStatusLabel->setText(calibration_reference_points_position_status_str[0]);
    ui->referencePointsAngle2RatioStatusLabel->setText(calibration_ratio_status_str[1]);
    ui->referencePointsAngle2PositionsStatusLabel->setText(calibration_reference_points_position_status_str[1]);
    ui->referencePointsAngle3RatioStatusLabel->setText(calibration_ratio_status_str[2]);
    ui->referencePointsAngle3PositionsStatusLabel->setText(calibration_reference_points_position_status_str[2]);
    ui->referencePointsAngle4RatioStatusLabel->setText(calibration_ratio_status_str[3]);
    ui->referencePointsAngle4PositionsStatusLabel->setText(calibration_reference_points_position_status_str[3]);
    ui->referencePointsAngle5RatioStatusLabel->setText(calibration_ratio_status_str[4]);
    ui->referencePointsAngle5PositionsStatusLabel->setText(calibration_reference_points_position_status_str[4]);
    ui->referencePointsAngle6RatioStatusLabel->setText(calibration_ratio_status_str[5]);
    ui->referencePointsAngle6PositionsStatusLabel->setText(calibration_reference_points_position_status_str[5]);
    ui->referencePointsAngle7RatioStatusLabel->setText("Не задано");
    ui->referencePointsAngle7PositionsStatusLabel->setText("Не задано");
    ui->referencePointsAngle8RatioStatusLabel->setText("Не задано");
    ui->referencePointsAngle8PositionsStatusLabel->setText("Не задано");
    ui->referencePointsAngle9RatioStatusLabel->setText("Не задано");
    ui->referencePointsAngle9PositionsStatusLabel->setText("Не задано");
    ui->referencePointsAngle10RatioStatusLabel->setText("Не задано");
    ui->referencePointsAngle10PositionsStatusLabel->setText("Не задано");
    ui->referencePointsAngle11RatioStatusLabel->setText("Не задано");
    ui->referencePointsAngle11PositionsStatusLabel->setText("Не задано");
    ui->referencePointsAngle12RatioStatusLabel->setText("Не задано");
    ui->referencePointsAngle12PositionsStatusLabel->setText("Не задано");
    ///////////////////////////////////

    drawIdealPuansonAndDimensionsGraphicsView(puanson_model);

    connect(ui->okButton, SIGNAL(pressed()), SLOT(okButtonPressed()));
}

EtalonResearchPropertiesDialog::~EtalonResearchPropertiesDialog()
{
    delete ui;
}

void EtalonResearchPropertiesDialog::drawIdealPuansonAndDimensionsGraphicsView(const PuansonModel puanson_model)
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

void EtalonResearchPropertiesDialog::okButtonPressed()
{
    close();
}
