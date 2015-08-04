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
  explicit FormAnalysis2D(QSettings &settings, XMLableDB<Gamma::Detector>& newDetDB, QWidget *parent = 0);
  ~FormAnalysis2D();

  void setSpectrum(Pixie::SpectraSet *newset, QString spectrum);

  void clear();

signals:
  void calibrationComplete();
  void detectorsChanged();

public slots:
  void update_spectrum();

private slots:
  void update_gates(Marker, Marker);

  void update_peaks(bool);
  void detectorsUpdated() {emit detectorsChanged();}

protected:
  void closeEvent(QCloseEvent*);
  void showEvent(QShowEvent* event);

private:
  Ui::FormAnalysis2D *ui;
  QSettings &settings_;


  Pixie::Spectrum::Template *tempx, *tempy;

  Pixie::Spectrum::Spectrum *gate_x;
  Pixie::Spectrum::Spectrum *gate_y;

  Gamma::Fitter fit_data_, fit_data_2_;
  int res;
  int xmin, xmax, ymin, ymax;

  double live_seconds,
         sum_inclusive,
         sum_exclusive,
         sum_no_peaks,
         sum_prism;

  Gamma::Detector detector1_;
  Gamma::Calibration nrg_calibration1_, fwhm_calibration1_;

  Gamma::Detector detector2_;
  Gamma::Calibration nrg_calibration2_, fwhm_calibration2_;


  //from parent
  QString data_directory_;
  Pixie::SpectraSet *spectra_;
  QString current_spectrum_;

  XMLableDB<Gamma::Detector> &detectors_;

  void loadSettings();
  void saveSettings();
  void make_gated_spectra();
  void fill_table();
};

#endif // FORM_CALIBRATION_H
