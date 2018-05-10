#include "puansonchecker.h"
#include "settingsdialog.h"

#include "mainwindow.h"
#include "currentform.h"
#include "contoursform.h"

#include <QtMath>
#include <vector>

#include <QMessageBox>

#include <QDebug>

PuansonChecker *PuansonChecker::instance = NULL;

PuansonChecker *PuansonChecker::getInstance(const QApplication *app)
{
    if(instance == NULL)
        instance = new PuansonChecker(app);

    return instance;
}

int PuansonChecker::loadImage(ImageType_e image_type, const QString &path, PuansonImage &output)
{
    LibRaw RawProcessor;
    int ret;

    cv::Mat loaded_image;
    libraw_processed_image_t *raw_image_ptr;

    RawProcessor.imgdata.params.use_camera_wb = 1;
    RawProcessor.imgdata.params.use_auto_wb = 0;

    if ((ret = RawProcessor.open_file(path.toStdString().c_str())) != LIBRAW_SUCCESS)
        return -1;

    if ((ret = RawProcessor.unpack()) != LIBRAW_SUCCESS)
        return -1;

    int check = RawProcessor.dcraw_process();
    raw_image_ptr = RawProcessor.dcraw_make_mem_image(&check);
    loaded_image = cv::Mat(cv::Size(raw_image_ptr->width, raw_image_ptr->height), CV_8UC3, raw_image_ptr->data, cv::Mat::AUTO_STEP);
    cvtColor(loaded_image, loaded_image, 4);

    output = PuansonImage(image_type, loaded_image, raw_image_ptr, path);

    return 0;
}

PuansonChecker::PuansonChecker(const QApplication *app)
{
    application = const_cast<QApplication *>(app);
    general_settings = new GeneralSettings(CONFIGURATION_FILE);
    camera = new PhotoCamera();

    CannyThreshold1 = DEFAULT_CANNY_THRES_1;
    CannyThreshold2 = DEFAULT_CANNY_THRES_2;

    draw_etalon_contour_flag = false;
    //calculate_contour_on_rotation = false;

    main_window = new MainWindow(this);
    current_image_window = new CurrentForm(this);
    contours_window = new ContoursForm(this);

    current_image_window->show();
    contours_window->show();
    main_window->show();

    etalon_angle = 1;
    loading_etalon_angle = 0;

    quint16 ext_tolerance_mkm_array[NUMBER_OF_ANGLES];
    quint16 int_tolerance_mkm_array[NUMBER_OF_ANGLES];

    quint32 reference_points_distance_array[NUMBER_OF_ANGLES];

    general_settings->getToleranceFields(ext_tolerance_mkm_array, int_tolerance_mkm_array);
    general_settings->getReferencePointDistancesMkm(reference_points_distance_array);

    for(int angle = 1; angle <= NUMBER_OF_ANGLES; angle++)
    {
        getEtalon(angle).setToleranceFields(ext_tolerance_mkm_array[angle - 1], int_tolerance_mkm_array[angle - 1]);
        getEtalon(angle).setReferencePointDistanceMkm(reference_points_distance_array[angle - 1]);
    }

    ignore_scroll_move_image = false;

    connect(&load_image_watcher, SIGNAL(finished()), SLOT(loadImageFinished()));
    connect(camera, SIGNAL(cameraConnectionStatusChanged(bool)), SLOT(cameraConnectionStatusChangedSlot(bool)));
}

PuansonChecker::~PuansonChecker()
{
    if(load_image_future.isRunning())
        load_image_future.waitForFinished();

    delete general_settings;
    general_settings = NULL;

    delete camera;
    camera = NULL;

    delete main_window;
    main_window = NULL;

    delete current_image_window;
    current_image_window = NULL;

    delete contours_window;
    contours_window = NULL;
}

void PuansonChecker::updateContoursImage()
{
    PuansonImage &current_image_ref = getCurrent();

    if(!etalon_contour.empty())
    {
       if(etalon_puanson_image[etalon_angle - 1].isIdealContourSet())
            current_image_ref.copyIdealContour(etalon_puanson_image[etalon_angle - 1]);

       etalon_contour.release();
       etalon_contour = getContour(etalon_puanson_image[etalon_angle - 1]);

       if(!current_image_ref.isEmpty())
            current_image_ref.setImageContour(getContour(current_image_ref));

       if(!current_image_ref.getImageContour().empty())
            drawContoursImage();
    }
}

void PuansonChecker::showSettingsDialog()
{
    SettingsDialog settings_dialog(this);

    settings_dialog.setFixedSize(settings_dialog.size());

    switch( settings_dialog.exec() )
    {
       case QDialog::Accepted:
            main_window->setWindowStatus("Применение настроек ...");
            main_window->drawIdealContour();  //  Нужно для пересчёта нормальных отрезков
            updateContoursImage();
            main_window->setWindowStatus("");
           break;
       case QDialog::Rejected:
           break;
       default:
           break;
    }
}

void PuansonChecker::quit()
{
    application->quit();
}

void PuansonChecker::activateContoursWindow()
{
    contours_window->show();
    contours_window->activateWindow();
}

void PuansonChecker::activateCurrentImageWindow()
{
    current_image_window->show();
    current_image_window->activateWindow();
}

void PuansonChecker::loadCurrentImage(const QString &path)
{
    load_image_future = QtConcurrent::run([&, path]() {
        PuansonImage &current_image = getCurrent();

        if(PuansonChecker::loadImage(CURRENT_IMAGE, path, current_image) == 0)
        {
            if(etalon_puanson_image[etalon_angle - 1].isIdealContourSet())
                current_image.copyIdealContour(etalon_puanson_image[etalon_angle-1]);

            current_image.setImageContour(getContour(current_image));
            current_image.setToleranceFields(0, 0);
        }
        /*else
            qDebug() << "PuansonChecker::loadCurrentImage error, current image!";*/

        return current_image.getImageType();
    });

    load_image_watcher.setFuture(load_image_future);
}

