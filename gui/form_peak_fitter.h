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
 *      FormPeakFitter - 
 *
 ******************************************************************************/


#ifndef FORM_PEAK_FITTER_H
#define FORM_PEAK_FITTER_H

#include <QWidget>
#include "spectrum1D.h"
#include "special_delegate.h"
#include "widget_plot1d.h"
#include <QItemSelection>
#include "spectrum.h"
#include "gamma_fitter.h"

namespace Ui {
class FormPeakFitter;
}

class FormPeakFitter : public QWidget
{
  Q_OBJECT

public:
  explicit FormPeakFitter(QSettings &settings, QWidget *parent = 0);
  ~FormPeakFitter();

  void update_peaks(std::vector<Gamma::Peak>);
  void update_peak_selection(std::set<double>);

  void clear();
  bool save_close();

signals:
  void peaks_changed(std::vector<Gamma::Peak>, bool);

private slots:
  void selection_changed_in_table();

  void replot_markers();
  void toggle_push();

private:
  Ui::FormPeakFitter *ui;
  QSettings &settings_;

  //from parent
  QString data_directory_;

  std::vector<Gamma::Peak> peaks_;

  void loadSettings();
  void saveSettings();
  void add_peak_to_table(Gamma::Peak, int);
};

#endif // FORM_PEAK_FITTER_H
