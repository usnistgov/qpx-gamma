#include "qp_button.h"
#include <QToolTip>

namespace QPlot
{

Button::Button(QCustomPlot *parentPlot,
               QPixmap pixmap,
               QString name,
               QString tooltip,
               int second_point)
  : QCPItemPixmap(parentPlot)
  , name_(name)
  , tooltip_(tooltip)
{
  setPixmap(pixmap);
  setSelectable(false);

  if (second_point == (Qt::AlignBottom | Qt::AlignRight))
  {
    bottomRight->setType(QCPItemPosition::ptAbsolute);
    bottomRight->setParentAnchor(topLeft);
    bottomRight->setCoords(pixmap.width(), pixmap.height());
  }
  else
  {
    topLeft->setType(QCPItemPosition::ptAbsolute);
    topLeft->setParentAnchor(bottomRight);
    topLeft->setCoords(-pixmap.width(), -pixmap.height());
  }

  connect(parentPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(showTip(QMouseEvent*)));
}

void Button::showTip(QMouseEvent *event)
{
  double x = event->pos().x();
  double y = event->pos().y();

  if (visible()
      && (left->pixelPosition().x() <= x)
      && (x <= right->pixelPosition().x())
      && (top->pixelPosition().y() <= y)
      && (y <= bottom->pixelPosition().y()))
  {
    QToolTip::showText(parentPlot()->mapToGlobal(event->pos()), tooltip_);
  }
}

//void QCPOverlayButton::set_image(QPixmap img)
//{
//  setPixmap(img);
//}

}
