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
}

void QCPOverlayButton::onMouseMove(QMouseEvent *event)
{
//  pos_current_ = QPointF(event->localPos().x() + grip_delta_.x(),
//                            event->localPos().y() + grip_delta_.y());
}
