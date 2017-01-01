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
 *      FormFitter -
 *
 ******************************************************************************/


#ifndef FORM_FITTER_H
#define FORM_FITTER_H

#include <QWidget>
#include "special_delegate.h"
#include <QItemSelection>
#include <QMediaPlayer>

#include "thread_fitter.h"
#include "qp_fitter.h"
#include "coord.h"

namespace Ui {
class FormFitter;
}

class FormFitter : public QWidget
{
  Q_OBJECT

public:
  explicit FormFitter(QWidget *parent = 0);
  ~FormFitter();

  void clear();

  void setFit(Qpx::Fitter *fit);
  void update_spectrum();

  bool busy() { return busy_; }

  void clearSelection();
  std::set<double> get_selected_peaks();

  void make_range(Coord);
//  void set_range(RangeSelector);

  void perform_fit();

  void loadSettings(QSettings &settings_);
  void saveSettings(QSettings &settings_);

public slots:
  void set_selected_peaks(std::set<double> selected_peaks);
  void updateData();

signals:

  void peak_selection_changed(std::set<double> selected_peaks);
//  void range_changed(RangeSelector range);
  void data_changed();
  void fitting_done();
  void fitter_busy(bool);

  void range_selection_changed(double l, double r);

private slots:

  void selection_changed();
  void update_range_selection(double l, double r);

  void fit_updated(Qpx::Fitter);
  void fitting_complete();

  void add_peak(double l, double r);
  void delete_selected_peaks();
  void adjust_sum4(double peak_id, double l, double r);
  void adjust_background_L(double roi_id, double l, double r);
  void adjust_background_R(double roi_id, double l, double r);
  void peak_info(double peak);

  void rollback_ROI(double);
  void roi_settings(double region);
  void refit_ROI(double);
  void delete_ROI(double);

  void toggle_push(bool busy);

  void on_pushFindPeaks_clicked();
  void on_pushStopFitter_clicked();
  void on_pushSettings_clicked();

private:
  Ui::FormFitter *ui;

  Qpx::Fitter *fit_data_;

  bool busy_ {false};

  ThreadFitter thread_fitter_;
  QMediaPlayer *player;

};



#endif
