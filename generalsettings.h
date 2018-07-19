#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include "puansonchecker.h"

class GeneralSettings
{
    QString xml_configuration_file;

    quint16 externalToleranceMkmArray[NUMBER_OF_ANGLES];
    quint16 internalToleranceMkmArray[NUMBER_OF_ANGLES];

    quint32 referencePointDistancesMkmArray[NUMBER_OF_ANGLES];

    QString left_bottom_reference_point_etalon_filename;
    QString right_top_reference_point_etalon_filename;

public:
    GeneralSettings(const QString &config_file);

    bool loadSettingsFromConfigFile();
    bool saveSettingsToConfigFile();

    void setToleranceFields(const quint16 ext_tolerance_px_array[], const quint16 int_tolerance_px_array[]);
    void getToleranceFields(quint16 ext_tolerance_array[], quint16 int_tolerance_array[]);

    void setReferencePointDistancesMkm(const quint32 distance_array[]);
    void getReferencePointDistancesMkm(quint32 distance_array[]);

    void setReferencePointEtalonFilenames(const QString &_left_bottom_reference_point_etalon_filename, const QString &_right_top_reference_point_etalon_filename);
    void getReferencePointEtalonFilenames(QString &_left_bottom_reference_point_etalon_filename, QString &_right_top_reference_point_etalon_filename);
};

#endif // GENERALSETTINGS_H
