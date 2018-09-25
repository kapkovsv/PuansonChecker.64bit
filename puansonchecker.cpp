#include "puansonchecker.h"
#include "settingsdialog.h"
#include "checkdetailvalidateddialog.h"

#include "mainwindow.h"
#include "currentform.h"
#include "contoursform.h"

#include "detailanglesourcedialog.h"
#include "etalonangleresultconfirmdialog.h"
#include "etalonresearchcreationdialog.h"
#include "currentresearchcreationdialog.h"

#include <QtMath>
#include <vector>
#include <functional>

#include <QMessageBox>
#include <QDebug>

PuansonChecker *PuansonChecker::instance = Q_NULLPTR;

const cv::Vec3b PuansonChecker::empty_contour_color = cv::Vec3b(180, 180, 180);
const cv::Vec3b PuansonChecker::empty_contour_color_2 = cv::Vec3b(0, 0, 0);

const cv::Vec3b PuansonChecker::inner_contour_color = cv::Vec3b(250, 100, 100);
const cv::Vec3b PuansonChecker::outer_contour_color = cv::Vec3b(100, 250, 100);
const cv::Vec3b PuansonChecker::etalon_contour_color = cv::Vec3b( 255, 255, 255 );
const cv::Vec3b PuansonChecker::current_contour_color = cv::Vec3b( 50, 50, 250 );

PuansonChecker *PuansonChecker::getInstance(const QApplication *app)
{
    if(instance == Q_NULLPTR)
        instance = new PuansonChecker(app);

    return instance;
}

int PuansonChecker::loadImage(ImageType_e image_type, const QString &path, PuansonImage &output, bool save_attributes)
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
    cvtColor(loaded_image, loaded_image, COLOR_BGR2RGB);

    if(save_attributes)
        output ^= PuansonImage(image_type, loaded_image, raw_image_ptr, path);
    else
        output = PuansonImage(image_type, loaded_image, raw_image_ptr, path);

    return 0;
}

PuansonChecker::PuansonChecker(const QApplication *app)
{
    application = const_cast<QApplication *>(app);
    general_settings = new GeneralSettings(CONFIGURATION_FILE);
    camera = new PhotoCamera();
    machine = new PuansonMachine();

    CannyThreshold1 = DEFAULT_CANNY_THRES_1;
    CannyThreshold2 = DEFAULT_CANNY_THRES_2;

    draw_etalon_contour_flag = false;
    //calculate_contour_on_rotation = false;

    main_window = new MainWindow();
    current_image_window = new CurrentForm(this);
    contours_window = new ContoursForm(this);

    current_image_window->show();
    contours_window->show();
    main_window->show();

    loading_etalon_angle = 0;

    quint16 ext_tolerance_mkm_array[PUANSON_IMAGE_MAX_ANGLE];
    quint16 int_tolerance_mkm_array[PUANSON_IMAGE_MAX_ANGLE];

    quint32 reference_points_distance;

    general_settings->getToleranceFields(ext_tolerance_mkm_array, int_tolerance_mkm_array);
    general_settings->getReferencePointDistanceMkm(reference_points_distance);

//    for(int angle = 1; angle <= PUANSON_IMAGE_MAX_ANGLE; angle++)
//    {
        getEtalon().setToleranceFields(ext_tolerance_mkm_array[/*angle - 1*/0], int_tolerance_mkm_array[/*angle - 1*/0]);
        getEtalon().setReferencePointDistanceMkm(reference_points_distance);
//    }

    QMap<quint8, QPoint> detail_photo_shooting_positions = general_settings->getDetailPhotoShootingPositions();
    machine->setAnglePositions(detail_photo_shooting_positions);

    ignore_scroll_move_image = false;

    connect(&load_image_watcher, SIGNAL(finished()), SLOT(loadImageFinished()));
    //connect(camera, SIGNAL(cameraConnectionStatusChanged(bool)), SLOT(cameraConnectionStatusChangedSlot(bool)));
}

PuansonChecker::~PuansonChecker()
{
    if(load_image_future.isRunning())
        load_image_future.waitForFinished();

    delete general_settings;
    general_settings = Q_NULLPTR;

    delete camera;
    camera = Q_NULLPTR;

    delete machine;
    machine = Q_NULLPTR;

    delete main_window;
    main_window = Q_NULLPTR;

    delete current_image_window;
    current_image_window = Q_NULLPTR;

    delete contours_window;
    contours_window = Q_NULLPTR;
}

void PuansonChecker::updateContoursImage()
{
    PuansonImage &current_image_ref = getCurrent();

    if(!etalon_puanson_image.imageContour().empty())
    {
       if(etalon_puanson_image.isIdealContourSet())
           current_image_ref.copyIdealContour(etalon_puanson_image);

       etalon_puanson_image.imageContour().release();
       etalon_puanson_image.setImageContour(getContour(etalon_puanson_image));

       if(!current_image_ref.isEmpty())
       {
           current_image_ref.imageContour().release();
           current_image_ref.setImageContour(getContour(current_image_ref));
       }

       if(!current_image_ref.imageContour().empty())
           drawContoursImage();
    }
}

void PuansonChecker::showSettingsDialog()
{
    SettingsDialog settings_dialog;

    settings_dialog.setFixedSize(settings_dialog.size());

    switch( settings_dialog.exec() )
    {
       case QDialog::Accepted:
            main_window->setWindowStatus("Применение настроек ...");

            if(getEtalon().isIdealContourSet())
                main_window->drawIdealContour(etalon_puanson_image.getIdealContourPointOfOrigin().toPoint(), etalon_puanson_image.getIdealContourRotationAngle());  //  Нужно для пересчёта нормальных отрезков

//PuansonChecker::getInstance()->drawEtalonImage();
            updateContoursImage();
            main_window->setWindowStatus("");
           break;
       case QDialog::Rejected:
           break;
       default:
           break;
    }
}

bool PuansonChecker::showConfirmCheckResultCorrectnessDialog(bool check_result)
{
    CheckDetailValidatedDialog check_detail_validated_dialog(check_result);
    bool return_value = false;

    check_detail_validated_dialog.setModal(false);
    check_detail_validated_dialog.setFixedSize(check_detail_validated_dialog.size());

    switch( check_detail_validated_dialog.exec() )
    {
       case QDialog::Accepted:
           return_value = true;
           break;
       case QDialog::Rejected:
           return_value = false;
           break;
       default:
           break;
    }

    return return_value;
}

void PuansonChecker::quit()
{
    application->quit();
}

void PuansonChecker::activateContoursWindow() const
{
    contours_window->show();
    contours_window->activateWindow();
}

void PuansonChecker::activateCurrentImageWindow() const
{
    current_image_window->show();
    current_image_window->activateWindow();
}

void PuansonChecker::loadCurrentImage(const QString &path)
{
    load_image_future = QtConcurrent::run([&, path]() {
        PuansonImage &current_image = getCurrent();

        current_image.release();

        if(PuansonChecker::loadImage(ImageType_e::CURRENT_IMAGE, path, current_image) == 0)
        {
            if(etalon_puanson_image.isIdealContourSet())
                current_image.copyIdealContour(etalon_puanson_image);

            if(getActiveCurrentResearchAngle() == 0)
                current_image.setImageContour(getContour(current_image));

            current_image.setToleranceFields(0, 0);
        }
        /*else
            qDebug() << "PuansonChecker::loadCurrentImage error, current image!";*/

        return current_image.getImageType();
    });

    load_image_watcher.setFuture(load_image_future);
}

bool PuansonChecker::loadEtalonImage(const quint8 angle, const QString &path, const QString &contours_path, bool save_attributes)
{
    if(angle > PUANSON_IMAGE_MAX_ANGLE)
        return false;

    loading_etalon_angle = angle;
    PuansonImage *etalon_puanson_image_ptr = &etalon_puanson_image;

    load_image_future = QtConcurrent::run([=]() {
        etalon_puanson_image_ptr->release();

        if(PuansonChecker::loadImage(ImageType_e::ETALON_IMAGE, path, *etalon_puanson_image_ptr, save_attributes) == 0)
        {
            loading_etalon_angle = angle;

            quint16 ext_tolerance_mkm_array[PUANSON_IMAGE_MAX_ANGLE];
            quint16 int_tolerance_mkm_array[PUANSON_IMAGE_MAX_ANGLE];
            quint32 reference_points_distance;

            general_settings->getToleranceFields(ext_tolerance_mkm_array, int_tolerance_mkm_array);
            general_settings->getReferencePointDistanceMkm(reference_points_distance);

            etalon_puanson_image_ptr->setToleranceFields(ext_tolerance_mkm_array[loading_etalon_angle - 1], int_tolerance_mkm_array[loading_etalon_angle - 1]);
            etalon_puanson_image_ptr->setReferencePointDistanceMkm(reference_points_distance);

            if(/*contours_path.isEmpty()*/true)
            {
                etalon_puanson_image_ptr->drawIdealContour(QRect(2, 2, etalon_puanson_image_ptr->getImage().cols - 2, etalon_puanson_image_ptr->getImage().rows - 2), etalon_puanson_image_ptr->getIdealContourPointOfOrigin(), etalon_puanson_image_ptr->getIdealContourRotationAngle());
                etalon_puanson_image_ptr->setImageContour(getContour(*etalon_puanson_image_ptr));
            }
            else
            {
                etalon_puanson_image_ptr->setImageContour(cv::imread(contours_path.toStdString()));
            }
        }

        return ImageType_e::ETALON_IMAGE;
    });

    load_image_watcher.setFuture(load_image_future);

    return true;
}

void PuansonChecker::loadImageFinished()
{
    ImageType_e image_type = load_image_watcher.result();

    main_window->loadImageFinished(image_type);

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
    cv::Mat current_contour = getCurrent().imageContour();

    if(current_contour.empty())
        return false;

    img = QImage(reinterpret_cast<const unsigned char*>(current_contour.data),
                  current_contour.cols, current_contour.rows,
                  static_cast<qint32>(current_contour.step), QImage::Format_RGB888).rgbSwapped();

    return true;
}

bool PuansonChecker::getEtalonImage(QImage &img)
{
    getEtalon().getQImage(img);

    return true;
}

