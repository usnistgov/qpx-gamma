#ifndef QP_PLOT1D_H
#define QP_PLOT1D_H

#include <QWidget>
#include "qp_generic.h"
#include "qp_appearance.h"
#include "qp_bounds.h"

namespace QPlot
{

using HistoData = std::map<double,double>;

class Plot1D : public GenericPlot
{
  Q_OBJECT

public:
  explicit Plot1D(QWidget *parent = 0);

  void clearPrimary() override;
  void clearExtras() override;
  void replotExtras() override;

  QCPGraph* addGraph(const HistoData &hist,
                     Appearance appearance, QString style = "");
  QCPGraph* addGraph(const QVector<double>& x, const QVector<double>& y,
                     Appearance appearance, QString style = "");
  void setAxisLabels(QString x, QString y);
  void setTitle(QString title);

  void tightenX();
  void setScaleType(QString) override;

public slots:
  void zoomOut() Q_DECL_OVERRIDE;

signals:
  void clicked(double x, double y, QMouseEvent *event);

protected slots:
  void mousePressed(QMouseEvent*);
  void mouseReleased(QMouseEvent*);

  void adjustY();

protected:
  void mouseClicked(double x, double y, QMouseEvent *event) override;

  QString title_text_;
  void plotTitle();

  Bounds bounds_;
  QCPRange getDomain();
  virtual QCPRange getRange(QCPRange domain = QCPRange());

  void configGraph(QCPGraph* graph, Appearance appearance, QString style);
};

}

#endif
