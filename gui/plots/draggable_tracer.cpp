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
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      DraggableTracer
 *
 ******************************************************************************/

#include "draggable_tracer.h"
#include "custom_logger.h"

DraggableTracer::DraggableTracer(QCustomPlot *parentPlot, QCPItemTracer *trc, int size)
  : QCPItemLine(parentPlot)
  , center_tracer_(trc)
  , grip_delta_()
  , pos_initial_()
  , pos_last_()
  , move_timer_(new QTimer(this))
  , pos_current_()
{
  start->setParentAnchor(center_tracer_->position);
  end->setParentAnchor(center_tracer_->position);

  start->setCoords(0, -(size + 1));
  end->setCoords(0, 0);
  setHead(QCPLineEnding(QCPLineEnding::esFlatArrow, size, size));

  setSelectable(true); // plot moves only selectable points, see Plot::mouseMoveEvent

  limit_l = parentPlot->xAxis->coordToPixel(center_tracer_->position->key() - 1);
  limit_l = parentPlot->xAxis->coordToPixel(center_tracer_->position->key() + 1);

  move_timer_->setInterval(25); // 40 FPS
  connect(move_timer_, SIGNAL(timeout()), this, SLOT(moveToWantedPos()));
}

void DraggableTracer::set_limits(double l , double r) {
  limit_l = parentPlot()->xAxis->coordToPixel(l);
  limit_r = parentPlot()->xAxis->coordToPixel(r);
}


void DraggableTracer::startMoving(const QPointF &mousePos)
{
//  DBG << "started moving";

  connect(parentPlot(), SIGNAL(mouseMove(QMouseEvent*)),
          this, SLOT(onMouseMove(QMouseEvent*)));

  if (connect(parentPlot(), SIGNAL(mouseRelease(QMouseEvent*)),
          this, SLOT(stopMov(QMouseEvent*))))
//    DBG << "connected successfully";


//  DBG << "connected";

  grip_delta_.setX(parentPlot()->xAxis->coordToPixel(center_tracer_->position->key()) - mousePos.x());

  pos_initial_ = center_tracer_->position->coords();
  pos_last_ = pos_initial_;
  pos_current_ = QPointF();

  move_timer_->start();

//  QApplication::setOverrideCursor(Qt::ClosedHandCursor);
}

void DraggableTracer::stopMov(QMouseEvent* evt)
{
//  DBG << "stopped moving";
  disconnect(parentPlot(), SIGNAL(mouseMove(QMouseEvent*)),
             this, SLOT(onMouseMove(QMouseEvent*)));

  disconnect(parentPlot(), SIGNAL(mouseRelease(QMouseEvent*)),
             this, SLOT(stopMov(QMouseEvent*)));

  move_timer_->stop();
  moveToWantedPos();

//  QApplication::restoreOverrideCursor();

  emit stoppedMoving();
}

void DraggableTracer::move(double x, double y, bool signalNeeded)
{
  pos_last_.setX(x);
  center_tracer_->setGraphKey(x);
  center_tracer_->updatePosition();
  parentPlot()->replot();

  if(signalNeeded) {
    emit moved(QPointF(x, y));
  }
}

void DraggableTracer::movePx(double x, double y)
{
  move(parentPlot()->xAxis->pixelToCoord(x),
       parentPlot()->yAxis->pixelToCoord(y));
}

void DraggableTracer::onMouseMove(QMouseEvent *event)
{
  pos_current_ = QPointF(event->localPos().x() + grip_delta_.x(),
                            event->localPos().y() + grip_delta_.y());
}

void DraggableTracer::moveToWantedPos()
{
  if (!pos_current_.isNull()) {
    double xx = pos_current_.x();
    if (xx <= limit_l)
      xx = limit_l + 1;
    if (xx >= limit_r)
      xx = limit_r - 1;
    movePx(xx, pos_current_.y());
    pos_current_ = QPointF();
  }
}
