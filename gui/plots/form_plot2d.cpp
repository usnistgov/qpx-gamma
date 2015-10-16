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

#include "form_plot2d.h"
#include "ui_form_plot2d.h"
#include "dialog_spectrum.h"
#include "custom_timer.h"

FormPlot2D::FormPlot2D(QWidget *parent) :
  QWidget(parent),
  scale_types_({{"Linear", QCPAxis::stLinear}, {"Logarithmic", QCPAxis::stLogarithmic}}),
  gradients_({{"Grayscale", QCPColorGradient::gpGrayscale}, {"Hot",  QCPColorGradient::gpHot},
             {"Cold", QCPColorGradient::gpCold}, {"Night", QCPColorGradient::gpNight},
             {"Candy", QCPColorGradient::gpCandy}, {"Geography", QCPColorGradient::gpGeography},
             {"Ion", QCPColorGradient::gpIon}, {"Thermal", QCPColorGradient::gpThermal},
             {"Polar", QCPColorGradient::gpPolar}, {"Spectrum", QCPColorGradient::gpSpectrum},
             {"Jet", QCPColorGradient::gpJet}, {"Hues", QCPColorGradient::gpHues}}),
  ui(new Ui::FormPlot2D)
{
  ui->setupUi(this);

  current_gradient_ = "Hot";
  current_scale_type_ = "Logarithmic";
  zoom_2d = 50;
  user_selects_ = true;
  gates_movable_ = true;
  show_boxes_ = false;

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
  for (auto &q : gradients_)
    gradientMenu.addAction(q.first);
  for (auto &q : gradientMenu.actions()) {
    q->setCheckable(true);
    if (q->text() == current_gradient_)
      q->setChecked(true);
  }
  ui->toolColors3d->setMenu(&gradientMenu);
  ui->toolColors3d->setPopupMode(QToolButton::InstantPopup);
  connect(ui->toolColors3d, SIGNAL(triggered(QAction*)), this, SLOT(gradientChosen(QAction*)));

  //scale type setup
  scaleTypeMenu.addAction("Linear");
  scaleTypeMenu.addAction("Logarithmic");
  for (auto &q : scaleTypeMenu.actions()) {
    q->setCheckable(true);
    if (q->text().toStdString() == current_scale_type_)
      q->setChecked(true);
  }
  ui->toolScaleType->setMenu(&scaleTypeMenu);
  ui->toolScaleType->setPopupMode(QToolButton::InstantPopup);
  connect(ui->toolScaleType, SIGNAL(triggered(QAction*)), this, SLOT(scaleTypeChosen(QAction*)));

  //colormap setup
  ui->coincPlot->setAlwaysSquare(true);
  colorMap = new QCPColorMap(ui->coincPlot->xAxis, ui->coincPlot->yAxis);
  colorMap->clearData();
  ui->coincPlot->addPlottable(colorMap);
  colorMap->setInterpolate(true);
  colorMap->setTightBoundary(true); //really?
  ui->coincPlot->axisRect()->setupFullAxesBox();
  ui->coincPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
  colorMap->setGradient(gradients_[current_gradient_]);
  colorMap->setDataScaleType(scale_types_[current_scale_type_]);
  colorMap->rescaleDataRange(true);
  connect(ui->coincPlot, SIGNAL(mouse_upon(double,double)), this, SLOT(plot_2d_mouse_upon(double,double)));
  connect(ui->coincPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*, bool)), this, SLOT(plot_2d_mouse_clicked(double,double,QMouseEvent*, bool)));
  //connect(ui->coincPlot, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(clicked_plottable(QCPAbstractPlottable*)));
  connect(ui->coincPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selection_changed()));

  ui->sliderZoom2d->setValue(zoom_2d);

  //scaling-related stuff
  bits = 0;
}

FormPlot2D::~FormPlot2D()
{
  delete ui;
}

void FormPlot2D::selection_changed() {
  emit stuff_selected();
}

