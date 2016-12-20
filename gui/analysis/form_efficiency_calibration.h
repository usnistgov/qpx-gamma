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
 *      FormEfficiencyCalibration -
 *
 ******************************************************************************/


#ifndef FORM_EFFICIENCY_CALIBRATION_H
#define FORM_EFFICIENCY_CALIBRATION_H

#include <QWidget>
#include "project.h"
#include "form_energy_calibration.h"
#include "widget_selector.h"

namespace Ui {
class FormEfficiencyCalibration;
}

class FormEfficiencyCalibration : public QWidget
{
  Q_OBJECT

public:
  explicit FormEfficiencyCalibration(XMLableDB<Qpx::Detector>& newDetDB, QWidget *parent = 0);
  ~FormEfficiencyCalibration();

  void setDetector(Qpx::Project *newset, QString detector);
//  void clear();

signals:
  void calibrationComplete();
  void detectorsChanged();

public slots:
  void update_spectrum();
  void update_detector_calibs();
  void update_selection(std::set<double> selected_peaks);

private slots:
  void update_data();

  void update_spectra();
  void detectorsUpdated() {emit detectorsChanged();}

  void setSpectrum(int64_t idx);
  void spectrumLooksChanged(SelectorItem);
  void spectrumDetails(SelectorItem);


  void selection_changed_in_table();
  void selection_changed_in_calib_plot();

  void replot_calib();
  void rebuild_table(bool);
  void toggle_push();
  void isotope_chosen();
  void on_pushApplyCalib_clicked();

  void on_pushFit_clicked();
  void on_pushDetDB_clicked();

  void on_pushCullPeaks_clicked();

  void on_doubleEpsilonE_editingFinished();

  void on_doubleScaleFactor_editingFinished();

  void on_doubleScaleFactor_valueChanged(double arg1);

  void on_pushFit_2_clicked();

  void on_pushFitEffit_clicked();

protected:
  void closeEvent(QCloseEvent*);

private:
  Ui::FormEfficiencyCalibration *ui;

  std::map<int64_t, Qpx::Fitter> peak_sets_;
  std::set<double> selected_peaks_;
  std::set<double> falgged_peaks_;

  Qpx::Fitter fit_data_;

  QString mca_load_formats_;  //valid mca file formats that can be opened

  //from parent
  QString data_directory_;
  QString settings_directory_;

  Qpx::Project          *spectra_;
  int64_t     current_spectrum_;
  std::string current_detector_;

  XMLableDB<Qpx::Detector> &detectors_;


  void loadSettings();
  void saveSettings();

  Qpx::Calibration new_calibration_;
  QPlot::Appearance style_fit, style_pts;
  void add_peak_to_table(const Qpx::Peak &, int, QColor);
  void select_in_table();
};

#endif // FORM_EFFICIENCY_CALIBRATION_H
