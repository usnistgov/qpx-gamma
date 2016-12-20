#include "qp_generic.h"
#include "qt_util.h"

namespace QPlot
{

GenericPlot::GenericPlot(QWidget *parent)
  : QCustomPlot(parent)
{
  export_menu_.addAction("png");
  export_menu_.addAction("jpg");
  export_menu_.addAction("pdf");
  export_menu_.addAction("bmp");
  connect(&export_menu_, SIGNAL(triggered(QAction*)), this, SLOT(exportPlot(QAction*)));
  connect(&options_menu_, SIGNAL(triggered(QAction*)), this, SLOT(optionsChanged(QAction*)));
}

void GenericPlot::setVisibleOptions(ShowOptions options)
{
  visible_options_ = options;

  setInteraction(QCP::iRangeDrag, options & ShowOptions::zoom);
  setInteraction(QCP::iRangeZoom, options & ShowOptions::zoom);

  rebuildOptionsMenu();
  this->replotExtras();
  replot();
}

ShowOptions GenericPlot::visibleOptions() const
{
  return visible_options_;
}

void GenericPlot::clearAll()
{  
  clearGraphs();
  clearItems();
  this->clearPrimary();
  this->clearExtras();
}

void GenericPlot::replotAll()
{
  clearGraphs();
  clearItems();
  this->replotPrimary();
  this->replotExtras();
  replot();
}

void GenericPlot::replotExtras()
{
  this->plotButtons();
}

//getters

bool GenericPlot::alwaysSquare() const
{
  return always_square_;
}

bool GenericPlot::antialiased() const
{
  return antialiased_;
}

bool GenericPlot::showGradientLegend() const
{
  return show_gradient_legend_;
}

bool GenericPlot::showMarkerLabels() const
{
  return show_marker_labels_;
}

bool GenericPlot::showTitle() const
{
  return show_title_;
}

uint16_t GenericPlot::lineThickness() const
{
  return line_thickness_;
}

QString GenericPlot::scaleType() const
{
  return current_scale_type_;
}

QString GenericPlot::gridStyle() const
{
  return current_grid_style_;
}

QString GenericPlot::gradient() const
{
  return current_gradient_;
}

QString GenericPlot::plotStyle() const
{
  return current_plot_style_;
}


//setters

void GenericPlot::setAlwaysSquare(bool sq)
{
  always_square_ = sq;
  updateGeometry();
}

void GenericPlot::setAntialiased(bool anti)
{
  antialiased_ = anti;
  for (int i=0; i < plottableCount(); ++i)
    if (QCPColorMap *colorMap = qobject_cast<QCPColorMap*>(plottable(i)))
      colorMap->setInterpolate(antialiased_);
  checkoffOptionsMenu();
}

void GenericPlot::setShowGradientLegend(bool show)
{
  if (show_gradient_legend_ == show)
    return;

  show_gradient_legend_ = show;

  QCPColorMap *colorMap {nullptr};
  for (int i=0; i < plottableCount(); ++i)
    if (colorMap = qobject_cast<QCPColorMap*>(plottable(i)))
      break;

  if (show_gradient_legend_ && colorMap)
  {
    addGradientLegend(colorMap);
    setScaleType(current_scale_type_);
  }

  updateGeometry();
  checkoffOptionsMenu();
}

void GenericPlot::removeGradientLegend()
{
  for (int i=0; i < plotLayout()->elementCount(); i++) {
    if (QCPColorScale *le = qobject_cast<QCPColorScale*>(plotLayout()->elementAt(i)))
      plotLayout()->remove(le);
  }
  plotLayout()->simplify();
}

void GenericPlot::addGradientLegend(QCPColorMap *colorMap)
{
  if (!colorMap)
    return;

  //add color scale
  QCPColorScale *colorScale = new QCPColorScale(this);
  plotLayout()->addElement(0, 1, colorScale);
  colorMap->setColorScale(colorScale);

  colorScale->setType(QCPAxis::atRight);
  colorScale->setRangeDrag(false);
  colorScale->setRangeZoom(false);
  colorScale->axis()->setTickLength(6,6);
  colorScale->axis()->setSubTickLength(2,1);
  colorScale->axis()->setLabel(colorMap->property("z_label").toString());
  colorScale->axis()->setNumberFormat("gbc");
  colorScale->axis()->setSubTicks(true);

  //readjust margins
  QCPMarginGroup *marginGroup = new QCPMarginGroup(this);
  axisRect()->setMarginGroup(static_cast<QCP::MarginSide>(QCP::msBottom|QCP::msTop), marginGroup);
  colorScale->setMarginGroup(static_cast<QCP::MarginSide>(QCP::msBottom|QCP::msTop), marginGroup);
}

void GenericPlot::setShowMarkerLabels(bool sl)
{
  show_marker_labels_ = sl;
  this->replotExtras();
  checkoffOptionsMenu();
  replot();
}

void GenericPlot::setShowTitle(bool st)
{
  show_title_ = st;
  this->replotExtras();
  checkoffOptionsMenu();
  replot();
}

void GenericPlot::setLineThickness(uint16_t th)
{
  line_thickness_ = th;
  for (int i=0; i < graphCount(); i++)
    setGraphThickness(graph(i));
}

void GenericPlot::setGridStyle(QString grd)
{
  if ((grd == "No grid") || (grd == "Grid") || (grd == "Grid + subgrid"))
  {
    current_grid_style_ = grd;
    xAxis->grid()->setVisible(current_grid_style_ != "No grid");
    yAxis->grid()->setVisible(current_grid_style_ != "No grid");
    xAxis->grid()->setSubGridVisible(current_grid_style_ == "Grid + subgrid");
    yAxis->grid()->setSubGridVisible(current_grid_style_ == "Grid + subgrid");
    checkoffOptionsMenu();
  }
}

void GenericPlot::setScaleType(QString scale_type)
{
  if (!scale_types_.count(scale_type))
    return;

  this->setCursor(Qt::WaitCursor);

  bool has2d {false};
  for (int i=0; i < plottableCount(); ++i)
    if (QCPColorMap *cm = qobject_cast<QCPColorMap*>(plottable(i)))
    {
      has2d = true;
      cm->setDataScaleType(scale_types_[scale_type]);
      //      cm->rescaleDataRange(true);
    }

  if (has2d)
  {
    for (int i=0; i < layerCount(); i++)
    {
      QCPLayer *this_layer = layer(i);
      for (auto &q : this_layer->children())
        if (QCPColorScale *colorScale = qobject_cast<QCPColorScale*>(q))
        {
          colorScale->axis()->setScaleType(scale_types_[scale_type]);
          colorScale->rescaleDataRange(true);
        }
    }
//    if (current_scale_type_ != scale_type)
//      rescaleAxes();
  }
  else
  {
    yAxis->setScaleType(scale_types_[scale_type]);
    this->zoomOut();
  }

  current_scale_type_ = scale_type;
  checkoffOptionsMenu();
  replot();
  this->setCursor(Qt::ArrowCursor);
}

void GenericPlot::setPlotStyle(QString stl)
{
  this->setCursor(Qt::WaitCursor);
  current_plot_style_ = stl;
  for (int i=0; i < graphCount(); i++)
    setGraphStyle(graph(i));
  checkoffOptionsMenu();
  //  replot();
  this->setCursor(Qt::ArrowCursor);
}

void GenericPlot::setGradient(QString grd)
{
  if (!gradients_.count(grd))
    return;

  this->setCursor(Qt::WaitCursor);
  current_gradient_ = grd;
  for (int i=0; i < plottableCount(); ++i)
    if (QCPColorMap *cm = qobject_cast<QCPColorMap*>(plottable(i)))
      cm->setGradient(gradients_[current_gradient_]);
  checkoffOptionsMenu();
  this->setCursor(Qt::ArrowCursor);
}

void GenericPlot::addCustomGradient(QString name, std::initializer_list<std::string> colors)
{
  QVector<QColor> cols;
  for (auto &c : colors)
    cols.push_back(QColor(QString::fromStdString(c)));

  if (name.isEmpty() || (cols.size() < 2))
    return;

  QCPColorGradient gr;
  gr.clearColorStops();
  for (int i=0; i < cols.size(); ++i)
    gr.setColorStopAt(double(i) / double(cols.size() - 1), cols[i]);
  gradients_[name] = gr;
}

void GenericPlot::addStandardGradients()
{
  gradients_["Grayscale"] = QCPColorGradient::gpGrayscale;
  gradients_["Hot"] =  QCPColorGradient::gpHot;
  gradients_["Cold"] = QCPColorGradient::gpCold;
  gradients_["Night"] = QCPColorGradient::gpNight;
  gradients_["Candy"] = QCPColorGradient::gpCandy;
  gradients_["Geography"] = QCPColorGradient::gpGeography;
  gradients_["Ion"] = QCPColorGradient::gpIon;
  gradients_["Thermal"] = QCPColorGradient::gpThermal;
  gradients_["Polar"] = QCPColorGradient::gpPolar;
  gradients_["Spectrum"] = QCPColorGradient::gpSpectrum;
  gradients_["Jet"] = QCPColorGradient::gpJet;
  gradients_["Hues"] = QCPColorGradient::gpHues;
}



QSize GenericPlot::sizeHint() const
{
  if (always_square_)
  {
    QSize s = size();
    previous_height_ = s.height();

    int extra_width = 0, extra_height = 0;
    for (int i=0; i < layerCount(); i++)
      for (auto &q : layer(i)->children())
        if (QCPAxisRect *le = qobject_cast<QCPAxisRect*>(q))
        {
          QMargins mar = le->margins();
          extra_width += (mar.left() + mar.right());
          extra_height += (mar.top() + mar.bottom());
        }

    s.setWidth(s.height() - extra_height + extra_width);
    s.setHeight(QCustomPlot::sizeHint().height());
    return s;
  }
  else
    return QCustomPlot::sizeHint();
}

void GenericPlot::resizeEvent(QResizeEvent * event)
{
  QCustomPlot::resizeEvent(event);
  // maybe this call should be scheduled for next iteration of event loop?
  if (always_square_ && (previous_height_ != height()))
    updateGeometry();
}

void GenericPlot::mousePressEvent(QMouseEvent *event)
{
  emit mousePress(event);

  Draggable *trc = qobject_cast<Draggable*>(itemAt(event->localPos(), true));
  if ((event->button() == Qt::LeftButton) && (trc != nullptr)) {
    trc->startMoving(event->localPos());
    return;
  }

  QCustomPlot::mousePressEvent(event);
}


void GenericPlot::mouseMoveEvent(QMouseEvent *event)
{
  emit mouseMove(event);
  double co_x, co_y;

  co_x = xAxis->pixelToCoord(event->x());
  co_y = yAxis->pixelToCoord(event->y());

  QVariant details;
  QCPLayerable *clickedLayerable = layerableAt(event->pos(), true, &details);
  if (QCPColorMap *ap = qobject_cast<QCPColorMap*>(clickedLayerable)) {
    int x = co_x, y = co_y;
    ap->data()->coordToCell(co_x, co_y, &x, &y);
    emit mouseHover(static_cast<double>(x), static_cast<double>(y));
  } //else
  //emit mouseHover(co_x, co_y);

  if (event->buttons() == Qt::NoButton) {
    Draggable *trc = qobject_cast<Draggable*>(itemAt(event->localPos(), false));
    Button *button = qobject_cast<Button*>(itemAt(event->localPos(), false));

    if (trc && trc->visible())
      setCursor(Qt::SizeHorCursor);
    else if (button && button->visible())
      setCursor(Qt::PointingHandCursor);
    else
      unsetCursor();
  }

  QCustomPlot::mouseMoveEvent(event);
}

void GenericPlot::mouseReleaseEvent(QMouseEvent *event)
{
  emit mouseRelease(event);

  if ((mMousePressPos-event->pos()).manhattanLength() < 5) {
    double co_x, co_y;

    co_x = xAxis->pixelToCoord(event->x());
    co_y = yAxis->pixelToCoord(event->y());

    QCPAbstractItem *ai = qobject_cast<QCPAbstractItem*>(itemAt(event->localPos(), false));

    if (Button *button = qobject_cast<Button*>(ai))
      if (button->visible())
        this->executeButton(button);

    if (!ai)
    {
      QVariant details;
      QCPLayerable *clickedLayerable = layerableAt(event->pos(), false, &details);
      if (QCPColorMap *ap = qobject_cast<QCPColorMap*>(clickedLayerable))
      {
        int xx, yy;
        ap->data()->coordToCell(co_x, co_y, &xx, &yy);
        this->mouseClicked(static_cast<double>(xx), static_cast<double>(yy), event);
      }
      else
        this->mouseClicked(co_x, co_y, event);
    }
  }
  QCustomPlot::mouseReleaseEvent(event);
}

void GenericPlot::executeButton(Button *button)
{
  if (button->name() == "options") {
    options_menu_.exec(QCursor::pos());
  } else if (button->name() == "export") {
    export_menu_.exec(QCursor::pos());
  } else if (button->name() == "reset_scales") {
    this->zoomOut();
  }
}

void GenericPlot::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Shift) {
    emit shiftStateChanged(true);
  }
  QCustomPlot::keyPressEvent(event);
}