void FormPlot2D::set_range_x(MarkerBox2D range) {
  range_ = range;
}


void FormPlot2D::setSpectra(Qpx::SpectraSet& new_set, QString spectrum) {
//  PL_DBG << "setSpectra with " << spectrum.toStdString();
  mySpectra = &new_set;

  name_2d = spectrum;

  updateUI();
}

void FormPlot2D::set_gates_movable(bool mov) {
  gates_movable_ = mov;
}

void FormPlot2D::set_show_boxes(bool show) {
  show_boxes_ = show;
  ui->coincPlot->setInteraction(QCP::iSelectItems, show);
  ui->coincPlot->setInteraction(QCP::iMultiSelect, false);
}

std::list<MarkerBox2D> FormPlot2D::get_selected_boxes() {
  std::list<MarkerBox2D> selection;
  for (auto &q : ui->coincPlot->selectedItems())
    if (QCPItemRect *b = qobject_cast<QCPItemRect*>(q)) {
      MarkerBox2D box;
      box.x_c = b->property("chan_x").toDouble();
      box.y_c = b->property("chan_y").toDouble();
      selection.push_back(box);
      //PL_DBG << "found selected " << txt->property("true_value").toDouble() << " chan=" << txt->property("chan_value").toDouble();
    }
  return selection;
}


void FormPlot2D::set_boxes(std::list<MarkerBox2D> boxes) {
  boxes_ = boxes;
}


void FormPlot2D::reset_content() {
  //PL_DBG << "reset content";
    colorMap->clearData();
    name_2d.clear();
    x_marker.visible = false;
    y_marker.visible = false;
    ext_marker.visible = false;
    replot_markers();
}

void FormPlot2D::on_comboChose2d_activated(const QString &arg1)
{
  if (arg1 == name_2d)
    return;
  std::list<Qpx::Spectrum::Spectrum*> spectra = mySpectra->spectra(2, -1);

  for (auto &q : spectra)
    if (q->name() == arg1.toStdString())
      q->set_visible(true);
    else
      q->set_visible(false);

  //name_2d = arg1;
  update_plot(true);
}

void FormPlot2D::updateUI()
{
 // PL_DBG << "updateUI";

  std::list<Qpx::Spectrum::Spectrum*> spectra = mySpectra->spectra(2, -1);
  std::string newname;
  std::set<std::string> names;
  for (auto &q : spectra) {
    Qpx::Spectrum::Metadata md;
    if (q)
      md = q->metadata();

    names.insert(md.name);
    if (md.visible)
      newname = md.name;
  }

  if (newname.empty() && !names.empty())
    newname = *names.begin();

  if (names.count(name_2d.toStdString()))
    newname = name_2d.toStdString();

  ui->comboChose2d->blockSignals(true);
  this->blockSignals(true);
  ui->comboChose2d->clear();
  for (auto &q: names)
    ui->comboChose2d->addItem(QString::fromStdString(q));
  ui->comboChose2d->setCurrentText(QString::fromStdString(newname));
  ui->comboChose2d->blockSignals(false);
  this->blockSignals(false);

//  if (newname != name_2d.toStdString())
//    update_plot(true);
}

void FormPlot2D::set_show_selector(bool vis) {
  ui->comboChose2d->setVisible(vis);
  user_selects_ = vis;
}

void FormPlot2D::set_show_analyse(bool vis) {
  ui->pushAnalyse->setVisible(vis);
}

void FormPlot2D::set_gate_width(int w) {
  ui->spinGateWidth->setValue(w);
}

void FormPlot2D::set_show_gate_width(bool vis) {
  ui->labelGateWidth->setVisible(vis);
  ui->spinGateWidth->setVisible(vis);
}

int FormPlot2D::gate_width() {
  return ui->spinGateWidth->value();
}

