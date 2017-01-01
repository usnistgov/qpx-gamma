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

#include "qp_range_selector.h"
#include "custom_logger.h"

void RangeSelector::set_bounds(const double &left, const double &right)
{
  left_ = left;
  right_ = right;
  if (left_ > right_)
    std::swap(left_, right_);
}

void RangeSelector::replot()
{
  unplot_self();

  if (!visible_ || (left_ == right_))
    return;

  QCPGraph* target_graph = find_target_graph();

  if (!target_graph)
    return;

  plot_self(target_graph);
}

QCPGraph* RangeSelector::find_target_graph()
{
  for (auto &q : latch_to)
    for (int i=0; i < parent_plot_->graphCount(); i++)
    {
      auto g = parent_plot_->graph(i);
      if ((g->name() == q) &&
          (g->property("minimum").toDouble() < left_) &&
          (right_ < g->property("maximum").toDouble()))
        return g;
    }
  return nullptr;
}


void RangeSelector::plot_self(QCPGraph* target)
{
  QPlot::Draggable *draggable_L = make_draggable(target, left_);
  connect(draggable_L, SIGNAL(moved(QPointF)), this, SLOT(moved_L(QPointF)));
  connect(draggable_L, SIGNAL(stoppedMoving()), this, SLOT(stopped_moving()));
  QPlot::Draggable *draggable_R = make_draggable(target, right_);
  connect(draggable_R, SIGNAL(moved(QPointF)), this, SLOT(moved_R(QPointF)));
  connect(draggable_R, SIGNAL(stoppedMoving()), this, SLOT(stopped_moving()));

  QCPItemLine *line = new QCPItemLine(target->parentPlot());
  line->setSelectable(false);
  line->setProperty("tracer", QVariant::fromValue(0));
  line->start->setParentAnchor(draggable_L->tracer()->position);
  line->start->setCoords(0, 0);
  line->end->setParentAnchor(draggable_R->tracer()->position);
  line->end->setCoords(0, 0);
  line->setPen(pen);
  line->setSelectedPen(pen);
  children_.insert(line);

  QPlot::Button *button =  new QPlot::Button(parent_plot_,
                                             button_icon_,
                                             purpose_, tooltip_,
                                             Qt::AlignTop | Qt::AlignLeft);

  QCPItemTracer* higher = draggable_L->tracer();
  //make this optional?
  //  if (draggable_R->tracer()->position->value() > draggable_L->tracer()->position->value())
  //    higher = draggable_R->tracer();
  button->bottomRight->setParentAnchor(higher->position);
  button->bottomRight->setCoords(11, -20);
  children_.insert(button);
}

void RangeSelector::stopped_moving()
{
  emit stoppedMoving();
}


QPlot::Draggable* RangeSelector::make_draggable(QCPGraph* target, double position)
{
  QCPItemTracer* tracer = new QCPItemTracer(target->parentPlot());
  tracer->setStyle(QCPItemTracer::tsNone);
  tracer->setGraph(target);
  tracer->setGraphKey(position);
  tracer->setInterpolating(true);
  tracer->setProperty("tracer", QVariant::fromValue(0));
  tracer->updatePosition();
  children_.insert(tracer);

  QPlot::Draggable *draggable = new QPlot::Draggable(target->parentPlot(), tracer, 12);
  draggable->setPen(QPen(pen.color(), 1));
  draggable->setSelectedPen(QPen(pen.color(), 1));
  draggable->setProperty("tracer", QVariant::fromValue(0));
  draggable->setSelectable(true);
  children_.insert(draggable);

  return draggable;
}

void RangeSelector::unplot_self()
{
  for (QCPAbstractItem* c : children_)
    if (parent_plot_->containsItem(c))
      parent_plot_->removeItem(c);
  children_.clear();
}

void RangeSelector::moved_L(QPointF p)
{
  left_ = p.x();
  if (left_ > right_)
    std::swap(left_, right_);
}

void RangeSelector::moved_R(QPointF p)
{
  right_ = p.x();
  if (left_ > right_)
    std::swap(left_, right_);
}

void RangeSelector::set_visible(bool vis)
{
  visible_ = vis;
  replot();
}

void RangeSelector::clear()
{
  left_ = 0;
  right_ = 0;
  purpose_.clear();
  clearProperties();
  set_visible(false);
}

void RangeSelector::clearProperties()
{
  for (auto p : dynamicPropertyNames())
    setProperty(p, QVariant());
}




