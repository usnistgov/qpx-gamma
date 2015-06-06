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
 *      WidgetPlot1D -
 *
 ******************************************************************************/

#include "widget_plot1d.h"
#include "ui_widget_plot1d.h"
#include "custom_logger.h"

WidgetPlot1D::WidgetPlot1D(QWidget *parent) :
  QWidget(parent),  ui(new Ui::WidgetPlot1D)
{
  ui->setupUi(this);

  setColorScheme(Qt::black, Qt::white, QColor(112, 112, 112), QColor(170, 170, 170));
  color_theme_ = "light";

  ui->mcaPlot->setInteraction(QCP::iRangeDrag, true);
  ui->mcaPlot->yAxis->axisRect()->setRangeDrag(Qt::Horizontal);
  ui->mcaPlot->setInteraction(QCP::iRangeZoom, true);
  ui->mcaPlot->yAxis->axisRect()->setRangeZoom(Qt::Horizontal);
  connectPlot();

  minx = 0; maxx = 0;
  minx_zoom = 0; maxx_zoom = 0;
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::max();

  force_rezoom_ = false;

  plotStyle = 1;
  menuPlotStyle.addAction("Scatter");
  menuPlotStyle.addAction("Lines");
  menuPlotStyle.addAction("Fill");
  for (auto &q : menuPlotStyle.actions()) {
    q->setCheckable(true);
    if (q->text() == "Lines")
      q->setChecked(true);
  }
  ui->toolPlotStyle->setMenu(&menuPlotStyle);
  ui->toolPlotStyle->setPopupMode(QToolButton::InstantPopup);
  connect(ui->toolPlotStyle, SIGNAL(triggered(QAction*)), this, SLOT(plotStyleChosen(QAction*)));
  ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  menuScaleType.addAction("Linear");
  menuScaleType.addAction("Logarithmic");
  for (auto &q : menuScaleType.actions()) {
    q->setCheckable(true);
    if (q->text() == "Logarithmic")
      q->setChecked(true);
  }
  ui->toolScaleType->setMenu(&menuScaleType);
  ui->toolScaleType->setPopupMode(QToolButton::InstantPopup);
  connect(ui->toolScaleType, SIGNAL(triggered(QAction*)), this, SLOT(scaleTypeChosen(QAction*)));

  redraw();
}

void WidgetPlot1D::connectPlot()
{
  connect(ui->mcaPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*)), this, SLOT(plot_mouse_clicked(double,double,QMouseEvent*)));
  connect(ui->mcaPlot, SIGNAL(beforeReplot()), this, SLOT(plot_rezoom()));
  connect(ui->mcaPlot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(plot_mouse_release(QMouseEvent*)));
  connect(ui->mcaPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(plot_mouse_press(QMouseEvent*)));
}

WidgetPlot1D::~WidgetPlot1D()
{
  delete ui;
}

void WidgetPlot1D::clearGraphs()
{
  ui->mcaPlot->clearGraphs();
  minima_.clear();
  maxima_.clear();
}

void WidgetPlot1D::clearExtras()
{
  PL_DBG << "Clear extras clled";
  my_markers_.clear();
  my_cursors_.clear();
  rect.clear();
}

void WidgetPlot1D::rescale() {
  force_rezoom_ = true;
  plot_rezoom();
}

void WidgetPlot1D::redraw() {
  ui->mcaPlot->replot();
}

void WidgetPlot1D::reset_scales()
{
  minx = maxx = 0;
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::max();
  ui->mcaPlot->rescaleAxes();
}

void WidgetPlot1D::setTitle(QString title) {
  ui->labelTitle->setText(title);
}

void WidgetPlot1D::setLabels(QString x, QString y) {
  ui->mcaPlot->xAxis->setLabel(x);
  ui->mcaPlot->yAxis->setLabel(y);
}

void WidgetPlot1D::setLogScale(bool log) {
  QString action_txt;
  if (log)
    action_txt = "Logarithmic";
  else
    action_txt = "Linear";
  for (auto &q : menuScaleType.actions()) {
    if (q->text() == action_txt) {
      scaleTypeChosen(q);
    }
  }
}

