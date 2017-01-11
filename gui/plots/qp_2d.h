#ifndef QP_PLOT2D_H
#define QP_PLOT2D_H

#include "qp_generic.h"
#include "qp_appearance.h"
#include "histogram.h"

namespace QPlot
{

struct Label2D
{
  int64_t id {-1};
  double x {0}, y {0};
  QString text;
  bool selected {false};
  bool selectable {false};
  bool vertical {false};
  bool vfloat {false}, hfloat {false};
};

struct MarkerBox2D
{
  bool operator== (const MarkerBox2D& other) const
  {
    if (x1 != other.x1) return false;
    if (x2 != other.x2) return false;
    if (y1 != other.y1) return false;
    if (y2 != other.y2) return false;
    if (xc != other.xc) return false;
    if (yc != other.yc) return false;
    return true;
  }

  bool operator!= (const MarkerBox2D& other) const
    { return !operator==(other); }

  bool visible {true};
  bool selected {false};
  bool selectable {false};
  bool mark_center {false};

  double x1, x2, y1, y2, xc, yc;
  int64_t id {-1};
  QString label;
  QColor border;
  QColor fill;
};


class Plot2D : public GenericPlot
{
  Q_OBJECT

public:
  explicit Plot2D(QWidget *parent = 0);

  void clearPrimary() override;
  void clearExtras() override;
  void replotExtras() override;

  void updatePlot(uint64_t sizex, uint64_t sizey, const HistMap2D &spectrum_data);
  void updatePlot(uint64_t sizex, uint64_t sizey, const HistList2D &spectrum_data);

  void setAxes(QString xlabel, double x1, double x2,
                QString ylabel, double y1, double y2,
                QString zlabel);

  bool inRange(double x1, double x2,
               double y1, double y2) const;
  void zoomOut(double x1, double x2,
               double y1, double y2);
  void setOrientation(Qt::Orientation);

  void setBoxes(std::list<MarkerBox2D> boxes);
  void addBoxes(std::list<MarkerBox2D> boxes);
  std::list<MarkerBox2D> selectedBoxes(); //const?

  void setLabels(std::list<Label2D> labels);
  void addLabels(std::list<Label2D> labels);
  std::list<Label2D> selectedLabels(); //const?

public slots:
  void zoomOut() Q_DECL_OVERRIDE;

signals:
  void clickedPlot(double x, double y, Qt::MouseButton button);

protected:
  void mouseClicked(double x, double y, QMouseEvent* event) override;

  QCPColorMap *colorMap {new QCPColorMap(xAxis, yAxis)};
  std::list<MarkerBox2D> boxes_;
  std::list<Label2D> labels_;


  void initializeGradients();
  void plotBoxes();
  void plotLabels();
};

}

#endif
