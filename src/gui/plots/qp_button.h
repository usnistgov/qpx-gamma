#pragma once

#include "qcustomplot.h"

namespace QPlot
{

class Button : public QCPItemPixmap
{
  Q_OBJECT
public:
  explicit Button(QCustomPlot *parentPlot,
                  QPixmap pixmap,
                  QString name,
                  QString tooltip,
                  int second_point = (Qt::AlignBottom | Qt::AlignRight)
      );

  QString name() {return name_;}
  //  void set_image(QPixmap img);

private slots:
  void showTip(QMouseEvent *event);

private:
  QString name_;
  QString tooltip_;
};

}
