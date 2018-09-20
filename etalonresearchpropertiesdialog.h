#ifndef ETALONRESEARCHPROPERTIESDIALOG_H
#define ETALONRESEARCHPROPERTIESDIALOG_H

#include "puansonchecker.h"

#include <QDialog>
#include <QDateTime>

namespace Ui {
class EtalonResearchPropertiesDialog;
}

class EtalonResearchPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EtalonResearchPropertiesDialog(const QString &etalon_research_folder_path, PuansonModel research_puanson_model, const QDateTime &research_date_time_of_creation, const quint8 research_number_of_angles, QWidget *parent = 0);
    ~EtalonResearchPropertiesDialog();

    void setEtalonDimensions(const quint16 _diameter_1_dimension, const quint16 _diameter_2_dimension, const quint16 _diameter_3_dimension, const quint16 _diameter_4_dimension, const quint16 _diameter_5_dimension, const quint16 _diameter_6_dimension, const quint16 _diameter_7_dimension, const quint16 _diameter_8_dimension, const quint16 _diameter_9_dimension, const quint16 _groove_width_dimension, const quint16 _groove_depth_dimension);

public slots:
    void okButtonPressed();

private:
    Ui::EtalonResearchPropertiesDialog *ui;
};

#endif // ETALONRESEARCHPROPERTIESDIALOG_H
