#include "generalsettings.h"

#include <QtXml>

GeneralSettings::GeneralSettings(const QString &config_file)
{
    xml_configuration_file = config_file;

    memset(externalTolerancePxArray, 0, sizeof(externalTolerancePxArray));
    memset(internalTolerancePxArray, 0, sizeof(internalTolerancePxArray));
    memset(referencePointDistancesMkmArray, 0, sizeof(referencePointDistancesMkmArray));

    loadSettingsFromConfigFile();
}

bool GeneralSettings::loadSettingsFromConfigFile()
{
    QFile config_file(xml_configuration_file);

    if(!config_file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Configuration file opening error";
        return false;
    }

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

            if (xml.name() == "etalon_settings")
            {
                QXmlStreamAttributes attributes = xml.attributes();
                if (attributes.hasAttribute("angle"))
                    angle = attributes.value("angle").toString().toUShort();
                xml.readNext();

                while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "etalon_settings"))
                {
                    if (angle >= 1 && angle <= NUMBER_OF_ANGLES && xml.tokenType() == QXmlStreamReader::StartElement)
                    {
                        if (xml.name() == "internal_tolerance")
                        {
                            if(xml.attributes().hasAttribute("value"))
                                internalTolerancePxArray[angle-1] = xml.attributes().value("value").toString().toUShort();
                        }
                        else if (xml.name() == "external_tolerance")
                        {
                            if(xml.attributes().hasAttribute("value"))
                                externalTolerancePxArray[angle-1] = xml.attributes().value("value").toString().toUShort();
                        }
                        else if (xml.name() == "reference_points_distance_mkm")
                        {
                            if(xml.attributes().hasAttribute("value"))
                                referencePointDistancesMkmArray[angle-1] = xml.attributes().value("value").toString().toUInt();
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

    for(quint8 angle = 1; angle <= NUMBER_OF_ANGLES; angle++)
    {
        stream.writeStartElement("etalon_settings");
        stream.writeAttribute("angle", QString::number(angle));

        stream.writeStartElement("internal_tolerance");
        stream.writeAttribute("value", QString::number(internalTolerancePxArray[angle-1]));
        stream.writeEndElement(); // internal_tolerance

        stream.writeStartElement("external_tolerance");
        stream.writeAttribute("value", QString::number(externalTolerancePxArray[angle-1]));
        stream.writeEndElement(); // external_tolerance

        stream.writeStartElement("reference_points_distance_mkm");
        stream.writeAttribute("value", QString::number(referencePointDistancesMkmArray[angle-1]));
        stream.writeEndElement(); // reference_points_distance_mkm

        stream.writeEndElement(); // etalon_settings
    }

    stream.writeEndElement(); // puanson_checker_settings
    stream.writeEndDocument();

    config_file.close();
}

void GeneralSettings::setToleranceFields(const quint16 ext_tolerance_px_array[], const quint16 int_tolerance_px_array[])
{
    memcpy(externalTolerancePxArray, ext_tolerance_px_array, sizeof(quint16) * NUMBER_OF_ANGLES);
    memcpy(internalTolerancePxArray, int_tolerance_px_array, sizeof(quint16) * NUMBER_OF_ANGLES);
}

void GeneralSettings::getToleranceFields(quint16 ext_tolerance_px_array[], quint16 int_tolerance_px_array[])
{
    memcpy(ext_tolerance_px_array, externalTolerancePxArray, sizeof(quint16) * NUMBER_OF_ANGLES);
    memcpy(int_tolerance_px_array, internalTolerancePxArray, sizeof(quint16) * NUMBER_OF_ANGLES);
}

void GeneralSettings::setReferencePointDistancesMkm(const quint32 distance_array[])
{
    memcpy(referencePointDistancesMkmArray, distance_array, sizeof(quint32) * NUMBER_OF_ANGLES);
}

void GeneralSettings::getReferencePointDistancesMkm(quint32 distance_array[])
{
    memcpy(distance_array, referencePointDistancesMkmArray, sizeof(quint32) * NUMBER_OF_ANGLES);
}
