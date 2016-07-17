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
 *      QSquareCustomPlot - modified custom plot that can outomatically
 *        maintain square shape and signals mouse events with coordinates
 *
 ******************************************************************************/

#include "qsquarecustomplot.h"
#include "custom_logger.h"
#include "qcp_overlay_button.h"

void QSquareCustomPlot::setAlwaysSquare(bool sq) {
  square = sq;
  updateGeometry();
}

QSize QSquareCustomPlot::sizeHint() const {
  if (square) {
    QSize s = size();
    lastHeight = s.height();

    int extra_width = 0, extra_height = 0;
    for (int i=0; i < layerCount(); i++) {
      QCPLayer *this_layer = layer(i);
      for (auto &q : this_layer->children()) {
        if (QCPColorScale *le = qobject_cast<QCPColorScale*>(q)) {
          QRect ler = le->outerRect();
          //extra_width += ler.width();
          /*} else if (QCPAxis *le = qobject_cast<QCPAxis*>(q)) {
          if (le->axisType() == QCPAxis::atBottom)
            extra_height += le->axisRect()->height();
          else if (le->axisType() == QCPAxis::atLeft)
            extra_width += le->axisRect()->width();*/
        } else if (QCPAxisRect *le = qobject_cast<QCPAxisRect*>(q)) {
          QMargins mar = le->margins();
          extra_width += (mar.left() + mar.right());
          extra_height += (mar.top() + mar.bottom());
        }
      }
    }

    s.setWidth(s.height() - extra_height + extra_width);
    s.setHeight(QCustomPlot::sizeHint().height());
    return s;
  } else
    return QCustomPlot::sizeHint();
}

void QSquareCustomPlot::resizeEvent(QResizeEvent * event) {
  QCustomPlot::resizeEvent(event);

  if (square && (lastHeight!=height())) {
    updateGeometry(); // maybe this call should be scheduled for next iteration of event loop
  }
}

void QSquareCustomPlot::mousePressEvent(QMouseEvent *event)  {
  emit mousePress(event);

  DraggableTracer *trc = qobject_cast<DraggableTracer*>(itemAt(event->localPos(), true));
  if ((event->button() == Qt::LeftButton) && (trc != nullptr)) {
    trc->startMoving(event->localPos());
    return;
  }

  QCustomPlot::mousePressEvent(event);
}


void QSquareCustomPlot::mouseMoveEvent(QMouseEvent *event)  {
  emit mouseMove(event);
  double co_x, co_y;

  co_x = xAxis->pixelToCoord(event->x());
  co_y = yAxis->pixelToCoord(event->y());

  QVariant details;
  QCPLayerable *clickedLayerable = layerableAt(event->pos(), true, &details);
  if (QCPColorMap *ap = qobject_cast<QCPColorMap*>(clickedLayerable)) {
    int x = co_x, y = co_y;
    ap->data()->coordToCell(co_x, co_y, &x, &y);
    emit mouse_upon(static_cast<double>(x), static_cast<double>(y));
  } //else
  //emit mouse_upon(co_x, co_y);

  if (event->buttons() == Qt::NoButton) {
    DraggableTracer *trc = qobject_cast<DraggableTracer*>(itemAt(event->localPos(), false));
    QCPOverlayButton *button = qobject_cast<QCPOverlayButton*>(itemAt(event->localPos(), false));

    if (trc && trc->visible())
      setCursor(Qt::SizeHorCursor);
    else if (button && button->visible())
      setCursor(Qt::PointingHandCursor);
    else
      unsetCursor();
  }

  QCustomPlot::mouseMoveEvent(event);
}

void QSquareCustomPlot::mouseReleaseEvent(QMouseEvent *event)  {
  emit mouseRelease(event);

  //DBG << "QSquareCustomPlot::mouseReleaseEvent";

  if ((mMousePressPos-event->pos()).manhattanLength() < 5) {
    double co_x, co_y;

    co_x = xAxis->pixelToCoord(event->x());
    co_y = yAxis->pixelToCoord(event->y());

    //DBG << "Custom plot mouse released at coords: " << co_x << ", " << co_y;

    QCPAbstractItem *ai = qobject_cast<QCPAbstractItem*>(itemAt(event->localPos(), false));
    if (ai != nullptr) {
      emit clickedAbstractItem(ai);
//      DBG << "clicked abstract item";
    }

    QVariant details;
    QCPLayerable *clickedLayerable = layerableAt(event->pos(), false, &details);
    if (QCPColorMap *ap = qobject_cast<QCPColorMap*>(clickedLayerable)) {
      int xx, yy;
      ap->data()->coordToCell(co_x, co_y, &xx, &yy);
      //DBG << "Corrected to cell : " << xx << ", " << yy;
      emit mouse_clicked(static_cast<double>(xx), static_cast<double>(yy), event, true); //true?
    } else if (ai == nullptr) {
      emit mouse_clicked(co_x, co_y, event, false);
      //DBG << "Uncorrected click coords";
    }
  }
  QCustomPlot::mouseReleaseEvent(event);
}

