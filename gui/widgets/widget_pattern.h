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
 *      QpxPattern - coincidence pattern with drawing and editing facilities
 *      QpxPatternEditor - widget that represents coincidence pattern
 *
 ******************************************************************************/

#ifndef QPX_PATTERN_H
#define QPX_PATTERN_H
#include <QObject>
#include <QMetaType>
#include <QWidget>
#include <QPointF>
#include <QVector>
#include <QStyledItemDelegate>
#include <vector>
#include <bitset>
#include "pattern.h"
#include "custom_logger.h"

Q_DECLARE_METATYPE(Qpx::Pattern)

class QpxPatternEditor : public QWidget
{
  Q_OBJECT
public:
  QpxPatternEditor(QWidget *parent = 0);

  QpxPatternEditor(Qpx::Pattern pattern = Qpx::Pattern(), double size = 25, size_t wrap = 0)
  {set_pattern(pattern, size, wrap);}

  void set_pattern(Qpx::Pattern pattern = Qpx::Pattern(), double size = 25, size_t wrap = 0);

  QSize sizeHint() const Q_DECL_OVERRIDE;
  Qpx::Pattern pattern() const;

  void paint(QPainter *painter, const QRect &rect,
             const QPalette &palette) const;

protected:
  void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
  void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

private:

  int flagAtPosition(int x, int y);
  void setFlag(int count);

  QRectF outer, inner;
  std::vector<bool> pattern_;
  size_t threshold_;

  double size_;
  int wrap_, rows_;
};

#endif
