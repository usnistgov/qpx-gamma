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
 *      WidgetPlotFit
 *
 ******************************************************************************/


#ifndef WIDGET_PLOT_FIT_H
#define WIDGET_PLOT_FIT_H

#include <QWidget>
#include "qsquarecustomplot.h"
#include "marker.h"
#include <set>
#include "gamma_fitter.h"

namespace Ui {
class WidgetPlotFit;
}

class WidgetPlotFit : public QWidget
{
  Q_OBJECT

public:
  explicit WidgetPlotFit(QWidget *parent = 0);
  ~WidgetPlotFit();
  virtual void clearGraphs();

  void clearSelection();

  void redraw();
  void reset_scales();

  void tight_x();
  void rescale();

  void setTitle(QString title);

  void set_scale_type(QString);
  QString scale_type();

  void select_peaks(const std::set<double> &peaks);
  std::set<double> get_selected_peaks();

  void set_range(Range);
  void setData(Gamma::Fitter* fit);
  void updateData();


public slots:
  void zoom_out();

signals:

  void clickedLeft(double);
  void clickedRight(double);
  void range_moved(double x, double y);
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
  std::set<double> selected_peaks_;

  Gamma::Fitter *fit_data_;

  void setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2);
  void calc_y_bounds(double lower, double upper);

  void build_menu();

  Ui::WidgetPlotFit *ui;

  Range my_range_;
  QCPItemTracer* edge_trc1;
  QCPItemTracer* edge_trc2;

  bool force_rezoom_;
  double minx, maxx;
  double miny, maxy;

  bool mouse_pressed_;

  double minx_zoom, maxx_zoom;

  std::map<double, double> minima_, maxima_;

  QMenu menuExportFormat;
  QMenu menuOptions;

  QString scale_type_;
  QString title_text_;

  void trim_log_lower(QVector<double> &array);
  void calc_visible();
  void add_bounds(const QVector<double>& x, const QVector<double>& y);
  void addGraph(const QVector<double>& x, const QVector<double>& y, QPen appearance,
                int fittable = 0, int32_t bits = 0,
                QString name = QString());
  void addEdge(Gamma::SUM4Edge edge, QPen pen);

  void follow_selection();
  void plotButtons();
  void plotEnergyLabels();
  void plotRange();

};

#endif // WIDGET_PLOST1D_H
