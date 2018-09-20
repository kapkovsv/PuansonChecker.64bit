#ifndef DETAILANGLESOURCEDIALOG_H
#define DETAILANGLESOURCEDIALOG_H

#include "types.h"
#include <QDialog>

enum /*class*/ DetailAngleSourceDialogResult_e
{
    PHOTO_SHOOTING_DIALOG_RESULT = 0,
    LOADING_FROM_FILE_DIALOG_RESULT = 1,
    CLOSE_DIALOG_RESULT = 2
};

namespace Ui {
class DetailAngleSourceDialog;
}

class DetailAngleSourceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DetailAngleSourceDialog(ImageType_e image_type, quint8 etalon_research_active_angle, QWidget *parent = 0);
    ~DetailAngleSourceDialog();

public slots:
    void loadingFromFileButtonPressed();
    void photoShootingButtonPressed();

private:
    void closeEvent(QCloseEvent *e) Q_DECL_OVERRIDE;

    Ui::DetailAngleSourceDialog *ui;
};

#endif // DETAILANGLESOURCEDIALOG_H
