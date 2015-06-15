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

  connect(ui->mcaPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*,bool)), this, SLOT(plot_mouse_clicked(double,double,QMouseEvent*,bool)));
  connect(ui->mcaPlot, SIGNAL(beforeReplot()), this, SLOT(plot_rezoom()));
  connect(ui->mcaPlot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(plot_mouse_release(QMouseEvent*)));
  connect(ui->mcaPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(plot_mouse_press(QMouseEvent*)));

  minx = 0; maxx = 0;
  minx_zoom = 0; maxx_zoom = 0;
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::max();

  force_rezoom_ = false;

  use_calibrated_ = false;

  marker_labels_ = false;

  menuPlotStyle.addAction("Scatter");
  menuPlotStyle.addAction("Step");
  menuPlotStyle.addAction("Lines");
  menuPlotStyle.addAction("Fill");
  for (auto &q : menuPlotStyle.actions())
    q->setCheckable(true);
  ui->toolPlotStyle->setMenu(&menuPlotStyle);
  ui->toolPlotStyle->setPopupMode(QToolButton::InstantPopup);
  connect(ui->toolPlotStyle, SIGNAL(triggered(QAction*)), this, SLOT(plotStyleChosen(QAction*)));
  set_plot_style("Lines");

  ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  menuScaleType.addAction("Linear");
  menuScaleType.addAction("Logarithmic");
  for (auto &q : menuScaleType.actions())
    q->setCheckable(true);
  ui->toolScaleType->setMenu(&menuScaleType);
  ui->toolScaleType->setPopupMode(QToolButton::InstantPopup);
  connect(ui->toolScaleType, SIGNAL(triggered(QAction*)), this, SLOT(scaleTypeChosen(QAction*)));
  set_scale_type("Logarithmic");

  redraw();
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
  use_calibrated_ = false;
}

void WidgetPlot1D::clearExtras()
{
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

void WidgetPlot1D::use_calibrated(bool uc) {
  use_calibrated_ = uc;
  replot_markers();
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
  set_graph_style(ui->mcaPlot->graph(g), plot_style_);

  if (x[0] < minx) {
    minx = x[0];
    ui->mcaPlot->xAxis->rescale();
  }
  if (x[x.size() - 1] > maxx) {
    maxx = x[x.size() - 1];
    ui->mcaPlot->xAxis->rescale();
  }
}

void WidgetPlot1D::addPoints(const QVector<double>& x, const QVector<double>& y, QColor color, int thickness, QCPScatterStyle::ScatterShape shape) {
  if (x.empty() || y.empty() || (x.size() != y.size()))
    return;

  ui->mcaPlot->addGraph();
  int g = ui->mcaPlot->graphCount() - 1;
  ui->mcaPlot->graph(g)->addData(x, y);
  QPen thispen = QPen(color);
  thispen.setWidth(thickness);
  ui->mcaPlot->graph(g)->setPen(thispen);
  ui->mcaPlot->graph(g)->setBrush(QBrush());
  ui->mcaPlot->graph(g)->setScatterStyle(QCPScatterStyle(shape, color, color, thickness));
  ui->mcaPlot->graph(g)->setLineStyle(QCPGraph::lsNone);

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

  maxy = ui->mcaPlot->yAxis->pixelToCoord(ui->mcaPlot->yAxis->coordToPixel(maxy) - 75);

  if ((maxy > 1) && (miny == 0))
    miny = 1;
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
      double pos = 0;
      if (use_calibrated_)
        pos = q.energy;
      else
        pos = q.channel;

      double max = std::numeric_limits<double>::lowest();
      int total = ui->mcaPlot->graphCount();
      for (int i=0; i < total; i++) {
        if ((ui->mcaPlot->graph(i)->data()->firstKey() <= pos)
            && (pos <= ui->mcaPlot->graph(i)->data()->lastKey()))
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
          crs->setGraphKey(pos);
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
      QPen qpen;
      if (q.themes.count(color_theme_))
        qpen = q.themes[color_theme_];
      else
        qpen = q.default_pen;
      QCPItemLine *line = new QCPItemLine(ui->mcaPlot);
      line->start->setParentAnchor(top_crs->position);
      line->start->setCoords(0, -30);
      line->end->setParentAnchor(top_crs->position);
      line->end->setCoords(0, -2);
      line->setHead(QCPLineEnding(QCPLineEnding::esSpikeArrow, 7, 7));
      line->setPen(qpen);
      ui->mcaPlot->addItem(line);

      if (marker_labels_) {
        QCPItemText *markerText = new QCPItemText(ui->mcaPlot);
        markerText->position->setParentAnchor(top_crs->position);
        markerText->setPositionAlignment(Qt::AlignHCenter|Qt::AlignBottom);
        markerText->position->setCoords(0, -30);
        markerText->setText(QString::number(q.energy));
        markerText->setTextAlignment(Qt::AlignLeft);
        markerText->setFont(QFont("Helvetica", 9));
        markerText->setPen(qpen);
        markerText->setColor(qpen.color());
        markerText->setPadding(QMargins(1, 1, 1, 1));
        ui->mcaPlot->addItem(markerText);
      }
    }
  }

  for (auto &q : my_cursors_) {
    double pos = 0;
    if (use_calibrated_)
      pos = q.energy;
    else
      pos = q.channel;

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
      crs->setGraphKey(pos);
      crs->setInterpolating(true);
      ui->mcaPlot->addItem(crs);
    }
  }

  if ((rect.size() == 2) && (rect[0].visible) && !maxima_.empty() && !minima_.empty()){
    double upperc = maxima_.rbegin()->first;
    double lowerc = maxima_.begin()->first;

    calc_y_bounds(lowerc, upperc);

    double pos1 = 0, pos2 = 0;
    if (use_calibrated_) {
      pos1 = rect[0].energy;
      pos2 = rect[1].energy;
    } else {
      pos1 = rect[0].channel;
      pos1 = rect[1].channel;
    }


    QCPItemRect *cprect = new QCPItemRect(ui->mcaPlot);
    double x1 = pos1;
    double y1 = maxy;
    double x2 = pos2;
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
  if (!floating_text_.isEmpty()) {
    QCPItemText *floatingText = new QCPItemText(ui->mcaPlot);
    ui->mcaPlot->addItem(floatingText);
    floatingText->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
    floatingText->position->setType(QCPItemPosition::ptAxisRectRatio);
    floatingText->position->setCoords(0.5, 0); // place position at center/top of axis rect
    floatingText->setText(floating_text_);
    floatingText->setFont(QFont("Helvetica", 10));
    if (color_theme_ == "light")
      floatingText->setColor(Qt::black);
    else
      floatingText->setColor(Qt::white);
  }
}


