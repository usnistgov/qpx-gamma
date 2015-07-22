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
 *      WidgetPlotCalib - 
 *
 ******************************************************************************/

#include "widget_plot_calib.h"
#include "ui_widget_plot_calib.h"
#include "custom_logger.h"

WidgetPlotCalib::WidgetPlotCalib(QWidget *parent) :
  QWidget(parent),  ui(new Ui::WidgetPlotCalib)
{
  ui->setupUi(this);

  setColorScheme(Qt::black, Qt::white, QColor(112, 112, 112), QColor(170, 170, 170));

  ui->mcaPlot->setInteraction(QCP::iSelectItems, true);
  ui->mcaPlot->setInteraction(QCP::iMultiSelect, true);

  connect(ui->mcaPlot, SIGNAL(selectionChangedByUser()), this, SLOT(update_selection()));

  ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLinear);
  ui->mcaPlot->yAxis2->setScaleType(QCPAxis::stLinear);

  redraw();
}

WidgetPlotCalib::~WidgetPlotCalib()
{
  delete ui;
}

void WidgetPlotCalib::clearGraphs()
{
  ui->mcaPlot->clearGraphs();
  ui->mcaPlot->clearItems();
  ui->mcaPlot->replot();
}

void WidgetPlotCalib::redraw() {
  ui->mcaPlot->clearGraphs();
  ui->mcaPlot->clearItems();

  double xmin, xmax;
  double ymin, ymax;
  xmin = std::numeric_limits<double>::max();
  xmax = - std::numeric_limits<double>::max();
  ymin = std::numeric_limits<double>::max();
  ymax = - std::numeric_limits<double>::max();

  if (!x_pts.empty()) {
    ui->mcaPlot->addGraph();
    int g = ui->mcaPlot->graphCount() - 1;
    ui->mcaPlot->graph(g)->addData(x_pts, y_pts);
    ui->mcaPlot->graph(g)->setPen(QPen(Qt::darkCyan));
    ui->mcaPlot->graph(g)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDot, Qt::black, Qt::black, 0));
    ui->mcaPlot->graph(g)->setLineStyle(QCPGraph::lsNone);

    for (int i = 0; i < x_pts.size(); ++i) {
      QCPItemTracer *crs = new QCPItemTracer(ui->mcaPlot);
      crs->setStyle(QCPItemTracer::tsSquare); //tsCirlce?
      crs->setSize(style_pts.default_pen.width());
      crs->setGraph(ui->mcaPlot->graph(g));
      crs->setGraphKey(x_pts[i]);
      crs->setInterpolating(true);
      crs->setPen(QPen(style_pts.default_pen.color(), 0));
      crs->setBrush(style_pts.default_pen.color());
      crs->setSelectedPen(QPen(style_pts.themes["selected"].color(), 0));
      crs->setSelectedBrush(style_pts.themes["selected"].color());
      crs->setSelectable(true);
      crs->setProperty("true_value", x_pts[i]);
      if (selection_.count(x_pts[i]) > 0)
        crs->setSelected(true);
      ui->mcaPlot->addItem(crs);
      crs->updatePosition();
    }

    for (auto &q : x_pts) {
      if (q < xmin)
        xmin = q;
      if (q > xmax)
        xmax = q;
    }
    for (auto &q : y_pts) {
      if (q < ymin)
        ymin = q;
      if (q > ymax)
        ymax = q;
    }
  }

  ui->mcaPlot->rescaleAxes();

  xmax = ui->mcaPlot->xAxis->pixelToCoord(ui->mcaPlot->xAxis->coordToPixel(xmax) + 15);
  xmin = ui->mcaPlot->xAxis->pixelToCoord(ui->mcaPlot->xAxis->coordToPixel(xmin) - 15);

  if (!x_fit.empty()) {
    ui->mcaPlot->addGraph();
    int g = ui->mcaPlot->graphCount() - 1;
    ui->mcaPlot->graph(g)->addData(x_fit, y_fit);
    ui->mcaPlot->graph(g)->setPen(style_fit.default_pen);
    ui->mcaPlot->graph(g)->setLineStyle(QCPGraph::lsLine);
    ui->mcaPlot->graph(g)->setScatterStyle(QCPScatterStyle::ssNone);

    for (auto &q : x_fit) {
      if (q < xmin)
        xmin = q;
      if (q > xmax)
        xmax = q;
    }
    for (auto &q : y_fit) {
      if (q < ymin)
        ymin = q;
      if (q > ymax)
        ymax = q;
    }
  }

  ui->mcaPlot->rescaleAxes();
  
  ymax = ui->mcaPlot->yAxis->pixelToCoord(ui->mcaPlot->yAxis->coordToPixel(ymax) - 60);
  ymin = ui->mcaPlot->yAxis->pixelToCoord(ui->mcaPlot->yAxis->coordToPixel(ymin) + 15);

  if (!floating_text_.isEmpty()) {
    QCPItemText *floatingText = new QCPItemText(ui->mcaPlot);
    ui->mcaPlot->addItem(floatingText);
    floatingText->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
    floatingText->position->setType(QCPItemPosition::ptAxisRectRatio);
    floatingText->position->setCoords(0.5, 0); // place position at center/top of axis rect
    floatingText->setText(floating_text_);
    floatingText->setFont(QFont("Helvetica", 10));
    floatingText->setSelectable(false);
    floatingText->setColor(Qt::black);
  }

  if (this->isVisible()) {
    ui->mcaPlot->xAxis->setRange(xmin, xmax);
    ui->mcaPlot->yAxis->setRange(ymin, ymax);
  }

  ui->mcaPlot->replot();
}


void WidgetPlotCalib::setLabels(QString x, QString y) {
  ui->mcaPlot->xAxis->setLabel(x);
  ui->mcaPlot->yAxis->setLabel(y);
}

std::set<double> WidgetPlotCalib::get_selected_pts() {
  return selection_;
}

void WidgetPlotCalib::addFit(const QVector<double>& x, const QVector<double>& y, AppearanceProfile style) {
  style_fit = style;
  x_fit.clear();
  y_fit.clear();

  if (!x.empty() && (x.size() == y.size())) {
    x_fit = x;
    y_fit = y;
  }

  redraw();
}

void WidgetPlotCalib::addPoints(const QVector<double>& x, const QVector<double>& y, AppearanceProfile style) {
  style_pts = style;
  x_pts.clear();
  y_pts.clear();

  if (!x.empty() && (x.size() == y.size())) {
    x_pts = x;
    y_pts = y;
  }

  redraw();
}

void WidgetPlotCalib::set_selected_pts(std::set<double> sel) {
  selection_ = sel;
  redraw();
}

void WidgetPlotCalib::setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2) {
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

void WidgetPlotCalib::update_selection() {
  PL_INFO << "<WidgetPlotCalib> selection changed";
  selection_.clear();
  for (auto &q : ui->mcaPlot->selectedItems())
    if (QCPItemTracer *txt = qobject_cast<QCPItemTracer*>(q))
      selection_.insert(txt->property("true_value").toDouble());

  emit selection_changed();
}

void WidgetPlotCalib::setFloatingText(QString txt) {
  floating_text_ = txt;
  redraw();
}