void FormPlot2D::scaleTypeChosen(QAction* choice) {
  this->setCursor(Qt::WaitCursor);
  current_scale_type_ = choice->text().toStdString();
  for (auto &q : scaleTypeMenu.actions())
    q->setChecked(false);
  choice->setChecked(true);
  colorMap->setDataScaleType(scale_types_[current_scale_type_]);
  colorMap->rescaleDataRange(true);
  ui->coincPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}

void FormPlot2D::gradientChosen(QAction* choice) {
  this->setCursor(Qt::WaitCursor);
  current_gradient_ = choice->text();
  for (auto &q : gradientMenu.actions())
    q->setChecked(false);
  choice->setChecked(true);
  colorMap->setGradient(gradients_[current_gradient_]);
  replot_markers();
  this->setCursor(Qt::ArrowCursor);
}

void FormPlot2D::refresh()
{
  ui->coincPlot->replot();
}

void FormPlot2D::replot_markers() {
  //PL_DBG << "replot markers";
  ui->coincPlot->clearItems();

  if (!ui->spinGateWidth->isVisible()) {
    make_marker(ext_marker);
    make_marker(x_marker);
    make_marker(y_marker);
  } else {
    QPen pen = my_marker.appearance.get_pen(current_gradient_);

    QColor cc = pen.color();
    cc.setAlpha(169);
    pen.setColor(cc);
    pen.setWidth(3);

    int width = ui->spinGateWidth->value() / 2;

    QPen pen2 = pen;
    cc.setAlpha(64);
    pen2.setColor(cc);
    pen2.setWidth(width * 2);

    QPen pen_strong = pen;
    cc.setAlpha(255);
    cc.setHsv(cc.hsvHue(), 255, 128, 255);
    pen_strong.setColor(cc);
    pen_strong.setWidth(3);

    if (show_boxes_) {
      pen.setWidth(1);
      QCPItemRect *box = new QCPItemRect(ui->coincPlot);
      box->setPen(pen);
      box->topLeft->setCoords(0, y_marker.channel - width);
      box->bottomRight->setCoords(2 * x_marker.channel, y_marker.channel + width);
      ui->coincPlot->addItem(box);
    }

    QCPItemStraightLine *one_line;

    if (gate_horizontal_ && y_marker.visible && !show_boxes_) {
      one_line = new QCPItemStraightLine(ui->coincPlot);
      one_line->setPen(pen);
      one_line->point1->setCoords(0, y_marker.channel - width - 0.5);
      one_line->point2->setCoords(1, y_marker.channel - width - 0.5);
      ui->coincPlot->addItem(one_line);

      one_line = new QCPItemStraightLine(ui->coincPlot);
      one_line->setPen(pen);
      one_line->point1->setCoords(0, y_marker.channel + width + 0.5);
      one_line->point2->setCoords(1, y_marker.channel + width + 0.5);
      ui->coincPlot->addItem(one_line);

      /*one_line = new QCPItemStraightLine(ui->coincPlot);
      one_line->setPen(pen2);
      one_line->point1->setCoords(0, y_marker.channel);
      one_line->point2->setCoords(1, y_marker.channel);
      ui->coincPlot->addItem(one_line);*/
    }

    if (gate_vertical_ && x_marker.visible) {
      one_line = new QCPItemStraightLine(ui->coincPlot);
      one_line->setPen(pen);
      one_line->point1->setCoords(x_marker.channel - width - 0.5, 0);
      one_line->point2->setCoords(x_marker.channel - width - 0.5, 1);
      ui->coincPlot->addItem(one_line);

      one_line = new QCPItemStraightLine(ui->coincPlot);
      one_line->setPen(pen);
      one_line->point1->setCoords(x_marker.channel + width + 0.5, 0);
      one_line->point2->setCoords(x_marker.channel + width + 0.5, 1);
      ui->coincPlot->addItem(one_line);

      /*one_line = new QCPItemStraightLine(ui->coincPlot);
      one_line->setPen(pen2);
      one_line->point1->setCoords(x_marker.channel, 0);
      one_line->point2->setCoords(x_marker.channel, 1);
      ui->coincPlot->addItem(one_line);*/
    }

    if (gate_diagonal_ && x_marker.visible && y_marker.visible) {
      int width = ui->spinGateWidth->value();
      int diag_width = std::round(std::sqrt((width*width)/2.0) / 2.0);
      if ((diag_width % 2) == 0)
        diag_width++;

      one_line = new QCPItemStraightLine(ui->coincPlot);
      one_line->setPen(pen);
      one_line->point1->setCoords(x_marker.channel - diag_width, y_marker.channel - diag_width);
      one_line->point2->setCoords(x_marker.channel - diag_width + 1, y_marker.channel - diag_width - 1);
      ui->coincPlot->addItem(one_line);

      one_line = new QCPItemStraightLine(ui->coincPlot);
      one_line->setPen(pen);
      one_line->point1->setCoords(x_marker.channel + diag_width, y_marker.channel + diag_width);
      one_line->point2->setCoords(x_marker.channel + diag_width + 1, y_marker.channel + diag_width - 1);
      ui->coincPlot->addItem(one_line);

    }

    if (show_boxes_) {
      QCPItemRect *box;

      for (auto &q : boxes_) {
        if (!q.visible)
          continue;
        box = new QCPItemRect(ui->coincPlot);
        box->setSelected(q.selected);
        box->setSelectable(true);
        QColor sel = box->selectedPen().color();
        box->setSelectedBrush(QBrush(QColor::fromHsv(sel.hsvHue(), sel.saturation(), sel.value(), 64)));
        box->setBrush(QBrush(pen2.color()));
        box->setProperty("chan_x", q.x_c);
        box->setProperty("chan_y", q.y_c);
        box->topLeft->setCoords(q.x1.channel, q.y1.channel);
        box->bottomRight->setCoords(q.x2.channel, q.y2.channel);
        ui->coincPlot->addItem(box);
      }
    }


    if (range_.visible) {
      QCPItemRect *box = new QCPItemRect(ui->coincPlot);
      box->setSelectable(false);
      box->setPen(pen_strong);
      box->setBrush(QBrush(pen2.color()));
      box->topLeft->setCoords(range_.x1.channel, range_.y1.channel);
      box->bottomRight->setCoords(range_.x2.channel, range_.y2.channel);
      ui->coincPlot->addItem(box);
    }

  }

  ui->coincPlot->replot();
}

