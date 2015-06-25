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

  start->setCoords(0, -size);
  end->setCoords(0, 0);
  setHead(QCPLineEnding(QCPLineEnding::esFlatArrow, size, size));

  setSelectable(true); // plot moves only selectable points, see Plot::mouseMoveEvent

  move_timer_->setInterval(25); // 40 FPS
  connect(move_timer_, SIGNAL(timeout()), this, SLOT(moveToWantedPos()));
}

QPointF DraggableTracer::pos() const
{
  return center_tracer_->position->coords();
}

void DraggableTracer::startMoving(const QPointF &mousePos)
{
  PL_DBG << "started moving";

  connect(parentPlot(), SIGNAL(mouseMove(QMouseEvent*)),
          this, SLOT(onMouseMove(QMouseEvent*)));

  if (connect(parentPlot(), SIGNAL(mouseRelease(QMouseEvent*)),
          this, SLOT(stopMov(QMouseEvent*))))
    PL_DBG << "connected successfully";


  PL_DBG << "connected";

  grip_delta_.setX(parentPlot()->xAxis->coordToPixel(center_tracer_->position->key()) - mousePos.x());

  pos_initial_ = pos();
  pos_last_ = pos_initial_;
  pos_current_ = QPointF();

  move_timer_->start();

//  QApplication::setOverrideCursor(Qt::ClosedHandCursor);
}

void DraggableTracer::setVisible(bool on)
{
  setSelectable(on);  // movable only when visible
  QCPItemLine::setVisible(on);
}

void DraggableTracer::stopMov(QMouseEvent* evt)
{
  PL_DBG << "stopped moving";
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

void DraggableTracer::setActive(bool isActive)
{
  setSelected(isActive);
  emit (isActive ? activated() : disactivated());
}

void DraggableTracer::onMouseMove(QMouseEvent *event)
{
  pos_current_ = QPointF(event->localPos().x() + grip_delta_.x(),
                            event->localPos().y() + grip_delta_.y());
}

void DraggableTracer::moveToWantedPos()
{
  if (!pos_current_.isNull()) {
    movePx(pos_current_.x(), pos_current_.y());
    pos_current_ = QPointF();
  }
}