bool PuansonChecker::loadEtalonImage(const quint8 angle, const QString &path)
{
    if(angle > NUMBER_OF_ANGLES)
        return false;

    loading_etalon_angle = angle;
    etalon_puanson_image_ptr = &etalon_puanson_image[angle - 1];

    load_image_future = QtConcurrent::run([&, path]() {

        if(PuansonChecker::loadImage(ETALON_IMAGE, path, *etalon_puanson_image_ptr) == 0)
        {
            quint16 ext_tolerance_mkm_array[NUMBER_OF_ANGLES];
            quint16 int_tolerance_mkm_array[NUMBER_OF_ANGLES];
            quint32 reference_points_distance_array[NUMBER_OF_ANGLES];

            general_settings->getToleranceFields(ext_tolerance_mkm_array, int_tolerance_mkm_array);
            general_settings->getReferencePointDistancesMkm(reference_points_distance_array);

            etalon_puanson_image_ptr->setToleranceFields(ext_tolerance_mkm_array[loading_etalon_angle - 1], int_tolerance_mkm_array[loading_etalon_angle - 1]);
            etalon_puanson_image_ptr->setReferencePointDistanceMkm(reference_points_distance_array[loading_etalon_angle - 1]);

            etalon_contour = getContour(*etalon_puanson_image_ptr);
        }

        return ETALON_IMAGE;
    });

    load_image_watcher.setFuture(load_image_future);

    return true;
}

void PuansonChecker::loadImageFinished()
{
    ImageType_e image_type = load_image_watcher.result();

    main_window->loadImageFinished(image_type, loading_etalon_angle);

    current_image_window->loadImageFinished();
    loading_etalon_angle = 0;
}

bool PuansonChecker::getCurrentImage(QImage &img)
{
    if(getCurrent().getImage().empty())
        return false;

    getCurrent().getQImage(img);

    return true;
}

bool PuansonChecker::getCurrentContour(QImage &img)
{
    cv::Mat current_contour = getCurrent().getImageContour();

    if(current_contour.empty())
        return false;

    img = QImage((const unsigned char*)(current_contour.data),
                  current_contour.cols, current_contour.rows,
                  current_contour.step, QImage::Format_RGB888).rgbSwapped();

    return true;
}

bool PuansonChecker::getEtalonImage(const quint8 angle, QImage &img)
{
    if(angle > NUMBER_OF_ANGLES || getEtalon(angle).isEmpty())
        return false;

    getEtalon(angle).getQImage(img);

    return true;
}


bool PuansonChecker::getEtalonContour(QImage &img)
{
    if(etalon_contour.empty())
        return false;

    img = QImage((const unsigned char*)(etalon_contour.data),
                  etalon_contour.cols, etalon_contour.rows,
                  etalon_contour.step, QImage::Format_Grayscale8);//.rgbSwapped();

    return true;
}

void PuansonChecker::startCurrentCalibration()
{
    current_image_window->setCalibrationMode(REFERENCE_POINT_1);
}

void PuansonChecker::moveImages(const int &dx, const int &dy, const ImageWindow *owner_window, const bool scroll_event)
{
    if(scroll_event && ignore_scroll_move_image)
        return;

    if(main_window)
    {
        if((!scroll_event || dynamic_cast<const QWidget *>(owner_window) != main_window) && isEtalonImageLoaded(etalon_angle))
            main_window->moveImage(dx, dy);
        else if(scroll_event && dynamic_cast<const QWidget *>(owner_window) == main_window)
            main_window->shiftImageCoords(dx, dy);
    }

    if(current_image_window)
    {
        if((!scroll_event || dynamic_cast<const QWidget *>(owner_window) != current_image_window) && isCurrentImageLoaded())
            current_image_window->moveImage(dx, dy);
        else if(scroll_event && dynamic_cast<const QWidget *>(owner_window) == current_image_window)
            current_image_window->shiftImageCoords(dx, dy);
    }

    if(contours_window)
    {
        if((!scroll_event || dynamic_cast<const QWidget *>(owner_window) != contours_window) && (!getEtalon(etalon_angle).isEmpty() && isCurrentImageLoaded()))
            contours_window->moveImage(dx, dy);
        else if(scroll_event && dynamic_cast<const QWidget *>(owner_window) == contours_window)
            contours_window->shiftImageCoords(dx, dy);
    }
}

void PuansonChecker::drawCurrentImage()
{
    QImage img;

    current_image_window->setLabel2Text("Файл " + getCurrent().getFilename());

    getCurrentImage(img);
    current_image_window->drawImage(img);
    current_image_window->drawReferencePoints();
}

void PuansonChecker::drawCurrentContour()
{
    QImage img;

    getCurrentContour(img);
    current_image_window->drawImage(img);
}

bool PuansonChecker::connectToCamera()
{
    return camera->Connect();
}

bool PuansonChecker::disconnectFromCamera()
{
    return camera->Disconnect();
}

QString PuansonChecker::getCameraStatus()
{
    return camera->getCameraStatus();
}

void PuansonChecker::shootAndLoadCurrentImage()
{
    QString path = QDir::currentPath() + "/" + CAPTURED_CURRENT_IMAGE_FILENAME;

    main_window->setWindowStatus("Фотосъёмка и загрузка изображения текущей детали ...");

    load_image_future = QtConcurrent::run([&, path]() {
        //main_window->setWindowStatus("Фотосъёмка текущей детали ...");
        camera->CaptureAndAcquireImage(CAPTURED_CURRENT_IMAGE_FILENAME);

        //main_window->setWindowStatus("Загрузка изображения текущей детали ...");
        PuansonImage &current_image = getCurrent();

        current_image.release();

        if(PuansonChecker::loadImage(CURRENT_IMAGE, path, current_image) == 0)
        {
            if(etalon_puanson_image[etalon_angle - 1].isIdealContourSet())
                current_image.copyIdealContour(etalon_puanson_image[etalon_angle-1]);

            current_image.setToleranceFields(0, 0);
            current_image.setImageContour(getContour(current_image));
        }

        return current_image.getImageType();
    });

    load_image_watcher.setFuture(load_image_future);
}

