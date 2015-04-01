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
 *      FormCalibration -
 *
 ******************************************************************************/


#ifndef FORM_CALIBRATION_H
#define FORM_CALIBRATION_H

#include <QWidget>
#include "spectrum1D.h"
#include "table_markers.h"
#include "special_delegate.h"
#include "isotope.h"
#include "widget_plot1d.h"
#include <QItemSelection>
#include "spectra_set.h"

namespace Ui {
class FormCalibration;
}

class FormCalibration : public QWidget
{
  Q_OBJECT

public:
  explicit FormCalibration(QWidget *parent = 0);
  ~FormCalibration();

  void setData(XMLableDB<Pixie::Detector>& newDetDB, QSettings &sets);
  void setSpectrum(Pixie::SpectraSet *newset, QString spectrum);

  void clear();

signals:
  void calibrationComplete();
  void detectorsChanged();

private slots:

  void addMovingMarker(double);
  void removeMovingMarker(double);
  void on_pushAdd_clicked();
  void replot_markers();
  void selection_changed(QItemSelection, QItemSelection);
  void toggle_push();
  void on_pushMarkerRemove_clicked();
  void on_pushFit_clicked();
  void on_pushClear_clicked();
  void isotope_energies_chosen();
  void on_pushPushEnergy_clicked();
  void on_pushApplyCalib_clicked();

  void toggle_radio();
  void detectorsUpdated() {emit detectorsChanged();}

  void on_pushFromDB_clicked();

  void on_pushFindPeaks_clicked();

  void update_plot();

protected:
  void closeEvent(QCloseEvent*);

private:
  Ui::FormCalibration *ui;

  //from parent
  QString data_directory_, load_formats_;
  Pixie::SpectraSet *spectra_;
  XMLableDB<Pixie::Detector> *detectors_;

  //data from selected spectrum
  QString current_spectrum_;
  QVector<double> x_chan, y;
  Pixie::Detector detector_;

  //markers
  Marker moving, list, selected;

  std::map<double, double> my_markers_; //channel, energy
  TableMarkers marker_table_;
  QItemSelectionModel selection_model_;
  QpxSpecialDelegate  special_delegate_;

  Pixie::Calibration old_calibration_, new_calibration_;

};

#endif // FORM_CALIBRATION_H
