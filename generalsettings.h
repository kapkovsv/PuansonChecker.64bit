#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include "puansonchecker.h"

class GeneralSettings
{
    QString xml_configuration_file;

    quint16 externalTolerancePxArray[NUMBER_OF_ANGLES];
    quint16 internalTolerancePxArray[NUMBER_OF_ANGLES];

    quint32 referencePointDistancesMkmArray[NUMBER_OF_ANGLES];

public:
    GeneralSettings(const QString &config_file);

    bool loadSettingsFromConfigFile();
    bool saveSettingsToConfigFile();

    void setToleranceFields(const quint16 ext_tolerance_px_array[], const quint16 int_tolerance_px_array[]);
    void getToleranceFields(quint16 ext_tolerance_array[], quint16 int_tolerance_array[]);

    void setReferencePointDistancesMkm(const quint32 distance_array[]);
    void getReferencePointDistancesMkm(quint32 distance_array[]);
};

#endif // GENERALSETTINGS_H
