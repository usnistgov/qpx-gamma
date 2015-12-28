/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * This software can be redistributed and/or modified freely provided that
 * any derivative works bear some notice that they are derived from it, and
 * any modified versions bear some notice that they have been modified.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *
 ******************************************************************************/

#include "widget_plot2d.h"
#include "ui_widget_plot2d.h"
#include "qt_util.h"


WidgetPlot2D::WidgetPlot2D(QWidget *parent) :
  QWidget(parent),
  scale_types_({{"Linear", QCPAxis::stLinear}, {"Logarithmic", QCPAxis::stLogarithmic}}),
  gradients_({{"Grayscale", QCPColorGradient::gpGrayscale}, {"Hot",  QCPColorGradient::gpHot},
{"Cold", QCPColorGradient::gpCold}, {"Night", QCPColorGradient::gpNight},
{"Candy", QCPColorGradient::gpCandy}, {"Geography", QCPColorGradient::gpGeography},
{"Ion", QCPColorGradient::gpIon}, {"Thermal", QCPColorGradient::gpThermal},
{"Polar", QCPColorGradient::gpPolar}, {"Spectrum", QCPColorGradient::gpSpectrum},
{"Jet", QCPColorGradient::gpJet}, {"Hues", QCPColorGradient::gpHues}}),
  ui(new Ui::WidgetPlot2D)
{
  ui->setupUi(this);

  current_gradient_ = "Hot";
  current_scale_type_ = "Logarithmic";
  show_gradient_scale_ = false;
  bits_ = 0;
  antialiased_ = true;
  show_labels_ = true;

  //color theme setup
  my_marker.appearance.themes["Grayscale"] = QPen(Qt::cyan, 1);
  my_marker.appearance.themes["Hot"] = QPen(Qt::cyan, 1);
  my_marker.appearance.themes["Cold"] = QPen(Qt::yellow, 1);
  my_marker.appearance.themes["Night"] = QPen(Qt::red, 1);
  my_marker.appearance.themes["Candy"] = QPen(Qt::red, 1);
  my_marker.appearance.themes["Geography"] = QPen(Qt::yellow, 1);
  my_marker.appearance.themes["Ion"] = QPen(Qt::magenta, 1);
  my_marker.appearance.themes["Thermal"] = QPen(Qt::cyan, 1);
  my_marker.appearance.themes["Polar"] = QPen(Qt::green, 1);
  my_marker.appearance.themes["Spectrum"] = QPen(Qt::cyan, 1);
  my_marker.appearance.themes["Jet"] = QPen(Qt::darkMagenta, 1);
  my_marker.appearance.themes["Hues"] = QPen(Qt::black, 1);

  //colormap setup
  ui->coincPlot->setAlwaysSquare(true);
  colorMap = new QCPColorMap(ui->coincPlot->xAxis, ui->coincPlot->yAxis);
  colorMap->clearData();
  ui->coincPlot->addPlottable(colorMap);
  colorMap->setInterpolate(antialiased_);
  colorMap->setTightBoundary(true); //really?
  ui->coincPlot->axisRect()->setupFullAxesBox();
  ui->coincPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
  ui->coincPlot->setNoAntialiasingOnDrag(true);

  colorMap->setGradient(gradients_[current_gradient_]);
  colorMap->setDataScaleType(scale_types_[current_scale_type_]);
  colorMap->rescaleDataRange(true);
  connect(ui->coincPlot, SIGNAL(mouse_upon(double,double)), this, SLOT(plot_2d_mouse_upon(double,double)));
  connect(ui->coincPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*, bool)), this, SLOT(plot_2d_mouse_clicked(double,double,QMouseEvent*, bool)));
  //connect(ui->coincPlot, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(clicked_plottable(QCPAbstractPlottable*)));
  connect(ui->coincPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selection_changed()));
  connect(ui->coincPlot, SIGNAL(clickedAbstractItem(QCPAbstractItem*)), this, SLOT(clicked_item(QCPAbstractItem*)));

  menuExportFormat.addAction("png");
  menuExportFormat.addAction("jpg");
  menuExportFormat.addAction("pdf");
  menuExportFormat.addAction("bmp");
  connect(&menuExportFormat, SIGNAL(triggered(QAction*)), this, SLOT(exportRequested(QAction*)));
  connect(&menuOptions, SIGNAL(triggered(QAction*)), this, SLOT(optionsChanged(QAction*)));

  build_menu();
  plot_2d_mouse_upon(0, 0);
}

