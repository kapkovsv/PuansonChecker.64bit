#ifndef PUANSONCHECKER_H
#define PUANSONCHECKER_H

#include <QObject>
#include <QApplication>
#include <QThread>
#include <QtMath>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

#include <opencv2/core/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#define LIBRAW_NODLL
#include <libraw/libraw.h>

// Номер ракурса для задания текущего изображения
#define CURRENT_IMAGE_ANGLE 0
// Количество ракурсов
#define PUANSON_IMAGE_MAX_ANGLE 6

#include "types.h"
#include "puansonimage.h"
#include "photocamera.h"
#include "puansonmachine.h"
#include "puansonresearch.h"
#include "generalsettings.h"

#define CONFIGURATION_FILE "puanson_checker_config.xml"

#define CAPTURED_CURRENT_IMAGE_FILENAME "CapturedCurrentImage.nef"
#define CAPTURED_ETALON_IMAGE_FILENAME "CapturedEtalonImage.nef"

#define DEFAULT_CANNY_THRES_1 20
#define DEFAULT_CANNY_THRES_2 30

#define REFERENCE_POINT_AUTO_SEARCH_AREA_DEFAULT_WIDTH 150
#define REFERENCE_POINT_AUTO_SEARCH_AREA_DEFAULT_HEIGHT 150

#define NUMBER_OF_ETALON_ANGLES 6

#define UNKNOWN_ETALON_ANGLE UCHAR_MAX

class MainWindow;
class CurrentForm;
class ContoursForm;
class ToleranceDialog;
class ImageWindow;

class GeneralSettings;

class PuansonChecker: public QObject
{
    Q_OBJECT

    static const cv::Vec3b empty_contour_color;
    static const cv::Vec3b empty_contour_color_2;

    static const cv::Vec3b inner_contour_color;
    static const cv::Vec3b outer_contour_color;
    static const cv::Vec3b etalon_contour_color;
    static const cv::Vec3b current_contour_color;

private:
    PuansonChecker(const QApplication *app);
    ~PuansonChecker();

public:
    PuansonChecker() = delete;
    PuansonChecker(const PuansonChecker&) = delete;
    PuansonChecker(const PuansonChecker&&) = delete;
    PuansonChecker& operator=(const PuansonChecker&) = delete;
    PuansonChecker& operator=(PuansonChecker&&) = delete;

    static PuansonChecker *getInstance(const QApplication *app = Q_NULLPTR);

    void quit();

    // UI
    void showSettingsDialog();
    bool showConfirmCheckResultCorrectnessDialog(bool check_result);

    void activateContoursWindow() const;
    void activateCurrentImageWindow() const;
    // --

    // Research
    inline void setEtalonResearchSettings(const QString &_etalon_research_folder_path, PuansonModel _detail_research_puanson_model, const QDateTime &_etalon_research_date_time_of_creation, const quint8 _etalon_research_number_of_angles)
    {
        etalon_research_folder_path = _etalon_research_folder_path;
        etalon_puanson_image.setEtalonResearchPuansonModel(_detail_research_puanson_model);
        etalon_research_date_time_of_creation = _etalon_research_date_time_of_creation;
        etalon_research_number_of_angles = _etalon_research_number_of_angles;

        loaded_research = PuansonResearch(_etalon_research_number_of_angles, _detail_research_puanson_model, _etalon_research_date_time_of_creation, _etalon_research_folder_path);

        for(quint8 angle = 1; angle <= etalon_research_number_of_angles; angle++)
            loaded_research.loadEtalonAngle(angle);
    }

    inline void setCurrentResearchSettings(const QString &_current_research_folder_path, const QDateTime &_current_research_date_time_of_creation, const bool _current_detail_angle_source_photo_shooting_only)
    {
        current_research_folder_path = _current_research_folder_path;
        current_research_date_time_of_creation = _current_research_date_time_of_creation;
        current_detail_angle_source_photo_shooting_only = _current_detail_angle_source_photo_shooting_only;
    }