void PuansonChecker::shootAndLoadEtalonImage()
{
    QString path = QDir::currentPath() + "/" + CAPTURED_ETALON_IMAGE_FILENAME;

    loading_etalon_angle = etalon_angle;
    etalon_puanson_image_ptr = &etalon_puanson_image[loading_etalon_angle - 1];

    main_window->setWindowStatus("Фотосъёмка и загрузка изображения эталонной детали ...");

    load_image_future = QtConcurrent::run([&, path]() {
        //main_window->setWindowStatus("Фотосъёмка эталонной детали ...");
        camera->CaptureAndAcquireImage(CAPTURED_ETALON_IMAGE_FILENAME);

        //main_window->setWindowStatus("Загрузка изображения эталонной детали ...");
        etalon_puanson_image_ptr->release();

        if(PuansonChecker::loadImage(ETALON_IMAGE, path, *etalon_puanson_image_ptr) == 0)
        {
            quint16 ext_tolerance_mkm_array[NUMBER_OF_ANGLES];
            quint16 int_tolerance_mkm_array[NUMBER_OF_ANGLES];
            quint32 reference_points_distance_array[NUMBER_OF_ANGLES];

            general_settings->getToleranceFields(ext_tolerance_mkm_array, int_tolerance_mkm_array);
            general_settings->getReferencePointDistancesMkm(reference_points_distance_array);

            etalon_puanson_image_ptr->setToleranceFields(ext_tolerance_mkm_array[loading_etalon_angle - 1], int_tolerance_mkm_array[loading_etalon_angle - 1]);
            etalon_puanson_image_ptr->setReferencePointDistanceMkm(reference_points_distance_array[loading_etalon_angle - 1]);

            etalon_contour = getContour(*etalon_puanson_image_ptr);
        }

        return etalon_puanson_image_ptr->getImageType();
    });

    load_image_watcher.setFuture(load_image_future);
}

void PuansonChecker::cameraConnectionStatusChangedSlot(bool connected)
{
    emit cameraConnectionStatusChanged(connected);
}

// Параллельная обработка контуров детали
#define PARALLEL_CONTOUR_COMPUTING

#define TRUNC_THRES 130

#define HORIZONTAL_FIELD_PX 750

#define DETAIL_COLOR_THRESHOLD 100
#define GLARE_COLOR_THRESHOLD 200
#define GLARE_COLOR_RANGE_W 100
#define NUMBER_OF_DETAIL_GLARES 2

