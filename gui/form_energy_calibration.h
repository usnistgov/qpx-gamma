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
#include "spectrum1D.h"
#include "table_markers.h"
#include "special_delegate.h"
#include "isotope.h"
#include "widget_plot1d.h"
#include <QItemSelection>
#include "spectrum.h"
#include "gamma_fitter.h"

namespace Ui {
class FormEnergyCalibration;
}

class FormEnergyCalibration : public QWidget
{
  Q_OBJECT

public:
  explicit FormEnergyCalibration(QSettings &settings, XMLableDB<Gamma::Detector>&, QWidget *parent = 0);
  ~FormEnergyCalibration();

  void setSpectrum(Pixie::Spectrum::Spectrum *);
  void update_peaks(std::vector<Gamma::Peak>);
  void update_peak_selection(std::set<double>);

  void clear();

signals:
  void calibrationComplete();
  void detectorsChanged();
  void peaks_chosen(std::set<double>);

private slots:


  void replot_markers();
  void selection_changed(QItemSelection, QItemSelection);
  void toggle_push();
  void isotope_energies_chosen();
  void on_pushApplyCalib_clicked();

  void toggle_radio();
  void set_conditions(bool, bool);
  void detectorsUpdated() {emit detectorsChanged();}

  void on_pushFit_clicked();
  void on_pushFromDB_clicked();
  void on_pushDetDB_clicked();
  void on_pushAllmarkers_clicked();
  void on_pushAllEnergies_clicked();


protected:
  void closeEvent(QCloseEvent*);

private:
  Ui::FormEnergyCalibration *ui;
  QSettings &settings_;

  //from parent
  QString data_directory_;

  Pixie::Spectrum::Spectrum *spectrum_;
  QString spectrum_name_;

  XMLableDB<Gamma::Detector> &detectors_;
  Gamma::Detector detector_;

  std::map<double, double> my_markers_; //channel, energy
  TableMarkers marker_table_;
  QItemSelectionModel selection_model_;
  QpxSpecialDelegate  special_delegate_;
  Gamma::Calibration old_calibration_, new_calibration_;

  bool others_have_det_;
  bool DB_has_detector_;

  void loadSettings();
  void saveSettings();
};

#endif // FORM_CALIBRATION_H