void FormPlot2D::make_marker(Marker &marker) {
  if (!marker.visible)
    return;

  QPen pen = my_marker.appearance.get_pen(current_gradient_);

  QCPItemStraightLine *one_line;


  if (marker.chan_valid && (calib_y_.units_ == "channels")) {
    //PL_DBG << "<PLot2D> marker option 1: channels valid and units = channels";
    one_line = new QCPItemStraightLine(ui->coincPlot);
    one_line->setPen(pen);
    one_line->point1->setCoords(0, marker.channel);
    one_line->point2->setCoords(1, marker.channel);
    ui->coincPlot->addItem(one_line);
    one_line = new QCPItemStraightLine(ui->coincPlot);
    one_line->setPen(pen);
    one_line->point1->setCoords(marker.channel, 0);
    one_line->point2->setCoords(marker.channel, 1);
    ui->coincPlot->addItem(one_line);
  } else if (marker.chan_valid && (calib_y_.units_ != "channels")) {
    //PL_DBG << "<PLot2D> marker option 2: channels valid and units not channels";
    one_line = new QCPItemStraightLine(ui->coincPlot);
    one_line->setPen(pen);
    one_line->point1->setCoords(0, calib_y_.transform(marker.channel, marker.bits));
    one_line->point2->setCoords(1, calib_y_.transform(marker.channel, marker.bits));
    ui->coincPlot->addItem(one_line);
    one_line = new QCPItemStraightLine(ui->coincPlot);
    one_line->setPen(pen);
    one_line->point1->setCoords(calib_x_.transform(marker.channel, marker.bits), 0);
    one_line->point2->setCoords(calib_x_.transform(marker.channel, marker.bits), 1);
    ui->coincPlot->addItem(one_line);
  } else if (marker.energy_valid && (calib_y_.units_ != "channels")) {
    //PL_DBG << "<PLot2D> marker option 3: energy valid and units not channels";
    one_line = new QCPItemStraightLine(ui->coincPlot);
    one_line->setPen(pen);
    one_line->point1->setCoords(0, marker.energy);
    one_line->point2->setCoords(1, marker.energy);
    ui->coincPlot->addItem(one_line);
    one_line = new QCPItemStraightLine(ui->coincPlot);
    one_line->setPen(pen);
    one_line->point1->setCoords(marker.energy, 0);
    one_line->point2->setCoords(marker.energy, 1);
    ui->coincPlot->addItem(one_line);
  }

}