bool PuansonChecker::getEtalonContour(QImage &img)
{
    if(etalon_puanson_image.imageContour().empty())
        return false;

    img = QImage(reinterpret_cast<const unsigned char*>(etalon_puanson_image.imageContour().data),
                  etalon_puanson_image.imageContour().cols, etalon_puanson_image.imageContour().rows,
                  static_cast<qint32>(etalon_puanson_image.imageContour().step), QImage::Format_Grayscale8);

    return true;
}

QVector<QPointF> PuansonChecker::findReferencePoints(const PuansonImage &reference_point_image) const
{
    QVector<QPointF> reference_points;

    reference_points.append(findReferencePoint(ReferencePointType_e::REFERENCE_POINT_1, reference_point_image));
    reference_points.append(findReferencePoint(ReferencePointType_e::REFERENCE_POINT_2, reference_point_image));

    return reference_points;
}

QPointF PuansonChecker::findReferencePoint(const ReferencePointType_e reference_point_type, const PuansonImage &reference_point_image)
{
    using namespace std;
    using namespace cv;

    Rect search_area;
    QRect q_search_area;

    q_search_area = PuansonChecker::getInstance()->getReferencePointSearchArea(reference_point_image.getImageType(), reference_point_type);

    if(q_search_area.topLeft().x() > reference_point_image.getImage().cols - 1 || q_search_area.topLeft().y() > reference_point_image.getImage().rows - 1)
        return QPointF();

    if(q_search_area.bottomRight().x() > reference_point_image.getImage().cols - 1)
        q_search_area.setRight(reference_point_image.getImage().cols - 1);

    if(q_search_area.bottomRight().y() > reference_point_image.getImage().rows - 1)
        q_search_area.setBottom(reference_point_image.getImage().rows - 1);

    search_area = Rect(q_search_area.x(), q_search_area.y(), q_search_area.width(), q_search_area.height());

    const Mat &colour_image = reference_point_image.getImage();
    Mat gray_image;

    cvtColor(colour_image, gray_image, COLOR_RGB2GRAY);

    Mat gray_image_reference_point_area = gray_image(search_area).clone();

    // Применяем фильтр
    medianBlur(gray_image_reference_point_area, gray_image_reference_point_area, 3);
    threshold(gray_image_reference_point_area, gray_image_reference_point_area, 50, 250, THRESH_BINARY);

    // Поиск средних точек
    int upper_level_y;
    int left_level_x;

    Point upper_point;
    Point left_point;

    Point incoming_point, outcoming_point;
    uchar point_color;

    vector<Point> vertical_points;

    for(upper_level_y = 1; upper_level_y < gray_image_reference_point_area.rows; upper_level_y += 5)
    {
        incoming_point = Point();
        outcoming_point = Point();

        for(int x = 0; x < gray_image_reference_point_area.cols; x++)
        {
            point_color = gray_image_reference_point_area.at<uchar>(upper_level_y, x);

            if(x != 0 && point_color == 0/*black*/ && incoming_point == Point(0, 0))
                incoming_point = Point(x, upper_level_y);
            else if(outcoming_point == Point(0, 0) && incoming_point != Point(0, 0) && point_color >= 200/*white*/)
            {
                outcoming_point = Point(x, upper_level_y);
                break;
            }
        }

        upper_point = Point();

        if(incoming_point != Point(0, 0) && outcoming_point != Point(0, 0))
        {
            upper_point = (incoming_point + outcoming_point) / 2;
        }
        else if(!vertical_points.empty())
        {
            upper_point.y = vertical_points.back().y + 5;
        }

        if(!vertical_points.empty() &&
            ((incoming_point == Point(0, 0) && outcoming_point == Point(0, 0)) || (abs(vertical_points.back().x - upper_point.x) > 1)))
            upper_point.x = vertical_points.back().x;

        if(upper_point != Point(0, 0))
            vertical_points.push_back(upper_point);
        else
            qDebug() << "!!!!! upper_point is NULL!!!";
    }

    vector<Point> horizontal_points;

    for(left_level_x = 1; left_level_x < gray_image_reference_point_area.cols; left_level_x += 5)
    {
        incoming_point = Point();
        outcoming_point = Point();

        for(int y = 0; y < gray_image_reference_point_area.rows; y++)
        {
            point_color = gray_image_reference_point_area.at<uchar>(y, left_level_x);

            if(point_color == 0/*black*/ && incoming_point == Point(0, 0))
                incoming_point = Point(left_level_x, y);
            else if(outcoming_point == Point(0, 0) && incoming_point != Point(0, 0) && point_color >= 200/*white*/)
            {
                outcoming_point = Point(left_level_x, y);
                break;
            }
        }

        if(incoming_point != Point(0, 0) && outcoming_point != Point(0, 0))
        {
            left_point = (incoming_point + outcoming_point) / 2;
        }
        else
            left_point.x = horizontal_points.back().x + 5;

        if(!horizontal_points.empty() &&
            ((incoming_point == Point(0, 0) && outcoming_point == Point(0, 0)) || (abs(horizontal_points.back().y - left_point.y) > 1)))
            left_point.y = horizontal_points.back().y;

        horizontal_points.push_back(left_point);
    }
    // ------------------

    // Аппроксимация прямой МНК
    int vertical_sum_xy = 0;
    int vertical_sum_x = 0;
    int vertical_sum_y = 0;
    int vertical_sum_x2 = 0;

    int horizontal_sum_xy = 0;
    int horizontal_sum_x = 0;
    int horizontal_sum_y = 0;
    int horizontal_sum_x2 = 0;

    for(const Point& pt: vertical_points)
    {
        vertical_sum_xy += pt.x * pt.y;
        vertical_sum_x += pt.x;
        vertical_sum_y += pt.y;
        vertical_sum_x2 += pt.x * pt.x;

        circle(gray_image_reference_point_area, pt, 5, Scalar(250, 250, 250), 1);
    }

    for(const Point& pt: horizontal_points)
    {
        horizontal_sum_xy += pt.x * pt.y;
        horizontal_sum_x += pt.x;
        horizontal_sum_y += pt.y;
        horizontal_sum_x2 += pt.x * pt.x;

        circle(gray_image_reference_point_area, pt, 5, Scalar(250, 250, 250), 1);
    }

    double A_vertical = (static_cast<double>(static_cast<qint32>(vertical_points.size()) * vertical_sum_xy) - vertical_sum_x * vertical_sum_y) / (static_cast<double>(static_cast<qint32>(vertical_points.size()) * vertical_sum_x2) - vertical_sum_x * vertical_sum_x);
    double B_vertical = static_cast<double>(vertical_sum_y - A_vertical * vertical_sum_x) / static_cast<qint32>(vertical_points.size());

    double A_horizontal = (static_cast<double>(static_cast<qint32>(horizontal_points.size()) * horizontal_sum_xy) - horizontal_sum_x * horizontal_sum_y) / (static_cast<qint32>(horizontal_points.size()) * horizontal_sum_x2 - horizontal_sum_x * horizontal_sum_x);
    double B_horizontal = static_cast<double>(horizontal_sum_y - A_horizontal * horizontal_sum_x) / static_cast<quint32>(horizontal_points.size());
    // ------------------

    // Горизонтальная и вертикальная прямые
    Point p1(0, qRound(B_horizontal));
    Point p2(qRound(gray_image_reference_point_area.cols - 1.0), qRound(A_horizontal * (gray_image_reference_point_area.cols - 1.0) + B_horizontal));

    //line(gray_image_reference_point_area, p1, p2, Scalar(250, 250, 250));

    p1 = Point(- qRound(B_vertical / A_vertical), 0);
    p2 = Point(qRound(((gray_image_reference_point_area.rows - 1.0) - B_vertical) / A_vertical), qRound(gray_image_reference_point_area.rows - 1.0));

    //line(gray_image_reference_point_area, p1, p2, Scalar(250, 250, 250));
    // ------------------

    // Искомая реперная точка
    QPointF reference_point;

    reference_point.setX((B_vertical - B_horizontal) / (A_horizontal - A_vertical));
    reference_point.setY(A_horizontal * reference_point.x() + B_horizontal);

    return QPointF(search_area.x, search_area.y) + reference_point;
    // -----------------------
}

