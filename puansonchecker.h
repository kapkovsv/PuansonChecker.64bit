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

#define LIBRAW_NODLL
#include <libraw/libraw.h>

// Номер ракурса для задания текущего изображения
#define CURRENT_IMAGE_ANGLE 0
// Количество ракурсов
#define NUMBER_OF_ANGLES 5

#include "puansonimage.h"
#include "photocamera.h"
#include "generalsettings.h"

#define CONFIGURATION_FILE "puanson_checker_config.xml"

#define CAPTURED_CURRENT_IMAGE_FILENAME "CapturedCurrentImage.nef"
#define CAPTURED_ETALON_IMAGE_FILENAME "CapturedEtalonImage.nef"

#define DEFAULT_CANNY_THRES_1 20
#define DEFAULT_CANNY_THRES_2 30

class MainWindow;
class CurrentForm;
class ContoursForm;
class ToleranceDialog;
class ImageWindow;

class GeneralSettings;

enum CalibrationMode_e
{
    NO_CALIBRATION = 0,
    // Реперные точки
    REFERENCE_POINT_1 = 1,
    REFERENCE_POINT_2 = 2,
    // Идеальный контур
    IDEAL_IMPOSE = 14
};

class PuansonChecker: public QObject
{
    Q_OBJECT

protected:
    PuansonChecker(const QApplication *app);
    ~PuansonChecker();

public:
    static PuansonChecker *getInstance(const QApplication *app = NULL);

    void quit();

    // UI
    void showSettingsDialog();

    void activateContoursWindow();
    void activateCurrentImageWindow();
    // --

    // Etalon image
    bool loadEtalonImage(const quint8 angle, const QString &path);

    inline bool isEtalonImageLoaded(const quint8 angle)
    {
        return angle <= NUMBER_OF_ANGLES && !etalon_puanson_image[angle - 1].isEmpty();
    }

    inline bool isCurrentImageLoaded()
    {
        return !current_puanson_image.isEmpty();
    }

    PuansonImage &getEtalon(const quint8 angle)
    {
        if(angle < NUMBER_OF_ANGLES)
            return etalon_puanson_image[angle-1];
        else
            return etalon_puanson_image[NUMBER_OF_ANGLES - 1];
    }

    bool getEtalonImage(const quint8 angle, QImage &img);
    bool getEtalonContour(QImage &img);

    inline bool setEtalonAngle(const quint8 angle)
    {
        if(angle > NUMBER_OF_ANGLES)
            return false;

        etalon_angle = angle;

//        if(areInnerSkelenonPointsSet(angle))
//            getContour(etalon_puanson_image[etalon_angle - 1]);

        return true;
    }

    inline quint8 getEtalonAngle()
    {
        return etalon_angle;
    }
    // --------------------

    // Current detail
    void loadCurrentImage(const QString &path);

    void shootAndLoadCurrentImage();
    void shootAndLoadEtalonImage();

    void shiftCurrentImage(const qreal dx, const qreal dy);
    void rotateCurrentImage(const double angle);
    bool combineImagesByReferencePoints();

    PuansonImage &getCurrent()
    {
        return current_puanson_image;
    }

    bool getCurrentImage(QImage &img);
    bool getCurrentContour(QImage &img);

    void startCurrentCalibration();
    // --------------

    // Move images
    void moveImages(const int &dx, const int &dy, const ImageWindow *owner_window = NULL, const bool scroll_event = false);
    // -----------

    // Camera
    bool connectToCamera();
    bool disconnectFromCamera();
    QString getCameraStatus();
    // ------

    // Drawing images
    void drawCurrentImage();
    void drawCurrentContour();
    void drawContoursImage();
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

    inline bool getDrawEtalonContourFlag()
    {
        return draw_etalon_contour_flag;
    }

    inline void setDrawEtalonContourFlag(const bool val)
    {
        draw_etalon_contour_flag = val;
    }

    inline quint16 getCannyThreshold1()
    {
        return CannyThreshold1;
    }

    inline void setCannyThreshold1(const quint16 val)
    {
        CannyThreshold1 = val;
    }

    inline void setCannyThreshold2(const quint16 val)
    {
        CannyThreshold1 = val;
    }

    inline quint16 getCannyThreshold2()
    {
        return CannyThreshold2;
    }
    // -----------------

    GeneralSettings *getGeneralSettings()
    {
        return general_settings;
    }

public slots:
    void loadImageFinished();
    void cameraConnectionStatusChangedSlot(bool connected);

signals:
    void cameraConnectionStatusChanged(bool connected);

private:
    static PuansonChecker *instance;

    static int loadImage(const ImageType_e type, const QString &path, PuansonImage &output);

    static void drawReferencePointOnContoursImage(const QPaintDevice &paint_device, const QPoint &position, const Qt::GlobalColor color, const QString &text);

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

    // Эталоны
    PuansonImage etalon_puanson_image[NUMBER_OF_ANGLES];
    PuansonImage *etalon_puanson_image_ptr; // Указатель на эталонное изображение
    cv::Mat etalon_contour;                 // Контур активного эталона
    quint8 etalon_angle;                    // Номер активного эталона
    // -------

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
};

#endif // PUANSONCHECKER_H
