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
 *      FormAnalysis2D - 
 *
 ******************************************************************************/


#ifndef FORM_ANALYSIS_2D_H
#define FORM_ANALYSIS_2D_H

#include <QWidget>
#include <QSettings>
#include "spectrum1D.h"
#include "spectra_set.h"
#include "form_energy_calibration.h"
#include "form_fwhm_calibration.h"
#include "form_peak_fitter.h"

namespace Ui {
class FormAnalysis2D;
}

class FormAnalysis2D : public QWidget
{
  Q_OBJECT

public:
  explicit FormAnalysis2D(QSettings &settings, XMLable2DB<Gamma::Detector>& newDetDB, QWidget *parent = 0);
  ~FormAnalysis2D();

  void setSpectrum(Qpx::SpectraSet *newset, QString spectrum);

  void clear();

  void reset();

signals:
  void spectraChanged();
  void detectorsChanged();

public slots:
  void update_spectrum();

private slots:
  void update_gates(Marker, Marker);

  void update_peaks(bool);
  void detectorsUpdated() {emit detectorsChanged();}

  void on_pushCalibGain_clicked();

  void on_pushCull_clicked();

  void on_pushSymmetrize_clicked();

  void on_pushAddGatedSpectra_clicked();

  void on_pushSaveCalib_clicked();

  void initialize();

  void on_comboPlot2_currentIndexChanged(const QString &arg1);

protected:
  void closeEvent(QCloseEvent*);
  void showEvent(QShowEvent* event);

private:
  Ui::FormAnalysis2D *ui;
  QSettings &settings_;


  Qpx::Spectrum::Template *tempx, *tempy;

  Qpx::Spectrum::Spectrum *gate_x;
  Qpx::Spectrum::Spectrum *gate_y;
  bool gatex_in_spectra, gatey_in_spectra;

  Gamma::Fitter fit_data_, fit_data_2_;
  int res;
  int xmin_, xmax_, ymin_, ymax_, xc_, yc_;

  double live_seconds,
         sum_inclusive,
         sum_exclusive,
         sum_no_peaks,
         sum_prism;

  Gamma::Detector detector1_;
  Gamma::Calibration nrg_calibration1_, fwhm_calibration1_;

  Gamma::Detector detector2_;
  Gamma::Calibration nrg_calibration2_, fwhm_calibration2_;

  Gamma::Calibration gain_match_cali_;

  AppearanceProfile style_pts, style_fit;

  //from parent
  QString data_directory_;
  Qpx::SpectraSet *spectra_;
  QString current_spectrum_;

  XMLable2DB<Gamma::Detector> &detectors_;

  bool initialized, symmetrized;

  void loadSettings();
  void saveSettings();
  void make_gated_spectra();
  void fill_table();
  void plot_calib();
  double sum_with_neighbors(Qpx::Spectrum::Spectrum* some_spectrum, uint16_t x, uint16_t y);
  double sum_diag(Qpx::Spectrum::Spectrum* some_spectrum, uint16_t x, uint16_t y, uint16_t w);
};

#endif // FORM_CALIBRATION_H
