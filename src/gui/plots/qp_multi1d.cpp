#include "qp_multi1d.h"

namespace QPlot
{


double Marker1D::worstVal() const
{
  if (alignment == Qt::AlignTop)
    return std::numeric_limits<double>::min();
  if (alignment == Qt::AlignBottom)
    return std::numeric_limits<double>::max();
  if (alignment == Qt::AlignAbsolute)
    return 0;
  else
    return nan("");
}

bool Marker1D::isValBetterThan(double newval, double oldval) const
{
  if ((alignment == Qt::AlignTop) && (newval > oldval))
    return true;
  else if ((alignment == Qt::AlignBottom) && (newval < oldval))
    return true;
  else if ((alignment == Qt::AlignAbsolute) &&
      (std::abs(newval - closest_val) <= std::abs(oldval - closest_val)) )
    return true;
  else
    return false;
}



Multi1D::Multi1D(QWidget *parent)
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

  setVisibleOptions(ShowOptions::zoom | ShowOptions::save |
                    ShowOptions::scale | ShowOptions::style |
                    ShowOptions::thickness | ShowOptions::title);

  replotExtras();
}

void Multi1D::clearPrimary()
{
  GenericPlot::clearGraphs();
  aggregate_.clear();
}

void Multi1D::clearExtras()
{
  markers_.clear();
  highlight_.clear();
  title_text_.clear();
}

void Multi1D::setAxisLabels(QString x, QString y)
{
  xAxis->setLabel(x);
  yAxis->setLabel(y);
}


void Multi1D::setTitle(QString title)
{
  title_text_ = title;
}

void Multi1D::setMarkers(const std::list<Marker1D>& markers)
{
  markers_ = markers;
}

void Multi1D::setHighlight(Marker1D a, Marker1D b)
{
  highlight_.resize(2);
  highlight_[0] = a;
  highlight_[1] = b;
}

std::set<double> Multi1D::selectedMarkers()
{
  std::set<double> selection;
  for (auto &q : selectedItems())
    if (QCPItemText *txt = qobject_cast<QCPItemText*>(q))
    {
      if (txt->property("position").isValid())
        selection.insert(txt->property("position").toDouble());
    }
    else if (QCPItemLine *line = qobject_cast<QCPItemLine*>(q))
    {
      if (line->property("position").isValid())
        selection.insert(line->property("position").toDouble());
    }

  return selection;
}

QCPRange Multi1D::getDomain()
{
  QCP::SignDomain sd = QCP::sdBoth;
  if (scaleType() == "Logarithmic")
    sd = QCP::sdPositive;
  bool ok{false};
  return aggregate_.keyRange(ok, sd);
}

QCPRange Multi1D::getRange(QCPRange domain)
{
  bool log = (scaleType() == "Logarithmic");
  QCP::SignDomain sd = QCP::sdBoth;
  if (log)
    sd = QCP::sdPositive;
  bool ok{false};
  QCPRange range = aggregate_.valueRange(ok, sd, domain);
  if (ok)
  {
    range.upper = yAxis->pixelToCoord(yAxis->coordToPixel(range.upper) -
                                      ((!showTitle() || title_text_.isEmpty()) ? 5 : 25));
    double lower = yAxis->pixelToCoord(yAxis->coordToPixel(range.lower) + 5);
    if (log && (lower < 0))
      range.lower = 0;
    else
      range.lower = lower;
  }

  bool markers_in_range {false};
  bool markers_with_val {false};
  for (auto &marker : markers_)
    if ((domain.lower <= marker.pos) && (marker.pos <= domain.upper))
    {
      markers_in_range = true;
      if (marker.alignment = Qt::AlignAbsolute)
        markers_with_val = true;
    }


  int upper = 0
      + (markers_in_range ? 30 : 0)
      + ((markers_in_range && showMarkerLabels()) ? 20 : 0)
      + ((markers_with_val && showMarkerLabels()) ? 20 : 0);

  range.upper = yAxis->pixelToCoord(yAxis->coordToPixel(range.upper) - upper);

  return range;
}

void Multi1D::tightenX()
{
  xAxis->setRange(getDomain());
}

void Multi1D::replotExtras()
{
  clearItems();
  plotMarkers();
  plotHighlight();
  plotTitle();
  plotButtons();
}

void Multi1D::plotMarkers()
{
  for (auto &marker : markers_)
  {
    if (!marker.visible)
      continue;
    if (QCPItemTracer *top_crs = placeMarker(marker))
    {
      addArrow(top_crs, marker);
      if (showMarkerLabels())
      {
        auto label = addLabel(top_crs, marker);
        if (marker.alignment == Qt::AlignAbsolute)
          label->setText(QString::number(marker.pos) + "\n"
                                         "v=" + QString::number(top_crs->positions().first()->value()));
      }
    }
  }
}

