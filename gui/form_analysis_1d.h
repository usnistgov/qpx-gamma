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
 *      FormAnalysis1D - 
 *
 ******************************************************************************/


#ifndef FORM_ANALYSIS_1D_H
#define FORM_ANALYSIS_1D_H

#include <QWidget>
#include <QSettings>
#include "spectrum1D.h"
#include "spectra_set.h"
#include "form_energy_calibration.h"
#include "form_fwhm_calibration.h"
#include "form_peak_fitter.h"

namespace Ui {
class FormAnalysis1D;
}

class FormAnalysis1D : public QWidget
{
  Q_OBJECT

public:
  explicit FormAnalysis1D(QSettings &settings, XMLableDB<Gamma::Detector>& newDetDB, QWidget *parent = 0);
  ~FormAnalysis1D();

  void setSpectrum(Pixie::SpectraSet *newset, QString spectrum);

  void clear();

signals:
  void calibrationComplete();
  void detectorsChanged();

public slots:
  void update_spectrum();
  void update_detector_calibs();

private slots:

  void update_peaks(bool);
  void update_peaks_from_fwhm(bool);
  void update_peaks_from_nrg(bool);
  void update_peaks_from_fitter(bool);
  void detectorsUpdated() {emit detectorsChanged();}

protected:
  void closeEvent(QCloseEvent*);

private:
  Ui::FormAnalysis1D *ui;
  QSettings &settings_;

  FormEnergyCalibration *my_energy_calibration_;
  FormFwhmCalibration *my_fwhm_calibration_;
  FormPeakFitter *my_peak_fitter_;


  Gamma::Fitter fit_data_;

  Gamma::Calibration nrg_calibration_, fwhm_calibration_;

  //from parent
  QString data_directory_;
  Pixie::SpectraSet *spectra_;
  QString current_spectrum_;

  XMLableDB<Gamma::Detector> &detectors_;
  Gamma::Detector detector_;

  void loadSettings();
  void saveSettings();
};

#endif // FORM_CALIBRATION_H
