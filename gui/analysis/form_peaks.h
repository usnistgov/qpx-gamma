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
 *      FormPeaks - 
 *
 ******************************************************************************/


#ifndef FORM_PEAKS_H
#define FORM_PEAKS_H

#include <QWidget>
#include "spectrum1D.h"
#include "special_delegate.h"
#include "isotope.h"
#include "widget_plot_fit.h"
#include <QItemSelection>
#include "spectra_set.h"
#include "gamma_fitter.h"
#include "thread_fitter.h"

namespace Ui {
class FormPeaks;
}

class FormPeaks : public QWidget
{
  Q_OBJECT

public:
  explicit FormPeaks(QWidget *parent = 0);
  ~FormPeaks();
  void setFit(Gamma::Fitter *);

  void new_spectrum(QString title = QString());
  void update_fit(bool content_changed = false);

  void perform_fit();

  void tighten();

  void clear();
  void update_spectrum();
  void make_range(Marker);

  void loadSettings(QSettings &settings_);
  void saveSettings(QSettings &settings_);

signals:
  void peaks_changed(bool content_changed);
  void range_changed(Range range);

private slots:

  void user_selected_peaks();

  void fit_updated(Gamma::Fitter);

  void addMovingMarker(Coord);
  void addMovingMarker(double);
  void removeMovingMarker(double);

  void replot_all();
  void replot_markers();
  void toggle_push();

  void range_moved(double, double);

  void on_pushAdd_clicked();
  void on_pushMarkerRemove_clicked();
  void on_pushFindPeaks_clicked();

  void on_spinSqWidth_editingFinished();
  void on_doubleOverlapWidth_editingFinished();
  void on_spinSum4EdgeW_editingFinished();
  void on_doubleThresh_editingFinished();


private:
  Ui::FormPeaks *ui;

  //from parent
  std::map<double, double> minima, maxima;

  //data from selected spectrum
  Gamma::Fitter *fit_data_;

  //markers
  Marker list, selected;
  Range range_;

  ThreadFitter thread_fitter_;

};

#endif // FORM_CALIBRATION_H
