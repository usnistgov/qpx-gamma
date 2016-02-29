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
 * Description:
 *      QpxPattern - coincidence pattern with drawing and editing facilities
 *      QpxPatternEditor - widget that represents coincidence pattern
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include <QtWidgets>
#include <math.h>
#include <limits>

#include "widget_pattern.h"
#include "custom_logger.h"


void QpxPatternEditor::set_pattern(Qpx::Pattern pattern, double size, int wrap)
{
  threshold_ = pattern.threshold();
  pattern_.clear();
  for (auto k : pattern.gates())
    if (k)
      pattern_.push_back(1);
    else
      pattern_.push_back(0);
  size_ = size;
  if (wrap > pattern_.size())
    wrap = pattern_.size();
  wrap_ = wrap;
  if (wrap > 0)
    rows_ = (pattern_.size() / wrap_) + ((pattern_.size() % wrap_) > 0);
  else
    rows_ = 0;

  outer = QRectF(2, 2, size_ - 4, size_ - 4);
  inner = QRectF(4, 4, size_ - 8, size_ - 8);
}

Qpx::Pattern QpxPatternEditor::pattern() const {
  Qpx::Pattern pt;
  pt.set_gates(pattern_);
  pt.set_theshold(threshold_);
  return pt;
}

int QpxPatternEditor::flagAtPosition(int x, int y)
{
  if ((sizeHint().width() == 0) || (wrap_ <= 0))
    return -1;

  int flag_x = (x - 40) / ((sizeHint().width() - 40) / wrap_);
  int flag_y = y / (sizeHint().height() / rows_);
  int flag = flag_y * wrap_ + flag_x;

  if (flag < 0 || flag > pattern_.size() || (x < 40) || (flag_x >= wrap_) || (flag_y >= rows_))
    return -1;

  return flag;
}

void QpxPatternEditor::setFlag(int count) {
  if ((count > -1) && (count < pattern_.size())) {
    pattern_[count] = !pattern_[count];
  }
}

QSize QpxPatternEditor::sizeHint() const
{
  return QSize(wrap_ * size_ + 40, rows_ * size_);
}

void QpxPatternEditor::paint(QPainter *painter, const QRect &rect,
                       const QPalette &palette) const
{
  painter->save();

  size_t tally = 0;
  for (auto t : pattern_)
    if (t != 0)
      tally++;

  bool enabled = this->isEnabled();
  bool irrelevant = (threshold_ == 0);
  bool valid = (threshold_ <= tally);

  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setRenderHint(QPainter::TextAntialiasing, true);

  int flags = Qt::TextWordWrap | Qt::AlignVCenter;
  if (enabled)
    painter->setPen(palette.color(QPalette::Active, QPalette::Text));
  else
    painter->setPen(palette.color(QPalette::Disabled, QPalette::Text));
  painter->drawText(rect, flags, " " + QString::number(threshold_) + " of ");

  painter->setPen(Qt::NoPen);
  int yOffset = (rect.height() - (size_*rows_)) / 2;
  painter->translate(rect.x() + 40, rect.y() + yOffset);

  QColor on_color = enabled ? Qt::cyan : Qt::darkCyan;
  if (irrelevant)
    on_color = enabled ? Qt::green : Qt::darkGreen;
  if (!valid)
    on_color = enabled ? Qt::red : Qt::darkRed;
  QColor border = enabled ? Qt::black : Qt::darkGray;
  QColor text_color = enabled ? Qt::black : Qt::lightGray;

  for (int i = 0; i < rows_; ++i) {
    painter->translate(0, i * size_);
    for (int j = 0; j < wrap_; ++j) {
      int flag = i * wrap_ + j;

      if (flag < pattern_.size()) {

        painter->setPen(Qt::NoPen);
        painter->setBrush(border);
        painter->drawEllipse(outer);

        if (pattern_[flag])
          painter->setBrush(on_color);
        else
          painter->setBrush(palette.background());

        painter->drawEllipse(inner);


        painter->setPen(QPen(text_color, 1));
        painter->setFont(QFont("Helvetica", std::floor(8.0*(size_/25.0)), QFont::Bold));
        painter->drawText(outer, Qt::AlignCenter, QString::number(flag));
      }

      painter->translate(size_, 0.0);
    }
    painter->translate((-1)*size_*wrap_, 0.0);
  }

  painter->restore();
}


////Editor/////////////

QpxPatternEditor::QpxPatternEditor(QWidget *parent)
  : QWidget(parent)
{
  setMouseTracking(true);
  setAutoFillBackground(true);
  set_pattern();
}

void QpxPatternEditor::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  paint(&painter, rect(), this->palette());
}


void QpxPatternEditor::mouseReleaseEvent(QMouseEvent *event)
{
  int flag = flagAtPosition(event->x(), event->y());

  if (isEnabled() && (flag != -1)) {
    setFlag(flag);
    update();
  }
}

void QpxPatternEditor::wheelEvent(QWheelEvent *event)
{
  if (!event->angleDelta().isNull()) {
    if (event->angleDelta().y() > 0)
      threshold_++;
    else if (threshold_ > 0)
      threshold_--;
    if (threshold_ > pattern_.size())
      threshold_ = pattern_.size();
    update();
  }
  event->accept();
}


