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
 *      FormSymmetrize2D -
 *
 ******************************************************************************/

#pragma once

#include <QWidget>
#include "project.h"
#include "form_gain_calibration.h"

namespace Ui {
class FormSymmetrize2D;
}

class FormSymmetrize2D : public QWidget
{
  Q_OBJECT

public:
  explicit FormSymmetrize2D(XMLableDB<Qpx::Detector>& newDetDB, QWidget *parent = 0);
  ~FormSymmetrize2D();

  void setSpectrum(Qpx::Project *newset, int64_t idx);
  void update_spectrum();
  void clear();
  void reset();

signals:
  void spectraChanged();
  void detectorsChanged();

public slots:
  void apply_gain_calibration();

private slots:
//  void update_gates(Coord, Coord);
  void update_peaks();
  void peak_selection_changed(std::set<double> selected_peaks);

  void detectorsUpdated() {emit detectorsChanged();}
  void on_pushAddGatedSpectra_clicked();
  void initialize();

  void symmetrize();

protected:
  void closeEvent(QCloseEvent*);
  void showEvent(QShowEvent* event);

private:
  Ui::FormSymmetrize2D *ui;

  Qpx::SinkPtr gate_x;
  Qpx::SinkPtr gate_y;

  Qpx::Fitter fit_data_, fit_data_2_;
  int res;

  double live_seconds;

  Qpx::Detector detector1_;
  Qpx::Detector detector2_;

  Qpx::Calibration nrg_calibration1_;
  Qpx::Calibration nrg_calibration2_;

  Qpx::Calibration gain_match_cali_;

  QPlot::Appearance style_pts, style_fit;

  FormGainCalibration* my_gain_calibration_;


  //from parent
  QString data_directory_;

  Qpx::Project *spectra_;
  int64_t current_spectrum_;

  XMLableDB<Qpx::Detector> &detectors_;

  bool initialized;

  void configure_UI();
  void loadSettings();
  void saveSettings();
  void make_gated_spectra();
};
