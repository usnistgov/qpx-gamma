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
 *      FormGainCalibration -
 *
 ******************************************************************************/


#ifndef FORM_GAIN_CALIBRATION_H
#define FORM_GAIN_CALIBRATION_H

#include <QWidget>
#include "gamma_fitter.h"
#include "widget_plot_calib.h"

namespace Ui {
class FormGainCalibration;
}

class FormGainCalibration : public QWidget
{
  Q_OBJECT

public:
  explicit FormGainCalibration(QSettings &settings, XMLableDB<Qpx::Detector>&, Qpx::Fitter&, Qpx::Fitter&, QWidget *parent = 0);
  ~FormGainCalibration();

  void newSpectrum();
  void update_peaks();
  void clear();

signals:
  void selection_changed(std::set<double> selected_peaks);
  void update_detector();
  void symmetrize_requested();

private slots:
  void selection_changed_in_plot();
  void plot_calib();

  void on_pushSaveCalib_clicked();
  void on_pushCalibGain_clicked();

  void on_pushSymmetrize_clicked();

private:
  Ui::FormGainCalibration *ui;
  QSettings &settings_;

  //from parent
  QString data_directory_;
  QString settings_directory_;

  XMLableDB<Qpx::Detector> &detectors_;

  Qpx::Fitter &fit_data1_;
  Qpx::Fitter &fit_data2_;

  Qpx::Detector detector1_;
  Qpx::Detector detector2_;
  Qpx::Calibration nrg_calibration1_;
  Qpx::Calibration nrg_calibration2_;


  Qpx::Calibration gain_match_cali_;
  AppearanceProfile style_pts, style_fit;

  void loadSettings();
  void saveSettings();

};

#endif // FORM_GAIN_CALIBRATION_H