bool PuansonChecker::checkDetail(QVector<QPoint> &bad_points)
{
    using namespace std;

    bool result = true;

    QPainterPath control_path;
    QVector<QLine> control_lines;

    bool up_dir_flag;

    int current_x = -1, current_y = -1, next_y = -1;
    QLine last_line;
    cv::Point inner_point, outer_point, current_detail_point, bad_point;

    //function<bool (const int, const QLine &)> upConditionLambda = [](const int current_x, const QLine &control_line) -> bool { qDebug() << "in upConditionLambda current_x " << current_x << " x1 " << control_line.x1() << " x2 " << control_line.x2(); return current_x <= (control_line.x1() > control_line.x2() ? control_line.x1() : control_line.x2()); };
    //function<bool (const int, const QLine &)> downConditionLambda = [](const int current_x, const QLine &control_line) -> bool { qDebug() << "in downConditionLambda current_x " << current_x << " x1 " << control_line.x1() << " x2 " << control_line.x2(); return current_x >= (control_line.x1() < control_line.x2() ? control_line.x1() : control_line.x2()); };

    function<bool (const int, const QLine &)> conditionLambda = [](const int current_x, const QLine &control_line) -> bool { return (current_x <= (control_line.x1() > control_line.x2() ? control_line.x1() : control_line.x2())) && (current_x >= (control_line.x1() < control_line.x2() ? control_line.x1() : control_line.x2())); };

    QVector<IdealInnerSegment> inner_segments = PuansonChecker::getInstance()->getEtalon().getActualIdealInnerSegments();

    QRect analysis_rect(qRound((contours_image.cols - contours_image.cols / 4.0) / 2.0),
            qRound((contours_image.rows - contours_image.rows / 4.0) / 2.0),
            qRound(contours_image.cols / 4.0),
            qRound(contours_image.rows / 4.0));

    for(IdealInnerSegment segment: inner_segments)
    {
        control_lines = segment.getControlLines(30);
        //conditionLambda = upConditionLambda;
        up_dir_flag = true;

        for(QLine control_line : control_lines)
        {
            control_path.moveTo(control_line.p1());
            control_path.lineTo(control_line.p2());

            if(control_line.x1() != control_line.x2())
            {
/*
                qDebug() << "New line " << control_line;

                qDebug() << "inner_contour_color: [" << inner_contour_color[0] << ", " << inner_contour_color[1] << ", " << inner_contour_color[2] << "]";
                qDebug() << "outer_contour_color: [" << outer_contour_color[0] << ", " << outer_contour_color[1] << ", " << outer_contour_color[2] << "]";
                qDebug() << "current_contour_color: [" << current_contour_color[0] << ", " << current_contour_color[1] << ", " << current_contour_color[2] << "]";
                qDebug() << "etalon_contour_color: [" << etalon_contour_color[0] << ", " << etalon_contour_color[1] << ", " << etalon_contour_color[2] << "]";
*/
                inner_point = outer_point = current_detail_point = bad_point = cv::Point(0, 0);

                int dx = control_line.x2() - control_line.x1();
                int dy = control_line.y2() - control_line.y1();

                current_x = up_dir_flag ? control_line.x1() : control_line.x2();
                current_y = qRound(static_cast<qreal>((current_x - control_line.x1()) * (dy)) / (dx) + control_line.y1());
                next_y = qRound(static_cast<qreal>(((up_dir_flag ? (current_x + dx/qAbs(dx)) : (current_x - dx/qAbs(dx))) - control_line.x1()) * (dy)) / (dx) + control_line.y1());

                bool previous_point_inside_analysis_rect = false;

                while(conditionLambda(current_x, control_line))
                {
                    if(previous_point_inside_analysis_rect == true && !analysis_rect.contains(QPoint(current_x, current_y)) && current_detail_point == cv::Point(0, 0))
                    {
                        if(inner_point == cv::Point(0, 0) && outer_point == cv::Point(0, 0))
                        {
                            inner_point = outer_point = current_detail_point = bad_point = cv::Point(0, 0);
                            last_line = control_line;

                            if(up_dir_flag)
                            {
                                //conditionLambda = downConditionLambda;
                                up_dir_flag = false;

                                current_x = control_line.x2();
                                current_y = control_line.y2();
                                next_y = qRound(static_cast<qreal>(((current_x - dx/qAbs(dx)) - control_line.x1()) * (dy)) / (dx) + control_line.y1());
                            }
                            else
                            {
                                //conditionLambda = upConditionLambda;
                                up_dir_flag = true;

                                current_x = control_line.x1();
                                current_y = control_line.y1();
                                next_y = qRound(static_cast<qreal>(((current_x + dx/qAbs(dx)) - control_line.x1()) * (dy)) / (dx) + control_line.y1());
                            }

                            if(analysis_rect.contains(QPoint(current_x, current_y)))
                            {
                                previous_point_inside_analysis_rect = false;
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }

                    if(previous_point_inside_analysis_rect == false && analysis_rect.contains(QPoint(current_x, current_y)))
                        previous_point_inside_analysis_rect = true;
                    else if(previous_point_inside_analysis_rect == true && !analysis_rect.contains(QPoint(current_x, current_y)))
                        previous_point_inside_analysis_rect = false;

                    cv::Vec3b current_point_color = contours_image.at<cv::Vec3b>(current_y, current_x);

                    //qDebug() << " line " << __LINE__ << "!!! current_point_color (" << current_x << ", " << current_y << "): [" << current_point_color[0] << ", " << current_point_color[1] << ", " << current_point_color[2] << "]";

                    if(current_point_color == empty_contour_color || current_point_color == empty_contour_color_2) // Если определена пустота, смотрим окрестности
                    {
                        const qreal normal_vector_len = 10.0;
                        QPointF normal_vector;

                        normal_vector.setX(qRound(normal_vector_len / qSqrt(1 + static_cast<qreal>(dx) / dy * static_cast<qreal>(dx) / dy)));
                        normal_vector.setY(qRound(-(static_cast<qreal>(dx) / dy) * normal_vector.x()));

                        QLine check_line;
                        qreal check_line_length;

                        check_line = QLine(QPoint(current_x, current_y) - normal_vector.toPoint(), QPoint(current_x, current_y) + normal_vector.toPoint());
#if 1
                        QPointF current_point = check_line.p1(), previous_interation_current_point = check_line.p1();
                        do
                        {
                          //  qDebug() << "check_line.p1(): " << check_line.p1() << "current_point: " << current_point << " check_line.p2(): " << check_line.p2();
                            current_point_color = contours_image.at<cv::Vec3b>(current_point.toPoint().y(), current_point.toPoint().x());

                            //qDebug() << " line " << __LINE__ << "!!! current_point_color (" << current_point.x() << ", " << current_point.y() << "): [" << current_point_color[0] << ", " << current_point_color[1] << ", " << current_point_color[2] << "]";

                            check_line_length = qSqrt(check_line.dx() * check_line.dx() + check_line.dy() * check_line.dy());
                            do
                            {
                                current_point += QPointF(static_cast<qreal>(check_line.dx()) / check_line_length, static_cast<qreal>(check_line.dy()) / check_line_length);
                            }
                            while(current_point.toPoint() == previous_interation_current_point);

                            previous_interation_current_point = current_point.toPoint();
                        }
                        while(current_point.toPoint() != check_line.p2() && !(current_point_color == inner_contour_color || current_point_color == outer_contour_color || current_point_color == current_contour_color));
#endif // 0
                        //qDebug() << "current_point_color (" << current_x << ", " << current_y << "): [" << current_point_color[0] << ", " << current_point_color[1] << ", " << current_point_color[2] << "]";

                        //line(contours_image, Point(check_line.x1(), check_line.y1()), Point(check_line.x2(), check_line.y2()), Scalar(180, 180, 180));
                    }

                    if(inner_point == cv::Point(0, 0) && current_point_color == inner_contour_color)
                    {
                        //qDebug() << "inner point!";

                        // Проверка на то, что направление поиска должно идти так, чтобы первой была найдена точка внешнего допуска
                        if(last_line != control_line && outer_point == cv::Point(0, 0))
                        {
                            inner_point = outer_point = current_detail_point = bad_point = cv::Point(0, 0);
                            last_line = control_line;

                            if(up_dir_flag)
                            {
                                //conditionLambda = downConditionLambda;
                                up_dir_flag = false;

                                current_x = control_line.x2();
                                current_y = control_line.y2();
                                next_y = qRound(static_cast<qreal>(((current_x - dx/qAbs(dx)) - control_line.x1()) * (dy)) / (dx) + control_line.y1());
                            }
                            else
                            {
                                //conditionLambda = upConditionLambda;
                                up_dir_flag = true;

                                current_x = control_line.x1();
                                current_y = control_line.y1();
                                next_y = qRound(static_cast<qreal>(((current_x + dx/qAbs(dx)) - control_line.x1()) * (dy)) / (dx) + control_line.y1());
                            }

                            if(analysis_rect.contains(QPoint(current_x, current_y)))
                            {
                                previous_point_inside_analysis_rect = false;
                                continue;
                            }

                            previous_point_inside_analysis_rect = false;
                            continue;
                        }
                        else
                        {
                            inner_point = cv::Point(current_x, current_y);
                        }
                    }
                    else if(outer_point == cv::Point(0, 0) && current_point_color == outer_contour_color)
                    {
                        //qDebug() << "outer point!";
                        outer_point = cv::Point(current_x, current_y);
                    }
                    else if(current_detail_point == cv::Point(0, 0) && current_point_color == current_contour_color)
                    {
                        if(outer_point != cv::Point(0, 0) && inner_point == cv::Point(0, 0))
                        {
                            //qDebug() << "current point!";
                            current_detail_point = cv::Point(current_x, current_y);
                        }
                        else if(analysis_rect.contains(QPoint(current_x, current_y)))
                        {
                            /*qDebug() << "bad point!";
                            qDebug() << "bad point color: (" << current_x << ", " << current_y << "): [" << current_point_color[0] << ", " << current_point_color[1] << ", " << current_point_color[2] << "]";*/
                            bad_point = cv::Point(current_x, current_y);
                        }
                    }

                    if(bad_point != cv::Point(0, 0) ||
                       (inner_point != cv::Point(0, 0) && outer_point != cv::Point(0, 0) && current_detail_point != cv::Point(0, 0))
                      )
                        break;

                    if(up_dir_flag)
                    {
                        if(current_y == next_y)
                        {
                            current_x += dx/qAbs(dx);
                            current_y = qRound(static_cast<qreal>((current_x - control_line.x1()) * (dy)) / (dx) + control_line.y1());
                            next_y = qRound(static_cast<qreal>(((current_x + dx/qAbs(dx)) - control_line.x1()) * (dy)) / (dx) + control_line.y1());
                        }
                        else
                            current_y += dy/qAbs(dy);
                    }
                    else
                    {
                        if(current_y == next_y)
                        {
                            current_x -= dx/qAbs(dx);
                            current_y = qRound(static_cast<qreal>((current_x - control_line.x1()) * (dy)) / (dx) + control_line.y1());
                            next_y = qRound(static_cast<qreal>(((current_x - dx/qAbs(dx)) - control_line.x1()) * (dy)) / (dx) + control_line.y1());
                        }
                        else
                            current_y -= dy/qAbs(dy);
                    }
                }

                if(current_detail_point == cv::Point(0, 0) && inner_point != cv::Point(0, 0) && outer_point != cv::Point(0, 0))
                    result = false;

                if(bad_point != cv::Point(0, 0) /*&& outer_point != cv::Point(0, 0)*/)
                {
                    /*qDebug() << "!!!! add bad_point bad_point: (" << bad_point.x << ", " << bad_point.y << ")";
                    qDebug() << "!!!! add bad_point outer_point: (" << outer_point.x << ", " << outer_point.y << ")";
                    qDebug() << "!!!! add bad_point inner_point: (" << inner_point.x << ", " << inner_point.y << ")";*/
                    bad_points.append(QPoint(bad_point.x, bad_point.y));
                }
            }
            // ---------------
        }
    }

    if(!bad_points.empty())
        result = false;

    return result;
}

void PuansonChecker::startCurrentCalibration()
{
    current_image_window->setCalibrationMode(CalibrationMode_e::REFERENCE_POINT_1);
}

void PuansonChecker::moveImages(const int &dx, const int &dy, const ImageWindow *owner_window, const bool scroll_event)
{
    if(scroll_event && ignore_scroll_move_image)
        return;

    if(main_window)
    {
        if((!scroll_event || dynamic_cast<const QWidget *>(owner_window) != main_window) && isEtalonImageLoaded())
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
        if((!scroll_event || dynamic_cast<const QWidget *>(owner_window) != contours_window) && (!getEtalon().isEmpty() && isCurrentImageLoaded()))
            contours_window->moveImage(dx, dy);
        else if(scroll_event && dynamic_cast<const QWidget *>(owner_window) == contours_window)
            contours_window->shiftImageCoords(dx, dy);
    }
}

void PuansonChecker::currentDetailResearchGotoNextAngle()
{
    main_window->currentDetailResearchGotoNextAngle();
}

void PuansonChecker::drawEtalonImage(bool draw_etalon_image)
{
    if(draw_etalon_image)
    {
        QImage img;

        getEtalonImage(img);
        main_window->drawImage(img);
    }

    QRect reference_point_1_area = PuansonChecker::getInstance()->getReferencePointSearchArea(ImageType_e::ETALON_IMAGE,  ReferencePointType_e::REFERENCE_POINT_1);
    QRect reference_point_2_area = PuansonChecker::getInstance()->getReferencePointSearchArea(ImageType_e::ETALON_IMAGE,  ReferencePointType_e::REFERENCE_POINT_2);

    if(!reference_point_1_area.isNull())
        main_window->drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_1, reference_point_1_area);

    if(!reference_point_2_area.isNull())
        main_window->drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_2, reference_point_2_area);

    QPoint reference_point_1, reference_point_2;
    PuansonChecker::getInstance()->getEtalon().getReferencePoints(reference_point_1, reference_point_2);

    if(reference_point_1 != reference_point_2)
    {
        main_window->drawReferencePoints(reference_point_1, reference_point_2);
        main_window->drawIdealContour(PuansonChecker::getInstance()->getEtalon().getIdealContourPointOfOrigin().toPoint(), PuansonChecker::getInstance()->getEtalon().getIdealContourRotationAngle());
        main_window->drawActualBorders();
    }
}

