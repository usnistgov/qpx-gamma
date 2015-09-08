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
 *      DraggableTracer
 *
 ******************************************************************************/

#ifndef Q_PLOT_POINT_H
#define Q_PLOT_POINT_H


#include "qcustomplot.h"

class DraggableTracer : public QCPItemLine
{
    Q_OBJECT
public:
    explicit DraggableTracer(QCustomPlot *parentPlot, QCPItemTracer *trc, int size);

    void startMoving(const QPointF &mousePos);
    void set_limits(double l , double r);

signals:
    void moved(const QPointF &pos);
    void stoppedMoving();

public slots:
    void move(double x, double y, bool signalNeeded = true);
    void movePx(double x, double y);
    void stopMov(QMouseEvent* evt);

private slots:
    void onMouseMove(QMouseEvent *event);
    void moveToWantedPos();

private:
    QCPItemTracer *center_tracer_;
    QPointF grip_delta_;
    QPointF pos_initial_;
    QPointF pos_last_;
    QTimer *move_timer_;
    QPointF pos_current_;

    double limit_l, limit_r;
};

#endif