cv::Mat PuansonChecker::getContour(PuansonImage &puanson_image)
{
    using namespace cv;
    using namespace std;

class ParallelDrawContours : public ParallelLoopBody
{
public:
    ParallelDrawContours(PuansonImage &puanson_image, Mat &image_contour, const vector<vector<Point> > &contours_vector, const quint16 contours_per_thread,
                     const vector<Vec4i> &hierarchy, const bool draw_etalon_contour_flag, const quint32 ext_tolerance, quint32 int_tolerance)
                : p_puanson_image(&puanson_image), image_type(puanson_image.getImageType()), image_contour_ref(image_contour),
                  detail_contours(contours_vector), contours_per_thread(contours_per_thread),
                  contours_hierarchy(hierarchy), draw_etalon_contour_flag(draw_etalon_contour_flag),
                  external_tolerance(ext_tolerance), internal_tolerance(int_tolerance)
    { }

    virtual void operator()(const Range& range) const
    {
        Scalar detail_contour_color;
        Scalar inner_contour_color = Scalar(250, 100, 100);
        Scalar outer_contour_color = Scalar(100, 250, 100);

        int thickness = CV_FILLED;

        switch(image_type)
        {
            case ETALON_IMAGE:
                detail_contour_color = Scalar( 255, 255, 255 );
                thickness = 1;
                break;
            case CURRENT_IMAGE:
            default:
                detail_contour_color = Scalar( 50, 50, 250 );
                thickness = 1;
                break;
        }

        vector<vector<Point> > outer_contours = detail_contours;
        vector<vector<Point> > inner_contours = detail_contours;

        // Внутренняя точка
        int j; // Индекс

        QLine last_skeleton_line;

        for(int i = range.start; i < range.end; i++)
        {
            for(int l = 0; l < contours_per_thread; l++)
            {
                j = i * contours_per_thread + l;

                // Рассчитывать контура допуска только тогда, когда заданы точки остова
                if(p_puanson_image->isIdealContourSet()/*(image_type == ETALON_IMAGE && p_puanson_image->isIdealContourSet()) || image_type == CURRENT_IMAGE*/)
                {
                    QPoint internal_normal_vector;
                    QPoint external_normal_vector;

                    bool near_ideal_line = p_puanson_image->findNearestIdealLineNormalVector(QPoint(detail_contours[j].begin()->x, detail_contours[j].begin()->y), last_skeleton_line, internal_normal_vector, external_normal_vector);

                    if((/*p_puanson_image->isIdealContourSet() && */near_ideal_line)/* || image_type == CURRENT_IMAGE*/)
                    {
                        if(image_type == ETALON_IMAGE)
                        {
                            for(auto it = inner_contours[j].begin(); it != inner_contours[j].end(); it++)
                                *it += Point(internal_normal_vector.x(), internal_normal_vector.y());

                            for(auto it = outer_contours[j].begin(); it != outer_contours[j].end(); it++)
                                *it += Point(external_normal_vector.x(), external_normal_vector.y());
                        }

                        // Рисование контуров
                        // Вывод контура детали
                        if((image_type == ETALON_IMAGE &&  draw_etalon_contour_flag) || image_type == CURRENT_IMAGE)
                        {
                            /*if(j % 10 == 0)
                            {
                                Point seedPoint(detail_contours[j].begin()->x + 10, detail_contours[j].begin()->y + 20);
                                char buf[32] = "";
                                sprintf(buf, "%d", j);
                                String text(buf);
                                putText(image_contour_ref, text, seedPoint, FONT_HERSHEY_PLAIN, 0.7, Scalar(255, 0, 255));
                            }*/

                            drawContours( image_contour_ref, detail_contours, j, detail_contour_color, thickness, LINE_AA, contours_hierarchy, 0, Point(0, 0) );
                        }

                        // Выводить контура допуска только тогда, когда заданы точки остова
                        if(image_type == ETALON_IMAGE/* && p_puanson_image->isIdealContourSet()*/)
                        {
                            //line(image_contour_ref, *detail_contours[j].begin(), *inner_contours[j].begin(), Scalar(255, 0, 0));
                            // Вывод контура внутреннего допуска
                            drawContours( image_contour_ref, inner_contours, j, inner_contour_color, 1, LINE_AA, contours_hierarchy, 0, Point(0, 0) );
                            //line(image_contour_ref, *detail_contours[j].begin(), *outer_contours[j].begin(), Scalar(0, 255, 0));
                            // Вывод контура внешнего допуска
                            drawContours( image_contour_ref, outer_contours, j, outer_contour_color, 1, LINE_AA, contours_hierarchy, 0, Point(0, 0) );
                        }
                        /////////////////////
                    }
                }
            }
        }
    }

private:
    PuansonImage *p_puanson_image;
    ImageType_e image_type;
    Mat &image_contour_ref;
    vector<vector<Point>> detail_contours;
    quint16 contours_per_thread;
    vector<Vec4i> contours_hierarchy;
    bool draw_etalon_contour_flag;
    quint32 external_tolerance;
    quint32 internal_tolerance;

    Point _top;
    Point _right;
    Point _bottom;
    Point _left;

    // Нормальные вектора
    // Внутренний допуск
    Point top_right_n_int;
    Point right_bottom_n_int;
    Point bottom_left_n_int;
    Point left_top_n_int;
    // Внешний допуск
    Point top_right_n_ext;
    Point right_bottom_n_ext;
    Point bottom_left_n_ext;
    Point left_top_n_ext;
};

    Mat gray_image = Mat(puanson_image.getImage().size(), CV_8UC1);
    Mat image_contour(Mat::zeros(puanson_image.getImage().size(), CV_8UC3));
    Mat bin1;

    if(puanson_image.isEmpty())
        return Mat();

    // Kernel matrix for filter
    /*Mat kernel_matrix = (Mat_<float>(3,3) <<
        -0.1f,  -0.1f, -0.1f,
        -0.1f, 2.0f, -0.1f,
        -0.1f, -0.1f, -0.1f);*/

    //Rect region_of_interest;
    Mat gray_roi;

    cvtColor(puanson_image.getImage(), gray_image, COLOR_RGB2GRAY);

    //filter2D(gray_image, gray_image, gray_image.depth(), kernel_matrix, cvPoint(-1,-1));
    //region_of_interest = Rect(0, HORIZONTAL_FIELD_PX, gray_image.cols, gray_image.rows - HORIZONTAL_FIELD_PX*2);
    gray_roi = gray_image;//gray_image(region_of_interest);

    // Убираем лишние детали в верхнем правом и нижнем левом углах
    Point poly_points[4];

    // Левый нижний угол
    poly_points[0] = Point(0, gray_image.rows * 0.3);
    poly_points[1] = Point(gray_image.cols * 0.5, gray_image.rows);
    poly_points[2] = Point(0, gray_image.rows);

    fillConvexPoly(gray_roi, poly_points, 3, Scalar(gray_image.at<uchar>(poly_points[0])));

    /*line(gray_roi, poly_points[0], poly_points[1], Scalar(0));
    line(gray_roi, poly_points[1], poly_points[2], Scalar(0));
    line(gray_roi, poly_points[2], poly_points[0], Scalar(0));*/
    //////////////////////

    // Правый верхний угол
    poly_points[0] = Point(gray_image.cols * 0.7, 0);
    poly_points[1] = Point(gray_image.cols, gray_image.rows * 0.3);
    poly_points[2] = Point(gray_image.cols, 0);

    fillConvexPoly(gray_roi, poly_points, 3, Scalar(gray_image.at<uchar>(poly_points[0])));

   /*line(gray_roi, poly_points[0], poly_points[1], Scalar(0));
    line(gray_roi, poly_points[1], poly_points[2], Scalar(0));
    line(gray_roi, poly_points[2], poly_points[0], Scalar(0));*/
    //////////////////////

    /*poly_points[0] = _top;
    poly_points[1] = _right;
    poly_points[2] = _bottom;
    poly_points[3] = _left;

    fillConvexPoly(gray_roi, poly_points, 4, Scalar(gray_image.at<uchar>(poly_points[0])));*/
    //////////////////////

    // Поиск бликов
    if(/*true*/false)
    {
        bool detail_area_flag = false;
        int number_of_detected_glares = 0;
        int y = 0;

        int number_of_glare_points = 0;

        while(y != gray_roi.rows)
        {
                uchar& c = gray_roi.at<uchar>(y, gray_roi.cols / 2);
                Point seedPoint(gray_roi.cols / 2, y);

                if(number_of_detected_glares == 0 && !detail_area_flag && c < DETAIL_COLOR_THRESHOLD)
                    detail_area_flag = true;
                else if(detail_area_flag && number_of_detected_glares == NUMBER_OF_DETAIL_GLARES)
                    detail_area_flag = false;

                if(detail_area_flag && c > GLARE_COLOR_THRESHOLD - GLARE_COLOR_RANGE_W)
                {
                    if(number_of_glare_points++)
                    {
                        // Fill
                        if(c > GLARE_COLOR_THRESHOLD)
                        {
                            floodFill(gray_roi, seedPoint, GLARE_COLOR_THRESHOLD - GLARE_COLOR_RANGE_W, NULL, Scalar(c - GLARE_COLOR_THRESHOLD + GLARE_COLOR_RANGE_W), Scalar(255 - c), FLOODFILL_FIXED_RANGE + 8);

                            number_of_glare_points = 0;
                            number_of_detected_glares++;
                        }

                        //seedPoint.y += HORIZONTAL_FIELD_PX;

                        //circle(puanson_image.getImage(), seedPoint, 50, Scalar(120, 150, 50));
                    }
                }
                else if(number_of_glare_points != 0)
                    number_of_glare_points = 0;

            y++;
        }
    }

   // threshold( gray_roi, gray_roi, TRUNC_THRES, 0, THRESH_TRUNC );
    //////////////////////////

    //blur( gray_roi, gray_roi, Size(3,3) );
    Canny( gray_roi, bin1, CannyThreshold1, CannyThreshold2, 3 );

    vector<vector<Point> > detail_contours;
    vector<Vec4i> contours_hierarchy;
    vector<vector<Point> > detail_contours0;

    findContours( bin1, detail_contours0, contours_hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0) );

    detail_contours.resize(detail_contours0.size());
    for( size_t k = 0; k < detail_contours0.size(); k++ )
        approxPolyDP(Mat(detail_contours0[k]), detail_contours[k], 3, true);

    //Scalar inner_line_color(229, 43, 80);