void PuansonChecker::drawCurrentImage(bool draw_reference_points)
{
    QImage img;

    current_image_window->setLabel2Text("Файл " + getCurrent().getFilename());

    getCurrentImage(img);
    current_image_window->drawImage(img);

    /*if(draw_reference_points)
        current_image_window->drawReferencePoints();*/

    if(draw_reference_points)
        drawCurrentImageReferencePoints();
}

void PuansonChecker::drawCurrentImageReferencePoints()
{
    if(!reference_point_1_search_area_rect.isNull())
        current_image_window->drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_1, getCurrent().getImageTransform().map(QRegion(reference_point_1_search_area_rect)).rects().first());

    if(!reference_point_2_search_area_rect.isNull())
        current_image_window->drawReferencePointAutoSearchArea(ReferencePointType_e::REFERENCE_POINT_2, getCurrent().getImageTransform().map(QRegion(reference_point_2_search_area_rect)).rects().first());

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

QString PuansonChecker::getCameraStatus() const
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

        if(PuansonChecker::loadImage(ImageType_e::CURRENT_IMAGE, path, current_image) == 0)
        {
            if(etalon_puanson_image.isIdealContourSet())
                current_image.copyIdealContour(etalon_puanson_image);

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

    loading_etalon_angle = etalon_research_active_angle;
    PuansonImage *etalon_puanson_image_ptr = &etalon_puanson_image;

    main_window->setWindowStatus("Фотосъёмка и загрузка изображения эталонной детали ...");

    load_image_future = QtConcurrent::run([&, path]() {
        //main_window->setWindowStatus("Фотосъёмка эталонной детали ...");
        camera->CaptureAndAcquireImage(CAPTURED_ETALON_IMAGE_FILENAME);

        //main_window->setWindowStatus("Загрузка изображения эталонной детали ...");
        etalon_puanson_image_ptr->release();

        if(PuansonChecker::loadImage(ImageType_e::ETALON_IMAGE, path, *etalon_puanson_image_ptr) == 0)
        {
            quint16 ext_tolerance_mkm_array[PUANSON_IMAGE_MAX_ANGLE];
            quint16 int_tolerance_mkm_array[PUANSON_IMAGE_MAX_ANGLE];
            quint32 reference_points_distance;

            general_settings->getToleranceFields(ext_tolerance_mkm_array, int_tolerance_mkm_array);
            general_settings->getReferencePointDistanceMkm(reference_points_distance);

            etalon_puanson_image_ptr->setToleranceFields(ext_tolerance_mkm_array[loading_etalon_angle - 1], int_tolerance_mkm_array[loading_etalon_angle - 1]);
            etalon_puanson_image_ptr->setReferencePointDistanceMkm(reference_points_distance);

            //etalon_puanson_image_ptr->setImageContour(getContour(*etalon_puanson_image_ptr));
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
                     const vector<Vec4i> &hierarchy, const bool draw_etalon_contour_flag/*, const quint32 ext_tolerance, quint32 int_tolerance*/)
                : p_puanson_image(&puanson_image), image_contour_ref(image_contour),
                  detail_contours(contours_vector), contours_hierarchy(hierarchy),
                  contours_per_thread(contours_per_thread), draw_etalon_contour_flag(draw_etalon_contour_flag),
                  image_type(puanson_image.getImageType())/*,
                  external_tolerance(ext_tolerance), internal_tolerance(int_tolerance)*/
    { }

    virtual void operator()(const Range& range) const
    {
        Scalar detail_contour_color;
        Scalar inner_contour_color = Scalar(250, 100, 100);
        Scalar outer_contour_color = Scalar(100, 250, 100);

        int thickness = CV_FILLED;

        switch(image_type)
        {
            case ImageType_e::ETALON_IMAGE:
                detail_contour_color = Scalar( 255, 255, 255 );
                thickness = 1;
                break;
            case ImageType_e::CURRENT_IMAGE:
            default:
                detail_contour_color = Scalar( 50, 50, 250 );
                thickness = 1;
                break;
        }

        vector<vector<Point> > outer_contours = detail_contours;
        vector<vector<Point> > inner_contours = detail_contours;

        // Внутренняя точка
        qint32 j; // Индекс

//        qDebug() << "in " << __PRETTY_FUNCTION__ << " PointOfOrigin: " << p_puanson_image->getIdealContourPointOfOrigin() << " RotationAngle: " << p_puanson_image->getIdealContourRotationAngle();
//        qDebug() << "image type: " << (p_puanson_image->getImageType() == ImageType_e::ETALON_IMAGE ? "ETALON" : "CURRENT");
        QLine last_skeleton_line;

        for(int i = range.start; i < range.end; i++)
        {
            for(int l = 0; l < contours_per_thread; l++)
            {
                j = i * contours_per_thread + l;
//qDebug() << "p_puanson_image->isIdealContourSet() " << p_puanson_image->isIdealContourSet();
                // Рассчитывать контура допуска только тогда, когда заданы точки остова
                if(p_puanson_image->isIdealContourSet()/*(image_type == ETALON_IMAGE && p_puanson_image->isIdealContourSet()) || image_type == CURRENT_IMAGE*/)
                {
                    QPoint internal_normal_vector;
                    QPoint external_normal_vector;

                    bool near_ideal_line = p_puanson_image->findNearestIdealLineNormalVector(QPoint(detail_contours[j].begin()->x, detail_contours[j].begin()->y), last_skeleton_line, internal_normal_vector, external_normal_vector);

                    if((/*p_puanson_image->isIdealContourSet() && */near_ideal_line)/* || image_type == CURRENT_IMAGE*/)
                    {
                        if(image_type == ImageType_e::ETALON_IMAGE)
                        {
                            for(auto it = inner_contours[j].begin(); it != inner_contours[j].end(); it++)
                                *it += Point(internal_normal_vector.x(), internal_normal_vector.y());

                            for(auto it = outer_contours[j].begin(); it != outer_contours[j].end(); it++)
                                *it += Point(external_normal_vector.x(), external_normal_vector.y());
                        }

                        // Рисование контуров
                        // Вывод контура детали
                        if((image_type == ImageType_e::ETALON_IMAGE &&  draw_etalon_contour_flag) || image_type == ImageType_e::CURRENT_IMAGE)
                        {
                            /*if(j % 10 == 0)
                            {
                                Point seedPoint(detail_contours[j].begin()->x + 10, detail_contours[j].begin()->y + 20);
                                char buf[32] = "";
                                sprintf(buf, "%d", j);
                                String text(buf);
                                putText(image_contour_ref, text, seedPoint, FONT_HERSHEY_PLAIN, 0.7, Scalar(255, 0, 255));
                            }*/

                            drawContours( image_contour_ref, detail_contours, j, detail_contour_color, thickness, LINE_4, contours_hierarchy, 0, Point(0, 0) );
                        }

                        // Выводить контура допуска только тогда, когда заданы точки остова
                        if(image_type == ImageType_e::ETALON_IMAGE/* && p_puanson_image->isIdealContourSet()*/)
                        {
                            //line(image_contour_ref, *detail_contours[j].begin(), *inner_contours[j].begin(), Scalar(255, 0, 0));
                            // Вывод контура внутреннего допуска
                            drawContours( image_contour_ref, inner_contours, j, inner_contour_color, 1, LINE_4, contours_hierarchy, 0, Point(0, 0) );
                            //line(image_contour_ref, *detail_contours[j].begin(), *outer_contours[j].begin(), Scalar(0, 255, 0));
                            // Вывод контура внешнего допуска

                            drawContours( image_contour_ref, outer_contours, j, outer_contour_color, 1, LINE_4, contours_hierarchy, 0, Point(0, 0) );
                        }

                        /////////////////////
                    }
                }
            }
        }
    }

private:
    PuansonImage *p_puanson_image;
    Mat &image_contour_ref;
    vector<vector<Point>> detail_contours;
    vector<Vec4i> contours_hierarchy;
    quint16 contours_per_thread;
    bool draw_etalon_contour_flag;
    ImageType_e image_type;
    /*quint32 external_tolerance;
    quint32 internal_tolerance;*/

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
    poly_points[0] = Point(0, qRound(gray_image.rows * 0.3));
    poly_points[1] = Point(qRound(gray_image.cols * 0.5), gray_image.rows);
    poly_points[2] = Point(0, gray_image.rows);

    fillConvexPoly(gray_roi, poly_points, 3, Scalar(gray_image.at<uchar>(poly_points[0])));

    /*line(gray_roi, poly_points[0], poly_points[1], Scalar(0));
    line(gray_roi, poly_points[1], poly_points[2], Scalar(0));
    line(gray_roi, poly_points[2], poly_points[0], Scalar(0));*/
    //////////////////////

    // Правый верхний угол
    poly_points[0] = Point(qRound(gray_image.cols * 0.7), 0);
    poly_points[1] = Point(gray_image.cols, qRound(gray_image.rows * 0.3));
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
                            floodFill(gray_roi, seedPoint, GLARE_COLOR_THRESHOLD - GLARE_COLOR_RANGE_W, Q_NULLPTR, Scalar(c - GLARE_COLOR_THRESHOLD + GLARE_COLOR_RANGE_W), Scalar(255 - c), FLOODFILL_FIXED_RANGE + 8);

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

    blur( gray_roi, gray_roi, Size(6,6) );

    if(!(CannyThreshold1 < CannyThreshold2))
    {
        return image_contour;
    }

    Canny( gray_roi, bin1, CannyThreshold1, CannyThreshold2, 3 );

    vector<vector<Point> > detail_contours;
    vector<Vec4i> contours_hierarchy;
    vector<vector<Point> > detail_contours0;

    findContours( bin1, detail_contours0, contours_hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0) );

    detail_contours.resize(detail_contours0.size());

    for( size_t k = 0; k < detail_contours0.size(); k++ )
    {
        approxPolyDP(Mat(detail_contours0[k]), detail_contours[k], 3, true);
    }

    //Scalar inner_line_color(229, 43, 80);

    if(true && puanson_image.getImageType() == ImageType_e::ETALON_IMAGE)
    {
        /* Обрамление области анализа корректности детали */
        line(image_contour, Point(qRound((image_contour.cols - image_contour.cols / 4.0) / 2.0),
                                  qRound((image_contour.rows - image_contour.rows / 4.0) / 2.0)),
                            Point(qRound((image_contour.cols + image_contour.cols / 4.0) / 2.0),
                                  qRound((image_contour.rows - image_contour.rows / 4.0) / 2.0)),
                            Scalar(250, 250, 250));

        line(image_contour, Point(qRound((image_contour.cols + image_contour.cols / 4.0) / 2.0),
                                  qRound((image_contour.rows - image_contour.rows / 4.0) / 2.0)),
                            Point(qRound((image_contour.cols + image_contour.cols / 4.0) / 2.0),
                                  qRound((image_contour.rows + image_contour.rows / 4.0) / 2.0)),
                            Scalar(250, 250, 250));

        line(image_contour, Point(qRound((image_contour.cols + image_contour.cols / 4.0) / 2.0),
                                  qRound((image_contour.rows + image_contour.rows / 4.0) / 2.0)),
                            Point(qRound((image_contour.cols - image_contour.cols / 4.0) / 2.0),
                                  qRound((image_contour.rows + image_contour.rows / 4.0) / 2.0)),
                            Scalar(250, 250, 250));

        line(image_contour, Point(qRound((image_contour.cols - image_contour.cols / 4.0) / 2.0),
                                  qRound((image_contour.rows + image_contour.rows / 4.0) / 2.0)),
                            Point(qRound((image_contour.cols - image_contour.cols / 4.0) / 2.0),
                                  qRound((image_contour.rows - image_contour.rows / 4.0) / 2.0)),
                            Scalar(250, 250, 250));
        /* ********************************************** */

        if(etalon_research_active_angle >= 1 && etalon_research_active_angle <= PUANSON_IMAGE_MAX_ANGLE)
        {
            QVector<IdealInnerSegment> inner_segments = getEtalon().getActualIdealInnerSegments();

            QRect analysis_rect(qRound((image_contour.cols - image_contour.cols / 4.0) / 2.0),
                    qRound((image_contour.rows - image_contour.rows / 4.0) / 2.0),
                    qRound(image_contour.cols / 4.0) ,
                    qRound(image_contour.rows / 4.0));

            for(IdealInnerSegment segment: inner_segments)
            {
                QVector<QLine> control_lines = segment.getControlLines(30, analysis_rect);

                for(QLine control_line : control_lines)
                {
                    line(image_contour, Point(control_line.x1(), control_line.y1()),
                         Point(control_line.x2(), control_line.y2()),
                         Scalar(180, 180, 180));
                }
            }
        }
    }

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
                drawContours( image_contour_ref, detail_contours, j, detail_contour_color, thickness, LINE_4, contours_hierarchy, 0, Point(0, 0) );

            if(image_type == ETALON_IMAGE)
            {
                // Вывод контура внутреннего допуска
                drawContours( image_contour_ref, inner_contours, j, inner_contour_color, 1, LINE_4, contours_hierarchy, 0, Point(0, 0) );

                // Вывод контура внешнего допуска
                drawContours( image_contour_ref, outer_contours, j, outer_contour_color, 1, LINE_4, contours_hierarchy, 0, Point(0, 0) );
            }
        }
        /////////////////////
    }
