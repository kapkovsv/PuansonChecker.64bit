#ifndef CURRENTRESEARCHCREATIONDIALOG_H
#define CURRENTRESEARCHCREATIONDIALOG_H

#include <QDialog>

namespace Ui {
class CurrentResearchCreationDialog;
}

class CurrentResearchCreationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CurrentResearchCreationDialog(QWidget *parent = 0);
    ~CurrentResearchCreationDialog();

public slots:
    void chooseSaveFolderPathPushButtonPressed();
    void saveResearchToDiskCheckBoxStateChanged(int state);
    void currentResearchCreationDialogAccepted();

private:
    Ui::CurrentResearchCreationDialog *ui;
};

#endif // CURRENTRESEARCHCREATIONDIALOG_H