#if !defined(PARALLEL_CONTOUR_COMPUTING)
    Scalar detail_contour_color;
    Scalar inner_contour_color = Scalar(250, 100, 100);
    Scalar outer_contour_color = Scalar(100, 250, 100);

    //int thickness = CV_FILLED;

    switch(image_type)
    {
        case ETALON_IMAGE:
            detail_contour_color = Scalar( 255, 255, 255 );
            thickness = 3;
            break;
        case getCurrent().getImage():
        default:
            detail_contour_color = Scalar( 50, 50, 250 );
            thickness = 1;
            break;
    }

    int internal_tolerance = 20; // Внутренний сдвиг контуров в пикселях
    int external_tolerance = 30; // Внешний сдвиг контуров в пикселях

    // Координаты нормального вектора
    int N_x = 0;
    int N_y = 0;

    vector<vector<Point> > outer_contours = detail_contours;
    vector<vector<Point> > inner_contours = detail_contours;

    // Поиск самой вехней, нижней, правой и левой точек
    Point _top(0, 10000);
    Point _left(10000, 0);
    Point _right(0, 0);
    Point _bottom(0, 0);

    for( size_t i = 0; i < detail_contours.size(); i++ )
    {
        for(auto it = detail_contours[i].begin(); it != detail_contours[i].end(); it++)
        {
            if(it->x < _left.x)
                _left = *it;

            if(it->x > _right.x)
                _right = *it;

            if(it->y < _top.y)
                _top = *it;

            if(it->y > _bottom.y)
                _bottom = *it;
        }
    }

    _left.x += 200;
    _right.x -= 200;
    _top.y += 400;
    _bottom.y -= 300;

    if(_left.y < _top.y)
        _left.y += 200;
    else if(_left.y > _bottom.y)
        _left.y -= 200;

    if(_right.y < _top.y)
        _right.y += 200;
    else if(_right.y > _bottom.y)
        _right.y -= 200;

    if(_top.x < _left.x)
        _top.x += 200;
    else if(_top.x > _right.x)
        _top.x -= 200;

    if(_bottom.x < _left.x)
        _bottom.x += 200;
    else if(_bottom.x > _right.x)
        _bottom.x -= 200;
    //////////////////////////////////////

    // Внутренняя точка
    Point inner_point;

    int _x1, _y1;
    int _x2, _y2;
    int __x2, __y2;
    int _x3, _y3;

    Point inner_line_pt1, inner_line_pt2;

    for( size_t j = 0; j < detail_contours.size(); j++ )
    {
        if(image_type == ETALON_IMAGE)
        {
            // Получение границы внутреннего допуска
            _x1 = 0, _y1 = 0;
            _x2 = 0, _y2 = 0;
            __x2 = 0, __y2 = 0;
            _x3 = 0, _y3 = 0;

            for(auto it = inner_contours[j].begin(); it != inner_contours[j].end(); it++)
            {
                if(abs(it->y - _bottom.y) < abs(it->y - _top.y))
                    inner_line_pt1 = _bottom;
                else
                    inner_line_pt1 = _top;

                if(it->x > inner_line_pt1.x)
                    inner_line_pt2 = _right;
                else
                    inner_line_pt2 = _left;

                if(it->x < _left.x)
                    inner_point = _left;
                else if(it->x > _right.x)
                    inner_point = _right;
                else
                {
                    inner_point.x = it->x;
                    inner_point.y = (inner_point.x - inner_line_pt1.x)*(inner_line_pt2.y - inner_line_pt1.y)/(inner_line_pt2.x - inner_line_pt1.x) + inner_line_pt1.y;
                }

                //circle(image_contour_ref, inner_point, 5, Scalar(120, 150, 50));

                if(it + 1 == inner_contours[j].end())  // Последняя точка
                {
                    if(it != inner_contours[j].begin())
                    {
                        it->x = _x2;
                        it->y = _y2;
                    }
                    else
                    {
                        ParallelDrawContours::shiftContour(inner_point, *it, *it, N_x, N_y, false);
                    }

                    continue;
                }

                // Вычисление нормального отрезка
                int dx = it->y - (it+1)->y;
                int dy = (it+1)->x - it->x;

                double mod = qSqrt(static_cast<double>(dx * dx + dy * dy));

                N_x = (int)(dx / mod * internal_tolerance); // Координаты нормального
                N_y = (int)(dy / mod * internal_tolerance); // вектора
                // Конец вычисления нормального отрезка

                // Работа с текущим значением it
                if(it == inner_contours[j].begin())
                {
                    Point new_point;

                    ParallelDrawContours::shiftContour(inner_point, *it, new_point, N_x, N_y, false);

                    _x1 = new_point.x;
                    _y1 = new_point.y;
                }
                else
                {
                    Point new_point;

                    ParallelDrawContours::shiftContour(inner_point, *it, new_point, N_x, N_y, false);

                    __x2 = new_point.x;
                    __y2 = new_point.y;
                }
                // -----------------------------

                // Работа со значением it + 1
                if(it + 1 != inner_contours[j].end())
                {
                    if(it == inner_contours[j].begin())
                    {
                        Point new_point;

                        ParallelDrawContours::shiftContour(inner_point, *(it + 1), new_point, N_x, N_y, false);

                        _x2 = new_point.x;
                        _y2 = new_point.y;
                    }
                    else
                    {
                        Point new_point;

                        ParallelDrawContours::shiftContour(inner_point, *(it + 1), new_point, N_x, N_y, false);

                        _x3 = new_point.x;
                        _y3 = new_point.y;
                    }
                }
                // -------------------------

                // Установка нового значения (*it)
                if(it == inner_contours[j].begin())         // Первая точка
                {
                    it->x = _x1;
                    it->y = _y1;
                }
                else if(it + 1 == inner_contours[j].end())  // Последняя точка
                {
                    it->x = __x2;
                    it->y = __y2;
                }
                else                                        // Пересечение перенесённых отрезков
                {
                    double znam = (__y2*(_x2 - _x1) + _y3*(_x1 - _x2) + _y2*(_x3 - __x2) + _y1*(__x2 - _x3));

                    if(znam == 0.0 || isnan(znam))
                    {
                        it->x = __x2;
                        it->y = __y2;
                    }
                    else
                    {
                        it->x = (_x2*_y1*(__x2 - _x3) + _x1*_y2*(_x3 - __x2) + __x2*_y3*(_x1 - _x2) + _x3*__y2*(_x2 - _x1))/znam;
                        if(_x2 - _x1 == 0)
                            it->y = __y2;
                        else
                            it->y = (_x1*_y2 - _x2*_y1 - it->x*(_y2 - _y1)) / (_x1 - _x2);
                    }
                }
                // -------------------------------

                // Сдвиг _x1, _x2
                if(it != inner_contours[j].begin())
                {
                    _x1 = __x2;
                    _y1 = __y2;

                    _x2 = _x3;
                    _y2 = _y3;
                }
                // --------------
            }
            /////////////////////////////////////

            // Получение границы внешнего допуска
            _x1 = 0, _y1 = 0;
            _x2 = 0, _y2 = 0;
            __x2 = 0, __y2 = 0;
            _x3 = 0, _y3 = 0;

            for(auto it = outer_contours[j].begin(); it != outer_contours[j].end(); it++)
            {
                if(abs(it->y - _bottom.y) < abs(it->y - _top.y))
                    inner_line_pt1 = _bottom;
                else
                    inner_line_pt1 = _top;

                if(it->x > inner_line_pt1.x)
                    inner_line_pt2 = _right;
                else
                    inner_line_pt2 = _left;

                if(it->x < _left.x)
                    inner_point = _left;
                else if(it->x > _right.x)
                    inner_point = _right;
                else
                {
                    inner_point.x = it->x;
                    inner_point.y = (inner_point.x - inner_line_pt1.x)*(inner_line_pt2.y - inner_line_pt1.y)/(inner_line_pt2.x - inner_line_pt1.x) + inner_line_pt1.y;
                }

                //circle(image_contour_ref, inner_point, 5, Scalar(120, 150, 50));

                if(it + 1 == outer_contours[j].end())  // Последняя точка
                {
                    if(it != outer_contours[j].begin())
                    {
                        it->x = _x2;
                        it->y = _y2;
                    }
                    else
                    {
                        ParallelDrawContours::shiftContour(inner_point, *it, *it, N_x, N_y, true);
                    }

                    continue;
                }

                // Вычисление нормального отрезка
                int dx = it->y - (it+1)->y;
                int dy = (it+1)->x - it->x;

                double mod = qSqrt(static_cast<double>(dx * dx + dy * dy));

                N_x = (int)(dx / mod * external_tolerance); // Координаты нормального
                N_y = (int)(dy / mod * external_tolerance); // вектора
                // Конец вычисления нормального отрезка

                // Работа с текущим значением it
                if(it == outer_contours[j].begin())
                {
                    Point new_point;

                    ParallelDrawContours::shiftContour(inner_point, *it, new_point, N_x, N_y, true);

                    _x1 = new_point.x;
                    _y1 = new_point.y;
                }
                else
                {
                    Point new_point;

                    ParallelDrawContours::shiftContour(inner_point, *it, new_point, N_x, N_y, true);

                    __x2 = new_point.x;
                    __y2 = new_point.y;
                }
                // -----------------------------

                // Работа со значением it + 1
                if(it + 1 != outer_contours[j].end())
                {
                    if(it == outer_contours[j].begin())
                    {
                        Point new_point;

                        ParallelDrawContours::shiftContour(inner_point, *(it + 1), new_point, N_x, N_y, true);

                        _x2 = new_point.x;
                        _y2 = new_point.y;
                    }
                    else
                    {
                        Point new_point;

                        ParallelDrawContours::shiftContour(inner_point, *(it + 1), new_point, N_x, N_y, true);

                        _x3 = new_point.x;
                        _y3 = new_point.y;
                    }
                }
                // -------------------------

                // Установка нового значения (*it)
                if(it == outer_contours[j].begin())         // Первая точка
                {
                    it->x = _x1;
                    it->y = _y1;
                }
                else if(it + 1 == outer_contours[j].end())  // Последняя точка
                {
                    it->x = __x2;
                    it->y = __y2;
                }
                else                                        // Пересечение перенесённых отрезков
                {
                    double znam = (__y2*(_x2 - _x1) + _y3*(_x1 - _x2) + _y2*(_x3 - __x2) + _y1*(__x2 - _x3));

                    if(znam == 0.0 || isnan(znam))
                    {
                        it->x = __x2;
                        it->y = __y2;
                    }
                    else
                    {
                        it->x = (_x2*_y1*(__x2 - _x3) + _x1*_y2*(_x3 - __x2) + __x2*_y3*(_x1 - _x2) + _x3*__y2*(_x2 - _x1))/znam;
                        if(_x2 - _x1 == 0)
                            it->y = __y2;
                        else
                            it->y = (_x1*_y2 - _x2*_y1 - it->x*(_y2 - _y1)) / (_x1 - _x2);
                    }
                }
                // -------------------------------

                // Сдвиг _x1, _x2
                if(it != outer_contours[j].begin())
                {
                    _x1 = __x2;
                    _y1 = __y2;

                    _x2 = _x3;
                    _y2 = _y3;
                }
                // --------------
            }
            /////////////////////////////////////
        }

        // Рисование контуров
        {
            // Вывод контура детали
            if(true || image_type == getCurrent().getImage())
                drawContours( image_contour_ref, detail_contours, j, detail_contour_color, thickness, LINE_AA, contours_hierarchy, 0, Point(0, 0) );

            if(image_type == ETALON_IMAGE)
            {
                // Вывод контура внутреннего допуска
                drawContours( image_contour_ref, inner_contours, j, inner_contour_color, 1, LINE_AA, contours_hierarchy, 0, Point(0, 0) );

                // Вывод контура внешнего допуска
                drawContours( image_contour_ref, outer_contours, j, outer_contour_color, 1, LINE_AA, contours_hierarchy, 0, Point(0, 0) );
            }
        }
        /////////////////////
    }
