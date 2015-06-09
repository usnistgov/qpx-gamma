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

namespace Ui {
class FormPlot2D;
}

class FormPlot2D : public QWidget
{
  Q_OBJECT

public:
  explicit FormPlot2D(QWidget *parent = 0);
  ~FormPlot2D();

  void setSpectra(Pixie::SpectraSet& new_set);

  void update_plot();
  void replot_markers();
  void reset_content();

  void set_scale_type(QString);
  void set_gradient(QString);
  void set_zoom(double);
  void set_show_legend(bool);
  QString scale_type();
  QString gradient();
  double zoom();
  bool show_legend();

public slots:
  void set_marker(Marker n);

signals:
  void markers_set(Marker x, Marker y);

private slots:
  void gradientChosen(QAction*);
  void scaleTypeChosen(QAction*);

  void on_comboChose2d_activated(const QString &arg1);

  void plot_2d_mouse_upon(double x, double y);
  void plot_2d_mouse_clicked(double x, double y, QMouseEvent* event, bool channels);

  void on_pushResetScales_clicked();

  void on_pushColorScale_clicked();

  void on_pushDetails_clicked();
  void spectrumDetailsClosed(bool);

  void on_sliderZoom2d_valueChanged(int value);

private:

  //gui stuff
  Ui::FormPlot2D *ui;
  Pixie::SpectraSet *mySpectra;
  QCPColorMap *colorMap;

  std::map<QString, QCPColorGradient> gradients_;
  QMenu gradientMenu;
  QString current_gradient_;

  std::map<std::string, QCPAxis::ScaleType> scale_types_;
  QMenu scaleTypeMenu;
  std::string current_scale_type_;

  //plot identity
  QString name_2d;
  double zoom_2d;

  //markers
  Marker my_marker, //template(style)
    ext_marker, x_marker, y_marker; //actual data

  //scaling
  int bits;
  Pixie::Calibration calib_x_, calib_y_;

  void calibrate_markers();
  void make_marker(Marker&);
};

#endif // WIDGET_PLOT2D_H
