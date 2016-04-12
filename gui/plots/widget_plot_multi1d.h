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
 *      WidgetPlotMulti1D
 *
 ******************************************************************************/


#ifndef WIDGET_PLOT_MULTI1D_H
#define WIDGET_PLOT_MULTI1D_H

#include <QWidget>
#include "qsquarecustomplot.h"
#include "marker.h"
#include <set>

namespace Ui {
class WidgetPlotMulti1D;
}

class WidgetPlotMulti1D : public QWidget
{
  Q_OBJECT

public:
  explicit WidgetPlotMulti1D(QWidget *parent = 0);
  ~WidgetPlotMulti1D();
  virtual void clearGraphs();

  void clearExtras();

  void redraw();
  void reset_scales();

  void tight_x();
  void rescale();
  void replot_markers();

  void setLabels(QString x, QString y);
  void setTitle(QString title);

  void set_scale_type(QString);
  void set_plot_style(QString);
  void set_grid_style(QString);
  void set_marker_labels(bool);
  void set_markers_selectable(bool);
  QString scale_type();
  QString plot_style();
  QString grid_style();
  bool marker_labels();

  void set_visible_options(ShowOptions);

  void set_markers(const std::list<Marker>&);
  void set_block(Marker, Marker);

//  void set_range(Range);
  std::set<double> get_selected_markers();

  void addPoints(const QVector<double>& x, const QVector<double>& y, AppearanceProfile appearance, QCPScatterStyle::ScatterShape);
  void addGraph(const QVector<double>& x, const QVector<double>& y, AppearanceProfile appearance, bool fittable = false, int32_t bits = 0);
  void setYBounds(const std::map<double, double> &minima, const std::map<double, double> &maxima);

  void use_calibrated(bool);

public slots:
  void zoom_out();

signals:

  void clickedLeft(double);
  void clickedRight(double);
//  void range_moved(double x, double y);
  void markers_selected();

protected slots:
  void plot_mouse_clicked(double x, double y, QMouseEvent *event, bool channels);
  void plot_mouse_press(QMouseEvent*);
  void plot_mouse_release(QMouseEvent*);
  void clicked_plottable(QCPAbstractPlottable *);
  void clicked_item(QCPAbstractItem *);

  void selection_changed();
  virtual void plot_rezoom();
  void exportRequested(QAction*);
  void optionsChanged(QAction*);


protected:
  void setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2);
  void calc_y_bounds(double lower, double upper);
  void set_graph_style(QCPGraph*, QString);

  void build_menu();

  Ui::WidgetPlotMulti1D *ui;

  std::list<Marker> my_markers_;
  std::vector<Marker> rect;

//  Range my_range_;
  QCPItemTracer* edge_trc1;
  QCPItemTracer* edge_trc2;

  bool force_rezoom_;
  double minx, maxx;
  double miny, maxy;

  bool use_calibrated_;
  bool marker_labels_;
  bool mouse_pressed_;

  bool markers_selectable_;

  double minx_zoom, maxx_zoom;

  std::map<double, double> minima_, maxima_;

  QMenu menuExportFormat;

  QMenu       menuOptions;
  ShowOptions visible_options_;

  QString color_theme_;
  QString scale_type_;
  QString plot_style_;
  QString grid_style_;
  int thickness_;

  QString title_text_;
};

#endif // WIDGET_PLOST1D_H
