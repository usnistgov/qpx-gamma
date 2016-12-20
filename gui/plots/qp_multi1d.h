#ifndef QP_MULTI1D_H
#define QP_MULTI1D_H

#include "qp_1d.h"
#include <QWidget>

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

class Multi1D : public Plot1D
{
  Q_OBJECT

public:
  explicit Multi1D(QWidget *parent = 0);

  void clearExtras() override;
  void replotExtras() override;

  void setMarkers(const std::list<Marker1D>&);
  std::set<double> selectedMarkers();
  void setHighlight(Marker1D, Marker1D);

protected:
  std::list<Marker1D> markers_;
  std::vector<Marker1D> highlight_;

  void plotMarkers();
  void plotHighlight();

  QCPRange getRange(QCPRange domain = QCPRange()) override;

  QCPItemTracer* placeMarker(const Marker1D &);
  QCPItemLine* addArrow(QCPItemTracer*, const Marker1D &);
  QCPItemText* addLabel(QCPItemTracer*, const Marker1D &);
};

}

#endif
