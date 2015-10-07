/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * This software can be redistributed and/or modified freely provided that
 * any derivative works bear some notice that they are derived from it, and
 * any modified versions bear some notice that they have been modified.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *
 ******************************************************************************/

#ifndef DIALOG_SPECTRUM_H
#define DIALOG_SPECTRUM_H

#include <QDialog>
#include "spectrum.h"
#include "special_delegate.h"
#include "table_spectrum_attrs.h"
#include "widget_detectors.h"


namespace Ui {
class dialog_spectrum;
}

class dialog_spectrum : public QDialog
{
    Q_OBJECT

public:
    explicit dialog_spectrum(Qpx::Spectrum::Spectrum &spec, QWidget *parent = 0);
    ~dialog_spectrum();


private slots:
    void on_pushRandColor_clicked();
    void on_pushLock_clicked();

    void on_buttonBox_rejected();

    void on_dateTimeStart_editingFinished();

    void on_lineDescription_editingFinished();

    void setNewColor(QColor);
    void det_selection_changed(QItemSelection,QItemSelection);

    void on_pushDetEdit_clicked();
    void changeDet(Gamma::Detector);

    void on_pushDetRename_clicked();

    void on_pushDelete_clicked();

signals:
    void finished(bool);
    void delete_spectrum();

private:
    Ui::dialog_spectrum *ui;
    Qpx::Spectrum::Spectrum &my_spectrum_;

    QpxSpecialDelegate      special_delegate_;
    TableSpectrumAttrs         table_model_;

    XMLableDB<Gamma::Detector> detectors_;
    TableDetectors det_table_model_;
    QItemSelectionModel det_selection_model_;

    Qpx::Spectrum::Metadata md_;

    XMLableDB<Gamma::Setting> attributes_;
    bool changed_;

    void updateData();
    void open_close_locks();
    void toggle_push();
};

#endif // DIALOG_SPECTRUM_H