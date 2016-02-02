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
#include "draggable_tracer.h"
#include <QWidget>

enum ShowOptions {
  empty     = 0,
  style     = 1 << 0,
  scale     = 1 << 1,
  labels    = 1 << 2,
  themes    = 1 << 3,
  thickness = 1 << 4,
  grid      = 1 << 5,
  title     = 1 << 6,
  zoom      = 1 << 7,
  save      = 1 << 8
};

inline ShowOptions operator|(ShowOptions a, ShowOptions b) {return static_cast<ShowOptions>(static_cast<int>(a) | static_cast<int>(b));}
inline ShowOptions operator&(ShowOptions a, ShowOptions b) {return static_cast<ShowOptions>(static_cast<int>(a) & static_cast<int>(b));}

class QSquareCustomPlot : public QCustomPlot
{
  Q_OBJECT
public:
  explicit QSquareCustomPlot(QWidget *parent = 0) : QCustomPlot(parent), square(false), under_mouse_(nullptr) {}

  void setAlwaysSquare(bool sq);

  QSize sizeHint() const;

signals:
  void mouse_upon(double x, double y);
  void mouse_clicked(double x, double y, QMouseEvent* e, bool on_item); //why on_item?
  void shiftStateChanged(bool);
  void clickedAbstractItem(QCPAbstractItem *);

protected:
  void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void resizeEvent(QResizeEvent * event);

  void keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;
  void keyReleaseEvent(QKeyEvent*) Q_DECL_OVERRIDE;


private:
  mutable int lastHeight;
  bool square;
  DraggableTracer *under_mouse_;

};

#endif