void GenericPlot::keyReleaseEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Shift) {
    emit shiftStateChanged(false);
  }
  QCustomPlot::keyReleaseEvent(event);
}

void GenericPlot::plotButtons()
{
  Button *previousButton {nullptr};
  Button *newButton {nullptr};

  if (visible_options_ & ShowOptions::zoom)
  {
    newButton = new Button(this,
                           QPixmap(":/icons/oxy/22/view_fullscreen.png"),
                           "reset_scales", "Zoom out",
                           Qt::AlignBottom | Qt::AlignRight);
    newButton->setClipToAxisRect(false);
    setButtonPosition(newButton->topLeft, nullptr);
    previousButton = newButton;
  }

  if (!options_menu_.isEmpty())
  {
    newButton = new Button(this, QPixmap(":/icons/oxy/22/view_statistics.png"),
                           "options", "Style options",
                           Qt::AlignBottom | Qt::AlignRight);
    newButton->setClipToAxisRect(false);
    setButtonPosition(newButton->topLeft, previousButton);
    previousButton = newButton;
  }

  if (visible_options_ & ShowOptions::save)
  {
    newButton = new Button(this,
                           QPixmap(":/icons/oxy/22/document_save.png"),
                           "export", "Export plot",
                           Qt::AlignBottom | Qt::AlignRight);
    newButton->setClipToAxisRect(false);
    setButtonPosition(newButton->topLeft, previousButton);
  }
}

