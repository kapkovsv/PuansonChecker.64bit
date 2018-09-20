#include "detailanglesourcedialog.h"
#include "ui_detailanglesourcedialog.h"

#include <QDebug>

DetailAngleSourceDialog::DetailAngleSourceDialog(ImageType_e image_type, quint8 etalon_research_active_angle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DetailAngleSourceDialog)
{
    ui->setupUi(this);

    QString detail_type_string = "";

    switch(image_type)
    {
    case ImageType_e::ETALON_IMAGE:
        detail_type_string = "эталонной ";
        break;
    case ImageType_e::CURRENT_IMAGE:
        detail_type_string = "текущей ";
        break;
    default:
        detail_type_string = "";
        break;
    }

    ui->questionLabel->setText("Выберете способ получения изображения " + QString::number(etalon_research_active_angle) + "-го ракурса " + detail_type_string + "детали");

    connect(ui->loadingFromFileButton, SIGNAL(pressed()), SLOT(loadingFromFileButtonPressed()));
    connect(ui->photoShootingButton, SIGNAL(pressed()), SLOT(photoShootingButtonPressed()));
}

DetailAngleSourceDialog::~DetailAngleSourceDialog()
{
    delete ui;
}

void DetailAngleSourceDialog::loadingFromFileButtonPressed()
{
    done(DetailAngleSourceDialogResult_e::LOADING_FROM_FILE_DIALOG_RESULT);
}

void DetailAngleSourceDialog::photoShootingButtonPressed()
{
    done(DetailAngleSourceDialogResult_e::PHOTO_SHOOTING_DIALOG_RESULT);
}

void DetailAngleSourceDialog::closeEvent(QCloseEvent *e)
{
    Q_UNUSED(e)

    done(DetailAngleSourceDialogResult_e::CLOSE_DIALOG_RESULT);
}
