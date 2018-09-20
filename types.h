#ifndef TYPES_H
#define TYPES_H

enum class PuansonModel
{
    PUANSON_MODEL_658 = 658,
    PUANSON_MODEL_660 = 660,
    PUANSON_MODEL_661 = 661
};

enum class ReferencePointType_e
{
    REFERENCE_POINT_1 = 1,
    REFERENCE_POINT_2 = 2,
};

enum class CalibrationMode_e
{
    NO_CALIBRATION = 0,
    // Реперные точки
    REFERENCE_POINT_1 = 1,
    REFERENCE_POINT_2 = 2,
    // Области автоматического поиска реперных точек
    REFERENCE_POINT_1_AREA = 3,
    REFERENCE_POINT_2_AREA = 4,
    // Идеальный контур
    IDEAL_IMPOSE = 14,
    // Ручная установка точек контуров для измерения
    MANUAL_SETTING_REFERENCE_POINTS = 20
};

enum class ImageType_e
{
    ETALON_IMAGE = 0,
    CURRENT_IMAGE = 1,
    UNDEFINED_IMAGE = 3
};

enum class ContourPoints_e
{
    OUTER_TOLERANCE_ETALON_CONTOUR_POINT = 0,
    INNER_TOLERANCE_ETALON_CONTOUR_POINT = 1,
    ETALON_DETAIL_CONTOUR_POINT = 2,
    CURRENT_DEATIL_CONTOUR_POINT = 3
};

enum ToleranceUnits_e
{
    TOLERANCE_UNITS_MKM = 0,
    TOLERANCE_UNITS_PX = 1
};

enum ImageMoveMode_e
{
    IMAGE_MOVE_VIEWING = 0,
    IMAGE_MOVE_EDITING
};

#endif // TYPES_H