    inline void cancelEtalonResearch()
    {
        /*
        QDir(etalon_research_folder_path).removeRecursively();
        QDir().mkdir(etalon_research_folder_path);*/
        resetEtalonResearchSettings();
    }

    inline void cancelCurrentResearch()
    {
        if(!current_research_folder_path.isEmpty())
        {
            QDir(current_research_folder_path).removeRecursively();
            QDir().mkdir(current_research_folder_path);
        }

        image_source_photo_shooting_only = true;
        use_machine_for_detail_movement = true;

        current_research_active_angle = 0;
    }

    inline void resetEtalonResearchSettings()
    {
        setEtalonResearchSettings("", PuansonModel::PUANSON_MODEL_658, QDateTime(), 0);

        image_source_photo_shooting_only = true;
        use_machine_for_detail_movement = true;

        etalon_research_active_angle = 0;
    }

    inline QString getEtalonResearchFolderPath() const
    {
        return etalon_research_folder_path;
    }

    inline QString getCurrentResearchFolderPath() const
    {
        return current_research_folder_path;
    }

    inline void getEtalonResearchSettings(QString &_etalon_research_folder_path, PuansonModel &_detail_research_puanson_model, QDateTime &_etalon_research_date_time_of_creation, quint8 &_etalon_research_number_of_angles) const
    {
        _etalon_research_folder_path = etalon_research_folder_path;
        _detail_research_puanson_model = etalon_puanson_image.getDetailPuansonModel();
        _etalon_research_date_time_of_creation = etalon_research_date_time_of_creation;
        _etalon_research_number_of_angles = etalon_research_number_of_angles;
    }

    inline void getCurrentResearchSettings(QString &_current_research_folder_path, QDateTime &_current_research_date_time_of_creation, bool &_current_detail_angle_source_photo_shooting_only) const
    {
        _current_research_folder_path = current_research_folder_path;
        _current_research_date_time_of_creation = current_research_date_time_of_creation;
        _current_detail_angle_source_photo_shooting_only = current_detail_angle_source_photo_shooting_only;
    }

    inline bool isCurrentDetailAngleSourcePhotoShootingOnly() const
    {
        return current_detail_angle_source_photo_shooting_only;
    }

    inline bool isEtalonResearchLoaded() const
    {
        return !etalon_research_folder_path.isEmpty();
    }

    inline bool isEtalonResearchCompleted() const
    {
        return etalon_research_completed;
    }

    inline void completeEtalonResearch()
    {
        image_source_photo_shooting_only = true;
        use_machine_for_detail_movement = true;

        etalon_research_completed = true;
    }

    inline quint8 completeCurrentResearch(const bool is_detail_suitable = true)
    {
        quint8 active_angle = current_research_active_angle;

        current_research_active_angle = 0;
        current_puanson_image.setCurrentDetailSuitable(is_detail_suitable);

        // Рассчёт размеров текущей детали детали на основе отклонений
        PuansonChecker::getInstance()->getCurrent().calculateDetailDimensionsFromDeviations(PuansonChecker::getInstance()->getEtalon());

        return active_angle;
    }
    // --------

    // Etalon image
    bool loadEtalonImage(const quint8 angle, const QString &path, const QString &contours_path = "", bool save_attributes = false);

    inline bool isEtalonImageLoaded() const
    {
        return !etalon_puanson_image.isEmpty();
    }

    inline bool isCurrentImageLoaded() const
    {
        return !current_puanson_image.isEmpty();
    }

    PuansonImage &getEtalon()
    {
        return etalon_puanson_image;
    }

    PuansonResearch &getLoadedResearch()
    {
        return loaded_research;
    }

    quint8 getActiveEtalonResearchAngle() const
    {
        return etalon_research_active_angle;
    }

    quint8 incrementActiveEtalonResearchAngle()
    {
        return ++etalon_research_active_angle;
    }

    void setActiveEtalonResearchAngle(quint8 angle)
    {
        etalon_research_active_angle = angle;
    }

