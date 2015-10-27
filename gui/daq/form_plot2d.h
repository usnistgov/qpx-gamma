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

#ifndef FORM_PLOT2D_H
#define FORM_PLOT2D_H

#include <QWidget>
#include <spectra_set.h>
#include "qsquarecustomplot.h"
#include "qtcolorpicker.h"
#include "widget_selector.h"
#include "marker.h"
#include <unordered_map>

namespace Ui {
class FormPlot2D;
}

class FormPlot2D : public QWidget
{
  Q_OBJECT

public:
  explicit FormPlot2D(QWidget *parent = 0);
  ~FormPlot2D();

  void setSpectra(Qpx::SpectraSet& new_set, QString spectrum = QString());

  void updateUI();

  void update_plot(bool force = false);
  void refresh();
  void replot_markers();
  void reset_content();

  void set_scale_type(QString);
  void set_gradient(QString);
  void set_show_legend(bool);
  QString scale_type();
  QString gradient();
  bool show_legend();

  void set_zoom(double);
  double zoom();

  int gate_width();
  void set_gate_width(int);
  void set_gates_visible(bool vertical, bool horizontal, bool diagonal);
  void set_gates_movable(bool);

public slots:
  void set_marker(Marker n);
  void set_markers(Marker x, Marker y);

signals:
  void markers_set(Marker x, Marker y);
  void requestAnalysis(QString);
  void requestSymmetrize(QString);

private slots:
  void spectrumDetailsDelete();

  void on_pushDetails_clicked();
  void spectrumDetailsClosed(bool);

  void markers_moved(Marker x, Marker y);
  void analyse();

  void choose_spectrum(SelectorItem item);
  void crop_changed();
  void spectrumDoubleclicked(SelectorItem item);

  void on_pushSymmetrize_clicked();

private:

  //gui stuff
  Ui::FormPlot2D *ui;
  Qpx::SpectraSet *mySpectra;

  //plot identity
  QString name_2d;
  double zoom_2d, new_zoom;
  uint32_t adjrange;

  //markers
  Marker my_marker, //template(style)
    ext_marker, x_marker, y_marker; //actual data

  //scaling
  int bits;
  Gamma::Calibration calib_x_, calib_y_;

  QMenu *crop_menu_;
  QLabel *crop_label_;
  QSlider *crop_slider_;

};

#endif // WIDGET_PLOT2D_H