#else // PARALLEL_CONTOUR_COMPUTING

    if(puanson_image.isIdealContourSet())
    {
        const quint16 contours_per_thread = 10; // Количество контуров, обрабатываемых одним потоком

        qDebug() << "this " << this << "before parallel_for_ " << QTime::currentTime();

        quint16 ext_tolerance_px, int_tolerance_px;

        puanson_image.getToleranceFields(ext_tolerance_px, int_tolerance_px);

        parallel_for_(Range(0, detail_contours.size() / contours_per_thread),
                      ParallelDrawContours(puanson_image, image_contour, detail_contours, contours_per_thread, contours_hierarchy,
                                           draw_etalon_contour_flag, ext_tolerance_px, int_tolerance_px));

        qDebug() << "this " << this << "after parallel_for_ " << QTime::currentTime();

        /*for(int j = 0; j < detail_contours.size(); j++)
            drawContours( image_contour, detail_contours, j, color, thickness, 8, contours_hierarchy, 0, Point() );*/
    }
#endif // PARALLEL_CONTOUR_COMPUTING

    // Рисование внутренних линий детали
    if(false)
    {
        /*cv::Point _top, _right, _bottom, _left;

        _top = cv::Point(inner_skeleton_top.x(), inner_skeleton_top.y());
        _right = cv::Point(inner_skeleton_right.x(), inner_skeleton_right.y());
        _bottom = cv::Point(inner_skeleton_bottom.x(), inner_skeleton_bottom.y());
        _left = cv::Point(inner_skeleton_left.x(), inner_skeleton_left.y());*/

        /*line(image_contour, _top, _right, inner_line_color, 3, LINE_AA);
        line(image_contour, _right, _bottom, inner_line_color, 3, LINE_AA);
        line(image_contour, _bottom, _left, inner_line_color, 3, LINE_AA);
        line(image_contour, _left, _top, inner_line_color, 3, LINE_AA);*/
    }
    //////////////////////////////

    bin1.release();
    gray_image.release();
    gray_roi.release();
    //image_contour.release();
    return image_contour;
}

