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
 *      FormFwhmCalibration -
 *
 ******************************************************************************/


#ifndef FORM_FWHM_CALIBRATION_H
#define FORM_FWHM_CALIBRATION_H

#include <QWidget>
#include "spectrum1D.h"
#include "special_delegate.h"
#include "widget_plot1d.h"
#include <QItemSelection>
#include "spectrum.h"
#include "gamma_fitter.h"

namespace Ui {
class FormFwhmCalibration;
}

class FormFwhmCalibration : public QWidget
{
  Q_OBJECT

public:
  explicit FormFwhmCalibration(QSettings &settings, XMLableDB<Gamma::Detector>&, QWidget *parent = 0);
  ~FormFwhmCalibration();

  void setData(Gamma::Calibration fwhm_calib, uint16_t bits);

  void update_peaks(std::vector<Gamma::Peak>);
  void update_peak_selection(std::set<double>);
  Gamma::Calibration get_new_calibration() {return new_fwhm_calibration_;}

  void clear();
  bool save_close();


signals:
  void detectorsChanged();
  void peaks_chosen(std::set<double>);
  void update_detector(bool, bool);

private slots:


  void replot_markers();
  void selection_changed();
  void toggle_push();
  void on_pushApplyCalib_clicked();

  void detectorsUpdated() {emit detectorsChanged();}

  void on_pushFit_clicked();
  void on_pushFromDB_clicked();
  void on_pushDetDB_clicked();


private:
  Ui::FormFwhmCalibration *ui;
  QSettings &settings_;

  //from parent
  QString data_directory_;

  Pixie::Spectrum::Spectrum *spectrum_;
  QString spectrum_name_;

  XMLableDB<Gamma::Detector> &detectors_;
  Gamma::Detector detector_;

  std::vector<Gamma::Peak> peaks_;
  uint16_t bits_;
  Gamma::Calibration old_fwhm_calibration_, new_fwhm_calibration_;

  AppearanceProfile calib_point, calib_line, calib_selected;

  void loadSettings();
  void saveSettings();
};

#endif // FORM_CALIBRATION_H
