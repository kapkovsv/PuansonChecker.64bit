#include "puansonresearch.h"
#include "puansonchecker.h"

PuansonResearch::PuansonResearch(const quint8 number_of_angles, const PuansonModel detail_puanson_model, const QDateTime date_time_of_creation, const QString &folder_path):
    number_of_angles(number_of_angles), detail_puanson_model(detail_puanson_model), date_time_of_creation(date_time_of_creation), folder_path(folder_path)
{
}

PuansonResearch::~PuansonResearch()
{
}

bool PuansonResearch::getAngleParameters(const quint8 angle, QPoint &ideal_contour_point_of_origin, qreal &ideal_contour_rotation_angle, QPair<QPoint, QPoint> &calibration_reference_points_position, qreal &calibration_ratio) const
{
    if(angle >= 1 && angle <= PUANSON_IMAGE_MAX_ANGLE)
    {
        if(ideal_contour_point_of_origin_map.contains(angle) &&
                ideal_contour_rotation_angle_map.contains(angle) &&
                calibration_reference_points_position_map.contains(angle) && calibration_ratio_map.contains(angle))
        {
            ideal_contour_point_of_origin = ideal_contour_point_of_origin_map[angle];
            ideal_contour_rotation_angle = ideal_contour_rotation_angle_map[angle];
            calibration_reference_points_position = calibration_reference_points_position_map[angle];
            calibration_ratio = calibration_ratio_map[angle];

            return true;
        }
    }

    return false;
}

bool PuansonResearch::setAngleParameters(const quint8 angle, const QPoint &ideal_contour_point_of_origin, const qreal ideal_contour_rotation_angle, const QPair<QPoint, QPoint> &calibration_reference_points_position, const qreal calibration_ratio)
{
    if(angle >= 1 && angle <= PUANSON_IMAGE_MAX_ANGLE)
    {
            ideal_contour_point_of_origin_map.insert(angle, ideal_contour_point_of_origin);
            ideal_contour_rotation_angle_map.insert(angle, ideal_contour_rotation_angle);
            calibration_reference_points_position_map.insert(angle, calibration_reference_points_position);
            calibration_ratio_map.insert(angle, calibration_ratio);

            return true;
    }

    return false;
}

void PuansonResearch::updateCalibrationRatio()
{
    // Расстояние между реперными точками
    quint32 reference_points_distance_mkm;
    PuansonChecker::getInstance()->getGeneralSettings()->getReferencePointDistanceMkm(reference_points_distance_mkm);

    for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
        calibration_ratio_map[angle] = reference_points_distance_mkm / QLineF(calibration_reference_points_position_map[angle].first, calibration_reference_points_position_map[angle].second).length();
}

bool PuansonResearch::loadEtalonAngle(const quint8 angle)
{
    using namespace std;
    using namespace cv;

    const QString xml_config_filename = "etalon_angle_configuration.xml";
    QFile config_file(folder_path + "/angle_" + QString::number(angle) + "/" + xml_config_filename);

    if(!config_file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Load angle error: Configuration file opening error";
        return false;
    }

    QPoint ideal_contour_point_of_origin;
    qreal ideal_contour_rotation_angle = 0.0;
    QPair<QPoint, QPoint> calibration_reference_points_position;

    QXmlStreamReader xml(&config_file);
    quint8 loaded_parameters = 0;

    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartDocument)
            continue;

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "etalon_angle_settings")
                continue;

#if 0
            if (xml.name() == "original_image")
            {
                if(xml.attributes().hasAttribute("filename"))
                {
                    original_path = xml.attributes().value("filename").toString();
                    loaded_parameters++;
                }
            }
            else if (xml.name() == "contours_image")
            {
                if(xml.attributes().hasAttribute("filename"))
                {
                    contours_path = xml.attributes().value("filename").toString();
                    loaded_parameters++;
                }
            }
            else
