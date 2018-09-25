#include "generalsettings.h"
#include <QtXml>

GeneralSettings::GeneralSettings(const QString &config_file)
{
    xml_configuration_file = config_file;

    loadSettingsFromConfigFile();
}

void GeneralSettings::resetSettings()
{
    referencePointDistanceMkm = 0;

    detailPhotoShootingPositions.clear();
    memset(internalToleranceMkmArray, 0, sizeof(internalToleranceMkmArray));
    memset(externalToleranceMkmArray, 0, sizeof(externalToleranceMkmArray));
}

bool GeneralSettings::loadSettingsFromConfigFile()
{
    QFile config_file(xml_configuration_file);

    if(!config_file.open(QIODevice::ReadOnly))
    {
        //qDebug() << "Configuration file opening error";
        return false;
    }

    resetSettings();

    QXmlStreamReader xml(&config_file);
    quint8 angle = 0;

    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartDocument)
            continue;

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "puanson_checker_settings")
                continue;

            if (xml.name() == "reference_points_distance_mkm")
            {
                if(xml.attributes().hasAttribute("value"))
                    referencePointDistanceMkm = xml.attributes().value("value").toString().toUInt();
            }
            else if (xml.name() == "etalon_settings")
            {
                QXmlStreamAttributes attributes = xml.attributes();
                if (attributes.hasAttribute("angle"))
                    angle = static_cast<quint8>(attributes.value("angle").toString().toUShort());
                xml.readNext();

                while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "etalon_settings"))
                {
                    if (angle >= 1 && angle <= PUANSON_IMAGE_MAX_ANGLE && xml.tokenType() == QXmlStreamReader::StartElement)
                    {
                        if (xml.name() == "internal_tolerance")
                        {
                            if(xml.attributes().hasAttribute("value"))
                                internalToleranceMkmArray[angle-1] = xml.attributes().value("value").toString().toUShort();
                        }
                        else if (xml.name() == "external_tolerance")
                        {
                            if(xml.attributes().hasAttribute("value"))
                                externalToleranceMkmArray[angle-1] = xml.attributes().value("value").toString().toUShort();
                        }
                        else if (xml.name() == "detail_photo_shooting_position")
                        {
                            if(xml.attributes().hasAttribute("x") && xml.attributes().hasAttribute("y"))
                                detailPhotoShootingPositions.insert(angle, QPoint(xml.attributes().value("x").toString().toInt(), xml.attributes().value("y").toString().toInt()));
                        }
                    }
                    xml.readNext();
                }
            }
        }
    }

    config_file.close();
    return true;
}

bool GeneralSettings::saveSettingsToConfigFile()
{
    QFile config_file(xml_configuration_file);

    if(!config_file.open(QIODevice::WriteOnly))
    {
        config_file.close();
        return false;
    }

    QXmlStreamWriter stream(&config_file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeStartElement("puanson_checker_settings");

    stream.writeStartElement("reference_points_distance_mkm");
    stream.writeAttribute("value", QString::number(referencePointDistanceMkm));
    stream.writeEndElement(); // reference_points_distance_mkm

    for(quint8 angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
    {
        stream.writeStartElement("etalon_settings");
        stream.writeAttribute("angle", QString::number(angle));

        stream.writeStartElement("internal_tolerance");
        stream.writeAttribute("value", QString::number(internalToleranceMkmArray[angle-1]));
        stream.writeEndElement(); // internal_tolerance

        stream.writeStartElement("external_tolerance");
        stream.writeAttribute("value", QString::number(externalToleranceMkmArray[angle-1]));
        stream.writeEndElement(); // external_tolerance

        stream.writeStartElement("detail_photo_shooting_position");
        stream.writeAttribute("x", QString::number(detailPhotoShootingPositions[angle].x()));
        stream.writeAttribute("y", QString::number(detailPhotoShootingPositions[angle].y()));
        stream.writeEndElement(); // external_tolerance

        stream.writeEndElement(); // etalon_settings
    }

    stream.writeEndElement(); // puanson_checker_settings
    stream.writeEndDocument();

    config_file.close();

    return true;
}

bool GeneralSettings::setDetailPhotoShootingPositions(const QMap<quint8, QPoint> &new_positions)
{
    if(new_positions.size() == NUMBER_OF_ETALON_ANGLES)
    {
        detailPhotoShootingPositions = new_positions;
        return true;
    }

    return false;
}

QMap<quint8, QPoint> GeneralSettings::getDetailPhotoShootingPositions()
{
    return detailPhotoShootingPositions;
}

void GeneralSettings::setToleranceFields(const quint16 ext_tolerance_px_array[], const quint16 int_tolerance_px_array[])
{
    memcpy(externalToleranceMkmArray, ext_tolerance_px_array, sizeof(quint16) * PUANSON_IMAGE_MAX_ANGLE);
    memcpy(internalToleranceMkmArray, int_tolerance_px_array, sizeof(quint16) * PUANSON_IMAGE_MAX_ANGLE);
}

void GeneralSettings::getToleranceFields(quint16 ext_tolerance_px_array[], quint16 int_tolerance_px_array[]) const
{
    memcpy(ext_tolerance_px_array, externalToleranceMkmArray, sizeof(quint16) * PUANSON_IMAGE_MAX_ANGLE);
    memcpy(int_tolerance_px_array, internalToleranceMkmArray, sizeof(quint16) * PUANSON_IMAGE_MAX_ANGLE);
}

void GeneralSettings::setReferencePointDistancesMkm(const quint32 distance)
{
    referencePointDistanceMkm = distance;
}

void GeneralSettings::getReferencePointDistanceMkm(quint32 &distance) const
{
    distance = referencePointDistanceMkm;
}
