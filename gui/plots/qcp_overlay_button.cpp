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
 *      QCPOverlayButton
 *
 ******************************************************************************/

#include "qcp_overlay_button.h"
#include "custom_logger.h"
#include <QToolTip>

QCPOverlayButton::QCPOverlayButton(QCustomPlot *parentPlot,
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

  if (second_point == (Qt::AlignBottom | Qt::AlignRight)) {
//    DBG << "Alightning bottom right";
    bottomRight->setParentAnchor(topLeft);
    bottomRight->setCoords(pixmap.width(), pixmap.height());
  } else {
//    DBG << "Alightning top left";
    topLeft->setParentAnchor(bottomRight);
    topLeft->setCoords(-pixmap.width(), -pixmap.height());
  }

  connect(parentPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(showTip(QMouseEvent*)));
}

void QCPOverlayButton::showTip(QMouseEvent *event)
{
  double x = event->pos().x();
  double y = event->pos().y();

  if ((left->pixelPoint().x() <= x)
      && (x <= right->pixelPoint().x())
      && (top->pixelPoint().y() <= y)
      && (y <= bottom->pixelPoint().y()))
  {
    QToolTip::showText(parentPlot()->mapToGlobal(event->pos()), tooltip_);
  }
}

//void QCPOverlayButton::set_image(QPixmap img)
//{
//  setPixmap(img);
//}