void WidgetPlot1D::set_markers(const std::list<Marker>& markers) {
  my_markers_ = markers;
}

void WidgetPlot1D::set_block(Marker a, Marker b) {
  rect.resize(2);
  rect[0] = a;
  rect[1] = b;
}

void WidgetPlot1D::set_cursors(const std::list<Marker>& cursors) {
  my_cursors_ = cursors;
}

void WidgetPlot1D::setYBounds(const std::map<double, double> &minima, const std::map<double, double> &maxima) {
  minima_ = minima;
  maxima_ = maxima;
  rescale();
}


void WidgetPlot1D::addGraph(const QVector<double>& x, const QVector<double>& y, QColor color, int thickness) {
  if (x.empty() || y.empty() || (x.size() != y.size()))
    return;

  ui->mcaPlot->addGraph();
  int g = ui->mcaPlot->graphCount() - 1;
  ui->mcaPlot->graph(g)->addData(x, y);
  QPen thispen = QPen(color);
  thispen.setWidth(thickness);
  ui->mcaPlot->graph(g)->setPen(thispen);

  if (plotStyle == 2)
    ui->mcaPlot->graph(g)->setBrush(QBrush(color));
  if (plotStyle) {
    ui->mcaPlot->graph(g)->setLineStyle(QCPGraph::lsLine);
    ui->mcaPlot->graph(g)->setScatterStyle(QCPScatterStyle::ssNone);
  } else {
    ui->mcaPlot->graph(g)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, thickness));
    ui->mcaPlot->graph(g)->setLineStyle(QCPGraph::lsNone);
  }

  if (x[0] < minx) {
    minx = x[0];
    ui->mcaPlot->xAxis->rescale();
  }
  if (x[x.size() - 1] > maxx) {
    maxx = x[x.size() - 1];
    ui->mcaPlot->xAxis->rescale();
  }
}

void WidgetPlot1D::plot_rezoom() {
  if (minima_.empty() || maxima_.empty()) {
    ui->mcaPlot->yAxis->rescale();
    return;
  }

  double upperc = ui->mcaPlot->xAxis->range().upper;
  double lowerc = ui->mcaPlot->xAxis->range().lower;

  if (!force_rezoom_ && (lowerc == minx_zoom) && (upperc == maxx_zoom))
    return;

  minx_zoom = lowerc;
  maxx_zoom = upperc;
  force_rezoom_ = false;

  calc_y_bounds(lowerc, upperc);

  ui->mcaPlot->yAxis->setRange(miny, maxy);
}

void WidgetPlot1D::calc_y_bounds(double lower, double upper) {
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::max();

  for (std::map<double, double>::const_iterator it = minima_.lower_bound(lower); it != minima_.upper_bound(upper); ++it)
    if (it->second < miny)
      miny = it->second;

  for (std::map<double, double>::const_iterator it = maxima_.lower_bound(lower); it != maxima_.upper_bound(upper); ++it) {
    if (it->second > maxy)
      maxy = it->second;
  }

  maxy = ui->mcaPlot->yAxis->pixelToCoord(ui->mcaPlot->yAxis->coordToPixel(maxy) - 50);

  if ((maxy > 0.7) && (miny == 0))
    miny = 0.7;
}