    quint8 getActiveCurrentResearchAngle() const
    {
        return current_research_active_angle;
    }

    quint8 incrementActiveCurrentResearchAngle()
    {
        return ++current_research_active_angle;
    }

    void setActiveCurrentResearchAngle(quint8 angle)
    {
        current_research_active_angle = angle;
    }

    bool getEtalonImage(QImage &img);
    bool getEtalonContour(QImage &img);

    void cropImages(qreal scale = 4.0)
    {
        etalon_puanson_image.cropImage(scale);
        etalon_puanson_image.setImageContour(getContour(etalon_puanson_image));

        current_puanson_image.cropImage(scale);
        current_puanson_image.setImageContour(getContour(current_puanson_image));
    }
    // --------------------

    // Current detail
    void loadCurrentImage(const QString &path);

    void shootAndLoadCurrentImage();
    void shootAndLoadEtalonImage();

    void shiftCurrentImage(const float dx, const float dy);
    void rotateCurrentImage(const double angle);
    bool combineImagesByReferencePoints();
    bool combineImagesByReferencePointsAndDrawBadPoints();
    bool detectDeviations(const Rect &analysis_area);

    QVector<QPointF> findReferencePoints(const PuansonImage &reference_point_image) const;
    bool checkDetail(QVector<QPoint> &bad_points);

    const PuansonImage &getCurrent() const
    {
        return current_puanson_image;
    }

    PuansonImage &getCurrent()
    {
        return current_puanson_image;
    }

    bool getCurrentImage(QImage &img);
    bool getCurrentContour(QImage &img);

    void startCurrentCalibration();
    // --------------

    // Research
    void researchEtalonAngle(quint8 angle = 0);
    void researchCurrentAngle(quint8 angle = 0);

    bool loadEtalonResearch(const QString &etalon_research_folder_path);

    inline bool isImageSourcePhotoShootingOnly() const
    {
        return image_source_photo_shooting_only;
    }

    inline void setImageSourcePhotoShootingOnly(const bool new_value)
    {
        image_source_photo_shooting_only = new_value;
    }

    inline bool useMachineForDetailMovement() const
    {
        return use_machine_for_detail_movement;
    }

    inline void setMachineForDetailMovement(const bool new_value)
    {
        image_source_photo_shooting_only = new_value;
    }
    // --------

    // Move images
    void moveImages(const int &dx, const int &dy, const ImageWindow *owner_window = Q_NULLPTR, const bool scroll_event = false);
    // -----------

    // Camera
    bool connectToCamera();
    bool disconnectFromCamera();
    QString getCameraStatus() const;
    // ------

    // Machine
    PuansonMachine *getMachine() const
    {
        return machine;
    }
    // ------

    // Drawing images
    void drawEtalonImage(bool draw_etalon_image = true);
    void drawCurrentImage(bool draw_reference_points = true);
    void drawCurrentImageReferencePoints();
    void drawCurrentContour();
    void drawContoursImage();
    // -----------

    // Current detail measurements
    bool findMeasurementContourPoints(const QRect &analysis_rect);
    void measurementPointsSettingMode();
    QPoint findNearestContourPoint(const ContourPoints_e contour_point_type, const QPoint &original_point, const QLine &measurement_line, const quint16 neighborhood_len);
    // -----------

    // Current detail research goto next angle
    void currentDetailResearchGotoNextAngle();
    // -----------

    // Calculate image contour
    cv::Mat getContour(PuansonImage &puanson_image);
    // -----------------------

    // Saving images
    bool saveCurrentImage(const QString &path);
    bool saveResultImage(const QString &path);
    // -----------

    // Flag
    inline void setIgnoreScrollMoveImage(const bool ignore)
    {
        ignore_scroll_move_image = ignore;
    }
    // ----

    // Contours image
    void updateContoursImage();

    inline bool getDrawEtalonContourFlag() const
    {
        return draw_etalon_contour_flag;
    }

