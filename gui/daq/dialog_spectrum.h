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

#ifndef DialogSpectrum_H
#define DialogSpectrum_H

#include <QDialog>
#include "special_delegate.h"
#include "tree_settings.h"
#include "widget_detectors.h"
#include "daq_sink.h"


namespace Ui {
class DialogSpectrum;
}

class DialogSpectrum : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSpectrum(Qpx::Sink &spec,
                             XMLableDB<Qpx::Detector>& detDB,
                             bool allow_edit,
                             QWidget *parent = 0);
    ~DialogSpectrum();


private slots:
    void on_pushLock_clicked();
    void on_buttonBox_rejected();
    void on_pushDetEdit_clicked();
    void on_pushDetRename_clicked();
    void on_pushDelete_clicked();
    void on_pushAnalyse_clicked();
    void on_pushDetFromDB_clicked();
    void on_pushDetToDB_clicked();
    void on_spinDets_valueChanged(int arg1);

    void push_settings();
    void changeDet(Qpx::Detector);
    void det_selection_changed(QItemSelection,QItemSelection);

signals:
    void finished(bool);
    void delete_spectrum();
    void analyse();

private:
    Ui::DialogSpectrum *ui;
    Qpx::Sink &my_spectrum_;

    TreeSettings               attr_model_;
    QpxSpecialDelegate         attr_delegate_;

    XMLableDB<Qpx::Detector> &detectors_;

    XMLableDB<Qpx::Detector> spectrum_detectors_;
    TableDetectors det_table_model_;
    QItemSelectionModel det_selection_model_;

    Qpx::Metadata md_;

    bool allow_edit_;
    bool changed_;

    void updateData();
    void open_close_locks();
    void toggle_push();
};

#endif // DialogSpectrum_H
