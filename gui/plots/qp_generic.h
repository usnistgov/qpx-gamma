#ifndef QP_GENERIC_PLOT_H
#define QP_GENERIC_PLOT_H

#include "qcustomplot.h"
#include "qp_draggable.h"
#include "qp_button.h"
#include <set>

namespace QPlot
{

enum ShowOptions {
  empty     = 0,
  style     = 1 << 0,
  scale     = 1 << 1,
  labels    = 1 << 2,
  themes    = 1 << 3,
  thickness = 1 << 4,
  grid      = 1 << 5,
  title     = 1 << 6,
  zoom      = 1 << 7,
  save      = 1 << 8,
  gradients = 1 << 9,
  dither    = 1 << 10
};

inline ShowOptions operator|(ShowOptions a, ShowOptions b) {return static_cast<ShowOptions>(static_cast<int>(a) | static_cast<int>(b));}
inline ShowOptions operator&(ShowOptions a, ShowOptions b) {return static_cast<ShowOptions>(static_cast<int>(a) & static_cast<int>(b));}

class GenericPlot : public QCustomPlot
{
  Q_OBJECT
public:
  explicit GenericPlot(QWidget *parent = 0);
  QSize sizeHint() const Q_DECL_OVERRIDE;

  void clearAll();
  virtual void clearPrimary() {}
  virtual void clearExtras() {}

  void replotAll();
  virtual void replotPrimary() {}
  virtual void replotExtras();

  void setVisibleOptions(ShowOptions);
  ShowOptions visibleOptions() const;

  bool alwaysSquare() const;
  bool antialiased() const;
  bool showGradientLegend() const;
  bool showMarkerLabels() const;
  bool showTitle() const;
  uint16_t lineThickness() const;
  QString gridStyle() const;
  QString scaleType() const;
  QString plotStyle() const;
  QString gradient() const;

  void setAlwaysSquare(bool);
  void setAntialiased(bool);
  void setShowGradientLegend(bool);
  void setShowMarkerLabels(bool);
  void setShowTitle(bool);
  void setLineThickness(uint16_t);
  void setGridStyle(QString);
  virtual void setScaleType(QString);
  void setPlotStyle(QString);
  void setGradient(QString);

  void addStandardGradients();
  void addCustomGradient(QString name,
                         std::initializer_list<std::string> colors);

public slots:
  virtual void zoomOut() {}

signals:
  void mouseHover(double x, double y);
  void shiftStateChanged(bool);
  void clickedAbstractItem(QCPAbstractItem *); //hack?

protected:
  virtual void executeButton(Button *);
  virtual void mouseClicked(double x, double y, QMouseEvent* e);

  void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void resizeEvent(QResizeEvent * event) Q_DECL_OVERRIDE;
  void keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;
  void keyReleaseEvent(QKeyEvent*) Q_DECL_OVERRIDE;

  void plotButtons();
  void setGraphThickness(QCPGraph* graph);
  void setGraphStyle(QCPGraph* graph, QString style = "");
  void setButtonPosition(QCPItemPosition*, Button* previous);

protected slots:
  void exportPlot(QAction*);
  void optionsChanged(QAction*);

private:
  mutable int previous_height_ {0};

  ShowOptions visible_options_;
  QMenu options_menu_;
  QMenu export_menu_;

  bool always_square_ {false};
  bool antialiased_ {false};
  bool show_gradient_legend_ {false};
  bool show_marker_labels_ {true};
  bool show_title_ {true};
  uint16_t line_thickness_ {1};
  QString current_scale_type_ {"Linear"};
  QString current_grid_style_ {"Grid"};
  QString current_plot_style_ {"Lines"};
  QString current_gradient_;

  std::map<QString, QCPColorGradient> gradients_;
  std::map<QString, QCPAxis::ScaleType> scale_types_
    { {"Linear", QCPAxis::stLinear}, {"Logarithmic", QCPAxis::stLogarithmic} };
  std::set<QString> grid_styles_ {"No grid", "Grid", "Grid + subgrid"};
  std::set<QString> plot_styles_ {"Step center", "Step left", "Step right",
                                  "Lines", "Scatter", "Fill"};

  //helper functions
  QFont rescaleFont(QFont font, double size_offset);
  void rescaleEverything(int fontUpscale, int plotThicken,
                         int marginUpscale, bool buttons_visible);
  void rebuildOptionsMenu();
  void checkoffOptionsMenu();
  void removeGradientLegend();
  void addGradientLegend(QCPColorMap* colorMap);
};


}

#endif
