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
 *      FormCoincPeaks -
 *
 ******************************************************************************/


#ifndef FORM_COINC_PEAKS_H
#define FORM_COINC_PEAKS_H

#include <QWidget>
#include <QSettings>
#include <QItemSelection>
#include "form_fitter.h"

namespace Ui {
class FormCoincPeaks;
}

class FormCoincPeaks : public QWidget
{
  Q_OBJECT

public:
  explicit FormCoincPeaks(QWidget *parent = 0);
  ~FormCoincPeaks();

  void setFit(Qpx::Fitter *);

  void update_spectrum();
  void perform_fit();

  std::set<double> get_selected_peaks();

  void make_range(Coord);

  void loadSettings(QSettings &settings_);
  void saveSettings(QSettings &settings_);

public slots:
  void update_selection(std::set<double> selected_peaks);

signals:
  void peaks_changed();
  void peak_selection_changed(std::set<double> selected_peaks);
//  void range_changed(RangeSelector range);
  void fitting_done();

private slots:
//  void change_range(RangeSelector range);
  void selection_changed_in_table();
  void update_peaks();
  void fitting_finished();

private:
  Ui::FormCoincPeaks *ui;

  //data from selected spectrum
  Qpx::Fitter *fit_data_;
  ThreadFitter thread_fitter_;

  std::set<double> selected_peaks_;

  void update_table(bool);
  void add_peak_to_table(const Qpx::Peak &, int, QColor);
};

#endif
