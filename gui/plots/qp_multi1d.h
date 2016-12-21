#ifndef QP_MULTI1D_H
#define QP_MULTI1D_H

#include <QWidget>
#include "qp_generic.h"
#include "qp_appearance.h"
#include "histogram.h"

namespace QPlot
{

struct Marker1D
{
  Marker1D() {}
  Marker1D(double p, Appearance app = Appearance(), bool vis = true)
    : pos(p), visible(vis), appearance(app) {}

  bool operator!= (const Marker1D& other) const { return (!operator==(other)); }
  bool operator== (const Marker1D& other) const { return (pos == other.pos); }

  bool visible {false};
  double pos {0};
  Appearance appearance;
  Qt::Alignment alignment {Qt::AlignTop};
  double closest_val{nan("")};

  double worstVal() const;
  bool isValBetterThan(double newval, double oldval) const;
};


class Multi1D : public GenericPlot
{
  Q_OBJECT

public:
  explicit Multi1D(QWidget *parent = 0);

  void clearPrimary() override;
  void clearExtras() override;
  void replotExtras() override;

  void addGraph(const HistMap1D&, Appearance, QString name = "");
  void addGraph(const HistList1D&, Appearance, QString name = "");
  void setAxisLabels(QString x, QString y);
  void setTitle(QString title);

  void setMarkers(const std::list<Marker1D>&);
  std::set<double> selectedMarkers();
  void setHighlight(Marker1D, Marker1D);

  void tightenX();
  void setScaleType(QString) override;

public slots:
  void zoomOut() Q_DECL_OVERRIDE;

signals:
  void clickedPlot(double x, double y, Qt::MouseButton button);

protected slots:
  void mousePressed(QMouseEvent*);
  void mouseReleased(QMouseEvent*);

  void adjustY();

protected:
  void mouseClicked(double x, double y, QMouseEvent *event) override;

  QString title_text_;
  std::list<Marker1D> markers_;
  std::vector<Marker1D> highlight_;

  QCPGraphDataContainer aggregate_;
  QCPRange getDomain();
  QCPRange getRange(QCPRange domain = QCPRange());

  void plotMarkers();
  void plotHighlight();
  void plotTitle();

  QCPItemTracer* placeMarker(const Marker1D &);
  QCPItemLine* addArrow(QCPItemTracer*, const Marker1D &);
  QCPItemText* addLabel(QCPItemTracer*, const Marker1D &);
};

}

#endif
