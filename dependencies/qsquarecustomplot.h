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
 *      QCustomPlot that maintains square proportions.
 *
 ******************************************************************************/

#ifndef Q_SQUARE_CP_H
#define Q_SQUARE_CP_H

#include "qcustomplot.h"
#include <QWidget>
#include "detector.h"

struct Marker {
  double energy, channel;
  uint16_t bits;
  QMap<QString, QPen> themes;
  QPen default_pen;
  bool visible;
  bool calibrated;

  Marker() : energy(0), channel(0), bits(0), visible(false), default_pen(Qt::gray, 1, Qt::SolidLine), calibrated(false) {}

  void shift(uint16_t to_bits) {
    if (bits > to_bits)
      channel = static_cast<int>(channel) >> (bits - to_bits);
    if (bits < to_bits)
      channel = static_cast<int>(channel) << (to_bits - bits);
    bits = to_bits;
  }

  void calibrate(Pixie::Detector det) {
    if (det.energy_calibrations_.has_a(Pixie::Calibration(bits))) {
      energy = det.energy_calibrations_.get(Pixie::Calibration(bits)).transform(channel);
      calibrated = true;
    } else
      calibrated = false;
  }
};

class QSquareCustomPlot : public QCustomPlot
{
  Q_OBJECT
public:
  explicit QSquareCustomPlot(QWidget *parent = 0) : QCustomPlot(parent), square(false) {}

  void setAlwaysSquare(bool sq);

  QSize sizeHint() const;

signals:
  void mouse_upon(double x, double y);
  void mouse_clicked(double x, double y, QMouseEvent* e);

protected:
  void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void resizeEvent(QResizeEvent * event);

private:
  mutable int lastHeight;
  bool square;

};

#endif