void WidgetPlot1D::plot_mouse_clicked(double x, double y, QMouseEvent* event, bool channels) {
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
  connect(ui->mcaPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*,bool)), this, SLOT(plot_mouse_clicked(double,double,QMouseEvent*,bool)));
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
  set_plot_style(choice->text());
}


void WidgetPlot1D::scaleTypeChosen(QAction* choice) {
  set_scale_type(choice->text());
}

void WidgetPlot1D::set_scale_type(QString sct) {
  this->setCursor(Qt::WaitCursor);
  scale_type_ = sct;
  for (auto &q : menuScaleType.actions())
    q->setChecked(q->text() == sct);
  if (scale_type_ == "Linear") {
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLinear);
  } else if (scale_type() == "Logarithmic") {
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  }
  ui->mcaPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}

QString WidgetPlot1D::scale_type() {
  return scale_type_;
}

void WidgetPlot1D::set_plot_style(QString stl) {
  this->setCursor(Qt::WaitCursor);
  plot_style_ = stl;
  for (auto &q : menuPlotStyle.actions())
    q->setChecked(q->text() == stl);
  int total = ui->mcaPlot->graphCount();
  for (int i=0; i < total; i++) {
    if (ui->mcaPlot->graph(i)->scatterStyle().shape() == QCPScatterStyle::ssDisc)
      set_graph_style(ui->mcaPlot->graph(i), stl);
  }
  ui->mcaPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}

QString WidgetPlot1D::plot_style() {
  return plot_style_;
}

void WidgetPlot1D::set_graph_style(QCPGraph* graph, QString style) {
  if (style == "Fill") {
    graph->setBrush(QBrush(graph->pen().color()));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
  } else if (style == "Lines") {
    graph->setBrush(QBrush());
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
  } else if (style == "Step") {
    graph->setBrush(QBrush());
    graph->setLineStyle(QCPGraph::lsStepCenter);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
  } else /*if (stl == "Scatter") default */{
    graph->setBrush(QBrush());
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 1));
    graph->setLineStyle(QCPGraph::lsNone);
  }
}

void WidgetPlot1D::on_pushLabels_clicked()
{
  marker_labels_ = !marker_labels_;
  replot_markers();
  ui->mcaPlot->replot();
}

void WidgetPlot1D::set_marker_labels(bool sl)
{
  marker_labels_ = sl;
  ui->pushLabels->setChecked(sl);
  replot_markers();
  ui->mcaPlot->replot();
}

bool WidgetPlot1D::marker_labels() {
  return marker_labels_;
}

void WidgetPlot1D::showButtonMarkerLabels(bool v) {
  ui->pushLabels->setVisible(v);
}

void WidgetPlot1D::showButtonPlotStyle(bool v) {
  ui->toolPlotStyle->setVisible(v);
}

void WidgetPlot1D::showButtonScaleType(bool v) {
  ui->toolScaleType->setVisible(v);
}

void WidgetPlot1D::showButtonColorThemes(bool v) {
  ui->pushNight->setVisible(v);
}

void WidgetPlot1D::showTitle(bool v) {
  ui->labelTitle->setVisible(v);
}

void WidgetPlot1D::setZoomable(bool v) {
  ui->pushResetScales->setVisible(v);
  ui->mcaPlot->setInteraction(QCP::iRangeDrag, v);
  ui->mcaPlot->setInteraction(QCP::iRangeZoom, v);
}

void WidgetPlot1D::setFloatingText(QString txt) {
  floating_text_ = txt;
  replot_markers();
  ui->mcaPlot->replot();
}