void WidgetPlot2D::clicked_item(QCPAbstractItem* itm) {
  if (QCPItemPixmap *pix = qobject_cast<QCPItemPixmap*>(itm)) {
//    QPoint p = this->mapFromGlobal(QCursor::pos());
    QString name = pix->property("button_name").toString();
    if (name == "options") {
      menuOptions.exec(QCursor::pos());
    } else if (name == "export") {
      menuExportFormat.exec(QCursor::pos());
    } else if (name == "reset_scales") {
      zoom_out();
    }
  }
}

void WidgetPlot2D::build_menu() {
  menuOptions.clear();
  for (auto &q : gradients_)
    menuOptions.addAction(q.first);

  menuOptions.addSeparator();
  menuOptions.addAction("Logarithmic");
  menuOptions.addAction("Linear");

  menuOptions.addSeparator();
  menuOptions.addAction("Show gradient scale");

  menuOptions.addSeparator();
  menuOptions.addAction("Show marker labels");

  menuOptions.addSeparator();
  menuOptions.addAction("Antialiased");

  for (auto &q : menuOptions.actions()) {
    q->setCheckable(true);
    q->setChecked((q->text() == current_scale_type_) ||
                  (q->text() == current_gradient_) ||
                  ((q->text() == "Show gradient scale") && show_gradient_scale_)||
                  ((q->text() == "Show marker labels") && show_labels_)||
                  ((q->text() == "Antialiased") && antialiased_));
  }
}


void WidgetPlot2D::optionsChanged(QAction* action) {
  this->setCursor(Qt::WaitCursor);
  QString choice = action->text();
  if (scale_types_.count(choice)) {
    current_scale_type_ = choice;
    colorMap->setDataScaleType(scale_types_[current_scale_type_]);
    colorMap->rescaleDataRange(true);
    ui->coincPlot->replot();
  } else if (gradients_.count(choice)) {
    current_gradient_ = choice;
    colorMap->setGradient(gradients_[current_gradient_]);
    replot_markers();
  } else if (choice == "Show gradient scale") {
    show_gradient_scale_ = !show_gradient_scale_;
    toggle_gradient_scale();
  } else if (choice == "Show marker labels") {
    show_labels_ = !show_labels_;
    replot_markers();
  } else if (choice == "Antialiased") {
    antialiased_= !antialiased_;
    colorMap->setInterpolate(antialiased_);
  }

  build_menu();
  this->setCursor(Qt::ArrowCursor);
}


WidgetPlot2D::~WidgetPlot2D()
{
  delete ui;
}

void WidgetPlot2D::selection_changed() {
  emit stuff_selected();
}

void WidgetPlot2D::set_range_x(MarkerBox2D range) {
  range_ = range;
}


std::list<MarkerBox2D> WidgetPlot2D::get_selected_boxes() {
  std::list<MarkerBox2D> selection;
  for (auto &q : ui->coincPlot->selectedItems())
    if (QCPItemRect *b = qobject_cast<QCPItemRect*>(q)) {
      MarkerBox2D box;
      box.x_c.set_bin(b->property("chan_x").toDouble(), bits_, calib_x_);
      box.y_c.set_bin(b->property("chan_y").toDouble(), bits_, calib_y_);
      selection.push_back(box);
      //PL_DBG << "found selected " << txt->property("true_value").toDouble() << " chan=" << txt->property("chan_value").toDouble();
    }
  return selection;
}