void PuansonChecker::drawReferencePointOnContoursImage(const QPaintDevice &paint_device, const QPoint &position, const Qt::GlobalColor color, const QString &text)
{
    QPainter painter;
    painter.begin(const_cast<QPaintDevice *>(&paint_device));
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(QBrush(color, Qt::Dense5Pattern));
    QRect rect(position.x() - GRAPHICAL_POINT_RADIUS, position.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2);
    painter.drawEllipse(rect);
    painter.setPen(QPen(color, 1));
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(QPointF(position.x() - GRAPHICAL_POINT_RADIUS - 60, position.y() + GRAPHICAL_POINT_RADIUS + 12), text);
    painter.end();
}

//#define DRAW_LINES_BETWEEN_REFERENCE_POINS

void PuansonChecker::drawContoursImage()
{
    using namespace cv;
    using namespace std;

    if(!(isCurrentImageLoaded() && isEtalonImageLoaded(getEtalonAngle())))
        return;

    Mat current_contour = getCurrent().getImageContour();
    Mat current_contours_mask(current_contour.size(), CV_8UC1);

    contours_image.release();
    etalon_contour.copyTo(contours_image);

    cvtColor(current_contour, current_contours_mask, CV_RGB2GRAY);
    current_contour.copyTo(contours_image, current_contours_mask);
    current_contours_mask.release();

    QImage img(QImage((const unsigned char*)(contours_image.data),
                  contours_image.cols, contours_image.rows,
                  contours_image.step, QImage::Format_RGB888).rgbSwapped());

    // Рисование реперных точек
    QPoint etalon_reference_point_1, etalon_reference_point_2;
    QPoint current_reference_point_1, current_reference_point_2;

    getEtalon(getEtalonAngle()).getReferencePoints(etalon_reference_point_1, etalon_reference_point_2);
    getCurrent().getReferencePoints(current_reference_point_1, current_reference_point_2);

    if(etalon_reference_point_1 != etalon_reference_point_2)
    {
        drawReferencePointOnContoursImage(img, etalon_reference_point_1, Qt::green, QString(etalon_reference_point_1 != current_reference_point_1 ? "Реперная точка 1 эталона" : "Совмещённая реперная точка 1"));
        drawReferencePointOnContoursImage(img, etalon_reference_point_2, Qt::green, QString(etalon_reference_point_2 != current_reference_point_2 ? "Реперная точка 2 эталона" : "Совмещённая реперная точка 2"));

#if defined(DRAW_LINES_BETWEEN_REFERENCE_POINS)
        {
            QPainter painter;
            painter.begin(dynamic_cast<QPaintDevice *>(&img));
            painter.setPen(QPen(Qt::green, 3));
            painter.setBrush(QBrush(Qt::green, Qt::Dense5Pattern));
            painter.drawLine(reference_point_1, reference_point_2);
            painter.end();
        }
#endif // DRAW_LINES_BETWEEN_REFERENCE_POINS
    }

    if(current_reference_point_1 != current_reference_point_2)
    {
        if(etalon_reference_point_1 != current_reference_point_1)
            drawReferencePointOnContoursImage(img, current_reference_point_1, Qt::red, QString("Реперная точка 1 текущей детали"));

        if(etalon_reference_point_2 != current_reference_point_2)
            drawReferencePointOnContoursImage(img, current_reference_point_2, Qt::red, QString("Реперная точка 2 текущей детали"));

#if defined(DRAW_LINES_BETWEEN_REFERENCE_POINS)
        {
            QPainter painter;
            painter.begin(dynamic_cast<QPaintDevice *>(&img));
            painter.setPen(QPen(Qt::red, 3));
            painter.setBrush(QBrush(Qt::red, Qt::Dense5Pattern));
            painter.drawLine(reference_point_1, reference_point_2);
            painter.end();
        }
#endif // DRAW_LINES_BETWEEN_REFERENCE_POINS
    }
    ////////////////////////
    contours_window->drawImage(img);
    //contours_window->drawIdealContour(getEtalon(getEtalonAngle()).getIdealContourPath());
}

void PuansonChecker::shiftCurrentImage(const qreal dx, const qreal dy)
{
    using namespace cv;

    Mat &current_contour = getCurrent().getImageContour();

    if(!current_contour.empty())
    {
        Point2f srcTri[3];
        Point2f dstTri[3];
        Mat warp_mat( 2, 3, CV_32FC1 );

        QPoint current_reference_point1;
        QPoint current_reference_point2;

        getCurrent().getReferencePoints(current_reference_point1, current_reference_point2);

        srcTri[0] = Point2f( 0,0 );
        srcTri[1] = Point2f( current_contour.cols - 1.f, 0 );
        srcTri[2] = Point2f( 0, current_contour.rows - 1.f );
        dstTri[0] = Point2f( dx, dy );
        dstTri[1] = Point2f( current_contour.cols - 1.f + dx, dy );
        dstTri[2] = Point2f( dx, current_contour.rows - 1.f + dy);

        warp_mat = getAffineTransform( srcTri, dstTri );

        warpAffine( getCurrent().getImage(), getCurrent().getImage(), warp_mat, getCurrent().getImage().size() );
        warpAffine( current_contour, current_contour, warp_mat, current_contour.size() );

        warp_mat.release();

        current_reference_point1 += QPoint(dx, dy);
        current_reference_point2 += QPoint(dx, dy);

        getCurrent().setReferencePoints(current_reference_point1, current_reference_point2);
    }
}

