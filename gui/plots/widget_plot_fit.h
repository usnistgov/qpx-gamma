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

#include "widget_plot1d.h"

namespace Ui {
class WidgetPlotFit;
}

class WidgetPlotFit : public WidgetPlot1D
{
  Q_OBJECT

public:
  explicit WidgetPlotFit(QWidget *parent = 0);
  ~WidgetPlotFit() {}

  void clearGraphs();

  void addResid(const QVector<double>& x, const QVector<double>& y, AppearanceProfile appearance, bool fittable = false, int32_t bits = 0);

protected slots:
  void plot_rezoom();

protected:
  QCPGraph *resid;
  QCPAxisRect *residAxisRect;

  std::map<double, double> resid_extrema_;

};

#endif // WIDGET_PLOST1D_H
