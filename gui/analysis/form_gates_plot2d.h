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
 *
 ******************************************************************************/

#ifndef FORM_GATES_PLOT2D_H
#define FORM_GATES_PLOT2D_H

#include <QWidget>
#include <QSettings>
#include <project.h>
#include "qp_2d.h"
#include "qtcolorpicker.h"
#include "widget_selector.h"
#include "peak2d.h"
#include <unordered_map>

//Make it derived from plot2d!!! no ui metacode

namespace Ui
{
  class FormGatesPlot2D;
}

class FormGatesPlot2D : public QWidget
{
  Q_OBJECT

public:
  explicit FormGatesPlot2D(QWidget *parent = 0);
  ~FormGatesPlot2D();

  void setSpectra(Qpx::ProjectPtr new_set, int64_t idx);

  void update_plot();
  void refresh();
  void replot_markers();
  void reset_content();

  void set_boxes(std::list<Bounds2D> boxes);
  void set_show_boxes(bool);

  std::list<Bounds2D> get_selected_boxes();

  void loadSettings(QSettings& settings);
  void saveSettings(QSettings& settings);

signals:
  void stuff_selected();
  void clickedPlot(double x, double y);
  void clearSelection();

private slots:
  //void clicked_plottable(QCPAbstractPlottable*);
  void selection_changed();
  void plotClicked(double x, double y, Qt::MouseButton button);

private:

  Ui::FormGatesPlot2D *ui;

  Qpx::ProjectPtr project_;
  int64_t current_spectrum_ {0};

  //markers
  std::list<Bounds2D> boxes_;

  //scaling
  int bits {0};
  Qpx::Calibration calib_x_, calib_y_;
};

#endif