void GenericPlot::setButtonPosition(QCPItemPosition* pos, Button* previous)
{
  if (previous)
  {
    pos->setParentAnchor(previous->bottomLeft);
    pos->setCoords(0, 5);
  }
  else
  {
    pos->setType(QCPItemPosition::ptAbsolute);
    pos->setCoords(5, 5);
  }
}


void GenericPlot::exportPlot(QAction* choice)
{
  QString filter = choice->text() + "(*." + choice->text() + ")";
  QString fileName = CustomSaveFileDialog(this, "Export plot",
                                          QStandardPaths::locate(QStandardPaths::HomeLocation, ""),
                                          filter);
  if (!validateFile(this, fileName, true))
    return;

  rescaleEverything(5, 2, 20, false);

  QFileInfo file(fileName);
  if (file.suffix() == "png")
    savePng(fileName,0,0,1,100);
  else if (file.suffix() == "jpg")
    saveJpg(fileName,0,0,1,100);
  else if (file.suffix() == "bmp")
    saveBmp(fileName);
  else if (file.suffix() == "pdf")
    savePdf(fileName, true);

  rescaleEverything(-5, -2, -20, true);
  replot();
  updateGeometry();
}

void GenericPlot::rescaleEverything(int fontUpscale, int plotThicken, int marginUpscale,
                                    bool buttons_visible)
{
  for (int i = 0; i < graphCount(); ++i)
  {
    QCPGraph *graph = this->graph(i);
    QPen pen = graph->pen();
    pen.setWidth(pen.width() + plotThicken);
    graph->setPen(pen);
  }
  for (int i = 0; i < itemCount(); ++i)
  {
    QCPAbstractItem* itm = item(i);
    if (QCPItemLine *line = qobject_cast<QCPItemLine*>(itm))
    {
      QCPLineEnding head = line->head();
      head.setWidth(head.width() + fontUpscale);
      head.setLength(head.length() + fontUpscale);
      line->setHead(head);
      line->start->setCoords(0, -50);
      line->end->setCoords(0, -15);
    }
    else if (QCPItemText *txt = qobject_cast<QCPItemText*>(itm))
    {
      txt->setFont(rescaleFont(txt->font(), fontUpscale));
      txt->position->setCoords(0, -50);
    }
    else if (Button *btn = qobject_cast<Button*>(itm))
      btn->setVisible(buttons_visible);
  }

  bool has_colormap {false};

  for (int i=0; i < layerCount(); i++)
  {
    QCPLayer *this_layer = layer(i);
    for (auto &q : this_layer->children()) {
      if (QCPColorScale *cs = qobject_cast<QCPColorScale*>(q))
      {
        cs->axis()->setLabelFont(rescaleFont(cs->axis()->labelFont(), fontUpscale));
        cs->axis()->setTickLabelFont(rescaleFont(cs->axis()->tickLabelFont(), fontUpscale));
        cs->axis()->setPadding(cs->axis()->padding() + marginUpscale);
        has_colormap = true;
      }
      else if (QCPAxisRect *ar = qobject_cast<QCPAxisRect*>(q))
      {
        for (auto ax : ar->axes())
        {
          ax->setLabelFont(rescaleFont(ax->labelFont(), fontUpscale));
          ax->setTickLabelFont(rescaleFont(ax->tickLabelFont(), fontUpscale));
          ax->setPadding(ax->padding() + marginUpscale);
        }
      }
    }
  }

  if (has_colormap)
    setScaleType(current_scale_type_);
}

