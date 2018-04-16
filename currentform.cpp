#include "currentform.h"
#include "ui_currentform.h"

#include <QGraphicsPixmapItem>
#include <QMessageBox>

CurrentFormGraphicsScene::CurrentFormGraphicsScene(CurrentForm *owner_window):
    window(owner_window), scene_context_menu(window), previousX(0), previousY(0), left_button_down(false)
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

                    qreal dx = __x - previousX;
                    qreal dy = __y - previousY;

                    PuansonChecker::getInstance()->moveImages(dx, dy);
                    previousX = __x;
                    previousY = __y;
                }
            break;
        }
    }
}

void CurrentFormGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if(mouseEvent->buttons() & Qt::LeftButton)
    {
        if(window->getCalibrationMode() != NO_CALIBRATION)
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
    if(window->getCalibrationMode() == NO_CALIBRATION)
    {
        switch(window->getImageMoveMode())
        {
            case IMAGE_MOVE_EDITING:
            {
                int x = mouseEvent->screenPos().x();
                int y = mouseEvent->screenPos().y();
                int dx = x - previousX;
                int dy = y - previousY;

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

void CurrentFormGraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    double rotate_angle = event->delta()/ 120.0 * 0.01;

    PuansonChecker::getInstance()->rotateCurrentImage(rotate_angle);
    PuansonChecker::getInstance()->drawCurrentImage();
    PuansonChecker::getInstance()->drawContoursImage();
}

void CurrentForm::setImageCursor(const QCursor &cursor)
{
    ui->graphicsView->setCursor(cursor);
}

void CurrentForm::moveImage(const qreal dx, const qreal dy)
{
    QGraphicsScene *scene = ui->graphicsView->scene();

    qDebug() << "in " << __PRETTY_FUNCTION__ << "main window line " << __LINE__;
    qDebug() << "scene " << scene;

    if(scene)
        qDebug() << "scene->items count " << scene->items().count();

    QGraphicsPixmapItem *pixmap_item = scene == NULL ? NULL : qgraphicsitem_cast<QGraphicsPixmapItem *>(scene->items().at(scene->items().count()-1));
qDebug() << "in " << __PRETTY_FUNCTION__ << "current window line " << __LINE__;
qDebug() << "pixmap_item " << pixmap_item;
    if(pixmap_item)
    {
        qreal new_x, new_y;
qDebug() << "in " << __PRETTY_FUNCTION__ << "current window line " << __LINE__;
        new_x = image_x - dx;
        new_y = image_y - dy;

        if(new_x <= 0)
            new_x = 1;
        else if(new_x >= pixmap_item->pixmap().width() - ui->graphicsView->width())
            new_x = pixmap_item->pixmap().width() - ui->graphicsView->width();
qDebug() << "in " << __PRETTY_FUNCTION__ << "current window line " << __LINE__;
        if(new_y <= 0)
            new_y = 1;
        else if(new_y >= pixmap_item->pixmap().height() - ui->graphicsView->height())
            new_y = pixmap_item->pixmap().height() - ui->graphicsView->height();
qDebug() << "in " << __PRETTY_FUNCTION__ << "current window line " << __LINE__;
        ui->graphicsView->centerOn(new_x + ui->graphicsView->width() / 2, new_y + ui->graphicsView->height() / 2);
qDebug() << "in " << __PRETTY_FUNCTION__ << "current window line " << __LINE__;
        image_x = new_x;
        image_y = new_y;
    }qDebug() << "in " << __PRETTY_FUNCTION__ << "current window line " << __LINE__;
}

void CurrentForm::drawImage(const QImage &img)
{
    if(!scene->items().isEmpty())
        scene->clear();
    scene->addPixmap(QPixmap::fromImage(img));

    ui->graphicsView->setScene(scene);
}

void CurrentForm::removeReferencePoints()
{
    while(ui->graphicsView->scene()->items().size() != 1)
    {
        ui->graphicsView->scene()->removeItem(ui->graphicsView->scene()->items().first());
        ui->graphicsView->scene()->update();
    }
    ui->graphicsView->scene()->update();
}

void CurrentForm::drawReferencePoints()
{
    qDebug() << "CurrentForm::drawReferencePoints line " << __LINE__;
    if(checker->getCurrent().isReferencePointsAreSet())
    {
        qreal R = 12.0;

        QPoint reference_point1;
        QPoint reference_point2;
qDebug() << "CurrentForm::drawReferencePoints line " << __LINE__;
        QGraphicsTextItem *text_item;

        checker->getCurrent().getReferencePoints(reference_point1, reference_point2);
qDebug() << "CurrentForm::drawReferencePoints line " << __LINE__;
        ui->graphicsView->scene()->addEllipse(reference_point1.x() - R, reference_point1.y() - R, R*2, R*2, QPen(Qt::black, 1), QBrush(Qt::red, Qt::Dense5Pattern));
        text_item = ui->graphicsView->scene()->addText("Реперная точка 1", QFont("Arial", 10, QFont::Bold));
        text_item->setDefaultTextColor(Qt::red);
        text_item->setPos(reference_point1.x() + R + 3, reference_point1.y() - R - 3);
qDebug() << "CurrentForm::drawReferencePoints line " << __LINE__;
        ui->graphicsView->scene()->addEllipse(reference_point2.x() - R, reference_point2.y() - R, R*2, R*2, QPen(Qt::black, 1), QBrush(Qt::red, Qt::Dense5Pattern));
        text_item = ui->graphicsView->scene()->addText("Реперная точка 2", QFont("Arial", 10, QFont::Bold));
        text_item->setDefaultTextColor(Qt::red);
        text_item->setPos(reference_point2.x() - R - 60, reference_point2.y() + R + 3);
        qDebug() << "CurrentForm::drawReferencePoints line " << __LINE__;
    }qDebug() << "CurrentForm::drawReferencePoints line " << __LINE__;
}

void CurrentForm::mousePressEvent(const QPoint &p)
{
    qreal R = 12.0;

    switch(calibration_mode)
    {
        case REFERENCE_POINT_1:
        {
            reference_point1 = p;

            ui->graphicsView->scene()->addEllipse(p.x() - R, p.y() - R, R*2, R*2, QPen(Qt::black, 1), QBrush(Qt::red, Qt::Dense5Pattern));
            QGraphicsTextItem *text_item = ui->graphicsView->scene()->addText("Реперная точка 1", QFont("Arial", 10, QFont::Bold));
            text_item->setDefaultTextColor(Qt::red);
            text_item->setPos(p.x() + R + 3, p.y() - R - 3);
        }
        break;
        case REFERENCE_POINT_2:
        {
            reference_point2 = p;
            checker->getCurrent().setReferencePoints(reference_point1, reference_point2);

            ui->graphicsView->scene()->addEllipse(p.x() - R, p.y() - R, R*2, R*2, QPen(Qt::black, 1), QBrush(Qt::red, Qt::Dense5Pattern));
            QGraphicsTextItem *text_item = ui->graphicsView->scene()->addText("Реперная точка 2", QFont("Arial", 10, QFont::Bold));
            text_item->setDefaultTextColor(Qt::red);
            text_item->setPos(p.x() - R - 60, p.y() + R + 3);
        }
        break;
        case NO_CALIBRATION:
        default:
        break;
    }
}

void CurrentForm::mouseReleaseEvent(const QPoint &p)
{
    Q_UNUSED(p)

    if(calibration_mode == REFERENCE_POINT_1)
        setCalibrationMode(REFERENCE_POINT_2);
    else if(calibration_mode == REFERENCE_POINT_2)
        setCalibrationMode(NO_CALIBRATION);
}

void CurrentForm::setLabel2Text(const QString &text)
{
    ui->label_2->setText(text);
}

void CurrentForm::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Qt::Key_Left:
            if(image_move_mode == IMAGE_MOVE_EDITING)
                checker->shiftCurrentImage(-1, 0);
                checker->drawCurrentImage();
                checker->drawContoursImage();
            break;
        case Qt::Key_Up:
            if(image_move_mode == IMAGE_MOVE_EDITING)
                checker->shiftCurrentImage(0, -1);
                checker->drawCurrentImage();
                checker->drawContoursImage();
            break;
        case Qt::Key_Right:
            if(image_move_mode == IMAGE_MOVE_EDITING)
                checker->shiftCurrentImage(1, 0);
                checker->drawCurrentImage();
                checker->drawContoursImage();
            break;
        case Qt::Key_Down:
            if(image_move_mode == IMAGE_MOVE_EDITING)
                checker->shiftCurrentImage(0, 1);
                checker->drawCurrentImage();
                checker->drawContoursImage();
            break;
        case Qt::Key_Escape:
            if(calibration_mode != NO_CALIBRATION)
            {
                removeReferencePoints();
                setCalibrationMode(NO_CALIBRATION);
            }
            break;
        default:
            break;
    }
}

/*void CurrentForm::calculateContourOnRotationCheckBoxStateChanged(int state)
{
    checker->setCalculateContourOnRotationFlag(state == Qt::Checked);
}*/

void CurrentForm::shotAndLoadButtonPressedSlot()
{
    ui->shotAndLoadButton->setEnabled(false);

    checker->shootAndLoadCurrentImage();
}

void CurrentForm::setReferencePointsButtonPressedSlot()
{
    if(PuansonChecker::getInstance()->isCurrentImageLoaded())
    {
        setCalibrationMode(REFERENCE_POINT_1);
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("Изображение текущей детали не загружено!");
        msgBox.exec();
    }
}

void CurrentForm::cameraConnectionStatusChangedSlot(bool connected)
{
    Q_UNUSED(connected);

    ui->cameraStatusLabel->setText("<b>Статус камеры:</b> " + checker->getCameraStatus());
}

void CurrentForm::loadImageFinished()
{
    qDebug() << "CurrentForm::loadImageFinished line " << __LINE__;
    if(!ui->shotAndLoadButton->isEnabled())
        ui->shotAndLoadButton->setEnabled(true);
    qDebug() << "CurrentForm::loadImageFinished line " << __LINE__;
}

void CurrentForm::setCalibrationMode(CalibrationMode_e mode)
{
    calibration_mode = mode;

    switch(calibration_mode)
    {
        case REFERENCE_POINT_1:
            removeReferencePoints();
            this->ui->label_2->setText("Калибровка. Укажите реперную точку 1.");
            setImageCursor(Qt::CrossCursor);
        break;
        case REFERENCE_POINT_2:
            this->ui->label_2->setText("Калибровка. Укажите реперную точку 2.");
        break;
        case NO_CALIBRATION:
        default:
            this->ui->label_2->setText("");
            switch(mode)
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
    QWidget(NULL),
    ImageWindow(checker),
    ui(new Ui::CurrentForm),
    image_move_mode(IMAGE_MOVE_VIEWING)
{
    this->scene = new CurrentFormGraphicsScene(this);
    ui->setupUi(this);

    //ui->calculateContourOnRotationCheckBox->setCheckState(checker->getCalculateContourOnRotationFlag() ? Qt::Checked : Qt::Unchecked);
    //setImageCursor(Qt::OpenHandCursor);
    setImageCursor(Qt::ArrowCursor);

    ui->cameraStatusLabel->setText("<b>Статус камеры:</b> " + checker->getCameraStatus());

    //connect(ui->calculateContourOnRotationCheckBox, SIGNAL(stateChanged(int)), SLOT(calculateContourOnRotationCheckBoxStateChanged(int)));
    connect(ui->shotAndLoadButton, SIGNAL(pressed()), SLOT(shotAndLoadButtonPressedSlot()));
    connect(checker, SIGNAL(cameraConnectionStatusChanged(bool)), SLOT(cameraConnectionStatusChangedSlot(bool)));
    connect(ui->setReferencePointsButton, SIGNAL(pressed()), SLOT(setReferencePointsButtonPressedSlot()));
}

CurrentForm::~CurrentForm()
{
    qDebug() << "in CurrentForm::~CurrentForm line " << __LINE__;
    delete ui;
    qDebug() << "in CurrentForm::~CurrentForm line " << __LINE__;
}
