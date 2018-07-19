#include "generalsettings.h"
#include <QtXml>

GeneralSettings::GeneralSettings(const QString &config_file)
{
    xml_configuration_file = config_file;

    memset(externalToleranceMkmArray, 0, sizeof(externalToleranceMkmArray));
    memset(internalToleranceMkmArray, 0, sizeof(internalToleranceMkmArray));
    memset(referencePointDistancesMkmArray, 0, sizeof(referencePointDistancesMkmArray));

    loadSettingsFromConfigFile();
}

bool GeneralSettings::loadSettingsFromConfigFile()
{
    QFile config_file(xml_configuration_file);

    if(!config_file.open(QIODevice::ReadOnly))
    {
        //qDebug() << "Configuration file opening error";
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
                                internalToleranceMkmArray[angle-1] = xml.attributes().value("value").toString().toUShort();
                        }
                        else if (xml.name() == "external_tolerance")
                        {
                            if(xml.attributes().hasAttribute("value"))
                                externalToleranceMkmArray[angle-1] = xml.attributes().value("value").toString().toUShort();
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

            if (xml.name() == "left_bottom_reference_point_etalon")
            {
                if(xml.attributes().hasAttribute("filepath"))
                    left_bottom_reference_point_etalon_filename = xml.attributes().value("filepath").toString();
            }

            if (xml.name() == "right_top_reference_point_etalon")
            {
                if(xml.attributes().hasAttribute("filepath"))
                    right_top_reference_point_etalon_filename = xml.attributes().value("filepath").toString();
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
        stream.writeAttribute("value", QString::number(internalToleranceMkmArray[angle-1]));
        stream.writeEndElement(); // internal_tolerance

        stream.writeStartElement("external_tolerance");
        stream.writeAttribute("value", QString::number(externalToleranceMkmArray[angle-1]));
        stream.writeEndElement(); // external_tolerance

        stream.writeStartElement("reference_points_distance_mkm");
        stream.writeAttribute("value", QString::number(referencePointDistancesMkmArray[angle-1]));
        stream.writeEndElement(); // reference_points_distance_mkm

        stream.writeEndElement(); // etalon_settings
    }

    stream.writeStartElement("left_bottom_reference_point_etalon");
    stream.writeAttribute("filepath", left_bottom_reference_point_etalon_filename);
    stream.writeEndElement(); // left_bottom_reference_point_etalon

    stream.writeStartElement("right_top_reference_point_etalon");
    stream.writeAttribute("filepath", right_top_reference_point_etalon_filename);
    stream.writeEndElement(); // right_top_reference_point_etalon
    qDebug() << " left_bottom_reference_point_etalon_filename " << left_bottom_reference_point_etalon_filename << " right_top_reference_point_etalon_filename " << right_top_reference_point_etalon_filename;
    stream.writeEndElement(); // puanson_checker_settings
    stream.writeEndDocument();

    config_file.close();

    return true;
}

void GeneralSettings::setToleranceFields(const quint16 ext_tolerance_px_array[], const quint16 int_tolerance_px_array[])
{
    memcpy(externalToleranceMkmArray, ext_tolerance_px_array, sizeof(quint16) * NUMBER_OF_ANGLES);
    memcpy(internalToleranceMkmArray, int_tolerance_px_array, sizeof(quint16) * NUMBER_OF_ANGLES);
}

void GeneralSettings::getToleranceFields(quint16 ext_tolerance_px_array[], quint16 int_tolerance_px_array[])
{
    memcpy(ext_tolerance_px_array, externalToleranceMkmArray, sizeof(quint16) * NUMBER_OF_ANGLES);
    memcpy(int_tolerance_px_array, internalToleranceMkmArray, sizeof(quint16) * NUMBER_OF_ANGLES);
}

void GeneralSettings::setReferencePointDistancesMkm(const quint32 distance_array[])
{
    memcpy(referencePointDistancesMkmArray, distance_array, sizeof(quint32) * NUMBER_OF_ANGLES);
}

void GeneralSettings::getReferencePointDistancesMkm(quint32 distance_array[])
{
    memcpy(distance_array, referencePointDistancesMkmArray, sizeof(quint32) * NUMBER_OF_ANGLES);
}

void GeneralSettings::setReferencePointEtalonFilenames(const QString &_left_bottom_reference_point_etalon_filename, const QString &_right_top_reference_point_etalon_filename)
{
    left_bottom_reference_point_etalon_filename = _left_bottom_reference_point_etalon_filename;
    right_top_reference_point_etalon_filename = _right_top_reference_point_etalon_filename;
}

void GeneralSettings::getReferencePointEtalonFilenames(QString &_left_bottom_reference_point_etalon_filename, QString &_right_top_reference_point_etalon_filename)
{
    _left_bottom_reference_point_etalon_filename = left_bottom_reference_point_etalon_filename;
    _right_top_reference_point_etalon_filename = right_top_reference_point_etalon_filename;
}