void FormPlot2D::update_plot(bool force) {
//  PL_DBG << "updating 2d";

  this->setCursor(Qt::WaitCursor);
  CustomTimer guiside(true);

  bool new_data = mySpectra->new_data();
  bool rescale2d = (zoom_2d != ui->sliderZoom2d->value());

  if (rescale2d || new_data || force) {
//    PL_DBG << "really updating 2d " << name_2d.toStdString();

    QString newname = ui->comboChose2d->currentText();
    //PL_DBG << "<Plot2D> getting " << newname.toStdString();

    Qpx::Spectrum::Spectrum* some_spectrum = mySpectra->by_name(newname.toStdString());
    uint32_t adjrange;

    ui->pushDetails->setEnabled((some_spectrum != nullptr));
    ui->pushAnalyse->setEnabled((some_spectrum != nullptr));
    zoom_2d = ui->sliderZoom2d->value();

    Qpx::Spectrum::Metadata md;
    if (some_spectrum)
      md = some_spectrum->metadata();

    if ((md.total_count > 0) && (md.dimensions == 2) && (adjrange = static_cast<uint32_t>(md.resolution * (zoom_2d / 100.0))) )
    {
//      PL_DBG << "really really updating 2d total count = " << some_spectrum->total_count();

      if (rescale2d || force || (name_2d != newname)) {
//        PL_DBG << "rescaling 2d";
        name_2d = newname;
        ui->coincPlot->clearGraphs();
        colorMap->clearData();
        colorMap->data()->setSize(adjrange, adjrange);

        int newbits = some_spectrum->bits();
        if (bits != newbits) {
          bits = newbits;
          ext_marker.shift(bits);
          x_marker.shift(bits);
          y_marker.shift(bits);
        }

        Gamma::Detector detector_x_;
        Gamma::Detector detector_y_;
        if (md.detectors.size() > 1) {
          detector_x_ = md.detectors[0];
          detector_y_ = md.detectors[1];
        }

        if (detector_x_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits)))
          calib_x_ = detector_x_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
        else
          calib_x_ = detector_x_.highest_res_calib();
        if (!ui->spinGateWidth->isVisible())
          colorMap->keyAxis()->setLabel(/*QString::fromStdString(detector_x_.name_) + */ "Energy (" + QString::fromStdString(calib_x_.units_) + ")");
        else
          colorMap->keyAxis()->setLabel("Energy (bin)");
        ui->labelCoDet1->setText(QString::fromStdString(detector_x_.name_) + "(" + QString::fromStdString(calib_x_.units_) + "):");
        if (!calib_x_.bits_)
          calib_x_.bits_ = bits;

        if (detector_y_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits)))
          calib_y_ = detector_y_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
        else
          calib_y_ = detector_y_.highest_res_calib();
        if (!ui->spinGateWidth->isVisible())
          colorMap->valueAxis()->setLabel(/*QString::fromStdString(detector_y_.name_) + */ "Energy (" + QString::fromStdString(calib_y_.units_) + ")");
        else
          colorMap->valueAxis()->setLabel("Energy (bin)");

        ui->labelCoDet2->setText(QString::fromStdString(detector_y_.name_) + "(" + QString::fromStdString(calib_y_.units_) + "):");
        if (!calib_y_.bits_)
          calib_y_.bits_ = bits;

        calibrate_marker(x_marker);
        calibrate_marker(y_marker);

        if (!ui->spinGateWidth->isVisible())
          colorMap->data()->setRange(QCPRange(0, calib_x_.transform(adjrange - 1, bits)),
                                     QCPRange(0, calib_y_.transform(adjrange - 1, bits)));
        else
          colorMap->data()->setRange(QCPRange(0, adjrange - 1),
                                     QCPRange(0, adjrange - 1));

        //PL_INFO << "range maxes at " << calib_x_.transform(adjrange - 1, bits)<< ", " << calib_y_.transform(adjrange - 1, bits);

        ui->coincPlot->rescaleAxes();
        //ui->coincPlot->xAxis->setRange(colorMap->keyAxis()->range());
        //ui->coincPlot->yAxis->setRange(colorMap->valueAxis()->range());
      }

      std::shared_ptr<Qpx::Spectrum::EntryList> spectrum_data =
          std::move(some_spectrum->get_spectrum({{0, adjrange}, {0, adjrange}}));
      for (auto it : *spectrum_data)
        colorMap->data()->setCell(it.first[0], it.first[1], it.second);

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

  PL_DBG << "<Plot2D> plotting took " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}

