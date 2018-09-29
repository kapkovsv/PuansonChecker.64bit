#include "currentform.h"
#include "ui_currentform.h"

#include <QGraphicsPixmapItem>
#include <QMessageBox>

CurrentFormGraphicsScene::CurrentFormGraphicsScene(CurrentForm *owner_window):
    previousX(0), previousY(0), window(owner_window), scene_context_menu(window), left_button_down(false)
{
    QAction *viewing_mode_action;
    QAction *editing_mode_action;

    viewing_mode_action = scene_context_menu.addAction("Режим просмотра");
    viewing_mode_action->setData(qVariantFromValue(IMAGE_MOVE_VIEWING));
    viewing_mode_action->setCheckable(true);
    viewing_mode_action->setChecked(true);

    editing_mode_action = scene_context_menu.addAction("Режим редактирования");
    editing_mode_action->setData(qVariantFromValue(IMAGE_MOVE_EDITING));
    editing_mode_action->setCheckable(true);

    connect(&scene_context_menu, SIGNAL(triggered(QAction*)), SLOT(sceneContextMenuTriggeredSlot(QAction*)));
}

void CurrentFormGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent)
{
    scene_context_menu.exec(contextMenuEvent->screenPos());
}

void CurrentFormGraphicsScene::sceneContextMenuTriggeredSlot(QAction *action)
{
    QMenu *parent_menu = dynamic_cast<QMenu *>(action->parent());

    if(parent_menu)
    {
        foreach(QAction *menu_action, parent_menu->actions())
            menu_action->setChecked(false);
    }

    ImageMoveMode_e mode = action->data().value<ImageMoveMode_e>();

    switch(mode)
    {
        case IMAGE_MOVE_EDITING:
            window->setImageCursor(Qt::OpenHandCursor);
        break;
        case IMAGE_MOVE_VIEWING:
        default:
            window->setImageCursor(Qt::ArrowCursor);
        break;
    }

    window->setImageMoveMode(mode);
    action->setChecked(true);
}

void CurrentFormGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    static int __x = 0;
    static int __y = 0;

    if(window && left_button_down)
    {
        switch(window->getImageMoveMode())
        {
            case IMAGE_MOVE_EDITING:
                {
                    int x = mouseEvent->screenPos().x();
                    int y = mouseEvent->screenPos().y();
                    int dx = x - previousX;
                    int dy = y - previousY;

                    window->setLabel2Text("Сдвиг: dx " + QString::number(dx) + " dy " + QString::number(dy));
                }
            break;
            case IMAGE_MOVE_VIEWING:
            default:
                if(__y != mouseEvent->screenPos().y() || __x != mouseEvent->screenPos().x())
                {
                    if(__x != mouseEvent->screenPos().x())
                        __x = mouseEvent->screenPos().x();
                    if(__y != mouseEvent->screenPos().y())
                        __y = mouseEvent->screenPos().y();

                    PuansonChecker::getInstance()->moveImages(__x - previousX, __y - previousY);
                    previousX = __x;
                    previousY = __y;
                }
            break;
        }
    }
}

void CurrentFormGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if(mouseEvent->button() == Qt::LeftButton)
    {
        if(window->getCalibrationMode() != CalibrationMode_e::NO_CALIBRATION)
        {
            window->mousePressEvent(mouseEvent->scenePos().toPoint());
        }
        else
        {
            left_button_down = true;

            previousX = mouseEvent->screenPos().x();
            previousY = mouseEvent->screenPos().y();

            switch(window->getImageMoveMode())
            {
                case IMAGE_MOVE_EDITING:
                    // добавить удаление реперных точек с изображения
                    window->setImageCursor(Qt::ClosedHandCursor);
                break;
                case IMAGE_MOVE_VIEWING:
                default:
                    PuansonChecker::getInstance()->setIgnoreScrollMoveImage(true);
                    window->setImageCursor(Qt::SizeAllCursor);
                break;
            }
        }
    }
}

void CurrentFormGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if(mouseEvent->button() == Qt::LeftButton)
    {
        if(window->getCalibrationMode() == CalibrationMode_e::NO_CALIBRATION)
        {
            switch(window->getImageMoveMode())
            {
                case IMAGE_MOVE_EDITING:
                {
                    qint32 x = mouseEvent->screenPos().x();
                    qint32 y = mouseEvent->screenPos().y();
                    qint32 dx = x - previousX;
                    qint32 dy = y - previousY;

                    PuansonChecker::getInstance()->shiftCurrentImage(dx, dy);
                    PuansonChecker::getInstance()->drawCurrentImage();
                    PuansonChecker::getInstance()->drawContoursImage();

                    window->setLabel2Text("");
                    window->setImageCursor(Qt::OpenHandCursor);
                }
                break;
                case IMAGE_MOVE_VIEWING:
                default:
                    PuansonChecker::getInstance()->setIgnoreScrollMoveImage(false);
                    window->setImageCursor(Qt::ArrowCursor);
                break;
            }

            left_button_down = false;
            previousX = 0;
            previousY = 0;
        }
        else
            window->mouseReleaseEvent(mouseEvent->pos().toPoint());
    }
}

void CurrentFormGraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    if(window->getCalibrationMode() == CalibrationMode_e::NO_CALIBRATION && window->getImageMoveMode() == IMAGE_MOVE_EDITING)
    {
        qint32 delta = event->delta();

        if(event->buttons() & Qt::RightButton)
            delta *= 10;

        qreal rotate_angle = delta / 120.0 * 0.01;

        PuansonChecker::getInstance()->rotateCurrentImage(rotate_angle);
        PuansonChecker::getInstance()->drawCurrentImage();
        PuansonChecker::getInstance()->drawContoursImage();
    }
}

void CurrentForm::setImageCursor(const QCursor &cursor)
{
    ui->graphicsView->viewport()->setCursor(cursor);
}

void CurrentForm::moveImage(const qreal dx, const qreal dy)
{
   // QGraphicsScene *scene = ui->graphicsView->scene();

    //QGraphicsPixmapItem *pixmap_item = scene == Q_NULLPTR ? Q_NULLPTR : qgraphicsitem_cast<QGraphicsPixmapItem *>(scene->items().at(scene->items().count()-1));
    if(pixmap_item)
    {
        qreal new_x, new_y;

        new_x = image_x - dx;
        new_y = image_y - dy;

        if(new_x < 0)
            new_x = 0;
        else if(new_x > pixmap_item->pixmap().width() - ui->graphicsView->width() + 7)
            new_x = pixmap_item->pixmap().width() - ui->graphicsView->width() + 7;

        if(new_y < 0)
            new_y = 0;
        else if(new_y > pixmap_item->pixmap().height() - ui->graphicsView->height() + 7)
            new_y = pixmap_item->pixmap().height() - ui->graphicsView->height() + 7;

        ui->graphicsView->centerOn(new_x + ui->graphicsView->width() / 2 - 3 , new_y + ui->graphicsView->height() / 2 - 3);

        image_x = new_x;
        image_y = new_y;
    }
}

void CurrentForm::drawImage(const QImage &img)
{
    if(!scene->items().isEmpty())
    {
        scene->clear();
        reference_point_1_auto_search_area_ideal_item = reference_point_2_auto_search_area_ideal_item = Q_NULLPTR;
    }

    scene->setSceneRect(0, 0, img.width(), img.height());
    pixmap_item = scene->addPixmap(QPixmap::fromImage(img));

    ui->graphicsView->setScene(scene.data());
}

void CurrentForm::removeReferencePoints()
{
    QGraphicsItem *current_item;

    if(ui->graphicsView->scene() == Q_NULLPTR)
        return;

    qint32 i = 0;
    while(i < ui->graphicsView->scene()->items().size() - 1)
    {
        current_item = ui->graphicsView->scene()->items()[i];
        if(current_item != pixmap_item && current_item != reference_point_1_auto_search_area_ideal_item && current_item != reference_point_2_auto_search_area_ideal_item)
        {
            ui->graphicsView->scene()->removeItem(current_item);
            continue;
        }

        i++;
    }

    ui->graphicsView->scene()->update();
}