QFont GenericPlot::rescaleFont(QFont font, double size_offset)
{
  font.setPointSize(font.pointSize() + size_offset);
  return font;
}

void GenericPlot::setGraphThickness(QCPGraph* graph)
{
  if (visible_options_ & ShowOptions::thickness)
  {
    QPen pen = graph->pen();
    pen.setWidth(line_thickness_);
    graph->setPen(pen);
  }
}

void GenericPlot::setGraphStyle(QCPGraph* graph, QString style)
{
  if (style.isEmpty())
    style = current_plot_style_;

  if (style == "Fill") {
    graph->setBrush(QBrush(graph->pen().color()));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
  } else if (style == "Lines") {
    graph->setBrush(QBrush());
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
  } else if (style == "Step center") {
    graph->setBrush(QBrush());
    graph->setLineStyle(QCPGraph::lsStepCenter);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
  } else if (style == "Step left") {
    graph->setBrush(QBrush());
    graph->setLineStyle(QCPGraph::lsStepLeft);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
  } else if (style == "Step right") {
    graph->setBrush(QBrush());
    graph->setLineStyle(QCPGraph::lsStepRight);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
  } else if (style == "Scatter") {
    graph->setBrush(QBrush());
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc));
    graph->setLineStyle(QCPGraph::lsNone);
  }

  setGraphThickness(graph);
}



