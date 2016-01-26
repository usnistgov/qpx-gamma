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
 *      WidgetPlotFit -
 *
 ******************************************************************************/

#include "widget_plot_fit.h"
#include "ui_widget_plot1d.h"
#include "custom_logger.h"
#include "qt_util.h"

WidgetPlotFit::WidgetPlotFit(QWidget *parent) :
  WidgetPlot1D(parent)
{
  resid = nullptr;

//   // create bottom axis rect for volume bar chart:
   residAxisRect = new QCPAxisRect(ui->mcaPlot);
   ui->mcaPlot->plotLayout()->addElement(1, 0, residAxisRect);
   residAxisRect->setMaximumSize(QSize(QWIDGETSIZE_MAX, 150));
   residAxisRect->axis(QCPAxis::atBottom)->setLayer("axes");
   residAxisRect->axis(QCPAxis::atBottom)->grid()->setLayer("grid");
   residAxisRect->axis(QCPAxis::atBottom)->setTickLabels(false);
   residAxisRect->setRangeDrag(Qt::Horizontal);

   //   // bring bottom and main axis rect closer together:
   ui->mcaPlot->plotLayout()->setRowSpacing(0);
   residAxisRect->setAutoMargins(QCP::msLeft|QCP::msRight|QCP::msBottom);
   residAxisRect->setMargins(QMargins(0, 0, 0, 0));

//   // interconnect x axis ranges of main and bottom axis rects:
   connect(ui->mcaPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), residAxisRect->axis(QCPAxis::atBottom), SLOT(setRange(QCPRange)));
   connect(residAxisRect->axis(QCPAxis::atBottom), SIGNAL(rangeChanged(QCPRange)), ui->mcaPlot->xAxis, SLOT(setRange(QCPRange)));

//   // make axis rects' left side line up:
   QCPMarginGroup *group = new QCPMarginGroup(ui->mcaPlot);
   ui->mcaPlot->axisRect()->setMarginGroup(QCP::msLeft|QCP::msRight, group);
   residAxisRect->setMarginGroup(QCP::msLeft|QCP::msRight, group);
}

void WidgetPlotFit::clearGraphs()
{
  WidgetPlot1D::clearGraphs();
  resid = nullptr;
  resid_extrema_.clear();
}

void WidgetPlotFit::plot_rezoom() {
  WidgetPlot1D::plot_rezoom();

  if (mouse_pressed_)
    return;

  if (resid_extrema_.empty()) {
    residAxisRect->axis(QCPAxis::atLeft)->rescale();
    return;
  }

//  double upperc = ui->mcaPlot->xAxis->range().upper;
//  double lowerc = ui->mcaPlot->xAxis->range().lower;

//  if (!force_rezoom_ && (lowerc == minx_zoom) && (upperc == maxx_zoom))
//    return;

//  minx_zoom = lowerc;
//  maxx_zoom = upperc;
//  force_rezoom_ = false;

  double extr = 0;

  for (std::map<double, double>::const_iterator it = resid_extrema_.lower_bound(minx_zoom); it != resid_extrema_.upper_bound(maxx_zoom); ++it)
    if (!std::isinf(it->second) && (it->second > extr))
      extr = it->second;

  //PL_DBG << "Rezoom";
  residAxisRect->axis(QCPAxis::atLeft)->setRangeLower(-extr*1.2);
  residAxisRect->axis(QCPAxis::atLeft)->setRangeUpper(extr*1.2);
}


void WidgetPlotFit::addResid(const QVector<double>& x, const QVector<double>& y, AppearanceProfile appearance, bool fittable, int32_t bits) {
  if (resid == nullptr) {
    resid = new QCPGraph(residAxisRect->axis(QCPAxis::atBottom), residAxisRect->axis(QCPAxis::atLeft));
    ui->mcaPlot->addPlottable(resid);
  }

  resid->setName("resid");
  resid->clearData();

  if (x.empty() || y.empty() || (x.size() != y.size()))
    return;


  resid->addData(x, y);
  QPen pen = appearance.get_pen(color_theme_);
//  if (fittable && (visible_options_ & ShowOptions::thickness))
//    pen.setWidth(thickness_);
  resid->setPen(pen);
  //resid->setProperty("fittable", fittable);
  resid->setProperty("bits", QVariant::fromValue(bits));
  //resid->setBrush(QBrush(pen.color(),Qt::SolidPattern));
  resid->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 4));
  resid->setLineStyle(QCPGraph::lsNone);

  double extr = 0;
  resid_extrema_.clear();
  for (int i=0; i < x.size(); ++i) {
    double extremum = std::abs(y[i]);
    resid_extrema_[x[i]] = extremum;
    if (!std::isinf(extremum))
      extr = qMax(extr, extremum);
  }

  plot_rezoom();

//  PL_DBG << "extremum " << extr;

//  residAxisRect->axis(QCPAxis::atLeft)->setRangeLower(-(extr*1.2));
//  residAxisRect->axis(QCPAxis::atLeft)->setRangeUpper(extr*1.2);

//  if (x[0] < minx) {
//    minx = x[0];
//    //PL_DBG << "new minx " << minx;
//    ui->mcaPlot->xAxis->rescale();
//  }
//  if (x[x.size() - 1] > maxx) {
//    maxx = x[x.size() - 1];
//    ui->mcaPlot->xAxis->rescale();
//  }
}

