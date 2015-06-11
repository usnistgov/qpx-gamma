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

class QSquareCustomPlot : public QCustomPlot
{
  Q_OBJECT
public:
  explicit QSquareCustomPlot(QWidget *parent = 0) : QCustomPlot(parent), square(false) {}

  void setAlwaysSquare(bool sq);

  QSize sizeHint() const;

signals:
  void mouse_upon(double x, double y);
  void mouse_clicked(double x, double y, QMouseEvent* e, bool channels);

protected:
  void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void resizeEvent(QResizeEvent * event);

private:
  mutable int lastHeight;
  bool square;

};

#endif