#else // PARALLEL_CONTOUR_COMPUTING

    if(puanson_image.isIdealContourSet())
    {
        const quint16 contours_per_thread = 10; // Количество контуров, обрабатываемых одним потоком
        quint16 ext_tolerance_px, int_tolerance_px;

        puanson_image.getToleranceFields(ext_tolerance_px, int_tolerance_px);

        //qDebug() << "this " << this << "before parallel_for_ " << QTime::currentTime();

        parallel_for_(Range(0, static_cast<qint32>(detail_contours.size() / contours_per_thread)),
                      ParallelDrawContours(puanson_image, image_contour, detail_contours, contours_per_thread, contours_hierarchy,
                                           draw_etalon_contour_flag/*, ext_tolerance_px, int_tolerance_px*/));

        //qDebug() << "this " << this << "after parallel_for_ " << QTime::currentTime();

        /*for(int j = 0; j < detail_contours.size(); j++)
            drawContours( image_contour, detail_contours, j, color, thickness, 8, contours_hierarchy, 0, Point() );*/
    }
#endif // PARALLEL_CONTOUR_COMPUTING
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

    if(!(isCurrentImageLoaded() && isEtalonImageLoaded()))
        return;

    Mat current_contour = getCurrent().imageContour();
    Mat current_contours_mask(current_contour.size(), CV_8UC1);

    contours_image.release();
    etalon_puanson_image.imageContour().copyTo(contours_image);

    cvtColor(current_contour, current_contours_mask, CV_RGB2GRAY);
    current_contour.copyTo(contours_image, current_contours_mask);
    current_contours_mask.release();

    QVector<QPoint> bad_points;

    if(images_combined_by_reference_points)
    {
        checkDetail(bad_points);
    }

    QImage img(QImage(reinterpret_cast<const unsigned char*>(contours_image.data),
                  contours_image.cols, contours_image.rows,
                  static_cast<qint32>(contours_image.step), QImage::Format_RGB888).rgbSwapped());

    // Рисование реперных точек
    QPoint etalon_reference_point_1, etalon_reference_point_2;
    QPoint current_reference_point_1, current_reference_point_2;

    getEtalon().getReferencePoints(etalon_reference_point_1, etalon_reference_point_2);
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
    //contours_window->drawIdealContour(getEtalon().getIdealContourPath(), getEtalon().getIdealContourMeasurementsPath());

    if(images_combined_by_reference_points)
    {
        contours_window->drawBadPoints(bad_points);
    }
}

void PuansonChecker::shiftCurrentImage(const float dx, const float dy)
{
    using namespace cv;

    Mat &current_contour = getCurrent().imageContour();

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

        current_reference_point1 += QPoint(qRound(dx), qRound(dy));
        current_reference_point2 += QPoint(qRound(dx), qRound(dy));

        getCurrent().setReferencePoints(current_reference_point1, current_reference_point2);
    }
}

void PuansonChecker::rotateCurrentImage(const double angle)
{
    using namespace cv;

    Mat current_image = current_puanson_image.getImage();
    //Mat &current_contour = current_puanson_image.imageContour();

    if(!current_image.empty())
    {
        Mat rot_mat( 2, 3, CV_32FC1 );
        Point center_of_rotation;
        double scale = 1.0;

        QPoint current_reference_point1;
        QPoint current_reference_point2;

        double angle_in_degrees = qRadiansToDegrees(angle);

        current_puanson_image.getReferencePoints(current_reference_point1, current_reference_point2);

        if(current_puanson_image.isReferencePointsAreSet())
            center_of_rotation = Point(current_reference_point1.x(), current_reference_point1.y());
        else
            center_of_rotation = Point( current_image.cols / 2 + 1, current_image.rows / 2 + 1 );

        rot_mat = getRotationMatrix2D( center_of_rotation, angle_in_degrees, scale );

        warpAffine( current_puanson_image.getImage(), current_puanson_image.getImage(), rot_mat, current_puanson_image.getImage().size() );

        if(current_puanson_image.isReferencePointsAreSet())
        {
            //Mat reference_point1_column_vector = (Mat_<int>(3,1) << current_reference_point1.x(), current_reference_point1.y());
            //reference_point1_column_vector = rot_mat.mul(reference_point1_column_vector);
            //current_reference_point1 = QPoint(reference_point1_column_vector.at<int>(0,0), reference_point1_column_vector.at<int>(1,0));

            Mat reference_point2_column_vector = (Mat_<double>(3,1) << current_reference_point2.x(), current_reference_point2.y(), 1.0);
            reference_point2_column_vector = rot_mat * reference_point2_column_vector; // Нужно именно так множить, согласно док-ции OpenCV
            current_reference_point2 = QPointF(reference_point2_column_vector.at<double>(0,0), reference_point2_column_vector.at<double>(1,0)).toPoint();
            current_puanson_image.setReferencePoints(current_reference_point1, current_reference_point2);
        }

//        if(this->getCalculateContourOnRotationFlag())
//        {
//            current_contour.release();
//            current_puanson_image.setImageContour(getContour(current_puanson_image));
//        }
//        else
//        {
//            warpAffine( current_contour, current_contour, rot_mat, current_contour.size() );
//        }
    }
}

