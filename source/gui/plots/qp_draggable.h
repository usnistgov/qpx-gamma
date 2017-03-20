#pragma once

#include "qcustomplot.h"

namespace QPlot
{

class Draggable : public QCPItemLine
{
    Q_OBJECT
public:
    explicit Draggable(QCustomPlot *parentPlot, QCPItemTracer *trc, int size);

    void startMoving(const QPointF &mousePos);
    void set_limits(double l , double r);
    QCPItemTracer *tracer() const;

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
    QTimer  move_timer_;
    QPointF pos_current_;

    double limit_l, limit_r;
};

}
