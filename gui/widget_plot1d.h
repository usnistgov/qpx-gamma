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

struct Marker {
  double position;
  QMap<QString, QPen> themes;
  QPen default_pen;
  bool visible;

  Marker() : position(0), visible(false), default_pen(Qt::gray, 1, Qt::SolidLine) {}
};

namespace Ui {
class WidgetPlot1D;
}

class WidgetPlot1D : public QWidget
{
  Q_OBJECT

public:
  explicit WidgetPlot1D(QWidget *parent = 0);
  ~WidgetPlot1D();

  void update_plot();
  void reset_scales();
  void set_markers(const std::list<Marker>& markers);
  void set_block(Marker, Marker);
  void setLogScale(bool);

  void clearGraphs();
  void addGraph(const QVector<double>& x, const QVector<double>& y, QColor color, int thickness);
  void setLabels(QString x, QString y);
  void setTitle(QString title);

signals:

  void clickedLeft(double);
  void clickedRight(double);

private slots:
  void plot_mouse_clicked(double x, double y, QMouseEvent *event);

  void on_pushResetScales_clicked();
  void scaleTypeChosen(QAction*);
  void plotStyleChosen(QAction*);

  void on_pushNight_clicked();

  void replot_markers();


private:
  void setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2);

  Ui::WidgetPlot1D *ui;

  std::list<Marker> my_markers_;
  std::vector<Marker> rect;

  double minx, maxx;

  QMenu menuScaleType;
  QMenu menuPlotStyle;
  int plotStyle;
  QString color_theme_;
};

#endif // WIDGET_PLOST1D_H
