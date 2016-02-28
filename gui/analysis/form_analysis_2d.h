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
 *      FormAnalysis2D - 
 *
 ******************************************************************************/


#ifndef FORM_ANALYSIS_2D_H
#define FORM_ANALYSIS_2D_H

#include <QWidget>
#include <QSettings>
#include "spectrum1D.h"
#include "spectra_set.h"
#include "form_multi_gates.h"
#include "form_integration2d.h"

namespace Ui {
class FormAnalysis2D;
}

class FormAnalysis2D : public QWidget
{
  Q_OBJECT

public:
  explicit FormAnalysis2D(QSettings &settings, XMLableDB<Qpx::Detector>& newDetDB, QWidget *parent = 0);
  ~FormAnalysis2D();

  void setSpectrum(Qpx::SpectraSet *newset, QString spectrum);
  void clear();
  void reset();

signals:
  void spectraChanged();
  void detectorsChanged();

public slots:
  void update_spectrum();

private slots:
  void take_boxes();
  void update_gates(Marker, Marker);
  void update_peaks();
  void detectorsUpdated() {emit detectorsChanged();}
  void initialize();
  void matrix_selection();
  void update_range(MarkerBox2D);

  void on_tabs_currentChanged(int index);

protected:
  void closeEvent(QCloseEvent*);
  void showEvent(QShowEvent* event);

private:
  Ui::FormAnalysis2D *ui;
  QSettings &settings_;


  int res;

  FormMultiGates*      my_gates_;
  FormIntegration2D*   form_integration_;

  //from parent
  QString data_directory_;
  QString settings_directory_;

  Qpx::SpectraSet *spectra_;
  QString current_spectrum_;
  MarkerBox2D range2d;

  XMLableDB<Qpx::Detector> &detectors_;

  bool initialized;

  void configure_UI();
  void loadSettings();
  void saveSettings();
};

#endif // FORM_ANALYSIS2D_H
