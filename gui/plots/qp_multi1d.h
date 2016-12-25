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

  template<typename T> QCPGraph* addGraph(const T& hist, const QPen&, QString name = "");
  template<typename T> QCPGraph* addGraph(const T& x, const T& y, const QPen&, QString name = "");

  void setAxisLabels(QString x, QString y);
  void setTitle(QString title);

  void setMarkers(const std::list<Marker1D>&);
  std::set<double> selectedMarkers();
  void setHighlight(Marker1D, Marker1D);

  void tightenX();
  void setScaleType(QString) override;

  virtual QCPRange getDomain();
  virtual QCPRange getRange(QCPRange domain = QCPRange());

public slots:
  void zoomOut() Q_DECL_OVERRIDE;

signals:
  void clickedPlot(double x, double y, Qt::MouseButton button);

protected slots:
  void mousePressed(QMouseEvent*);
  void mouseReleased(QMouseEvent*);
  virtual void adjustY();

protected:
  void mouseClicked(double x, double y, QMouseEvent *event) override;

  QString title_text_;
  std::list<Marker1D> markers_;
  std::vector<Marker1D> highlight_;

  QCPGraphDataContainer aggregate_;

  void plotMarkers();
  void plotHighlight();
  void plotTitle();

  QCPItemTracer* placeMarker(const Marker1D &);
  QCPItemLine* addArrow(QCPItemTracer*, const Marker1D &);
  QCPItemText* addLabel(QCPItemTracer*, const Marker1D &);
};

template<typename T>
QCPGraph* Multi1D::addGraph(const T& hist, const QPen& pen, QString name)
{
  if (hist.empty())
    return nullptr; // is this wise?

  double minimum {std::numeric_limits<double>::max()};
  double maximum {std::numeric_limits<double>::min()};

  GenericPlot::addGraph();
  auto ret = graph(graphCount() - 1);
  ret->setName(name);
  auto data = ret->data();
  for (auto p : hist)
  {
    QCPGraphData point(p.first, p.second);
    data->add(point);
    aggregate_.add(point);
    minimum = std::min(p.first, minimum);
    maximum = std::max(p.first, maximum);
  }

  ret->setProperty("minimum", QVariant::fromValue(minimum));
  ret->setProperty("maximum", QVariant::fromValue(maximum));

  ret->setPen(pen);
  setGraphStyle(ret);
  setGraphThickness(ret);
  return ret;
}

template<typename T>
QCPGraph* Multi1D::addGraph(const T& x, const T& y, const QPen& pen, QString name)
{
  if (x.empty() || y.empty() || (x.size() != y.size()))
    return nullptr; // is this wise?

  double minimum {std::numeric_limits<double>::max()};
  double maximum {std::numeric_limits<double>::min()};

  GenericPlot::addGraph();
  auto ret = graph(graphCount() - 1);
  ret->setName(name);
  auto data = ret->data();
  auto ix = x.begin();
  auto iy = y.begin();
  while (ix != x.end())
  {
    QCPGraphData point(*ix, *iy);
    data->add(point);
    aggregate_.add(point);
    ++ix;
    ++iy;
    minimum = std::min(*ix, minimum);
    maximum = std::max(*ix, maximum);
  }

  ret->setProperty("minimum", QVariant::fromValue(minimum));
  ret->setProperty("maximum", QVariant::fromValue(maximum));

  ret->setPen(pen);
  setGraphStyle(ret);
  setGraphThickness(ret);
  return ret;
}

}

#endif