void CurrentForm::drawReferencePoints()
{
    removeReferencePoints();

    if(PuansonChecker::getInstance()->getCurrent().isReferencePointsAreSet())
    {
        QPoint reference_point1;
        QPoint reference_point2;
        QGraphicsTextItem *text_item;

        PuansonChecker::getInstance()->getCurrent().getReferencePoints(reference_point1, reference_point2);

        ui->graphicsView->scene()->addEllipse(reference_point1.x() - GRAPHICAL_POINT_RADIUS, reference_point1.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::red, Qt::Dense5Pattern));

        text_item = ui->graphicsView->scene()->addText("Реперная точка 1", QFont("Arial", 10, QFont::Bold));
        text_item->setDefaultTextColor(Qt::red);
        text_item->setPos(reference_point1.x() + GRAPHICAL_POINT_RADIUS + 3, reference_point1.y() - GRAPHICAL_POINT_RADIUS - 3);

        ui->graphicsView->scene()->addEllipse(reference_point2.x() - GRAPHICAL_POINT_RADIUS, reference_point2.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::red, Qt::Dense5Pattern));
        text_item = ui->graphicsView->scene()->addText("Реперная точка 2", QFont("Arial", 10, QFont::Bold));
        text_item->setDefaultTextColor(Qt::red);
        text_item->setPos(reference_point2.x() - GRAPHICAL_POINT_RADIUS - 60, reference_point2.y() + GRAPHICAL_POINT_RADIUS + 3);
    }
}

void CurrentForm::drawReferencePointAutoSearchArea(const ReferencePointType_e reference_point_type, const QRect &search_area_rect)
{
    QGraphicsRectItem **reference_point_auto_search_area_ideal_item = Q_NULLPTR;

    switch(reference_point_type)
    {
    case ReferencePointType_e::REFERENCE_POINT_1:
    default:
        reference_point_auto_search_area_ideal_item = &reference_point_1_auto_search_area_ideal_item;
        break;
    case ReferencePointType_e::REFERENCE_POINT_2:
        reference_point_auto_search_area_ideal_item = &reference_point_2_auto_search_area_ideal_item;
        break;
    }

    if(ui->graphicsView->scene() != Q_NULLPTR && reference_point_auto_search_area_ideal_item && *reference_point_auto_search_area_ideal_item && (*reference_point_auto_search_area_ideal_item)->scene() == ui->graphicsView->scene())
    {
        ui->graphicsView->scene()->removeItem(*reference_point_auto_search_area_ideal_item);
        ui->graphicsView->scene()->update();
    }

    if(ui->graphicsView->scene())
        *reference_point_auto_search_area_ideal_item = ui->graphicsView->scene()->addRect(search_area_rect, QPen(Qt::red, 3));
}

void CurrentForm::mousePressEvent(const QPoint &p)
{
    switch(calibration_mode)
    {
        case CalibrationMode_e::REFERENCE_POINT_1:
        {
            reference_point1 = p;

            ui->graphicsView->scene()->addEllipse(p.x() - GRAPHICAL_POINT_RADIUS, p.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::red, Qt::Dense5Pattern));
            QGraphicsTextItem *text_item = ui->graphicsView->scene()->addText("Реперная точка 1", QFont("Arial", 10, QFont::Bold));
            text_item->setDefaultTextColor(Qt::red);
            text_item->setPos(p.x() + GRAPHICAL_POINT_RADIUS + 3, p.y() - GRAPHICAL_POINT_RADIUS - 3);
        }
        break;
        case CalibrationMode_e::REFERENCE_POINT_2:
        {
            reference_point2 = p;
            PuansonChecker::getInstance()->getCurrent().setReferencePoints(reference_point1, reference_point2);

            ui->graphicsView->scene()->addEllipse(p.x() - GRAPHICAL_POINT_RADIUS, p.y() - GRAPHICAL_POINT_RADIUS, GRAPHICAL_POINT_RADIUS*2, GRAPHICAL_POINT_RADIUS*2, QPen(Qt::black, 1), QBrush(Qt::red, Qt::Dense5Pattern));
            QGraphicsTextItem *text_item = ui->graphicsView->scene()->addText("Реперная точка 2", QFont("Arial", 10, QFont::Bold));
            text_item->setDefaultTextColor(Qt::red);
            text_item->setPos(p.x() - GRAPHICAL_POINT_RADIUS - 60, p.y() + GRAPHICAL_POINT_RADIUS + 3);
        }
        break;
        case CalibrationMode_e::NO_CALIBRATION:
        default:
        break;
    }
}

