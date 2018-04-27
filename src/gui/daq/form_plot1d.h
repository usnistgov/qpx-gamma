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

#pragma once

#include <QWidget>
#include <project.h>
#include "widget_selector.h"
#include "QPlot1D.h"
#include "coord.h"

namespace Ui {
class FormPlot1D;
}

class FormPlot1D : public QWidget
{
  Q_OBJECT

public:
  explicit FormPlot1D(QWidget *parent = 0);
  ~FormPlot1D();

  void setSpectra(Qpx::Project& new_set);
  void setDetDB(XMLableDB<Qpx::Detector>& detDB);

  void updateUI();

  void update_plot();
  void refresh();

  void replot_markers();
  void reset_content();

  void set_scale_type(QString);
  void set_plot_style(QString);
  QString scale_type();
  QString plot_style();

public slots:
  void set_markers2d(Coord x, Coord y);

signals:
  void marker_set(Coord n);
  void requestAnalysis(int64_t);
  void requestEffCal(QString);

private slots:
  void spectrumDetailsDelete();

  void spectrumLooksChanged(SelectorItem);
  void spectrumDetails(SelectorItem);
  void spectrumDoubleclicked(SelectorItem);


  void clicked(double x, double y, Qt::MouseButton button);
  void addMovingMarker(double x, double y);
  void removeMarkers();

  void on_pushFullInfo_clicked();

  void showAll();
  void hideAll();
  void randAll();

  void deleteSelected();
  void deleteShown();
  void deleteHidden();

  void analyse();

  void on_pushPerLive_clicked();
  void on_pushRescaleToThisMax_clicked();
  void on_pushRescaleReset_clicked();

  void effCalRequested(QAction*);

private:

  Ui::FormPlot1D *ui;

  Qpx::Calibration calib_;

  XMLableDB<Qpx::Detector> * detectors_;

  Qpx::Project *mySpectra;
  SelectorWidget *spectraSelector;

  QPlot::Marker1D moving, markx, marky;
  QMenu menuColors;
  QMenu menuDelete;
  QMenu menuEffCal;

  bool nonempty_{false};

};