void PuansonChecker::rotateCurrentImage(const double angle)
{
    using namespace cv;

    Mat &current_contour = getCurrent().getImageContour();

    if(!current_contour.empty())
    {
        Mat rot_mat( 2, 3, CV_32FC1 );
        Point center_of_rotation;
        double scale = 1.0;

        QPoint current_reference_point1;
        QPoint current_reference_point2;

        double angle_in_degrees = qRadiansToDegrees(angle);

        getCurrent().getReferencePoints(current_reference_point1, current_reference_point2);

        if(getCurrent().isReferencePointsAreSet())
            center_of_rotation = Point(current_reference_point1.x(), current_reference_point1.y());
        else
            center_of_rotation = Point( current_contour.cols / 2 + 1, current_contour.rows / 2 + 1 );

        rot_mat = getRotationMatrix2D( center_of_rotation, angle_in_degrees, scale );

        warpAffine( getCurrent().getImage(), getCurrent().getImage(), rot_mat, getCurrent().getImage().size() );

        if(getCurrent().isReferencePointsAreSet())
        {
            //Mat reference_point1_column_vector = (Mat_<int>(3,1) << current_reference_point1.x(), current_reference_point1.y());
            //reference_point1_column_vector = rot_mat.mul(reference_point1_column_vector);
            //current_reference_point1 = QPoint(reference_point1_column_vector.at<int>(0,0), reference_point1_column_vector.at<int>(1,0));

            Mat reference_point2_column_vector = (Mat_<double>(3,1) << current_reference_point2.x(), current_reference_point2.y(), 1.0);
            reference_point2_column_vector = rot_mat * reference_point2_column_vector; // Нужно именно так множить, согласно док-ции OpenCV
            current_reference_point2 = QPointF(reference_point2_column_vector.at<double>(0,0), reference_point2_column_vector.at<double>(1,0)).toPoint();
            getCurrent().setReferencePoints(current_reference_point1, current_reference_point2);
        }

//        if(this->getCalculateContourOnRotationFlag())
//        {
            current_contour.release();
            current_puanson_image.setImageContour(getContour(current_puanson_image));
//        }
//        else
//        {
//            warpAffine( current_contour, current_contour, rot_mat, current_contour.size() );
//        }
    }
}

bool PuansonChecker::combineImagesByReferencePoints()
{
    if((isEtalonImageLoaded(etalon_angle) && getEtalon(etalon_angle).isReferencePointsAreSet()) &&
       (isCurrentImageLoaded() && getCurrent().isReferencePointsAreSet()))
    {
        QPoint etalon_reference_point1, etalon_reference_point2;
        QPoint current_reference_point1, current_reference_point2;

        getEtalon(etalon_angle).getReferencePoints(etalon_reference_point1, etalon_reference_point2);
        getCurrent().getReferencePoints(current_reference_point1, current_reference_point2);

        // Сдвиг изображения текущей детали для совмещения изображений по первой реперной точке
        qreal shift_dx, shift_dy;

        shift_dx = (etalon_reference_point1-current_reference_point1).x();
        shift_dy = (etalon_reference_point1-current_reference_point1).y();

        shiftCurrentImage(shift_dx, shift_dy);
        ///////////////////////////////////////////////////////////////////////////////////////

        getCurrent().getReferencePoints(current_reference_point1, current_reference_point2);

        // Поворот изображения текущей детали для совмещения изображений по второй реперной точке
        qreal x_etal = etalon_reference_point2.x() - etalon_reference_point1.x();
        qreal x_cur = current_reference_point2.x() - current_reference_point1.x();

        qreal y_etal = etalon_reference_point2.y() - etalon_reference_point1.y();
        qreal y_cur = current_reference_point2.y() - current_reference_point1.y();

        qreal cos_value = (x_etal*x_cur + y_etal*y_cur) / (qSqrt(x_etal*x_etal + y_etal*y_etal) * qSqrt(x_cur*x_cur + y_cur*y_cur));
        qreal rotation_angle_in_radians = qAcos(cos_value);

        // Вычисление косого произведения векторов (Rep2_etal - Rep1_etal) и (Rep2_cur - Rep1_cur) для определения направления вращения
        qreal pseudoscalar_product = x_etal*y_cur - y_etal*x_cur;
        if(pseudoscalar_product < 0)
            rotation_angle_in_radians = -rotation_angle_in_radians;

        rotateCurrentImage(rotation_angle_in_radians);
        ///////////////////////////////////////////////////////////////////////////////////////

        QString combine_transformation_message;
        combine_transformation_message = "Изображения совмещены: сдвиг dx " + QString::number(shift_dx) + " dy " + QString::number(shift_dy) + "; угол поворота " + QString::number(rotation_angle_in_radians) +  " радиан.";

        QMessageBox msgBox;
        msgBox.setText(combine_transformation_message);
        msgBox.setModal(false);
        msgBox.show();

        return true;
    }

    return false;
}

bool PuansonChecker::saveCurrentImage(const QString &path)
{
    using namespace cv;
    using namespace std;

    bool res = true;

    if(path.endsWith(".tiff", Qt::CaseInsensitive))
    {
        imwrite(path.toStdString().c_str(), getCurrent().getImage());
    }
    else if(path.endsWith(".png", Qt::CaseInsensitive))
    {
        vector<int> compression_params;

        compression_params.push_back(IMWRITE_PNG_COMPRESSION);
        compression_params.push_back(3);

        imwrite(path.toStdString().c_str(), getCurrent().getImage(), compression_params);
    }
    else if(path.endsWith(".jpg", Qt::CaseInsensitive) || path.endsWith(".jpeg", Qt::CaseInsensitive))
    {
        vector<int> compression_params;

        compression_params.push_back(IMWRITE_JPEG_QUALITY);
        compression_params.push_back(95);

        imwrite(path.toStdString().c_str(), getCurrent().getImage(), compression_params);
    }
    else
        res = false;

    return res;
}

bool PuansonChecker::saveResultImage(const QString &path)
{
    using namespace cv;
    using namespace std;

    bool res = true;

    if(path.endsWith(".tiff", Qt::CaseInsensitive) || path.endsWith(".tif", Qt::CaseInsensitive))
    {
        imwrite(path.toStdString().c_str(), contours_image);
    }
    else if(path.endsWith(".png", Qt::CaseInsensitive))
    {
        vector<int> compression_params;

        compression_params.push_back(IMWRITE_PNG_COMPRESSION);
        compression_params.push_back(3);

        imwrite(path.toStdString().c_str(), contours_image, compression_params);
    }
    else if(path.endsWith(".jpg", Qt::CaseInsensitive) || path.endsWith(".jpeg", Qt::CaseInsensitive))
    {
        vector<int> compression_params;

        compression_params.push_back(IMWRITE_JPEG_QUALITY);
        compression_params.push_back(95);

        imwrite(path.toStdString().c_str(), contours_image, compression_params);
    }
    else
        res = false;

    return res;
}
