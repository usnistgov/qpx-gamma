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
 *      SelectorWidget - colorful clickable things
 *
 ******************************************************************************/

#ifndef SELECTOR_WIDGET_H
#define SELECTOR_WIDGET_H
#include <QObject>
#include <QMetaType>
#include <QWidget>
#include <QPointF>
#include <QVector>
#include <QStaticText>
#include <QVariant>

struct SelectorItem {
  QString text;
  QColor color;
  bool visible;
  QVariant data;

  SelectorItem() : visible(false) {}
};

class SelectorWidget : public QWidget
{
  Q_OBJECT

public:
  SelectorWidget(QWidget *parent = 0);

  QSize sizeHint() const Q_DECL_OVERRIDE;
  QSize minimumSizeHint() const Q_DECL_OVERRIDE;
  void setItems(QVector<SelectorItem>);
  void replaceSelected(SelectorItem);
  QVector<SelectorItem> items();
  SelectorItem selected();
  virtual void show_all();
  virtual void hide_all();
  void set_only_one(bool);

signals:
  void itemSelected(SelectorItem);
  void itemToggled(SelectorItem);
  void itemDoubleclicked(SelectorItem);

protected:
  void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
  void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

  QVector<SelectorItem> my_items_;
  int selected_;

  int flagAt(int, int);

private:
  int rect_w_, rect_h_, border;
  int width_last, height_total, max_wide;

  QRectF inner, outer, text;
  bool   only_one_;

  void recalcDim(int, int);
};

#endif
