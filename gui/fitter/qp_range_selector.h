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
 *      QpFitter -
 *
 ******************************************************************************/


#ifndef RANGE_SELECTOR_H
#define RANGE_SELECTOR_H

#include "qp_multi1d.h"

class RangeSelector : public QObject
{
  Q_OBJECT

public:
  explicit RangeSelector(QPlot::Multi1D* parent_plot)
    : QObject(parent_plot)
    , parent_plot_(parent_plot)
  {}

  void set_bounds(const double& left, const double& right);

  double left() const { return left_; }
  double right() const { return right_; }

  void plot_self(QPlot::Button *);
  void plot_self(QCPGraph* target, QPlot::Button *);
  void unplot_self();

public:
  bool visible {false};
  QPlot::Appearance appearance;
  QStringList latch_to;

private slots:
  void moved_L(QPointF);
  void moved_R(QPointF);

private:
  QPlot::Multi1D* parent_plot_;

  double left_ {0};
  double right_ {0};
  QSet<QCPAbstractItem*> children_;

  QPlot::Draggable* make_draggable(QCPGraph* target, double position);
  QCPGraph* find_target_graph();
};


#endif