std::list<MarkerLabel2D> WidgetPlot2D::get_selected_labels() {
  std::list<MarkerLabel2D> selection;
  for (auto &q : ui->coincPlot->selectedItems())
    if (QCPItemText *b = qobject_cast<QCPItemText*>(q)) {
      MarkerLabel2D label;
      label.x.set_bin(b->property("chan_x").toDouble(), bits_, calib_x_);
      label.y.set_bin(b->property("chan_y").toDouble(), bits_, calib_y_);
      selection.push_back(label);
      //PL_DBG << "found selected " << txt->property("true_value").toDouble() << " chan=" << txt->property("chan_value").toDouble();
    }
  return selection;
}

void WidgetPlot2D::set_boxes(std::list<MarkerBox2D> boxes) {
  boxes_ = boxes;
}

void WidgetPlot2D::set_labels(std::list<MarkerLabel2D> labels) {
  labels_ = labels;
}


void WidgetPlot2D::reset_content() {
  //PL_DBG << "reset content";
  colorMap->clearData();
  boxes_.clear();
  labels_.clear();
  replot_markers();
}

void WidgetPlot2D::refresh()
{
  ui->coincPlot->replot();
}

void WidgetPlot2D::replot_markers() {
  //PL_DBG << "replot markers";
  double minx = std::numeric_limits<double>::max();
  double maxx = - std::numeric_limits<double>::max();
  double miny = std::numeric_limits<double>::max();
  double maxy = - std::numeric_limits<double>::max();
  //int total_markers = 0;

  ui->coincPlot->clearItems();

  QPen pen = my_marker.appearance.get_pen(current_gradient_);

  QColor cc = pen.color();
  cc.setAlpha(169);
  pen.setColor(cc);
  pen.setWidth(2);

  QPen pen2 = pen;
  cc.setAlpha(48);
  pen2.setColor(cc);

  QPen pen_strong = pen;
  cc.setAlpha(255);
  cc.setHsv((cc.hsvHue() + 180) % 180, 255, 128, 255);
  pen_strong.setColor(cc);
  pen_strong.setWidth(3);

  QPen pen_strong2 = pen_strong;
  cc.setAlpha(64);
  pen_strong2.setColor(cc);

  QCPItemRect *box;

  for (auto &q : boxes_) {
    if (!q.visible)
      continue;
    box = new QCPItemRect(ui->coincPlot);
    box->setSelectable(q.selectable);
    if (q.selectable) {
      box->setPen(pen);
      box->setBrush(QBrush(pen2.color()));
      box->setSelected(q.selected);
      QColor sel = box->selectedPen().color();
      box->setSelectedBrush(QBrush(QColor::fromHsv(sel.hsvHue(), sel.saturation(), sel.value(), 48)));
    } else {
      box->setPen(pen);
    }

    double xc(0), x1(0), x2(0), yc(0), y1(0), y2(0);

    if (calib_x_.valid()) {
      xc = q.x_c.energy();
      x1 = q.x1.energy();
      x2 = q.x2.energy();
    } else {
      xc = q.x_c.bin(bits_);
      x1 = q.x1.bin(bits_);
      x2 = q.x2.bin(bits_);
    }

    if (calib_y_.valid()) {
      yc = q.y_c.energy();
      y1 = q.y1.energy();
      y2 = q.y2.energy();
    } else {
      yc = q.y_c.bin(bits_);
      y1 = q.y1.bin(bits_);
      y2 = q.y2.bin(bits_);
    }

    box->setProperty("chan_x", q.x_c.bin(bits_));
    box->setProperty("chan_y", q.y_c.bin(bits_));
    box->setProperty("nrg_x", q.x_c.energy());
    box->setProperty("nrg_y", q.y_c.energy());
    box->topLeft->setCoords(x1, y1);
    box->bottomRight->setCoords(x2, y2);
    ui->coincPlot->addItem(box);

    if (q.selected) {
//      total_markers++;
      if (q.vertical) {
        if (x2 > maxx)
          maxx = x2;
        if (x1 < minx)
          minx = x1;
      }

      if (q.horizontal) {
        if (y2 > maxy)
          maxy = y2;
        if (y1 < miny)
          miny = y1;
      }
    }

    if (q.mark_center) {
      QCPItemLine *linev = new QCPItemLine(ui->coincPlot);
      QCPItemLine *lineh = new QCPItemLine(ui->coincPlot);
      lineh->setPen(pen_strong);
      linev->setPen(pen_strong);
//      linev->setSelectedPen(QPen(QColor::fromHsv(sel.hsvHue(), sel.saturation(), sel.value(), 48)));
//      lineh->setSelectedPen(QPen(QColor::fromHsv(sel.hsvHue(), sel.saturation(), sel.value(), 48)));
      linev->setSelected(q.selected);
      lineh->setSelected(q.selected);
      lineh->setTail(QCPLineEnding::esBar);
      lineh->setHead(QCPLineEnding::esBar);
      linev->setTail(QCPLineEnding::esBar);
      linev->setHead(QCPLineEnding::esBar);
      lineh->start->setCoords(x1, yc);
      lineh->end->setCoords(x2, yc);
      linev->start->setCoords(xc, y1);
      linev->end->setCoords(xc, y2);
      ui->coincPlot->addItem(lineh);
      ui->coincPlot->addItem(linev);
    }

  }

  if (show_labels_) {
    for (auto &q : labels_) {
      QCPItemText *labelItem = new QCPItemText(ui->coincPlot);
      //    labelItem->setClipToAxisRect(false);
      labelItem->setText(q.text);

      labelItem->setProperty("chan_x", q.x.bin(bits_));
      labelItem->setProperty("chan_y", q.y.bin(bits_));
      labelItem->setProperty("nrg_x", q.x.energy());
      labelItem->setProperty("nrg_y", q.y.energy());

      labelItem->position->setType(QCPItemPosition::ptPlotCoords);

      double x = 0, y = 0;

      if (calib_x_.valid())
        x = q.x.energy();
      else
        x = q.x.bin(bits_);

      if (calib_y_.valid())
        y = q.y.energy();
      else
        y = q.y.bin(bits_);

      if (q.hfloat) {
        labelItem->position->setTypeX(QCPItemPosition::ptAxisRectRatio);
        x = 0.90;
      }
      if (q.vfloat) {
        labelItem->position->setTypeY(QCPItemPosition::ptAxisRectRatio);
        y = 0.10;
      }

      labelItem->position->setCoords(x, y);
      labelItem->position->setCoords(x, y);

      if (q.vertical) {
        labelItem->setRotation(90);
        labelItem->setPositionAlignment(Qt::AlignTop|Qt::AlignRight);
      } else
        labelItem->setPositionAlignment(Qt::AlignTop|Qt::AlignLeft);

      labelItem->setFont(QFont("Helvetica", 10));
      labelItem->setSelectable(q.selectable);
      labelItem->setSelected(q.selected);

      labelItem->setColor(pen_strong.color());
      labelItem->setPen(pen_strong);
      labelItem->setBrush(QBrush(Qt::white));

      QColor sel = labelItem->selectedColor();
      QPen selpen(QColor::fromHsv(sel.hsvHue(), sel.saturation(), sel.value(), 255));
      selpen.setWidth(3);
      labelItem->setSelectedPen(selpen);
      labelItem->setSelectedBrush(QBrush(Qt::white));

      labelItem->setPadding(QMargins(1, 1, 1, 1));

      ui->coincPlot->addItem(labelItem);
    }
  }


  if (boxes_.size()) {
    ui->coincPlot->setInteraction(QCP::iSelectItems, true);
    ui->coincPlot->setInteraction(QCP::iMultiSelect, false);
  } else {
    ui->coincPlot->setInteraction(QCP::iSelectItems, false);
    ui->coincPlot->setInteraction(QCP::iMultiSelect, false);
  }

  if (range_.visible) {
    QCPItemRect *box = new QCPItemRect(ui->coincPlot);
    box->setSelectable(false);
    box->setPen(pen_strong);
    box->setBrush(QBrush(pen_strong2.color()));
    double x1 = range_.x1.bin(bits_);
    double y1 = range_.y1.bin(bits_);
    double x2 = range_.x2.bin(bits_);
    double y2 = range_.y2.bin(bits_);
    if (calib_x_.valid()) {
      x1 = range_.x1.energy();
      y1 = range_.y1.energy();
    }
    if (calib_y_.valid()) {
      x2 = range_.x2.energy();
      y2 = range_.y2.energy();
    }

    box->topLeft->setType(QCPItemPosition::ptPlotCoords);
    box->topLeft->setCoords(x1, y1);
    box->bottomRight->setType(QCPItemPosition::ptPlotCoords);
    box->bottomRight->setCoords(x2, y2);
    ui->coincPlot->addItem(box);

    if (range_.vertical) {
      if (x2 > maxx)
        maxx = x2;
      if (x1 < minx)
        minx = x1;
    }
    if (range_.horizontal) {
      if (y2 > maxy)
        maxy = y2;
      if (y1 < miny)
        miny = y1;
    }
  }

  QCPItemPixmap *overlayButton;

  overlayButton = new QCPItemPixmap(ui->coincPlot);
  overlayButton->setClipToAxisRect(false);
  overlayButton->setPixmap(QPixmap(":/new/icons/oxy/view_fullscreen.png"));
  overlayButton->topLeft->setType(QCPItemPosition::ptAbsolute);
  overlayButton->topLeft->setCoords(5, 5);
  overlayButton->bottomRight->setParentAnchor(overlayButton->topLeft);
  overlayButton->bottomRight->setCoords(18, 18);
  overlayButton->setScaled(true);
  overlayButton->setSelectable(false);
  overlayButton->setProperty("button_name", QString("reset_scales"));
  overlayButton->setProperty("tooltip", QString("Reset plot scales"));
  ui->coincPlot->addItem(overlayButton);

  if (!menuOptions.isEmpty()) {
    QCPItemPixmap *newButton = new QCPItemPixmap(ui->coincPlot);
    newButton->setClipToAxisRect(false);
    newButton->setPixmap(QPixmap(":/new/icons/oxy/view_statistics.png"));
    newButton->topLeft->setType(QCPItemPosition::ptAbsolute);
    newButton->topLeft->setParentAnchor(overlayButton->bottomLeft);
    newButton->topLeft->setCoords(0, 5);
    newButton->bottomRight->setParentAnchor(newButton->topLeft);
    newButton->bottomRight->setCoords(18, 18);
    newButton->setScaled(false);
    newButton->setSelectable(false);
    newButton->setProperty("button_name", QString("options"));
    newButton->setProperty("tooltip", QString("Style options"));
    ui->coincPlot->addItem(newButton);
    overlayButton = newButton;
  }

  QCPItemPixmap *newButton = new QCPItemPixmap(ui->coincPlot);
  newButton->setClipToAxisRect(false);
  newButton->setPixmap(QPixmap(":/new/icons/oxy/document_save.png"));
  newButton->topLeft->setType(QCPItemPosition::ptAbsolute);
  newButton->topLeft->setParentAnchor(overlayButton->bottomLeft);
  newButton->topLeft->setCoords(0, 5);
  newButton->bottomRight->setParentAnchor(newButton->topLeft);
  newButton->bottomRight->setCoords(18, 18);
  newButton->setScaled(true);
  newButton->setSelectable(false);
  newButton->setProperty("button_name", QString("export"));
  newButton->setProperty("tooltip", QString("Export plot"));
  ui->coincPlot->addItem(newButton);
  overlayButton = newButton;

  int margin = 50;
  bool xaxis_changed = false;
  bool yaxis_changed = false;
  double difx_lower = minx - ui->coincPlot->xAxis->range().lower;
  double difx_upper = maxx - ui->coincPlot->xAxis->range().upper;
  double dify_lower = miny - ui->coincPlot->yAxis->range().lower;
  double dify_upper = maxy - ui->coincPlot->yAxis->range().upper;
  if (difx_upper > 0) {
    ui->coincPlot->xAxis->setRangeUpper(maxx + margin);
    if (difx_lower > (difx_upper + margin))
      ui->coincPlot->xAxis->setRangeLower(ui->coincPlot->xAxis->range().lower + difx_upper + margin);
    xaxis_changed = true;
  }

  if (difx_lower < 0) {
    ui->coincPlot->xAxis->setRangeLower(minx - margin);
    if (difx_upper < (difx_lower - margin))
      ui->coincPlot->xAxis->setRangeUpper(ui->coincPlot->xAxis->range().upper + difx_lower - margin);
    xaxis_changed = true;
  }

  if (dify_upper > 0) {
    ui->coincPlot->yAxis->setRangeUpper(maxy + margin);
    if (dify_lower > (dify_upper + margin))
      ui->coincPlot->yAxis->setRangeLower(ui->coincPlot->yAxis->range().lower + dify_upper + margin);
    yaxis_changed = true;
  }

  if (dify_lower < 0) {
    ui->coincPlot->yAxis->setRangeLower(miny - margin);
    if (dify_upper < (dify_lower - margin))
      ui->coincPlot->yAxis->setRangeUpper(ui->coincPlot->yAxis->range().upper + dify_lower - margin);
    yaxis_changed = true;
  }


//  if (xaxis_changed) {
//    ui->coincPlot->replot();
//    plot_rezoom();
//  }

  ui->coincPlot->replot();
}

