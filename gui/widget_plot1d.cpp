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

WidgetPlot1D::WidgetPlot1D(QWidget *parent) :
  QWidget(parent),  ui(new Ui::WidgetPlot1D)
{
  ui->setupUi(this);

  setColorScheme(Qt::black, Qt::white, QColor(112, 112, 112), QColor(170, 170, 170));
  color_theme_ = "light";

  ui->mcaPlot->setInteraction(QCP::iRangeDrag, true);
  //ui->mcaPlot->xAxis->axisRect()->setRangeDrag(Qt::Horizontal);
  ui->mcaPlot->yAxis->axisRect()->setRangeDrag(Qt::Horizontal);
  ui->mcaPlot->setInteraction(QCP::iRangeZoom, true);
  //ui->mcaPlot->xAxis->axisRect()->setRangeZoom(Qt::Horizontal);
  ui->mcaPlot->yAxis->axisRect()->setRangeZoom(Qt::Horizontal);
  connect(ui->mcaPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*)), this, SLOT(plot_mouse_clicked(double,double,QMouseEvent*)));


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

  update_plot();
}

WidgetPlot1D::~WidgetPlot1D()
{
  delete ui;
}

void WidgetPlot1D::setTitle(QString title) {
  ui->labelTitle->setText(title);
}

void WidgetPlot1D::on_pushResetScales_clicked()
{
  reset_scales();
}

void WidgetPlot1D::reset_scales()
{
  ui->mcaPlot->rescaleAxes();
  ui->mcaPlot->replot();
}

void WidgetPlot1D::clearGraphs()
{
  ui->mcaPlot->clearGraphs();
  //ui->mcaPlot->replot();
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
  ui->mcaPlot->yAxis->rescale();
  ui->mcaPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}

void WidgetPlot1D::addGraph(const QVector<double>& x, const QVector<double>& y, QColor color, int thickness) {
  ui->mcaPlot->addGraph();
  int g = ui->mcaPlot->graphCount() - 1;
  ui->mcaPlot->graph(g)->addData(x, y);
  ui->mcaPlot->graph(g)->setPen(QPen(color));

  if (plotStyle == 2)
    ui->mcaPlot->graph(g)->setBrush(QBrush(color));
  if (plotStyle) {
    ui->mcaPlot->graph(g)->setLineStyle(QCPGraph::lsLine);
    ui->mcaPlot->graph(g)->setScatterStyle(QCPScatterStyle::ssNone);
  } else {
    ui->mcaPlot->graph(g)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 1));
    ui->mcaPlot->graph(g)->setLineStyle(QCPGraph::lsNone);
  }
}

void WidgetPlot1D::setLabels(QString x, QString y) {
  ui->mcaPlot->xAxis->setLabel(x);
  ui->mcaPlot->yAxis->setLabel(y);
}

void WidgetPlot1D::update_plot() {

  ui->mcaPlot->yAxis->rescale();

  ui->mcaPlot->replot();
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

void WidgetPlot1D::plot_mouse_clicked(double x, double y, QMouseEvent* event) {
  if (event->button() == Qt::RightButton) {
    emit clickedRight(x);
  } else {
    emit clickedLeft(x);
  }
}

void WidgetPlot1D::set_markers(const std::list<Marker>& markers) {
  my_markers_ = markers;
  replot_markers();
}

void WidgetPlot1D::replot_markers() {
  ui->mcaPlot->clearItems();

  for (auto &q : my_markers_) {
    if (!q.visible)
      continue;
    QCPItemStraightLine *line = new QCPItemStraightLine(ui->mcaPlot);
    if (q.themes.count(color_theme_))
      line->setPen(q.themes[color_theme_]);
    else
      line->setPen(q.default_pen);
    line->point1->setCoords(q.position, 0);
    line->point2->setCoords(q.position, 1);
    ui->mcaPlot->addItem(line);
  }

  if ((rect.size() == 2) && (rect[0].visible)){
    QCPItemRect *cprect = new QCPItemRect(ui->mcaPlot);
    cprect->topLeft->setCoords(rect[0].position, (std::numeric_limits<double>::max() - 1));
    cprect->bottomRight->setCoords(rect[1].position, 0);
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
  ui->mcaPlot->replot();
}

void WidgetPlot1D::set_block(Marker a, Marker b) {
  rect.resize(2);
  rect[0] = a;
  rect[1] = b;
  replot_markers();
}

