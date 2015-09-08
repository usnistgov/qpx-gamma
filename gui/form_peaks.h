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
#include "widget_plot1d.h"
#include <QItemSelection>
#include "spectra_set.h"
#include "gamma_fitter.h"

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
  void setSpectrum(Pixie::Spectrum::Spectrum *newspectrum);

  void clear();
  void update_fit(bool content_changed = false);
  void update_spectrum();

  void loadSettings(QSettings &settings_);
  void saveSettings(QSettings &settings_);

signals:
  void peaks_changed(bool content_changed);

private slots:

  void user_selected_peaks();

  void addMovingMarker(double);
  void removeMovingMarker(double);

  void replot_all();
  void replot_markers();
  void toggle_push();

  void range_moved();

  void on_pushAdd_clicked();
  void on_pushMarkerRemove_clicked();
  void on_pushFindPeaks_clicked();
  void on_checkShowMovAvg_clicked();
  void on_checkShowPrelimPeaks_clicked();
  void on_checkShowGaussians_clicked();
  void on_checkShowBaselines_clicked();
  void on_checkShowFilteredPeaks_clicked();

  void on_spinMovAvg_editingFinished();

  void on_spinMinPeakWidth_editingFinished();

  void on_checkShowPseudoVoigt_clicked();

  void on_doubleOverlapWidth_editingFinished();

private:
  Ui::FormPeaks *ui;

  //from parent
  Pixie::Spectrum::Spectrum *spectrum_;
  std::map<double, double> minima, maxima;

  //data from selected spectrum
  Gamma::Fitter *fit_data_;

  //markers
  Marker list, selected;
  Range range_;

  AppearanceProfile main_graph_, prelim_peak_, filtered_peak_,
                    gaussian_, pseudo_voigt_, baseline_,
                    multiplet_,
                    rise_, fall_, even_;

  void plot_derivs();
};

#endif // FORM_CALIBRATION_H