void WidgetPlot2D::update_plot(uint64_t size, std::shared_ptr<Qpx::Spectrum::EntryList> spectrum_data) {
  //  PL_DBG << "updating 2d";

  ui->coincPlot->clearGraphs();
  colorMap->clearData();

  if ((size > 0) && (spectrum_data->size())) {
    colorMap->data()->setSize(size, size);
    for (auto it : *spectrum_data)
      colorMap->data()->setCell(it.first[0], it.first[1], it.second.convert_to<double>());
    colorMap->rescaleDataRange(true);
    ui->coincPlot->updateGeometry();
  } else {
    ui->coincPlot->clearGraphs();
    colorMap->clearData();
    colorMap->keyAxis()->setLabel("");
    colorMap->valueAxis()->setLabel("");
  }

  replot_markers();
}

void WidgetPlot2D::set_axes(Gamma::Calibration cal_x, Gamma::Calibration cal_y, int bits) {
  calib_x_ = cal_x;
  calib_y_ = cal_y;
  bits_ = bits;

  colorMap->keyAxis()->setLabel(/*QString::fromStdString(detector_x_.name_) + */ "Energy (" + QString::fromStdString(calib_x_.units_) + ")");
  colorMap->valueAxis()->setLabel(/*QString::fromStdString(detector_y_.name_) + */ "Energy (" + QString::fromStdString(calib_y_.units_) + ")");
  colorMap->data()->setRange(QCPRange(0, calib_x_.transform(colorMap->data()->keySize() - 1, bits_)),
                             QCPRange(0, calib_y_.transform(colorMap->data()->keySize() - 1, bits_)));
  ui->coincPlot->rescaleAxes();
}


