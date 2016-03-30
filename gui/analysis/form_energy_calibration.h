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
 *      FormEnergyCalibration - 
 *
 ******************************************************************************/


#ifndef FORM_ENERGY_CALIBRATION_H
#define FORM_ENERGY_CALIBRATION_H

#include <QWidget>
#include "special_delegate.h"
#include "isotope.h"
#include "widget_plot_calib.h"
#include <QItemSelection>
#include "gamma_fitter.h"

namespace Ui {
class FormEnergyCalibration;
}

class FormEnergyCalibration : public QWidget
{
  Q_OBJECT

public:
  explicit FormEnergyCalibration(QSettings &settings, XMLableDB<Qpx::Detector>&, Qpx::Fitter&, QWidget *parent = 0);
  ~FormEnergyCalibration();

  Qpx::Calibration get_new_calibration() {return new_calibration_;}

  void newSpectrum();
  bool save_close();

  void clear();

public slots:
  void update_selection(std::set<double> selected_peaks);
  void update_data();

signals:
  void selection_changed(std::set<double> selected_peaks);
  void change_peaks(std::vector<Qpx::Peak>);
  void detectorsChanged();
  void update_detector();
  void new_fit();

private slots:
  void selection_changed_in_table();
  void selection_changed_in_plot();

  void toggle_push();
  void isotope_energies_chosen();
  void on_pushApplyCalib_clicked();

  void detectorsUpdated() {emit detectorsChanged();}

  void on_pushFit_clicked();
  void on_pushFromDB_clicked();
  void on_pushDetDB_clicked();
  void on_pushPeaksToNuclide_clicked();
  void on_pushEnergiesToPeaks_clicked();

private:
  Ui::FormEnergyCalibration *ui;
  QSettings &settings_;

  //from parent
  QString data_directory_;
  QString settings_directory_;

  XMLableDB<Qpx::Detector> &detectors_;
  Qpx::Fitter &fit_data_;
  std::set<double> selected_peaks_;

  Qpx::Calibration new_calibration_;
  AppearanceProfile style_fit, style_pts;

  void loadSettings();
  void saveSettings();

  void replot_calib();
  void rebuild_table();
  void select_in_table();
  void select_in_plot();

  void add_peak_to_table(const Qpx::Peak &, int, bool);
  void data_to_table(int row, int column, double value, QBrush background);
};

#endif // FORM_CALIBRATION_H