#endif // 0
            if (xml.name() == "left_bottom_reference_point_position")
            {
                if(xml.attributes().hasAttribute("x") && xml.attributes().hasAttribute("y"))
                {
                    calibration_reference_points_position.first = QPoint(xml.attributes().value("x").toString().toInt(), xml.attributes().value("y").toString().toInt());
                    loaded_parameters++;
                }
            }
            else if (xml.name() == "right_top_reference_point_position")
            {
                if(xml.attributes().hasAttribute("x") && xml.attributes().hasAttribute("y"))
                {
                    calibration_reference_points_position.second = QPoint(xml.attributes().value("x").toString().toInt(), xml.attributes().value("y").toString().toInt());
                    loaded_parameters++;
                }
            }
#if 0
            else if (xml.name() == "left_bottom_reference_point_search_area")
            {
                if(xml.attributes().hasAttribute("x") && xml.attributes().hasAttribute("y") && xml.attributes().hasAttribute("width") && xml.attributes().hasAttribute("height"))
                {
                    PuansonChecker::getInstance()->setReferencePointSearchArea(ReferencePointType_e::REFERENCE_POINT_1, QRect(xml.attributes().value("x").toString().toInt(), xml.attributes().value("y").toString().toInt(), xml.attributes().value("width").toString().toInt(), xml.attributes().value("height").toString().toInt()));
                    loaded_parameters++;
                }
            }
            else if (xml.name() == "right_top_reference_point_search_area")
            {
                if(xml.attributes().hasAttribute("x") && xml.attributes().hasAttribute("y") && xml.attributes().hasAttribute("width") && xml.attributes().hasAttribute("height"))
                {
                    PuansonChecker::getInstance()->setReferencePointSearchArea(ReferencePointType_e::REFERENCE_POINT_2, QRect(xml.attributes().value("x").toString().toInt(), xml.attributes().value("y").toString().toInt(), xml.attributes().value("width").toString().toInt(), xml.attributes().value("height").toString().toInt()));
                    loaded_parameters++;
                }
            } else
#endif // 0
            if (xml.name() == "ideal_contour_position")
            {
                xml.readNext();

                while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "ideal_contour_position"))
                {
                    if (xml.tokenType() == QXmlStreamReader::StartElement)
                    {
                        if (xml.name() == "point_of_origin_position")
                        {
                            if(xml.attributes().hasAttribute("x") && xml.attributes().hasAttribute("y"))
                            {
                                ideal_contour_point_of_origin = QPoint(xml.attributes().value("x").toString().toInt(), xml.attributes().value("y").toString().toInt());
                                loaded_parameters++;
                            }
                        }
                        else if (xml.name() == "rotation_angle")
                        {
                            if(xml.attributes().hasAttribute("degrees"))
                            {
                                ideal_contour_rotation_angle = xml.attributes().value("degrees").toString().toDouble();
                                loaded_parameters++;
                            }
                        }
                    }
                    xml.readNext();
                }
            }
#if 0
            else if (xml.name() == "detail_check_result")
            {
                if(xml.attributes().hasAttribute("suitability"))
                {
                    // current detail is valid
                    if(xml.attributes().value("suitability") == "yes")
                        isCurrentDetailSuitable = true;
                    else if(xml.attributes().value("suitability") == "no")
                        isCurrentDetailSuitable = false;

                    image_type = ImageType_e::CURRENT_IMAGE;
                }
            }
#endif // 0
        }
    }

    config_file.close();

    // Расстояние между реперными точками
    quint32 reference_points_distance_mkm;
    PuansonChecker::getInstance()->getGeneralSettings()->getReferencePointDistanceMkm(reference_points_distance_mkm);

    if(loaded_parameters == ETALON_ANGLE_FULL_NUMBER_OF_PARAMETERS - 4)
    {
        setAngleParameters(angle, ideal_contour_point_of_origin, ideal_contour_rotation_angle, calibration_reference_points_position, reference_points_distance_mkm / QLineF(calibration_reference_points_position.first, calibration_reference_points_position.second).length());
        return true;
    }
    else
        return false;
}
