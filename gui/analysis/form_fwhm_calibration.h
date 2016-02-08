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
#include "widget_plot_calib.h"
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
  explicit FormFwhmCalibration(QSettings &settings, XMLableDB<Gamma::Detector>&, Gamma::Fitter&, QWidget *parent = 0);
  ~FormFwhmCalibration();

  void newSpectrum();
  Gamma::Calibration get_new_calibration() {return new_calibration_;}

  void clear();
  bool save_close();

public slots:
  void update_selection(std::set<double> selected_peaks);
  void update_data();

signals:
  void selection_changed(std::set<double> selected_peaks);
  void detectorsChanged();
  void update_detector();
  void new_fit();

private slots:
  void selection_changed_in_table();
  void selection_changed_in_plot();

  void toggle_push();
  void detectorsUpdated() {emit detectorsChanged();}

  void on_pushApplyCalib_clicked();
  void on_pushFit_clicked();
  void on_pushFromDB_clicked();
  void on_pushDetDB_clicked();


  void on_doubleMaxErr_valueChanged(double arg1);

private:
  Ui::FormFwhmCalibration *ui;
  QSettings &settings_;

  //from parent
  QString data_directory_;
  QString settings_directory_;

  XMLableDB<Gamma::Detector> &detectors_;
  Gamma::Fitter &fit_data_;
  std::set<double> selected_peaks_;
  
  Gamma::Calibration new_calibration_;
  AppearanceProfile style_pts, style_relevant, style_fit;

  void loadSettings();
  void saveSettings();
  Polynomial fit_calibration();

  void replot_calib();
  void rebuild_table();
  void select_in_table();
  void select_in_plot();

  void add_peak_to_table(const Gamma::Peak &, int, bool);
  void data_to_table(int row, int column, double value, QBrush background);

};

#endif // FORM_FWHM_CALIBRATION_H
