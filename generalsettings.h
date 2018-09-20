#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include "puansonchecker.h"

class GeneralSettings
{
    QString xml_configuration_file;

    quint16 externalToleranceMkmArray[PUANSON_IMAGE_MAX_ANGLE];
    quint16 internalToleranceMkmArray[PUANSON_IMAGE_MAX_ANGLE];

    quint32 referencePointDistancesMkmArray[PUANSON_IMAGE_MAX_ANGLE];

public:
    GeneralSettings(const QString &config_file);

    bool loadSettingsFromConfigFile();
    bool saveSettingsToConfigFile();

    void setToleranceFields(const quint16 ext_tolerance_px_array[], const quint16 int_tolerance_px_array[]);
    void getToleranceFields(quint16 ext_tolerance_array[], quint16 int_tolerance_array[]) const;

    void setReferencePointDistancesMkm(const quint32 distance_array[]);
    void getReferencePointDistancesMkm(quint32 distance_array[]) const;
};

#endif // GENERALSETTINGS_H