bool PuansonChecker::combineImagesByReferencePoints()
{
    if((isEtalonImageLoaded() && getEtalon().isReferencePointsAreSet()) &&
       (isCurrentImageLoaded() && getCurrent().isReferencePointsAreSet()))
    {
        QPoint etalon_reference_point1, etalon_reference_point2;
        QPoint current_reference_point1, current_reference_point2;
        QTransform current_puanson_image_transform;

        getEtalon().getReferencePoints(etalon_reference_point1, etalon_reference_point2);
        getCurrent().getReferencePoints(current_reference_point1, current_reference_point2);

        // Сдвиг изображения текущей детали для совмещения изображений по первой реперной точке
        float shift_dx, shift_dy;

        shift_dx = (etalon_reference_point1-current_reference_point1).x();
        shift_dy = (etalon_reference_point1-current_reference_point1).y();

        shiftCurrentImage(shift_dx, shift_dy);
        current_puanson_image_transform.translate(static_cast<qreal>(shift_dx), static_cast<qreal>(shift_dy));
        ///////////////////////////////////////////////////////////////////////////////////////

        getCurrent().getReferencePoints(current_reference_point1, current_reference_point2);

        // Поворот изображения текущей детали для совмещения изображений по второй реперной точке
        qreal x_etal = etalon_reference_point2.x() - etalon_reference_point1.x();
        qreal x_cur = current_reference_point2.x() - current_reference_point1.x();

        qreal y_etal = etalon_reference_point2.y() - etalon_reference_point1.y();
        qreal y_cur = current_reference_point2.y() - current_reference_point1.y();

        qreal cos_value = (x_etal*x_cur + y_etal*y_cur) / (qSqrt(x_etal*x_etal + y_etal*y_etal) * qSqrt(x_cur*x_cur + y_cur*y_cur));
        qreal rotation_angle_in_radians;

        if(cos_value > 1.0)
            cos_value = 1.0;

        rotation_angle_in_radians = qAcos(cos_value);

        //qDebug() << "rotation_angle_in_radians " << rotation_angle_in_radians << " cos_value " << cos_value;

        // Вычисление косого произведения векторов (Rep2_etal - Rep1_etal) и (Rep2_cur - Rep1_cur) для определения направления вращения
        qreal pseudoscalar_product = x_etal*y_cur - y_etal*x_cur;
        if(pseudoscalar_product < 0)
            rotation_angle_in_radians = -rotation_angle_in_radians;

        rotateCurrentImage(rotation_angle_in_radians);
        current_puanson_image_transform.rotateRadians(rotation_angle_in_radians);
        ///////////////////////////////////////////////////////////////////////////////////////

        current_puanson_image.imageContour().release();
        current_puanson_image.setImageContour(getContour(current_puanson_image));

        /*QString combine_transformation_message;
        combine_transformation_message = "Изображения совмещены: сдвиг dx " + QString::number(shift_dx) + " dy " + QString::number(shift_dy) + "; угол поворота " + QString::number(rotation_angle_in_radians) +  " радиан.";

        QMessageBox msgBox;
        msgBox.setText(combine_transformation_message);
        msgBox.setModal(false);
        msgBox.show();*/

        getCurrent().setImageTransform(current_puanson_image_transform);

        images_combined_by_reference_points = true;

        return true;
    }

    return false;
}

bool PuansonChecker::combineImagesByReferencePointsAndDrawBadPoints()
{
    return contours_window->combineImagesByReferencePointsButtonPressed();
}

QPoint PuansonChecker::findNearestContourPoint(const ContourPoints_e contour_point_type, const QPoint &original_point, const QLine &measurement_line, const quint16 neighborhood_len)
{
    using namespace std;

    auto findNeighbourContourPoint = [] (const cv::Mat &contours_image, const cv::Vec3b contour_color, const QLine &neighborhood_line) -> QPoint {
        function<bool (const int, const QLine &)> conditionLambda = [](const int current_x, const QLine &control_line) -> bool { return (current_x <= (control_line.x1() > control_line.x2() ? control_line.x1() : control_line.x2())) && (current_x >= (control_line.x1() < control_line.x2() ? control_line.x1() : control_line.x2())); };

        QPoint original_point = neighborhood_line.p1();
        QPoint new_contour_point;

        const qreal normal_vector_len = 2.0;
        cv::Vec3b current_point_color;
        qint32 current_x = -1, current_y = -1, next_y = -1;
        qint32 dx, dy;

        dx = neighborhood_line.dx();
        dy = neighborhood_line.dy();

        new_contour_point = QPoint(0, 0);

        current_x = original_point.x();
        current_y = original_point.y();
        next_y = qRound(static_cast<qreal>(((current_x + dx/qAbs(dx)) - original_point.x()) * (dy)) / (dx) + original_point.y());

        while(conditionLambda(current_x, neighborhood_line))
        {
            current_point_color = contours_image.at<cv::Vec3b>(current_y, current_x);

            if(current_point_color == PuansonChecker::empty_contour_color || current_point_color == PuansonChecker::empty_contour_color_2) // Если определена пустота, смотрим окрестности
            {
                QPointF normal_vector;

                normal_vector.setX(qRound(normal_vector_len / qSqrt(1 + static_cast<qreal>(dx) / dy * static_cast<qreal>(dx) / dy)));
                normal_vector.setY(qRound(-(static_cast<qreal>(dx) / dy) * normal_vector.x()));

                QLine check_line(QPoint(current_x, current_y) - normal_vector.toPoint(), QPoint(current_x, current_y) + normal_vector.toPoint());
                qreal check_line_length;

                QPointF current_point = check_line.p1(), previous_interation_current_point = check_line.p1();

                do
                {
                    current_point_color = contours_image.at<cv::Vec3b>(current_point.toPoint().y(), current_point.toPoint().x());
                    check_line_length = qSqrt(check_line.dx() * check_line.dx() + check_line.dy() * check_line.dy());

                    do
                    {
                        current_point += QPointF(static_cast<qreal>(check_line.dx()) / check_line_length, static_cast<qreal>(check_line.dy()) / check_line_length);
                    }
                    while(current_point.toPoint() == previous_interation_current_point);

                    previous_interation_current_point = current_point.toPoint();
                }
                while(current_point.toPoint() != check_line.p2() && !(current_point_color == contour_color));
            }

            if(new_contour_point.isNull() && current_point_color == contour_color)
                new_contour_point = QPoint(current_x, current_y);

            if(!new_contour_point.isNull())
                break;

            if(current_y == next_y)
            {
                current_x += dx/qAbs(dx);
                current_y = qRound(static_cast<qreal>((current_x - original_point.x()) * (dy)) / (dx) + original_point.y());
                next_y = qRound(static_cast<qreal>(((current_x + dx/qAbs(dx)) - original_point.x()) * (dy)) / (dx) + original_point.y());
            }
            else
                current_y += dy/qAbs(dy);
        }

        return new_contour_point;
    };

    QPoint new_contour_point;
    cv::Vec3b contour_color;

    switch(contour_point_type)
    {
    case ContourPoints_e::OUTER_TOLERANCE_ETALON_CONTOUR_POINT:
        contour_color = outer_contour_color;
        break;
    case ContourPoints_e::INNER_TOLERANCE_ETALON_CONTOUR_POINT:
        contour_color = inner_contour_color;
        break;
    case ContourPoints_e::ETALON_DETAIL_CONTOUR_POINT:
        contour_color = etalon_contour_color;
        break;
    case ContourPoints_e::CURRENT_DEATIL_CONTOUR_POINT:
        contour_color = current_contour_color;
        break;
    }

    qint32 line_len = qRound(QLineF(measurement_line).length());
    QPointF measurement_line_ort = QPointF(measurement_line.dx(), measurement_line.dy()) / line_len;

    // Поиск точек контура вверх по линии измерений
    new_contour_point = findNeighbourContourPoint(contours_image, contour_color, QLine(original_point, original_point + (measurement_line_ort * neighborhood_len).toPoint()));

    if(!new_contour_point.isNull())
        return new_contour_point;

    // Поиск точек контура вниз по линии измерений
    new_contour_point = findNeighbourContourPoint(contours_image, contour_color, QLine(original_point, original_point - (measurement_line_ort * neighborhood_len).toPoint()));

    return new_contour_point;
}

