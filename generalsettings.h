#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include "puansonchecker.h"

#include <QMap>

class GeneralSettings
{
    QString xml_configuration_file;

    quint16 externalToleranceMkmArray[PUANSON_IMAGE_MAX_ANGLE] = { 0 };
    quint16 internalToleranceMkmArray[PUANSON_IMAGE_MAX_ANGLE] = { 0 };
    QMap<quint8, QPoint> detailPhotoShootingPositions;

    quint32 referencePointDistanceMkm = 0;

    void resetSettings();

public:
    GeneralSettings(const QString &config_file);

    bool loadSettingsFromConfigFile();
    bool saveSettingsToConfigFile();

    bool setDetailPhotoShootingPositions(const QMap<quint8, QPoint> &new_positions);
    QMap<quint8, QPoint> getDetailPhotoShootingPositions();

    void setToleranceFields(const quint16 ext_tolerance_px_array[], const quint16 int_tolerance_px_array[]);
    void getToleranceFields(quint16 ext_tolerance_array[], quint16 int_tolerance_array[]) const;

    void setReferencePointDistancesMkm(const quint32 distance);
    void getReferencePointDistanceMkm(quint32 &distance) const;
};

#endif // GENERALSETTINGS_H