    inline void setDrawEtalonContourFlag(const bool val)
    {
        draw_etalon_contour_flag = val;
    }

    inline void setCannyThreshold1(const quint16 val)
    {
        CannyThreshold1 = val;
    }

    inline quint16 getCannyThreshold1() const
    {
        return CannyThreshold1;
    }

    inline void setCannyThreshold2(const quint16 val)
    {
        CannyThreshold2 = val;
    }

    inline quint16 getCannyThreshold2() const
    {
        return CannyThreshold2;
    }
    // -----------------

    inline GeneralSettings *getGeneralSettings() const
    {
        return general_settings;
    }

    void setReferencePointSearchArea(const ReferencePointType_e reference_point_type, const QRect &area_rect);
    QRect getReferencePointSearchArea(const ImageType_e image_type, const ReferencePointType_e reference_point_type) const;

    static int loadImage(const ImageType_e type, const QString &path, PuansonImage &output, bool save_attributes = false);

    inline bool isImageLoadingFinished()
    {
        return load_image_watcher.isFinished();
    }

public slots:
    void loadImageFinished();
    void cameraConnectionStatusChangedSlot(bool connected);

signals:
    void cameraConnectionStatusChanged(bool connected);

private:
    static PuansonChecker *instance;

    static void drawReferencePointOnContoursImage(const QPaintDevice &paint_device, const QPoint &position, const Qt::GlobalColor color, const QString &text);
    static QPointF findReferencePoint(const ReferencePointType_e reference_point_type, const PuansonImage &reference_point_image);

    // Приложение
    QApplication *application; // Указатель на единственный экземляр PuansonChecker
    // ----------

    // Общие настройки программы
    GeneralSettings *general_settings;
    // -------------------------

    // Все окна программы
    MainWindow *main_window;
    CurrentForm *current_image_window;
    ContoursForm *contours_window;
    // ------------------

    // Фотокамера
    PhotoCamera *camera;
    // ----------

    // Станок
    PuansonMachine *machine;
    // ------

    // Ракурс эталона
    PuansonImage etalon_puanson_image;      // Активный ракурс эталона
    // -------

    // Флаг получения изображений только с помощью фотоаппарата
    bool image_source_photo_shooting_only = true;

    // Флаг автоматического изменения позиции детали с помощью станка
    bool use_machine_for_detail_movement = true;

    // Загруженное исследование
    PuansonResearch loaded_research;

    // Исследование эталона
    QString etalon_research_folder_path;
    QDateTime etalon_research_date_time_of_creation;
    quint8 etalon_research_number_of_angles = 0;
    quint8 etalon_research_completed = false;
    quint8 etalon_research_active_angle = 0;    // Номер активного ракурса эталона в исследовании
    // -------

    // Исследование текущей детали
    QString current_research_folder_path;
    QDateTime current_research_date_time_of_creation;
    bool current_detail_angle_source_photo_shooting_only = false;
    quint8 current_research_active_angle = 0;   // Номер активного ракурса текущей детали в исследовании
    // -------

    // Области для автоматического поиска реперных точек
    QRect reference_point_1_search_area_rect;
    QRect reference_point_2_search_area_rect;
    // --------------------

    // Текущая деталь
    PuansonImage current_puanson_image;
    // -------------

    // Загрузка изображений
    QFuture<ImageType_e> load_image_future;
    QFutureWatcher<ImageType_e> load_image_watcher;
    quint8 loading_etalon_angle;            // Флаг загружаемого эталона
    // --------------------

    // Contours image settings
    cv::Mat contours_image;
    //bool calculate_contour_on_rotation;
    quint16 CannyThreshold1;
    quint16 CannyThreshold2;
    bool draw_etalon_contour_flag;
    // ------------------

    // Флаг
    bool ignore_scroll_move_image;
    // ----

    // Флаг, что изображения эталона и текущей детали совмещены
    bool images_combined_by_reference_points = false;
    // ----
};

#endif // PUANSONCHECKER_H