void CurrentForm::mouseReleaseEvent(const QPoint &p)
{
    Q_UNUSED(p)

    if(calibration_mode == CalibrationMode_e::REFERENCE_POINT_1)
    {
        setCalibrationMode(CalibrationMode_e::REFERENCE_POINT_2);
    }
    else if(calibration_mode == CalibrationMode_e::REFERENCE_POINT_2)
    {
        setCalibrationMode(CalibrationMode_e::NO_CALIBRATION);
        PuansonChecker::getInstance()->updateContoursImage();
    }
}

void CurrentForm::setLabel2Text(const QString &text)
{
    ui->label_2->setText(text);
}

void CurrentForm::drawActualBorders()
{
    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

    ui->graphicsView->scene()->addRect( qRound((ui->graphicsView->scene()->width() - ui->graphicsView->scene()->width() / 4.0) / 2.0),
                                        qRound((ui->graphicsView->scene()->height() - ui->graphicsView->scene()->height() / 4.0) / 2.0),
                                        qRound(ui->graphicsView->scene()->width() / 4.0) ,
                                        qRound(ui->graphicsView->scene()->height() / 4.0), QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin) );
}

bool CurrentForm::windowKeyPressEvent(QKeyEvent *event)
{
    if(image_move_mode == IMAGE_MOVE_EDITING)
    {
        switch(event->key())
        {
            case Qt::Key_Left:
                    PuansonChecker::getInstance()->shiftCurrentImage(-1, 0);
                break;
            case Qt::Key_Up:
                    PuansonChecker::getInstance()->shiftCurrentImage(0, -1);
                break;
            case Qt::Key_Right:
                    PuansonChecker::getInstance()->shiftCurrentImage(1, 0);
                break;
            case Qt::Key_Down:
                    PuansonChecker::getInstance()->shiftCurrentImage(0, 1);
                break;
            default:
                break;
        }

        PuansonChecker::getInstance()->drawCurrentImage();
        PuansonChecker::getInstance()->drawContoursImage();

        return true;
    }
    else if(calibration_mode != CalibrationMode_e::NO_CALIBRATION && event->key() == Qt::Key_Escape)
    {
        removeReferencePoints();
        drawReferencePoints();
        setCalibrationMode(CalibrationMode_e::NO_CALIBRATION);

        return true;
    }

    return false;
}

/*void CurrentForm::calculateContourOnRotationCheckBoxStateChanged(int state)
{
    PuansonChecker::getInstance()->setCalculateContourOnRotationFlag(state == Qt::Checked);
}*/

void CurrentForm::shotAndLoadButtonPressedSlot()
{
    ui->shotAndLoadButton->setEnabled(false);

    PuansonChecker::getInstance()->shootAndLoadCurrentImage();
}

void CurrentForm::setReferencePointsManuallyButtonPressedSlot()
{
    if(PuansonChecker::getInstance()->isCurrentImageLoaded())
        setCalibrationMode(CalibrationMode_e::REFERENCE_POINT_1);
    else
        QMessageBox::warning(this, "Внимание!", "Изображение текущей детали не загружено!");
}