void Multi1D::plotHighlight()
{
  if ((highlight_.size() == 2) && (highlight_[0].visible) && !aggregate_.isEmpty())
  {
    QCPRange range = this->getRange(this->getDomain());

    QCPItemRect *cprect = new QCPItemRect(this);

    cprect->topLeft->setCoords(highlight_[0].pos, range.upper);
    cprect->bottomRight->setCoords( highlight_[1].pos, range.lower);
    cprect->setPen(highlight_[0].appearance.default_pen);
    cprect->setBrush(QBrush(highlight_[1].appearance.default_pen.color()));
    cprect->setSelectable(false);
  }
}

void Multi1D::plotTitle()
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

void Multi1D::mouseClicked(double x, double y, QMouseEvent* event)
{
  emit clickedPlot(x, y, event->button());
}

void Multi1D::zoomOut()
{
  xAxis->setRange(getDomain());
  this->adjustY();
  replot();
}

void Multi1D::mousePressed(QMouseEvent*)
{
  disconnect(this, SIGNAL(beforeReplot()), this, SLOT(adjustY()));
}

void Multi1D::mouseReleased(QMouseEvent*)
{
  connect(this, SIGNAL(beforeReplot()), this, SLOT(adjustY()));
  this->adjustY();
  replot();
}

void Multi1D::adjustY()
{
  if (aggregate_.isEmpty())
    rescaleAxes();
  else
    yAxis->setRange(this->getRange(xAxis->range()));
}

void Multi1D::setScaleType(QString type)
{
  bool diff = (type != scaleType());
  GenericPlot::setScaleType(type);
  if (diff)
  {
    replotExtras();
    replot();
  }
}

QCPItemTracer* Multi1D::placeMarker(const Marker1D& marker)
{
  if (!marker.visible)
    return nullptr;

  QCPItemTracer *optimal_crs {nullptr};

  double optimal_val = marker.worstVal();

  foreach (QCPGraph *graph, mGraphs)
  {
    bool ok{false};
    QCP::SignDomain sd = QCP::sdBoth;
    if (scaleType() == "Logarithmic")
      sd = QCP::sdPositive;
    QCPRange keyrange = graph->data()->keyRange(ok, sd);

    if (!ok || (keyrange.lower > marker.pos)
        || (marker.pos > keyrange.upper))
      continue;

    QCPItemTracer *crs = new QCPItemTracer(this);
    crs->setGraph(graph);
    crs->setInterpolating(true);
    crs->setGraphKey(marker.pos);
    crs->updatePosition();
    double val = crs->positions().first()->value();

    if (marker.isValBetterThan(val, optimal_val))
    {
      if (optimal_crs)
        removeItem(optimal_crs);
      optimal_val = val;
      optimal_crs = crs;
    }
    else
      removeItem(crs);
  }

  if (optimal_crs)
  {
    optimal_crs->setStyle(QCPItemTracer::tsNone);
    optimal_crs->setProperty("position", marker.pos);
    optimal_crs->setSelectable(false);
  }

  return optimal_crs;
}

QCPItemLine* Multi1D::addArrow(QCPItemTracer* crs, const Marker1D& marker)
{
  if (!crs)
    return nullptr;

  QPen pen = marker.appearance.default_pen;
  QPen selpen = marker.appearance.get_pen("selected");

  QCPItemLine *line = new QCPItemLine(this);
  line->start->setParentAnchor(crs->position);
  line->start->setCoords(0, -30);
  line->end->setParentAnchor(crs->position);
  line->end->setCoords(0, -5);
  line->setHead(QCPLineEnding(QCPLineEnding::esLineArrow, 7, 7));
  line->setPen(pen);
  line->setSelectedPen(selpen);
  line->setProperty("true_value", crs->graphKey());
  line->setProperty("position", crs->property("position"));
  line->setSelectable(false);

  return line;
}

QCPItemText* Multi1D::addLabel(QCPItemTracer* crs, const Marker1D& marker)
{
  if (!crs)
    return nullptr;

  QPen pen = marker.appearance.default_pen;
  QPen selpen = marker.appearance.get_pen("selected");

  QCPItemText *markerText = new QCPItemText(this);
  markerText->setProperty("true_value", crs->graphKey());
  markerText->setProperty("position", crs->property("position"));

  markerText->position->setParentAnchor(crs->position);
  markerText->setPositionAlignment(static_cast<Qt::AlignmentFlag>(Qt::AlignHCenter|Qt::AlignBottom));
  markerText->position->setCoords(0, -30);
  markerText->setText(QString::number(marker.pos));
  markerText->setTextAlignment(Qt::AlignCenter);
  markerText->setFont(QFont("Helvetica", 9 + pen.width()));
  markerText->setPen(pen);
  markerText->setSelectedPen(selpen);
  markerText->setColor(pen.color());
  markerText->setSelectedColor(selpen.color());
  markerText->setBrush(QBrush(Qt::white));
  markerText->setSelectedBrush(QBrush(Qt::white));
  markerText->setPadding(QMargins(1, 1, 1, 1));
  markerText->setSelectable(false);

  return markerText;
}



}