void WidgetPlot2D::plot_2d_mouse_upon(double x, double y) {
//  colorMap->keyAxis()->setLabel("Energy (" + QString::fromStdString(calib_x_.units_) + "): " + QString::number(calib_x_.transform(x, bits_)));
//  colorMap->valueAxis()->setLabel("Energy (" + QString::fromStdString(calib_y_.units_) + "): " + QString::number(calib_y_.transform(y, bits_)));
//  ui->coincPlot->replot();
}

void WidgetPlot2D::plot_2d_mouse_clicked(double x, double y, QMouseEvent *event, bool channels) {
//  PL_INFO << "<WidgetPlot2D> mouse clicked at " << x << " & " << y << " chans?=" << channels;

  bool visible = (event->button() == Qt::LeftButton);

  Marker xt, yt;

  if (channels) {
//    PL_DBG << "markers chan";
    xt.pos.set_bin(x, bits_, calib_x_);
    yt.pos.set_bin(y, bits_, calib_y_);
//    PL_DBG << "xformed to nrg " << xt.pos.energy() << " " << yt.pos.energy();
  } else {
//    PL_DBG << "markers nrg";
    xt.pos.set_energy(x, calib_x_);
    yt.pos.set_energy(y, calib_y_);
  }

//  if (calib_x_.valid())
//    xt.pos.set_energy(x, calib_x_);
//  else
//    xt.pos.set_bin(x, bits_, calib_x_);

//  if (calib_y_.valid())
//    yt.pos.set_energy(y, calib_y_);
//  else
//    yt.pos.set_bin(y, bits_, calib_y_);

  xt.visible = visible;
  yt.visible = visible;

  emit markers_set(xt, yt);
}


