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

#ifndef QCP_OVERLAY_BUTTON
#define QCP_OVERLAY_BUTTON


#include "qcustomplot.h"

class QCPOverlayButton : public QCPItemPixmap
{
    Q_OBJECT
public:
    explicit QCPOverlayButton(QCustomPlot *parentPlot,
                           QPixmap pixmap,
                           QString name,
                           QString tooltip,
                           int second_point = (Qt::AlignBottom | Qt::AlignRight)
                           );

  QString name() {return name_;}

private slots:
    void onMouseMove(QMouseEvent *event);

private:
    QString name_;
    QString tooltip_;

};

#endif
