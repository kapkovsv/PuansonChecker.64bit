#ifndef PUANSONRESEARCH_H
#define PUANSONRESEARCH_H

#include <QtGlobal>
#include <QDateTime>
#include <QPair>

#include "puansonimage.h"

class PuansonResearch
{
    // Общие параметры
    quint8 number_of_angles = 0;
    PuansonModel detail_puanson_model = PuansonModel::PUANSON_MODEL_660;
    QDateTime date_time_of_creation;
    QString folder_path;

    // Параметры ракурсов
    QMap<quint8, QPoint> ideal_contour_point_of_origin_map;
    QMap<quint8, qreal> ideal_contour_rotation_angle_map;
    QMap<quint8, QPair<QPoint, QPoint>> calibration_reference_points_position_map;
    QMap<quint8, qreal> calibration_ratio_map;

    // Размеры детали
    EtalonDetailDimensions detail_dimesions;

public:
    PuansonResearch() = default;
    PuansonResearch(const quint8 number_of_angles, const PuansonModel detail_puanson_model, const QDateTime date_time_of_creation, const QString &folder_path);

    inline quint8 getNumberOfAngles() const
    {
        return number_of_angles;
    }

    inline PuansonModel getDetailPuansonModel() const
    {
        return detail_puanson_model;
    }

    inline QDateTime getDateTimeOfCreation() const
    {
        return date_time_of_creation;
    }

    inline QString getFolderPath() const
    {
        return folder_path;
    }

    bool getAngleParameters(const quint8 angle, QPoint &ideal_contour_point_of_origin, qreal &ideal_contour_rotation_angle, QPair<QPoint, QPoint> &calibration_reference_points_position, qreal &calibration_ratio) const;
    bool setAngleParameters(const quint8 angle, const QPoint &ideal_contour_point_of_origin, const qreal ideal_contour_rotation_angle, const QPair<QPoint, QPoint> &calibration_reference_points_position, const qreal calibration_ratio);

    bool loadEtalonAngle(const quint8 angle);

    inline EtalonDetailDimensions getDetailDimensions() const
    {
        return detail_dimesions;
    }

    inline void setDetailDimensions(const EtalonDetailDimensions &dimensions)
    {
        detail_dimesions = dimensions;
    }

    void updateCalibrationRatio();
};

#endif // PUANSONRESEARCH_H
