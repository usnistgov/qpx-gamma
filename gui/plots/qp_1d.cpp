#include "qp_1d.h"
//#include "custom_logger.h"

namespace QPlot
{

Plot1D::Plot1D(QWidget *parent)
  : GenericPlot(parent)
{
  setInteraction(QCP::iSelectItems, true);
  setInteraction(QCP::iRangeDrag, true);
  yAxis->axisRect()->setRangeDrag(Qt::Horizontal);
  setInteraction(QCP::iRangeZoom, true);
  setInteraction(QCP::iMultiSelect, true);
  yAxis->setPadding(28);
  setNoAntialiasingOnDrag(true);

  connect(this, SIGNAL(beforeReplot()), this, SLOT(adjustY()));
  connect(this, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(mouseReleased(QMouseEvent*)));
  connect(this, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePressed(QMouseEvent*)));

  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), this);
  connect(shortcut, SIGNAL(activated()), this, SLOT(zoomOut()));

  setScaleType("Logarithmic");
  setPlotStyle("Step center");
  setGridStyle("Grid + subgrid");

  setVisibleOptions(ShowOptions::zoom | ShowOptions::save | ShowOptions::labels |
                    ShowOptions::scale | ShowOptions::style |
                    ShowOptions::thickness | ShowOptions::title);

  replotExtras();
}

void Plot1D::clearPrimary()
{
  GenericPlot::clearGraphs();
  bounds_.clear();
}

void Plot1D::clearExtras()
{
  GenericPlot::clearExtras();
  title_text_.clear();
}

void Plot1D::setAxisLabels(QString x, QString y)
{
  xAxis->setLabel(x);
  yAxis->setLabel(y);
}


void Plot1D::setTitle(QString title)
{
  title_text_ = title;
}


QCPGraph* Plot1D::addGraph(const HistoData &hist, Appearance appearance, QString style)
{
  if (hist.empty())
    return nullptr;

  QCPGraph* graph = GenericPlot::addGraph();
  auto data = graph->data();
  for (auto p : hist)
  {
    data->add(QCPGraphData(p.first, p.second));
    bounds_.add(p.first, p.second);
  }

  configGraph(graph, appearance, style);
  return graph;
}

QCPGraph* Plot1D::addGraph(const QVector<double>& x, const QVector<double>& y,
                   Appearance appearance, QString style)
{
  if ((y.size() != x.size()) || x.isEmpty())
    return nullptr;

  QCPGraph* graph = GenericPlot::addGraph();
  auto data = graph->data();
  for (int i=0; i < x.size(); ++i)
  {
    auto xx = x[i];
    auto yy = y[i];
    data->add(QCPGraphData(xx, yy));
    bounds_.add(xx, yy);
  }

  configGraph(graph, appearance, style);
  return graph;
}

void Plot1D::configGraph(QCPGraph* graph, Appearance appearance, QString style)
{
  graph->setPen(appearance.default_pen);

  if (!style.isEmpty())
    setGraphStyle(graph, style);
  else if (visibleOptions() && ShowOptions::style)
    setGraphStyle(graph);

  if (visibleOptions() && ShowOptions::thickness)
    setGraphThickness(graph);
}


QCPRange Plot1D::getDomain()
{
  if (scaleType() == "Logarithmic")
    return bounds_.getDomain(QCP::sdPositive);
  else
    return bounds_.getDomain(QCP::sdBoth);
}

QCPRange Plot1D::getRange(QCPRange domain)
{

  bool log = (scaleType() == "Logarithmic");
  QCPRange range = bounds_.getRange(domain);

//  DBG << "Domain (" << domain.lower << ", " << domain.upper << ")"
//      << " -> Range(" << range.lower << ", " << range.upper << ")";

  int upper = 5 + ((!showTitle() || title_text_.isEmpty()) ? 0 : 20);

  range.upper = yAxis->pixelToCoord(yAxis->coordToPixel(range.upper) - upper);
  double lower = yAxis->pixelToCoord(yAxis->coordToPixel(range.lower) + 5);
  if (log && (lower < 0))
    range.lower = 0;
  else
    range.lower = lower;
  return range;
}

void Plot1D::tightenX()
{
  xAxis->setRange(getDomain());
}

void Plot1D::replotExtras()
{
  GenericPlot::replotExtras();
  plotTitle();
}

void Plot1D::plotTitle()
{
  if (showTitle() && !title_text_.isEmpty())
  {
    QCPItemText *floatingText = new QCPItemText(this);
    floatingText->setPositionAlignment(static_cast<Qt::AlignmentFlag>(Qt::AlignTop|Qt::AlignHCenter));
    floatingText->position->setType(QCPItemPosition::ptAxisRectRatio);
    floatingText->position->setCoords(0.5, 0); // place position at center/top of axis rect
    floatingText->setText(title_text_);
    floatingText->setFont(QFont("Helvetica", 10));
    floatingText->setSelectable(false);
    floatingText->setColor(Qt::black);
  }
}

void Plot1D::mouseClicked(double x, double y, QMouseEvent* event)
{
  emit clicked(x, y, event);
}

void Plot1D::zoomOut()
{
  rescaleAxes();
  xAxis->setRange(getDomain());
  adjustY();
  replot();
}

void Plot1D::mousePressed(QMouseEvent*)
{
  disconnect(this, SIGNAL(beforeReplot()), this, SLOT(adjustY()));
}

void Plot1D::mouseReleased(QMouseEvent*)
{
  connect(this, SIGNAL(beforeReplot()), this, SLOT(adjustY()));
  adjustY();
  replot();
}

void Plot1D::adjustY()
{
  if (bounds_.empty())
    rescaleAxes();
  else
  {
    yAxis->rescale();;
    yAxis->setRange(getRange(xAxis->range()));
  }
}

void Plot1D::setScaleType(QString type)
{
  bool diff = (type != scaleType());
  GenericPlot::setScaleType(type);
  if (diff)
  {
    replotExtras();
    replot();
  }
}


}