void GenericPlot::optionsChanged(QAction* action)
{
  this->setCursor(Qt::WaitCursor);
  QString choice = action->text();

  if (scale_types_.count(choice))
    setScaleType(choice);
  else if (grid_styles_.count(choice))
    setGridStyle(choice);
  else if (choice == "Show marker labels")
    setShowMarkerLabels(!show_marker_labels_);
  else if (choice == "Show title")
    setShowTitle(!show_title_);
  else if (choice == "Antialiased")
    setAntialiased(!antialiased_);
  else if (plot_styles_.count(choice))
    setPlotStyle(choice);
  else if (gradients_.count(choice))
    setGradient(choice);
  else if (choice == "Show gradient scale")
    setShowGradientLegend(!show_gradient_legend_);
  else if ((choice == "1") || (choice == "2") || (choice == "3"))
    setLineThickness(choice.toInt());

  checkoffOptionsMenu();
  this->setCursor(Qt::ArrowCursor);
}

void GenericPlot::rebuildOptionsMenu()
{
  options_menu_.clear();

  if (visible_options_ & ShowOptions::scale)
  {
    for (auto &s : scale_types_)
      options_menu_.addAction(s.first);
    options_menu_.addSeparator();
  }

  if (visible_options_ & ShowOptions::labels)
  {
    options_menu_.addAction("Show marker labels");
    options_menu_.addSeparator();
  }

  if (visible_options_ & ShowOptions::title)
  {
    options_menu_.addAction("Show title");
    options_menu_.addSeparator();
  }

  if (visible_options_ & ShowOptions::style)
  {
    for (auto &s : plot_styles_)
      options_menu_.addAction(s);
    options_menu_.addSeparator();
  }

  if (visible_options_ & ShowOptions::thickness)
  {
    options_menu_.addAction("1");
    options_menu_.addAction("2");
    options_menu_.addAction("3");
    options_menu_.addSeparator();
  }

  if (visible_options_ & ShowOptions::gradients)
  {
    if (!gradients_.empty())
    {
      options_menu_.addAction("Show gradient scale");
      options_menu_.addSeparator();
    }
    for (auto &q : gradients_)
      options_menu_.addAction(q.first);
    options_menu_.addSeparator();
  }

  if (visible_options_ & ShowOptions::grid)
  {
    for (auto &s : grid_styles_)
      options_menu_.addAction(s);
    options_menu_.addSeparator();
  }

  if (visible_options_ & ShowOptions::dither)
  {
    options_menu_.addAction("Antialiased");
  }

  checkoffOptionsMenu();
}

void GenericPlot::checkoffOptionsMenu()
{
  for (auto &q : options_menu_.actions())
  {
    q->setCheckable(true);
    q->setChecked((q->text() == current_scale_type_) ||
                  (q->text() == current_plot_style_) ||
                  (q->text() == current_grid_style_) ||
                  (q->text() == current_gradient_) ||
                  (q->text() == QString::number(line_thickness_)) ||
                  ((q->text() == "Show marker labels") && show_marker_labels_) ||
                  ((q->text() == "Show title") && show_title_) ||
                  ((q->text() == "Show gradient scale") && show_gradient_legend_)||
                  ((q->text() == "Antialiased") && antialiased_));
  }
}


}
