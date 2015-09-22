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


QpxPattern::QpxPattern(QVector<int16_t> pattern, double size, bool tristate, int wrap)
{
  pattern_ = pattern;
  tristate_ = tristate;
  size_ = size;
  if (wrap > pattern.size())
    wrap = pattern.size();
  wrap_ = wrap;
  if (wrap > 0)
    rows_ = (pattern.size() / wrap_) + ((pattern.size() % wrap_) > 0);
  else
    rows_ = 0;

  outer = QRectF(2, 2, size_ - 4, size_ - 4);
  inner = QRectF(4, 4, size_ - 8, size_ - 8);
}

QpxPattern::QpxPattern(std::bitset<Qpx::kNumChans> pattern, double size, bool tristate, int wrap) {
  tristate_ = tristate;

  pattern_.resize(Qpx::kNumChans);

  int sum = 0;
  for (int i=0; i < Qpx::kNumChans; i++) {
    sum+=pattern[i];
    if (pattern[i])
      pattern_[i] = 1;
    else
      pattern_[i] = 0;
  }
  size_ = size;
  if (!wrap || (wrap>Qpx::kNumChans))
    wrap = Qpx::kNumChans;
  wrap_ = wrap;
  rows_ = (Qpx::kNumChans / wrap_) + ((Qpx::kNumChans % wrap_) > 0);

  outer = QRectF(2, 2, size_ - 4, size_ - 4);
  inner = QRectF(4, 4, size_ - 8, size_ - 8);
}

QSize QpxPattern::sizeHint() const
{
  return QSize(wrap_ * size_, rows_ * size_);
}

int QpxPattern::flagAtPosition(int x, int y)
{
  int flag_x = (x / (sizeHint().width() / wrap_));
  int flag_y = (y / (sizeHint().height() / rows_));
  int flag = flag_y * wrap_ + flag_x;

  if (flag < 0 || flag > pattern_.size() || (flag_x >= wrap_) || (flag_y >= rows_))
    return -1;

  return flag;
}

void QpxPattern::setFlag(int count) {
  if ((count > -1) && (count < pattern_.size())) {
    int new_value = (pattern_[count] + 1);
    if (new_value > 1)
      new_value = tristate_ ? -1 : 0;
    pattern_[count] = new_value;
  }
}

void QpxPattern::paint(QPainter *painter, const QRect &rect,
                       const QPalette &palette, bool enabled) const
{
  painter->save();

  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setRenderHint(QPainter::TextAntialiasing, true);
  painter->setPen(Qt::NoPen);

  int yOffset = (rect.height() - (size_*rows_)) / 2;
  painter->translate(rect.x(), rect.y() + yOffset);

  QColor on_color = tristate_ ? Qt::green : Qt::cyan;
  if (!enabled)
    on_color = tristate_ ? Qt::darkGreen : Qt::darkCyan;
  QColor border = enabled ? Qt::black : Qt::darkGray;
  QColor off_color = enabled ? Qt::red : Qt::darkRed;
  QColor text_color = enabled ? Qt::black : Qt::lightGray;

  for (int i = 0; i < rows_; ++i) {
    painter->translate(0, i * size_);
    for (int j = 0; j < wrap_; ++j) {
      int flag = i * wrap_ + j;

      if (flag < pattern_.size()) {

        painter->setPen(Qt::NoPen);
        painter->setBrush(border);
        painter->drawEllipse(outer);

        if (pattern_[flag] == 1)
          painter->setBrush(on_color);
        else if (pattern_[flag] == -1)
          painter->setBrush(off_color);
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
}

QSize QpxPatternEditor::sizeHint() const
{
  return myQpxPattern.sizeHint();
}

void QpxPatternEditor::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  myQpxPattern.paint(&painter, rect(), this->palette(), isEnabled());
}


void QpxPatternEditor::mouseReleaseEvent(QMouseEvent *event)
{
  int flag = myQpxPattern.flagAtPosition(event->x(), event->y());

  if (isEnabled() && (flag != -1)) {
    myQpxPattern.setFlag(flag);
    update();
  }

}