void WidgetPlot1D::setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2) {
  ui->mcaPlot->xAxis->setBasePen(QPen(fore, 1));
  ui->mcaPlot->yAxis->setBasePen(QPen(fore, 1));
  ui->mcaPlot->xAxis->setTickPen(QPen(fore, 1));
  ui->mcaPlot->yAxis->setTickPen(QPen(fore, 1));
  ui->mcaPlot->xAxis->setSubTickPen(QPen(fore, 1));
  ui->mcaPlot->yAxis->setSubTickPen(QPen(fore, 1));
  ui->mcaPlot->xAxis->setTickLabelColor(fore);
  ui->mcaPlot->yAxis->setTickLabelColor(fore);
  ui->mcaPlot->xAxis->setLabelColor(fore);
  ui->mcaPlot->yAxis->setLabelColor(fore);
  ui->mcaPlot->xAxis->grid()->setPen(QPen(grid1, 1, Qt::DotLine));
  ui->mcaPlot->yAxis->grid()->setPen(QPen(grid1, 1, Qt::DotLine));
  ui->mcaPlot->xAxis->grid()->setSubGridPen(QPen(grid2, 1, Qt::DotLine));
  ui->mcaPlot->yAxis->grid()->setSubGridPen(QPen(grid2, 1, Qt::DotLine));
  ui->mcaPlot->xAxis->grid()->setSubGridVisible(true);
  ui->mcaPlot->yAxis->grid()->setSubGridVisible(true);
  ui->mcaPlot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
  ui->mcaPlot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
  ui->mcaPlot->setBackground(QBrush(back));
}

void WidgetPlot1D::on_pushNight_clicked()
{
  if (ui->pushNight->isChecked()) {
    setColorScheme(Qt::white, Qt::black, QColor(144, 144, 144), QColor(80, 80, 80));
    color_theme_ = "dark";
  } else {
    setColorScheme(Qt::black, Qt::white, QColor(112, 112, 112), QColor(170, 170, 170));
    color_theme_ = "light";
  }
  replot_markers();
  ui->mcaPlot->replot();
}

void WidgetPlot1D::replot_markers() {
  ui->mcaPlot->clearItems();

  for (auto &q : my_markers_) {
    QCPItemTracer *top_crs = nullptr;
    if (q.visible) {
      double max = std::numeric_limits<double>::lowest();
      int total = ui->mcaPlot->graphCount();
      for (int i=0; i < total; i++) {
        if ((ui->mcaPlot->graph(i)->data()->firstKey() <= q.position)
            && (q.position <= ui->mcaPlot->graph(i)->data()->lastKey()))
        {
          QPen thispen = ui->mcaPlot->graph(i)->pen();
          QColor thiscol = thispen.color();
          thiscol.setAlpha(255);
          thispen.setColor(thiscol);
          if (q.themes.count(color_theme_))
            thispen = QPen(q.themes[color_theme_]);

          QCPItemTracer *crs = new QCPItemTracer(ui->mcaPlot);
          crs->setStyle(QCPItemTracer::tsCircle);
          crs->setSize(4);
          crs->setGraph(ui->mcaPlot->graph(i));
          crs->setGraphKey(q.position);
          crs->setInterpolating(true);
          crs->setPen(thispen);
          ui->mcaPlot->addItem(crs);

          crs->updatePosition();
          double val = crs->positions().first()->value();
          if (val > max) {
            max = val;
            top_crs = crs;
          }
        }
      }
    }
    if (top_crs != nullptr) {
      QCPItemLine *line = new QCPItemLine(ui->mcaPlot);
      line->start->setParentAnchor(top_crs->position);
      line->start->setCoords(0, -27);
      line->end->setParentAnchor(top_crs->position);
      line->end->setCoords(0, -2);
      line->setHead(QCPLineEnding(QCPLineEnding::esSpikeArrow, 7, 7));
      if (q.themes.count(color_theme_))
        line->setPen(q.themes[color_theme_]);
      else
        line->setPen(q.default_pen);
      ui->mcaPlot->addItem(line);
    }
  }

  for (auto &q : my_cursors_) {
    if (!q.visible)
      continue;
    int total = ui->mcaPlot->graphCount();
    for (int i=0; i < total; i++) {
      QCPItemTracer *crs = new QCPItemTracer(ui->mcaPlot);
      if (q.themes.count(color_theme_))
        crs->setPen(q.themes[color_theme_]);
      else
        crs->setPen(q.default_pen);
      crs->setStyle(QCPItemTracer::tsCircle);
      crs->setSize(4);
      crs->setGraph(ui->mcaPlot->graph(i));
      crs->setGraphKey(q.position);
      crs->setInterpolating(true);
      ui->mcaPlot->addItem(crs);
    }
  }

  if ((rect.size() == 2) && (rect[0].visible) && !maxima_.empty() && !minima_.empty()){
    double upperc = maxima_.rbegin()->first;
    double lowerc = maxima_.begin()->first;

    calc_y_bounds(lowerc, upperc);

    QCPItemRect *cprect = new QCPItemRect(ui->mcaPlot);
    double x1 = rect[0].position;
    double y1 = maxy;
    double x2 = rect[1].position;
    double y2 = miny;

    cprect->topLeft->setCoords(x1, y1);
    cprect->bottomRight->setCoords(x2, y2);
    if (rect[0].themes.count(color_theme_))
      cprect->setPen(rect[0].themes[color_theme_]);
    else
      cprect->setPen(rect[0].default_pen);
    if (rect[1].themes.count(color_theme_))
      cprect->setBrush(QBrush(rect[1].themes[color_theme_].color()));
    else
      cprect->setBrush(QBrush(rect[1].default_pen.color()));

    ui->mcaPlot->addItem(cprect);
  }
}


