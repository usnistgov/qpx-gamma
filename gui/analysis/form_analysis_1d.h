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
#include "project.h"
#include "form_energy_calibration.h"
#include "form_fwhm_calibration.h"
#include "form_fit_results.h"

namespace Ui {
class FormAnalysis1D;
}

class FormAnalysis1D : public QWidget
{
  Q_OBJECT

public:
  explicit FormAnalysis1D(QSettings &settings, XMLableDB<Qpx::Detector>& newDetDB, QWidget *parent = 0);
  ~FormAnalysis1D();

  void setSpectrum(Qpx::Project *newset, QString spectrum);

  void clear();

signals:
  void calibrationComplete();
  void detectorsChanged();

public slots:
  void update_spectrum();
  void update_detector_calibs();
  void save_report();

private slots:

  void detectorsUpdated() {emit detectorsChanged();}
  void update_fits();

protected:
  void closeEvent(QCloseEvent*);

private:
  Ui::FormAnalysis1D *ui;
  QSettings &settings_;

  FormEnergyCalibration *form_energy_calibration_;
  FormFwhmCalibration *form_fwhm_calibration_;
  FormFitResults *form_fit_results_;

  Qpx::Fitter fit_data_;

  Qpx::Calibration new_energy_calibration_;
  Qpx::Calibration new_fwhm_calibration_;

  //from parent
  QString data_directory_;
  QString settings_directory_;
  Qpx::Project *spectra_;

  XMLableDB<Qpx::Detector> &detectors_;


  void loadSettings();
  void saveSettings();
};

#endif // FORM_CALIBRATION_H
