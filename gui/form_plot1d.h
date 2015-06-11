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
 *
 ******************************************************************************/

#ifndef FORM_PLOT1D_H
#define FORM_PLOT1D_H

#include <QWidget>
#include <spectra_set.h>
#include "qsquarecustomplot.h"
#include "widget_spectra_selector.h"
#include "widget_plot1d.h"

namespace Ui {
class FormPlot1D;
}

class FormPlot1D : public QWidget
{
  Q_OBJECT

public:
  explicit FormPlot1D(QWidget *parent = 0);
  ~FormPlot1D();

  void setSpectra(Pixie::SpectraSet& new_set);

  void update_plot();

  void replot_markers();
  void reset_content();

public slots:
  void set_markers2d(Marker x, Marker y);

signals:
  void marker_set(Marker n);
  void requestCalibration(QString);

private slots:
  void on_comboResolution_currentIndexChanged(int index);
  void spectrumDetails();
  void spectrumDetailsClosed(bool);
  void spectraLooksChanged();

  void addMovingMarker(double);
  void removeMovingMarker(double);


  void on_pushFullInfo_clicked();

  void on_pushCalibrate_clicked();


  void on_pushShowAll_clicked();

  void on_pushHideAll_clicked();

private:

  Ui::FormPlot1D *ui;

  int bits;
  Pixie::Calibration calib_;

  Pixie::SpectraSet *mySpectra;

  Marker moving, markx, marky;

  void calibrate_markers();
};

#endif // WIDGET_PLOT1D_H