void FormPlot2D::plot_2d_mouse_upon(double x, double y) {
  ui->labelCoEn0->setText(QString::number(calib_x_.transform(x, bits)));
  ui->labelCoEn1->setText(QString::number(calib_y_.transform(y, bits)));
}

void FormPlot2D::plot_2d_mouse_clicked(double x, double y, QMouseEvent *event, bool channels) {
  PL_INFO << "<Plot2D> markers at " << x << " & " << y;

  bool visible = (event->button() == Qt::LeftButton);

  Marker xt(x_marker), yt(y_marker);

  xt.bits = bits;
  yt.bits = bits;
  xt.visible = visible;
  yt.visible = visible;

  if (visible && channels) {
    xt.channel = x;
    yt.channel = y;
    xt.chan_valid = true;
    yt.chan_valid = true;
    xt.energy_valid = false;
    yt.energy_valid = false;
    calibrate_marker(xt);
    calibrate_marker(yt);
  } else if (visible && !channels){
    xt.energy = x;
    yt.energy = y;
    xt.energy_valid = true;
    yt.energy_valid = true;
    xt.chan_valid = false;
    yt.chan_valid = false;
  }


  if (gates_movable_) {
    x_marker = xt;
    y_marker = yt;
    ext_marker.visible = ext_marker.visible & visible;
    replot_markers();
  }

  emit markers_set(xt, yt);
}

void FormPlot2D::calibrate_marker(Marker &marker) {
  if (!marker.energy_valid && marker.chan_valid) {
    marker.energy = calib_x_.transform(marker.channel, marker.bits);
    marker.energy_valid = (calib_x_.units_ != "channels");
  }
}


void FormPlot2D::set_marker(Marker n) {
  ext_marker = n;
  ext_marker.shift(bits);
  if (!ext_marker.visible) {
    x_marker.visible = false;
    y_marker.visible = false;
  }
  replot_markers();
}

void FormPlot2D::set_markers(Marker x, Marker y) {
  x_marker = x;
  y_marker = y;

  x_marker.shift(bits);
  y_marker.shift(bits);

  replot_markers();
}

void FormPlot2D::on_pushResetScales_clicked()
{
  this->setCursor(Qt::WaitCursor);
  ui->coincPlot->rescaleAxes();
  ui->coincPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}