void WidgetPlot2D::zoom_out()
{
  this->setCursor(Qt::WaitCursor);
  ui->coincPlot->rescaleAxes();
  ui->coincPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}


void WidgetPlot2D::toggle_gradient_scale()
{
  if (show_gradient_scale_) {
    //add color scale
    QCPColorScale *colorScale = new QCPColorScale(ui->coincPlot);
    ui->coincPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("Event count");

    //readjust margins
    QCPMarginGroup *marginGroup = new QCPMarginGroup(ui->coincPlot);
    ui->coincPlot->axisRect()->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);

    colorMap->setGradient(gradients_[current_gradient_]);
    colorMap->setDataScaleType(scale_types_[current_scale_type_]);
    colorMap->rescaleDataRange(true);

    colorScale->axis()->setScaleLogBase(10);
    colorScale->axis()->setNumberFormat("gbc");
    colorScale->axis()->setNumberPrecision(0);
    colorScale->axis()->setRangeLower(1);

    ui->coincPlot->replot();
    ui->coincPlot->updateGeometry();
  } else {
    for (int i=0; i < ui->coincPlot->plotLayout()->elementCount(); i++) {
      if (QCPColorScale *le = qobject_cast<QCPColorScale*>(ui->coincPlot->plotLayout()->elementAt(i)))
        ui->coincPlot->plotLayout()->remove(le);
    }
    ui->coincPlot->plotLayout()->simplify();
    ui->coincPlot->updateGeometry();
  }
}