void CurrentForm::setReferencePointsAutomaticButtonPressedSlot()
{
    if(!PuansonChecker::getInstance()->isCurrentImageLoaded())
    {
        QMessageBox::warning(this, "Внимание!", "Изображение текущей детали не загружено!");

        return;
    }

    if(PuansonChecker::getInstance()->getReferencePointSearchArea(ImageType_e::CURRENT_IMAGE, ReferencePointType_e::REFERENCE_POINT_1).isNull())
    {
        QMessageBox::warning(this, "Внимание", "Не задана область поиска левой нижней реперной точки");
        return;
    }

    if(PuansonChecker::getInstance()->getReferencePointSearchArea(ImageType_e::CURRENT_IMAGE, ReferencePointType_e::REFERENCE_POINT_2).isNull())
    {
        QMessageBox::warning(this, "Внимание", "Не задана область поиска правой верхней реперной точки");
        return;
    }

    QVector<QPointF> reference_points = PuansonChecker::getInstance()->findReferencePoints(PuansonChecker::getInstance()->getCurrent());
    PuansonChecker::getInstance()->getCurrent().setReferencePoints(reference_points[0].toPoint(), reference_points[1].toPoint());
    PuansonChecker::getInstance()->drawCurrentImageReferencePoints();
    PuansonChecker::getInstance()->updateContoursImage();
}

void CurrentForm::cameraConnectionStatusChangedSlot(bool connected)
{
    Q_UNUSED(connected);

    ui->cameraStatusLabel->setText("<b>Статус камеры:</b> " + PuansonChecker::getInstance()->getCameraStatus());
}

void CurrentForm::loadImageFinished()
{
    if(!ui->shotAndLoadButton->isEnabled())
        ui->shotAndLoadButton->setEnabled(true);
}

void CurrentForm::setCalibrationMode(CalibrationMode_e mode)
{
    calibration_mode = mode;

    switch(calibration_mode)
    {
        case CalibrationMode_e::REFERENCE_POINT_1:
            removeReferencePoints();
            setLabel2Text("Калибровка. Укажите реперную точку 1.");
            setImageCursor(Qt::CrossCursor);
        break;
        case CalibrationMode_e::REFERENCE_POINT_2:
            setLabel2Text("Калибровка. Укажите реперную точку 2.");
        break;
        case CalibrationMode_e::NO_CALIBRATION:
        default:
            setLabel2Text("Файл " + PuansonChecker::getInstance()->getCurrent().getFilename());

            switch(image_move_mode)
            {
                case IMAGE_MOVE_EDITING:
                    setImageCursor(Qt::OpenHandCursor);
                break;
                case IMAGE_MOVE_VIEWING:
                default:
                    setImageCursor(Qt::ArrowCursor);
                break;
            }
        break;
    }
}

CurrentForm::CurrentForm(PuansonChecker *checker) :
    QWidget(Q_NULLPTR),
    ImageWindow(new CurrentFormGraphicsScene(this)),
    ui(new Ui::CurrentForm),
    image_move_mode(IMAGE_MOVE_VIEWING)
{
    ui->setupUi(this);

    //ui->calculateContourOnRotationCheckBox->setCheckState(checker->getCalculateContourOnRotationFlag() ? Qt::Checked : Qt::Unchecked);
    //setImageCursor(Qt::OpenHandCursor);
    setImageCursor(Qt::ArrowCursor);

    ui->cameraStatusLabel->setText("<b>Статус камеры:</b> " + checker->getCameraStatus());

    //connect(ui->calculateContourOnRotationCheckBox, SIGNAL(stateChanged(int)), SLOT(calculateContourOnRotationCheckBoxStateChanged(int)));
    connect(ui->shotAndLoadButton, SIGNAL(pressed()), SLOT(shotAndLoadButtonPressedSlot()));
    connect(checker, SIGNAL(cameraConnectionStatusChanged(bool)), SLOT(cameraConnectionStatusChangedSlot(bool)));
    connect(ui->setReferencePointsManuallyButton, SIGNAL(pressed()), SLOT(setReferencePointsManuallyButtonPressedSlot()));
    connect(ui->setReferencePointsAutomaticButton, SIGNAL(pressed()), SLOT(setReferencePointsAutomaticButtonPressedSlot()));
}

CurrentForm::~CurrentForm()
{
    delete ui;
}
