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


#ifndef WIDGET_PLOT_MULTI_1D_H
#define WIDGET_PLOT_MULTI_1D_H

#include "widget_plot1d.h"

namespace Ui {
class WidgetPlotMulti1D;
}

class WidgetPlotMulti1D : public WidgetPlot1D
{
  Q_OBJECT

public:
  explicit WidgetPlotMulti1D(QWidget *parent = 0);
  ~WidgetPlotMulti1D() {}

  void clearGraphs();

protected slots:
  void plot_rezoom();

protected:

};

#endif // WIDGET_PLOST1D_H
