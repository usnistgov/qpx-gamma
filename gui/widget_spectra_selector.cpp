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
 *      QpxSpectra - displays boxes for multiple spectra with name and color
 *      QpxSpectraWidget - widget that allows selection of spectra
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include <QtWidgets>
#include <math.h>

#include "widget_spectra_selector.h"
#include "custom_logger.h"

QpxSpectraWidget::QpxSpectraWidget(QWidget *parent)
  : QWidget(parent),
    all_spectra_(nullptr)
{
  setMouseTracking(true);
  setAutoFillBackground(true);
  max_wide = 3; //make this a parameter
  rect_w_ = 140;
  rect_h_ = 20;
  border = 3;
  selected_ = -1;

  inner = QRectF(1, 1, rect_w_-1, rect_h_-1);
  outer = QRectF(2, 2, rect_w_-2, rect_h_-2);

  setToolTipDuration(10000); //hardcoded to 10 secs. Make this a parameter?
}

void QpxSpectraWidget::setQpxSpectra(Pixie::SpectraSet &newset, int dim, int res) {
  all_spectra_ = &newset;
  my_spectra_.clear();

  for (auto &q : all_spectra_->spectra(dim, res)) {
    SpectrumAvatar new_spectrum;
    new_spectrum.name = QString::fromStdString(q->name());
    new_spectrum.color = QColor::fromRgba(q->appearance());
    new_spectrum.selected = q->visible();
    my_spectra_.push_back(new_spectrum);
  }

  height_total = my_spectra_.size() / max_wide;
  width_last = my_spectra_.size() % max_wide;
  if (width_last)
    height_total++;
  selected_ = -1;
  update();
}

void QpxSpectraWidget::update_looks() {
  if (all_spectra_ == nullptr)
    return;
  for (auto &q : my_spectra_) {
    Pixie::Spectrum::Spectrum *someSpectrum = all_spectra_->by_name(q.name.toStdString());
    if (someSpectrum != nullptr)
      q.color = QColor::fromRgba(someSpectrum->appearance());
  }
  update();
}

void QpxSpectraWidget::recalcDim(int w, int h) {
  max_wide = w / rect_w_;
  height_total = my_spectra_.size() / max_wide;
  width_last = my_spectra_.size() % max_wide;
  if (width_last)
    height_total++;
}

QSize QpxSpectraWidget::sizeHint() const
{
  return QSize(4*rect_w_, 4*rect_h_);
}

void QpxSpectraWidget::paintEvent(QPaintEvent *evt)
{
  QPainter painter(this);

  recalcDim(evt->rect().width(), evt->rect().height());

  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setPen(QPen(Qt::white));
  painter.setFont(QFont("Times", 10, QFont::Normal));

  for (int i=0; i < my_spectra_.size(); i++) {
    int cur_high = i / max_wide;
    int cur_wide = i % max_wide;
    painter.resetTransform();
    painter.translate(evt->rect().x() + cur_wide*rect_w_, evt->rect().y() + cur_high*rect_h_);

    if (my_spectra_[i].selected)
      painter.setBrush(my_spectra_[i].color);
    else
      painter.setBrush(Qt::black);
    painter.setPen(Qt::NoPen);
    painter.drawRect(inner);

    painter.setPen(QPen(Qt::white, 1));
    painter.drawText(inner, Qt::AlignLeft, my_spectra_[i].name);

    if (i == selected_) {
      painter.setBrush(Qt::NoBrush);
      painter.setPen(QPen(Qt::white, border));
      painter.drawRect(outer);
      painter.setPen(QPen(Qt::black, 1));
      painter.drawRect(outer);
    }
  }
}


void QpxSpectraWidget::mouseReleaseEvent(QMouseEvent *event)
{
  int flag = flagAt(event->x(), event->y());

  if (event->button()==Qt::LeftButton) {
    selected_ = flag;
    update();
    emit contextRequested();
  }

  if ((event->button()==Qt::RightButton) && (flag > -1) && (flag < my_spectra_.size())) {
    my_spectra_[flag].selected = !my_spectra_[flag].selected;
    all_spectra_->by_name(my_spectra_[flag].name.toStdString())->set_visible(my_spectra_[flag].selected);
    update();
    emit stateChanged();
  }
}


QString QpxSpectraWidget::selected() {
  if ((selected_ > -1) && (selected_ < my_spectra_.size()))
    return my_spectra_[selected_].name;
  else
    return "";
}

int QpxSpectraWidget::flagAt(int x, int y) {
  if (!my_spectra_.size())
    return -1;
  int phigh = y / rect_h_;
  int pwide = x / rect_w_;
  if ((phigh > height_total) || (pwide >= max_wide))
    return -1;
  if ((phigh == height_total) && (pwide > width_last))
    return -1;
  return (phigh * max_wide) + pwide;
}

void QpxSpectraWidget::show_all() {
  for (auto &q : my_spectra_)
    if (!q.selected) {
      all_spectra_->by_name(q.name.toStdString())->set_visible(true);
      q.selected = true;
    }
  update();
  emit stateChanged();
}

void QpxSpectraWidget::hide_all() {
  for (auto &q : my_spectra_)
    if (q.selected) {
      all_spectra_->by_name(q.name.toStdString())->set_visible(false);
      q.selected = false;
    }
  update();
  emit stateChanged();
}

int QpxSpectraWidget::available_count() {
  return my_spectra_.size();
}
