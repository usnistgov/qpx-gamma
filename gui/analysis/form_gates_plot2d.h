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
#include <project.h>
#include "qsquarecustomplot.h"
#include "qtcolorpicker.h"
#include "widget_selector.h"
#include "marker.h"
#include <unordered_map>

namespace Ui {
class FormGatesPlot2D;
}

class FormGatesPlot2D : public QWidget
{
  Q_OBJECT

public:
  explicit FormGatesPlot2D(QWidget *parent = 0);
  ~FormGatesPlot2D();

  void setSpectra(Qpx::Project& new_set, int64_t idx);

  void update_plot();
  void refresh();
  void replot_markers();
  void reset_content();

  void set_boxes(std::list<MarkerBox2D> boxes);
  void set_show_boxes(bool);

  void set_scale_type(QString);
  void set_gradient(QString);
  void set_show_legend(bool);
  QString scale_type();
  QString gradient();
  bool show_legend();

  void set_range_x(MarkerBox2D);

  std::list<MarkerBox2D> get_selected_boxes();

  void set_gates_visible(bool vertical, bool horizontal, bool diagonal);
  void set_gates_movable(bool);

public slots:
  void set_marker(MarkerBox2D m);

signals:
  void marker_set(MarkerBox2D m);
  void stuff_selected();

private slots:
  //void clicked_plottable(QCPAbstractPlottable*);
  void selection_changed();
  void markers_moved(Marker x, Marker y);

private:

  //gui stuff
  Ui::FormGatesPlot2D *ui;
  Qpx::Project *mySpectra;

  //plot identity
  int64_t current_spectrum_;

  uint32_t adjrange;

  //markers
  MarkerBox2D my_marker_;
  bool gate_vertical_, gate_horizontal_, gate_diagonal_, gates_movable_, show_boxes_;

  std::list<MarkerBox2D> boxes_;
  MarkerBox2D range_;

  //scaling
  int bits;
  Qpx::Calibration calib_x_, calib_y_;
};

#endif // WIDGET_PLOT2D_H