bool PuansonChecker::findMeasurementContourPoints(const QRect &analysis_rect)
{
    using namespace std;

    QVector<QLine> measurement_lines = getEtalon().getMeasurementLines(analysis_rect);

    if(measurement_lines.isEmpty())
        return false;

    bool up_dir_flag;
    int current_x = -1, current_y = -1, next_y = -1;
    QLine last_line;
    QPoint inner_point, outer_point, current_detail_point;

    function<bool (const int, const QLine &)> conditionLambda = [](const int current_x, const QLine &control_line) -> bool { return (current_x <= (control_line.x1() > control_line.x2() ? control_line.x1() : control_line.x2())) && (current_x >= (control_line.x1() < control_line.x2() ? control_line.x1() : control_line.x2())); };
    up_dir_flag = true;

    for(const QLine &measurement_line : measurement_lines)
    {
//        if(0)
        {
            inner_point = outer_point = current_detail_point = QPoint(0, 0);

            int dx = measurement_line.x2() - measurement_line.x1();
            int dy = measurement_line.y2() - measurement_line.y1();

            current_x = up_dir_flag ? measurement_line.x1() : measurement_line.x2();
            current_y = qRound(static_cast<qreal>((current_x - measurement_line.x1()) * (dy)) / (dx) + measurement_line.y1());
            next_y = qRound(static_cast<qreal>(((up_dir_flag ? (current_x + dx/qAbs(dx)) : (current_x - dx/qAbs(dx))) - measurement_line.x1()) * (dy)) / (dx) + measurement_line.y1());

            bool previous_point_inside_analysis_rect = false;

            while(conditionLambda(current_x, measurement_line))
            {
                if(previous_point_inside_analysis_rect == true && !analysis_rect.contains(QPoint(current_x, current_y)) && current_detail_point.isNull())
                {
                    if(inner_point.isNull() && outer_point.isNull())
                    {
                        inner_point = outer_point = current_detail_point = QPoint(0, 0);
                        last_line = measurement_line;

                        if(up_dir_flag)
                        {
                            up_dir_flag = false;

                            current_x = measurement_line.x2();
                            current_y = measurement_line.y2();
                            next_y = qRound(static_cast<qreal>(((current_x - dx/qAbs(dx)) - measurement_line.x1()) * (dy)) / (dx) + measurement_line.y1());
                        }
                        else
                        {
                            up_dir_flag = true;

                            current_x = measurement_line.x1();
                            current_y = measurement_line.y1();
                            next_y = qRound(static_cast<qreal>(((current_x + dx/qAbs(dx)) - measurement_line.x1()) * (dy)) / (dx) + measurement_line.y1());
                        }

                        if(analysis_rect.contains(QPoint(current_x, current_y)))
                        {
                            previous_point_inside_analysis_rect = false;
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                if(previous_point_inside_analysis_rect == false && analysis_rect.contains(QPoint(current_x, current_y)))
                    previous_point_inside_analysis_rect = true;
                else if(previous_point_inside_analysis_rect == true && !analysis_rect.contains(QPoint(current_x, current_y)))
                    previous_point_inside_analysis_rect = false;

                cv::Vec3b current_point_color = contours_image.at<cv::Vec3b>(current_y, current_x);

                if(current_point_color == empty_contour_color || current_point_color == empty_contour_color_2) // Если определена пустота, смотрим окрестности
                {
                    const qreal normal_vector_len = 10.0;
                    QPointF normal_vector;

                    normal_vector.setX(qRound(normal_vector_len / qSqrt(1 + static_cast<qreal>(dx) / dy * static_cast<qreal>(dx) / dy)));
                    normal_vector.setY(qRound(-(static_cast<qreal>(dx) / dy) * normal_vector.x()));

                    QLine check_line;
                    qreal check_line_length;

                    check_line = QLine(QPoint(current_x, current_y) - normal_vector.toPoint(), QPoint(current_x, current_y) + normal_vector.toPoint());

                    QPointF current_point = check_line.p1(), previous_interation_current_point = check_line.p1();
                    do
                    {
                        current_point_color = contours_image.at<cv::Vec3b>(current_point.toPoint().y(), current_point.toPoint().x());

                        check_line_length = qSqrt(check_line.dx() * check_line.dx() + check_line.dy() * check_line.dy());
                        do
                        {
                            current_point += QPointF(static_cast<qreal>(check_line.dx()) / check_line_length, static_cast<qreal>(check_line.dy()) / check_line_length);
                        }
                        while(current_point.toPoint() == previous_interation_current_point);

                        previous_interation_current_point = current_point.toPoint();
                    }
                    while(current_point.toPoint() != check_line.p2() && !(current_point_color == inner_contour_color || current_point_color == outer_contour_color || current_point_color == current_contour_color));
                }

                if(inner_point.isNull() && current_point_color == inner_contour_color)
                {
                    // Проверка на то, что направление поиска должно идти так, чтобы первой была найдена точка внешнего допуска
                    if(last_line != measurement_line && outer_point.isNull())
                    {
                        inner_point = outer_point = current_detail_point = QPoint(0, 0);
                        last_line = measurement_line;

                        if(up_dir_flag)
                        {
                            up_dir_flag = false;

                            current_x = measurement_line.x2();
                            current_y = measurement_line.y2();
                            next_y = qRound(static_cast<qreal>(((current_x - dx/qAbs(dx)) - measurement_line.x1()) * (dy)) / (dx) + measurement_line.y1());
                        }
                        else
                        {
                            up_dir_flag = true;

                            current_x = measurement_line.x1();
                            current_y = measurement_line.y1();
                            next_y = qRound(static_cast<qreal>(((current_x + dx/qAbs(dx)) - measurement_line.x1()) * (dy)) / (dx) + measurement_line.y1());
                        }

                        if(analysis_rect.contains(QPoint(current_x, current_y)))
                        {
                            previous_point_inside_analysis_rect = false;
                            continue;
                        }

                        previous_point_inside_analysis_rect = false;
                        continue;
                    }
                    else
                    {
                        inner_point = QPoint(current_x, current_y);
                    }
                }
                else if(outer_point.isNull() && current_point_color == outer_contour_color)
                {
                      outer_point = QPoint(current_x, current_y);
                }
                else if(current_detail_point.isNull() && current_point_color == current_contour_color)
                {
                    if(!outer_point.isNull() && inner_point.isNull())
                    {
                        current_detail_point = QPoint(current_x, current_y);
                    }
                }

                if(!inner_point.isNull() && !outer_point.isNull() && !current_detail_point.isNull())
                    break;

                if(up_dir_flag)
                {
                    if(current_y == next_y)
                    {
                        current_x += dx/qAbs(dx);
                        current_y = qRound(static_cast<qreal>((current_x - measurement_line.x1()) * (dy)) / (dx) + measurement_line.y1());
                        next_y = qRound(static_cast<qreal>(((current_x + dx/qAbs(dx)) - measurement_line.x1()) * (dy)) / (dx) + measurement_line.y1());
                    }
                    else
                        current_y += dy/qAbs(dy);
                }
                else
                {
                    if(current_y == next_y)
                    {
                        current_x -= dx/qAbs(dx);
                        current_y = qRound(static_cast<qreal>((current_x - measurement_line.x1()) * (dy)) / (dx) + measurement_line.y1());
                        next_y = qRound(static_cast<qreal>(((current_x - dx/qAbs(dx)) - measurement_line.x1()) * (dy)) / (dx) + measurement_line.y1());
                    }
                    else
                        current_y -= dy/qAbs(dy);
                }
            }
        }

        // Нарисовать линию и точки контуров эталона и текущей детали
        contours_window->drawMeasurementLineAndPoints(measurement_line, outer_point, current_detail_point);
    }

    return true;
}

void PuansonChecker::measurementPointsSettingMode()
{
    contours_window->measurementPointsSettingMode();
}

bool PuansonChecker::saveCurrentImage(const QString &path)
{
    using namespace cv;
    using namespace std;

    bool res = true;

    if(getCurrent().isEmpty())
        res = false;

    if(res == true)
    {
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
    }

    return res;
}

bool PuansonChecker::saveResultImage(const QString &path)
{
    using namespace cv;
    using namespace std;

    bool res = true;

    if(contours_image.empty())
        res = false;

    if(res == true)
    {
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
    }

    return res;
}

void PuansonChecker::setReferencePointSearchArea(const ReferencePointType_e reference_point_type, const QRect &area_rect)
{
    switch(reference_point_type)
    {
    case ReferencePointType_e::REFERENCE_POINT_1:
    default:
        reference_point_1_search_area_rect = area_rect;
        break;
    case ReferencePointType_e::REFERENCE_POINT_2:
        reference_point_2_search_area_rect = area_rect;
        break;
    }
}

QRect PuansonChecker::getReferencePointSearchArea(const ImageType_e image_type, const ReferencePointType_e reference_point_type) const
{
    QTransform t;

    if(image_type == ImageType_e::CURRENT_IMAGE)
        t = getCurrent().getImageTransform();

    switch(reference_point_type)
    {
    case ReferencePointType_e::REFERENCE_POINT_1:
    default:
        return t.map(QRegion(reference_point_1_search_area_rect)).rects().first();
        break;
    case ReferencePointType_e::REFERENCE_POINT_2:
        return t.map(QRegion(reference_point_2_search_area_rect)).rects().first();
        break;
    }
}

void PuansonChecker::researchEtalonAngle(quint8 angle)
{
    if(angle == 0)
    {
        EtalonResearchCreationDialog etalon_research_creation_dialog;

        etalon_research_creation_dialog.setModal(false);
        etalon_research_creation_dialog.setFixedSize(etalon_research_creation_dialog.size());

        etalon_research_creation_dialog.exec();

        switch(etalon_research_creation_dialog.result()){
        case QDialog::Accepted:
            break;
        case QDialog::Rejected:
        default:
            return;
            break;
        }

        if(!etalon_research_folder_path.isEmpty())
        {
            QDir(etalon_research_folder_path).removeRecursively();
            QDir().mkdir(etalon_research_folder_path);
        }

        const QString xml_config_filename = "etalon_research_configuration.xml";
        QFile config_file(etalon_research_folder_path + "/" + xml_config_filename);

        if(!config_file.open(QIODevice::WriteOnly))
        {
            config_file.close();
            return;
        }

        QXmlStreamWriter stream(&config_file);
        stream.setAutoFormatting(true);

        stream.writeStartDocument();
        stream.writeStartElement("puanson_research_settings");
            stream.writeStartElement("general_settings");
                stream.writeTextElement("puanson_model", QString::number(static_cast<int>(etalon_puanson_image.getDetailPuansonModel())));
                stream.writeTextElement("date_time_of_creation", etalon_research_date_time_of_creation.toString(Qt::ISODate));
                stream.writeTextElement("number_of_angles", QString::number(etalon_research_number_of_angles));
            stream.writeEndElement(); // general_settings

            stream.writeStartElement("detail_measurements");
                stream.writeTextElement("diameter_1", QString::number(loaded_research.getDetailDimensions().diameter_1_dimension));
                stream.writeTextElement("diameter_2", QString::number(loaded_research.getDetailDimensions().diameter_2_dimension));
                stream.writeTextElement("diameter_3", QString::number(loaded_research.getDetailDimensions().diameter_3_dimension));
                stream.writeTextElement("diameter_4", QString::number(loaded_research.getDetailDimensions().diameter_4_dimension));
                stream.writeTextElement("diameter_5", QString::number(loaded_research.getDetailDimensions().diameter_5_dimension));
                stream.writeTextElement("diameter_6", QString::number(loaded_research.getDetailDimensions().diameter_6_dimension));
                stream.writeTextElement("diameter_7", QString::number(loaded_research.getDetailDimensions().diameter_7_dimension));
                stream.writeTextElement("diameter_8", QString::number(loaded_research.getDetailDimensions().diameter_8_dimension));
                stream.writeTextElement("diameter_9", QString::number(loaded_research.getDetailDimensions().diameter_9_dimension));
                stream.writeTextElement("groove_width", QString::number(loaded_research.getDetailDimensions().groove_width_dimension));
                stream.writeTextElement("groove_depth", QString::number(loaded_research.getDetailDimensions().groove_depth_dimension));
            stream.writeEndElement(); // detail_measurements

        stream.writeEndElement(); // puanson_research_settings
        stream.writeEndDocument();

        config_file.close();

        etalon_research_active_angle = 1;
    }

    // 1. Диалоговое окно для выбора источника изображения ракурса эталона
    DetailAngleSourceDialog etalon_angle_source_dialog(ImageType_e::ETALON_IMAGE, etalon_research_active_angle);

    etalon_angle_source_dialog.setModal(false);
    etalon_angle_source_dialog.setFixedSize(etalon_angle_source_dialog.size());

    etalon_angle_source_dialog.exec();

    switch(etalon_angle_source_dialog.result())
    {
    case PHOTO_SHOOTING_DIALOG_RESULT:
        if(PuansonChecker::getInstance()->useMachineForDetailMovement())
        {
            if(PuansonChecker::getInstance()->getMachine()->moveToAnglePosition(etalon_research_active_angle))
            {
                main_window->menuImageWorkEtalonShootAndLoadActionTriggered();
            }
            else
            {
                qint32 reply;

                QMessageBox question(main_window);
                question.setWindowTitle("Внимание");
                question.setText("Невозможно поместить деталь в нужную позицию с помощью станка. Отменить исследование или продолжить в ручном режиме?");
                question.addButton("Отменить", QMessageBox::RejectRole);
                question.addButton("Продолжить в ручном режиме", QMessageBox::ApplyRole);
                reply = question.exec();

                if (reply == QMessageBox::ApplyRole)
                {
                    QMessageBox::information(main_window, "Информационное сообщение", "Поместите деталь в нужную позицию и нажмите \"OK\"");
                    main_window->menuImageWorkEtalonShootAndLoadActionTriggered();
                }
                else
                {
                    PuansonChecker::getInstance()->cancelEtalonResearch();
                    QMessageBox::information(main_window, "Исследование завершено", "Результат исследования: \"Отменено пользователем\"");
                }
            }
        }
        break;
    case LOADING_FROM_FILE_DIALOG_RESULT:
        main_window->menuImageWorkEtalonLoadActionTriggered();
        break;
    case CLOSE_DIALOG_RESULT:
    default:
        PuansonChecker::getInstance()->cancelEtalonResearch();
        QMessageBox::information(main_window, "Исследование завершено", "Результат исследования: \"Отменено пользователем\"");
        break;
    }
}

void PuansonChecker::researchCurrentAngle(quint8 angle)
{
    if(angle == 0)
    {
        CurrentResearchCreationDialog current_research_creation_dialog;

        current_research_creation_dialog.setModal(false);
        current_research_creation_dialog.setFixedSize(current_research_creation_dialog.size());

        current_research_creation_dialog.exec();

        switch(current_research_creation_dialog.result())
        {
        case QDialog::Accepted:
            break;
        case QDialog::Rejected:
        default:
            return;
            break;
        }

        if(!current_research_folder_path.isEmpty())
        {
            QDir(current_research_folder_path).removeRecursively();
            QDir().mkdir(current_research_folder_path);

            const QString xml_config_filename = "current_research_configuration.xml";
            QFile config_file(current_research_folder_path + "/" + xml_config_filename);

            if(!config_file.open(QIODevice::WriteOnly))
            {
                config_file.close();
                return;
            }

            QXmlStreamWriter stream(&config_file);
            stream.setAutoFormatting(true);

            stream.writeStartDocument();
            stream.writeStartElement("current_angle_settings");
                stream.writeStartElement("general_settings");
                    stream.writeTextElement("puanson_model", QString::number(static_cast<int>(etalon_puanson_image.getDetailPuansonModel())));
                    stream.writeTextElement("date_time_of_creation", current_research_date_time_of_creation.toString(Qt::ISODate));
                    stream.writeTextElement("number_of_angles", QString::number(etalon_research_number_of_angles));
                stream.writeEndElement(); // general_settings
            stream.writeEndElement(); // current_angle_settings
            stream.writeEndDocument();

            config_file.close();
        }

        current_puanson_image.setCurrentDetailSuitable(true);
        current_research_active_angle = 1;
    }
    else
        current_research_active_angle = angle;

    // 1. Загрузка необходимого ракурса эталона
    if(current_research_active_angle != etalon_research_active_angle)
    {
        switch(current_research_active_angle)
        {
        case 1:
            main_window->menuEtalonAngleSetActive1ActionTriggered();
            break;
        case 2:
            main_window->menuEtalonAngleSetActive2ActionTriggered();
            break;
        case 3:
            main_window->menuEtalonAngleSetActive3ActionTriggered();
            break;
        case 4:
            main_window->menuEtalonAngleSetActive4ActionTriggered();
            break;
        case 5:
            main_window->menuEtalonAngleSetActive5ActionTriggered();
            break;
        case 6:
            main_window->menuEtalonAngleSetActive6ActionTriggered();
            break;
        }
    }
    else
    {
        main_window->loadImageFinished(ImageType_e::ETALON_IMAGE);
    }
}

bool PuansonChecker::loadEtalonResearch(const QString &etalon_research_folder_path)
{
    QDir research_dir(etalon_research_folder_path);
    const QString xml_config_filename = "etalon_research_configuration.xml";
    bool bad_research = false;

    PuansonModel puanson_model = PuansonModel::PUANSON_MODEL_660;
    QDateTime date_time_of_creation;
    quint8 number_of_angles = 0;

    EtalonDetailDimensions detail_dimensions;

    if(research_dir.exists(xml_config_filename))
    {
        QFile config_file(etalon_research_folder_path + "/" + xml_config_filename);

        if(!config_file.open(QIODevice::ReadOnly))
        {
            qDebug() << "Load research error: Configuration file opening error";
            return false;
        }

        QXmlStreamReader xml(&config_file);
        quint8 loaded_parameters = 0;

        while (!xml.atEnd() && !xml.hasError())
        {
            QXmlStreamReader::TokenType token = xml.readNext();
            if (token == QXmlStreamReader::StartDocument)
                continue;

            if (token == QXmlStreamReader::StartElement)
            {
                if (xml.name() == "puanson_research_settings")
                    continue;

                if (xml.name() == "general_settings")
                {
                    xml.readNext();

                    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "general_settings"))
                    {
                        if (xml.tokenType() == QXmlStreamReader::StartElement)
                        {
                            if (xml.name() == "puanson_model")
                            {
                                xml.readNext();
                                puanson_model = static_cast<PuansonModel>(xml.text().toString().toInt());
                                loaded_parameters++;
                            }
                            else if (xml.name() == "date_time_of_creation")
                            {
                                xml.readNext();
                                date_time_of_creation = QDateTime::fromString(xml.text().toString(), Qt::ISODate);
                                loaded_parameters++;
                            }
                            else if (xml.name() == "number_of_angles")
                            {
                                xml.readNext();
                                number_of_angles = static_cast<quint8>(xml.text().toUInt());
                                loaded_parameters++;
                            }
                        }
                        xml.readNext();
                    }
                }
                else if (xml.name() == "detail_measurements")
                {
                    xml.readNext();

                    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "detail_measurements"))
                    {
                        if (xml.tokenType() == QXmlStreamReader::StartElement)
                        {
                            if (xml.name() == "diameter_1")
                            {
                                xml.readNext();
                                detail_dimensions.diameter_1_dimension = xml.text().toString().toInt();
                                loaded_parameters++;
                            }
                            else if (xml.name() == "diameter_2")
                            {
                                xml.readNext();
                                detail_dimensions.diameter_2_dimension = xml.text().toString().toInt();
                                loaded_parameters++;
                            }
                            else if (xml.name() == "diameter_3")
                            {
                                xml.readNext();
                                detail_dimensions.diameter_3_dimension = xml.text().toString().toInt();
                                loaded_parameters++;
                            }
                            else if (xml.name() == "diameter_4")
                            {
                                xml.readNext();
                                detail_dimensions.diameter_4_dimension = xml.text().toString().toInt();
                                loaded_parameters++;
                            }
                            else if (xml.name() == "diameter_5")
                            {
                                xml.readNext();
                                detail_dimensions.diameter_5_dimension = xml.text().toString().toInt();
                                loaded_parameters++;
                            }
                            else if (xml.name() == "diameter_6")
                            {
                                xml.readNext();
                                detail_dimensions.diameter_6_dimension = xml.text().toString().toInt();
                                loaded_parameters++;
                            }
                            else if (xml.name() == "diameter_7")
                            {
                                xml.readNext();
                                detail_dimensions.diameter_7_dimension = xml.text().toString().toInt();
                                loaded_parameters++;
                            }
                            else if (xml.name() == "diameter_8")
                            {
                                xml.readNext();
                                detail_dimensions.diameter_8_dimension = xml.text().toString().toInt();
                                loaded_parameters++;
                            }
                            else if (xml.name() == "diameter_9")
                            {
                                xml.readNext();
                                detail_dimensions.diameter_9_dimension = xml.text().toString().toInt();
                                loaded_parameters++;
                            }
                            else if (xml.name() == "groove_width")
                            {
                                xml.readNext();
                                detail_dimensions.groove_width_dimension = xml.text().toString().toInt();
                                loaded_parameters++;
                            }
                            else if (xml.name() == "groove_depth")
                            {
                                xml.readNext();
                                detail_dimensions.groove_depth_dimension = xml.text().toString().toInt();
                                loaded_parameters++;
                            }
                        }
                        xml.readNext();
                    }
                }
            }
        }

        config_file.close();

        if(loaded_parameters == 3 + 11)
        {
            for(int angle = 1; angle <= number_of_angles; angle++)
            {
                if(!research_dir.exists(QString("angle_") + QString::number(angle)) &&
                        QFile::exists(research_dir.absolutePath() + "/" + QString("angle_") + QString::number(angle) + "/" + "original_image.nef") &&
                        QFile::exists(research_dir.absolutePath() + "/" + QString("angle_") + QString::number(angle) + "/" + "contours_image.jpg") &&
                        QFile::exists(research_dir.absolutePath() + "/" + QString("angle_") + QString::number(angle) + "/" + "etalon_angle_configuration.xml"))
                {
                    bad_research = true;
                    break;
                }
            }
        }
        else
            bad_research = true;
    }
    else
        bad_research = true;

    if(!bad_research)
    {
        setEtalonResearchSettings(etalon_research_folder_path, puanson_model, date_time_of_creation, number_of_angles);
        loaded_research.setDetailDimensions(detail_dimensions);
        completeEtalonResearch();
    }

    return !bad_research;
}