void WidgetPlot1D::plot_mouse_clicked(double x, double y, QMouseEvent* event) {
  if (event->button() == Qt::RightButton) {
    emit clickedRight(x);
  } else {
    emit clickedLeft(x);
  }
}

void WidgetPlot1D::plot_mouse_press(QMouseEvent*) {
  disconnect(ui->mcaPlot, 0, this, 0);
  connect(ui->mcaPlot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(plot_mouse_release(QMouseEvent*)));
  force_rezoom_ = false;
}

void WidgetPlot1D::plot_mouse_release(QMouseEvent*) {
  connect(ui->mcaPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*)), this, SLOT(plot_mouse_clicked(double,double,QMouseEvent*)));
  connect(ui->mcaPlot, SIGNAL(beforeReplot()), this, SLOT(plot_rezoom()));
  connect(ui->mcaPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(plot_mouse_press(QMouseEvent*)));
  force_rezoom_ = true;
  plot_rezoom();
  ui->mcaPlot->replot();
}

void WidgetPlot1D::on_pushResetScales_clicked()
{
  ui->mcaPlot->xAxis->rescale();
  force_rezoom_ = true;
  plot_rezoom();
  ui->mcaPlot->replot();
}

void WidgetPlot1D::plotStyleChosen(QAction* choice) {
  this->setCursor(Qt::WaitCursor);
  for (auto &q : menuPlotStyle.actions())
    q->setChecked(false);
  choice->setChecked(true);
  QString str = choice->text();
  if (str == "Scatter")
    plotStyle = 0;
  else if (str == "Lines")
    plotStyle = 1;
  else if (str == "Fill")
    plotStyle = 2;
  int total = ui->mcaPlot->graphCount();
  for (int i=0; i < total; i++) {
    QColor this_color = ui->mcaPlot->graph(i)->pen().color();
    if (plotStyle == 2) {
      ui->mcaPlot->graph(i)->setBrush(QBrush(this_color));
      ui->mcaPlot->graph(i)->setLineStyle(QCPGraph::lsLine);
      ui->mcaPlot->graph(i)->setScatterStyle(QCPScatterStyle::ssNone);
    } else if (plotStyle == 1) {
      ui->mcaPlot->graph(i)->setBrush(QBrush());
      ui->mcaPlot->graph(i)->setLineStyle(QCPGraph::lsLine);
      ui->mcaPlot->graph(i)->setScatterStyle(QCPScatterStyle::ssNone);
    } else {
      ui->mcaPlot->graph(i)->setBrush(QBrush());
      ui->mcaPlot->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 1));
      ui->mcaPlot->graph(i)->setLineStyle(QCPGraph::lsNone);
    }
  }
  ui->mcaPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}


void WidgetPlot1D::scaleTypeChosen(QAction* choice) {
  this->setCursor(Qt::WaitCursor);
  for (auto &q : menuScaleType.actions())
    q->setChecked(false);
  choice->setChecked(true);
  QString str = choice->text();
  if (str == "Linear") {
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLinear);
  } else if (str == "Logarithmic") {
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  }
  ui->mcaPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}