void QSquareCustomPlot::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Shift) {
    emit shiftStateChanged(true);
  }
  QCustomPlot::keyPressEvent(event);
}

void QSquareCustomPlot::keyReleaseEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Shift) {
    emit shiftStateChanged(false);
  }
  QCustomPlot::keyReleaseEvent(event);
}

void QSquareCustomPlot::prepPlotExport(int plotThicken, int fontUpscale, int marginUpscale)
{
//  for (size_t i = 0; i < itemCount(); ++i) {
//    QCPAbstractItem* item = this->item(i);

//    if (QCPItemLine *line = qobject_cast<QCPItemLine*>(item))
//    {
//      QCPLineEnding head = line->head();
//      QPen pen = line->selectedPen();
//      head.setWidth(head.width() + fontUpscale);
//      head.setLength(head.length() + fontUpscale);
//      line->setHead(head);
//      line->setPen(pen);
//      line->start->setCoords(0, -50);
//      line->end->setCoords(0, -15);
//    }
//    else if (QCPItemText *txt = qobject_cast<QCPItemText*>(item))
//    {
//      QPen pen = txt->selectedPen();
//      txt->setPen(pen);
//      QFont font = txt->font();
//      font.setPointSize(font.pointSize() + fontUpscale);
//      txt->setFont(font);
//      txt->setColor(pen.color());
//      txt->position->setCoords(0, -50);
//      txt->setText(
//            txt->text() + " " + QString::fromStdString(fit_data_->settings().cali_nrg_.units_)
//            );
//    }
//  }

  for (int i = 0; i < graphCount(); ++i)
  {
    QCPGraph *graph = this->graph(i);
    QPen pen = graph->pen();
    pen.setWidth(pen.width() + plotThicken);
    graph->setPen(pen);
  }

  QFont xfont = xAxis->labelFont();
  QFont yfont = yAxis->labelFont();
  xfont.setPointSize(xfont.pointSize() + fontUpscale);
  yfont.setPointSize(yfont.pointSize() + fontUpscale);
  xAxis->setLabelFont(xfont);
  yAxis->setLabelFont(yfont);

  QFont xtickfont = xAxis->tickLabelFont();
  QFont ytickfont = yAxis->tickLabelFont();
  xtickfont.setPointSize(xtickfont.pointSize() + fontUpscale);
  ytickfont.setPointSize(ytickfont.pointSize() + fontUpscale);
  xAxis->setTickLabelFont(xtickfont);
  yAxis->setTickLabelFont(ytickfont);

  xAxis->setPadding(xAxis->padding() + marginUpscale);
  yAxis->setPadding(yAxis->padding() + marginUpscale);
}

void QSquareCustomPlot::postPlotExport(int plotThicken, int fontUpscale, int marginUpscale)
{
//  for (size_t i = 0; i < itemCount(); ++i) {
//    QCPAbstractItem* item = item(i);

//    if (QCPItemLine *line = qobject_cast<QCPItemLine*>(item))
//    {
//      QCPLineEnding head = line->head();
//      QPen pen = line->selectedPen();
//      head.setWidth(head.width() + fontUpscale);
//      head.setLength(head.length() + fontUpscale);
//      line->setHead(head);
//      line->setPen(pen);
//      line->start->setCoords(0, -50);
//      line->end->setCoords(0, -15);
//    }
//    else if (QCPItemText *txt = qobject_cast<QCPItemText*>(item))
//    {
//      QPen pen = txt->selectedPen();
//      txt->setPen(pen);
//      QFont font = txt->font();
//      font.setPointSize(font.pointSize() + fontUpscale);
//      txt->setFont(font);
//      txt->setColor(pen.color());
//      txt->position->setCoords(0, -50);
//      txt->setText(
//            txt->text() + " " + QString::fromStdString(fit_data_->settings().cali_nrg_.units_)
//            );
//    }
//  }

  for (int i = 0; i < graphCount(); ++i)
  {
    QCPGraph *graph = this->graph(i);
    QPen pen = graph->pen();
    pen.setWidth(pen.width() - plotThicken);
    graph->setPen(pen);
  }

  QFont xfont = xAxis->labelFont();
  QFont yfont = yAxis->labelFont();
  xfont.setPointSize(xfont.pointSize() - fontUpscale);
  yfont.setPointSize(yfont.pointSize() - fontUpscale);
  xAxis->setLabelFont(xfont);
  yAxis->setLabelFont(yfont);

  QFont xtickfont = xAxis->tickLabelFont();
  QFont ytickfont = yAxis->tickLabelFont();
  xtickfont.setPointSize(xtickfont.pointSize() - fontUpscale);
  ytickfont.setPointSize(ytickfont.pointSize() - fontUpscale);
  xAxis->setTickLabelFont(xtickfont);
  yAxis->setTickLabelFont(ytickfont);

  xAxis->setPadding(xAxis->padding() - marginUpscale);
  yAxis->setPadding(yAxis->padding() - marginUpscale);
}

