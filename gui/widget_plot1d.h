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
 *      WidgetPlot1D
 *
 ******************************************************************************/


#ifndef WIDGET_PLOT1D_H
#define WIDGET_PLOT1D_H

#include <QWidget>
#include "qsquarecustomplot.h"
#include "marker.h"

namespace Ui {
class WidgetPlot1D;
}

class WidgetPlot1D : public QWidget
{
  Q_OBJECT

public:
  explicit WidgetPlot1D(QWidget *parent = 0);
  ~WidgetPlot1D();
  void clearGraphs();
  void clearExtras();

  void redraw();
  void reset_scales();
  void rescale();
  void replot_markers();

  void setLabels(QString x, QString y);
  void setTitle(QString title);
  void setFloatingText(QString);

  void set_scale_type(QString);
  void set_plot_style(QString);
  void set_marker_labels(bool);
  QString scale_type();
  QString plot_style();
  bool marker_labels();

  void showButtonMarkerLabels(bool);
  void showButtonPlotStyle(bool);
  void showButtonScaleType(bool);
  void showButtonColorThemes(bool);
  void showTitle(bool);
  void setZoomable(bool);

  void set_markers(const std::list<Marker>&);
  void set_block(Marker, Marker);
  void set_cursors(const std::list<Marker>&);
  void set_edges(const Marker, const Marker);

  void addPoints(const QVector<double>& x, const QVector<double>& y, QColor color, int thickness, QCPScatterStyle::ScatterShape);
  void addGraph(const QVector<double>& x, const QVector<double>& y, QColor color, int thickness);
  void setYBounds(const std::map<double, double> &minima, const std::map<double, double> &maxima);

  void use_calibrated(bool);

signals:

  void clickedLeft(double);
  void clickedRight(double);

private slots:
  void plot_mouse_clicked(double x, double y, QMouseEvent *event, bool channels);
  void plot_mouse_press(QMouseEvent*);
  void plot_mouse_release(QMouseEvent*);
  void plot_rezoom();

  void on_pushResetScales_clicked();
  void scaleTypeChosen(QAction*);
  void plotStyleChosen(QAction*);

  void on_pushNight_clicked();

  void on_pushLabels_clicked();

private:
  void setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2);
  void calc_y_bounds(double lower, double upper);
  void set_graph_style(QCPGraph*, QString);

  Ui::WidgetPlot1D *ui;

  std::list<Marker> my_markers_;
  std::list<Marker> my_cursors_;
  std::vector<Marker> rect;
  std::vector<Marker> my_edges_;

  bool force_rezoom_;
  double minx, maxx;
  double miny, maxy;

  bool use_calibrated_;
  bool marker_labels_;

  double minx_zoom, maxx_zoom;

  std::map<double, double> minima_, maxima_;

  QMenu menuScaleType;
  QMenu menuPlotStyle;
  QString color_theme_;
  QString scale_type_;
  QString plot_style_;

  QString floating_text_;
};

#endif // WIDGET_PLOST1D_H
