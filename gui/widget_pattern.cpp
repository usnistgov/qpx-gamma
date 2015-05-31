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


QpxPattern::QpxPattern(QVector<int16_t> pattern, bool tristate)
{
  if (pattern.empty())
    pattern.resize(Pixie::kNumChans);
  pattern_ = pattern;
  tristate_ = tristate;

  outer = QRectF(0.1, 0.1, 0.8, 0.8);
  inner = QRectF(0.2, 0.2, 0.6, 0.6);
  PaintingScaleFactor = 20;
}

QpxPattern::QpxPattern(std::bitset<Pixie::kNumChans> pattern, bool tristate) {
    tristate_ = tristate;

    pattern_.resize(Pixie::kNumChans);

    int sum = 0;
    for (int i=0; i < Pixie::kNumChans; i++) {
        sum+=pattern[i];
        if (pattern[i])
            pattern_[i] = 1;
        else
            pattern_[i] = 0;
    }

    outer = QRectF(0.1, 0.1, 0.8, 0.8);
    inner = QRectF(0.2, 0.2, 0.6, 0.6);
    PaintingScaleFactor = 20;
}

QSize QpxPattern::sizeHint() const
{
    return PaintingScaleFactor * QSize(maxCount(), 1);
}

void QpxPattern::setFlag(int count) {
    if (count < maxCount()) {
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
    painter->setPen(Qt::NoPen);

    int yOffset = (rect.height() - PaintingScaleFactor) / 2;
    painter->translate(rect.x(), rect.y() + yOffset);
    painter->scale(PaintingScaleFactor, PaintingScaleFactor);

    QColor on_color = tristate_ ? Qt::green : Qt::blue;
    if (!enabled)
        on_color = tristate_ ? Qt::darkGreen : Qt::darkBlue;
    QColor border = enabled ? Qt::black : Qt::darkGray;
    QColor off_color = enabled ? Qt::red : Qt::darkRed;

    for (int i = 0; i < maxCount(); ++i) {
        painter->setBrush(border);
        painter->drawEllipse(outer);

        if (pattern_[i] == 1)
            painter->setBrush(on_color);
        else if (pattern_[i] == -1)
            painter->setBrush(off_color);
        else
            painter->setBrush(palette.background());

        painter->drawEllipse(inner);
        painter->translate(1.0, 0.0);
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
    int flag = flagAtPosition(event->x());

    if (isEnabled() && (flag != -1)) {
        myQpxPattern.setFlag(flag);
        update();
    }

}

int QpxPatternEditor::flagAtPosition(int x)
{
    int flag = (x / (myQpxPattern.sizeHint().width()
                     / myQpxPattern.maxCount()));
    if (flag < 0 || flag > myQpxPattern.maxCount())
        return -1;

    return flag;
}