void WidgetPlot2D::set_scale_type(QString sct) {
  this->setCursor(Qt::WaitCursor);
  current_scale_type_ = sct;
  colorMap->setDataScaleType(scale_types_[current_scale_type_]);
  colorMap->rescaleDataRange(true);
  ui->coincPlot->replot();
  build_menu();
  this->setCursor(Qt::ArrowCursor);
}

QString WidgetPlot2D::scale_type() {
  return current_scale_type_;
}

void WidgetPlot2D::set_gradient(QString grd) {
  this->setCursor(Qt::WaitCursor);
  current_gradient_ = grd;
  colorMap->setGradient(gradients_[current_gradient_]);
  replot_markers();
  build_menu();
  this->setCursor(Qt::ArrowCursor);
}

QString WidgetPlot2D::gradient() {
  return current_gradient_;
}

void WidgetPlot2D::set_show_legend(bool show) {
  show_gradient_scale_ = show;
  toggle_gradient_scale();
  build_menu();
}

bool WidgetPlot2D::show_legend() {
  return show_gradient_scale_;
}

void WidgetPlot2D::exportRequested(QAction* choice) {
  QString filter = choice->text() + "(*." + choice->text() + ")";
  QString fileName = CustomSaveFileDialog(this, "Export plot",
                                          QStandardPaths::locate(QStandardPaths::HomeLocation, ""),
                                          filter);
  if (validateFile(this, fileName, true)) {
    QFileInfo file(fileName);
    if (file.suffix() == "png") {
      //PL_INFO << "Exporting plot to png " << fileName.toStdString();
      ui->coincPlot->savePng(fileName,0,0,1,100);
    } else if (file.suffix() == "jpg") {
      //PL_INFO << "Exporting plot to jpg " << fileName.toStdString();
      ui->coincPlot->saveJpg(fileName,0,0,1,100);
    } else if (file.suffix() == "bmp") {
      //PL_INFO << "Exporting plot to bmp " << fileName.toStdString();
      ui->coincPlot->saveBmp(fileName);
    } else if (file.suffix() == "pdf") {
      //PL_INFO << "Exporting plot to pdf " << fileName.toStdString();
      ui->coincPlot->savePdf(fileName, true);
    }
  }
}