void FormPlot2D::on_pushColorScale_clicked()
{
  this->setCursor(Qt::WaitCursor);
  if (ui->pushColorScale->isChecked()) {
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
  this->setCursor(Qt::ArrowCursor);
}

void FormPlot2D::on_pushDetails_clicked()
{
  Qpx::Spectrum::Spectrum* someSpectrum = mySpectra->by_name(name_2d.toStdString());
  if (someSpectrum == nullptr)
    return;

  dialog_spectrum* newSpecDia = new dialog_spectrum(*someSpectrum, this);
  connect(newSpecDia, SIGNAL(finished(bool)), this, SLOT(spectrumDetailsClosed(bool)));
  connect(newSpecDia, SIGNAL(delete_spectrum()), this, SLOT(spectrumDetailsDelete()));
  newSpecDia->exec();
}

void FormPlot2D::spectrumDetailsClosed(bool changed) {
  if (changed) {
    //replot?
  }
}

void FormPlot2D::spectrumDetailsDelete()
{

  PL_INFO << "will delete " << name_2d.toStdString();
  mySpectra->delete_spectrum(name_2d.toStdString());

  updateUI();

  QString name = ui->comboChose2d->currentText();
  std::list<Qpx::Spectrum::Spectrum*> spectra = mySpectra->spectra(2, -1);

  for (auto &q : spectra)
    if (q->name() == name.toStdString())
      q->set_visible(true);
    else
      q->set_visible(false);

  update_plot(true);
}

void FormPlot2D::on_sliderZoom2d_valueChanged(int value)
{
  if (this->isVisible() && (mySpectra != nullptr))
    mySpectra->activate();
}

void FormPlot2D::set_scale_type(QString sct) {
  this->setCursor(Qt::WaitCursor);
  current_scale_type_ = sct.toStdString();
  for (auto &q : scaleTypeMenu.actions())
    q->setChecked(q->text() == sct);
  colorMap->setDataScaleType(scale_types_[current_scale_type_]);
  colorMap->rescaleDataRange(true);
  ui->coincPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}

QString FormPlot2D::scale_type() {
  return QString::fromStdString(current_scale_type_);
}

void FormPlot2D::set_gradient(QString grd) {
  this->setCursor(Qt::WaitCursor);
  current_gradient_ = grd;
  for (auto &q : gradientMenu.actions())
    q->setChecked(q->text() == grd);
  colorMap->setGradient(gradients_[current_gradient_]);
  replot_markers();
  this->setCursor(Qt::ArrowCursor);
}

QString FormPlot2D::gradient() {
  return current_gradient_;
}

void FormPlot2D::set_zoom(double zm) {
  zoom_2d = zm;
  ui->sliderZoom2d->setValue(zoom_2d);
}

double FormPlot2D::zoom() {
  return zoom_2d;
}

void FormPlot2D::set_show_legend(bool show) {
  ui->pushColorScale->setChecked(show);
  on_pushColorScale_clicked();
}

bool FormPlot2D::show_legend() {
  return ui->pushColorScale->isChecked();
}

void FormPlot2D::on_pushAnalyse_clicked()
{
  emit requestAnalysis(name_2d);
}

void FormPlot2D::on_spinGateWidth_valueChanged(int arg1)
{
  /*int width = ui->spinGateWidth->value() / 2;
  if ((ui->spinGateWidth->value() % 2) == 0)
    ui->spinGateWidth->setValue(width*2 + 1);*/

  replot_markers();
  //emit markers_set(x_marker, y_marker);
}

void FormPlot2D::set_gates_visible(bool vertical, bool horizontal, bool diagonal)
{
  gate_vertical_ = vertical;
  gate_horizontal_ = horizontal;
  gate_diagonal_ = diagonal;

  replot_markers();
}


void FormPlot2D::on_spinGateWidth_editingFinished()
{
  int width = ui->spinGateWidth->value() / 2;
  if ((ui->spinGateWidth->value() % 2) == 0)
    ui->spinGateWidth->setValue(width*2 + 1);

  replot_markers();
  emit markers_set(x_marker, y_marker);
}
