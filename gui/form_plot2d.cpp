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

#include "gui/form_plot2d.h"
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

  //color theme setup
  my_marker.themes["Grayscale"] = QPen(Qt::cyan, 1);
  my_marker.themes["Hot"] = QPen(Qt::cyan, 1);
  my_marker.themes["Cold"] = QPen(Qt::yellow, 1);
  my_marker.themes["Night"] = QPen(Qt::red, 1);
  my_marker.themes["Candy"] = QPen(Qt::red, 1);
  my_marker.themes["Geography"] = QPen(Qt::yellow, 1);
  my_marker.themes["Ion"] = QPen(Qt::magenta, 1);
  my_marker.themes["Thermal"] = QPen(Qt::cyan, 1);
  my_marker.themes["Polar"] = QPen(Qt::green, 1);
  my_marker.themes["Spectrum"] = QPen(Qt::cyan, 1);
  my_marker.themes["Jet"] = QPen(Qt::darkMagenta, 1);
  my_marker.themes["Hues"] = QPen(Qt::black, 1);
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
  current_gradient_ = "Hot";

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
  current_scale_type_ = "Logarithmic";

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
  connect(ui->coincPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*)), this, SLOT(plot_2d_mouse_clicked(double,double,QMouseEvent*)));
  zoom_2d = ui->sliderZoom2d->value(); //from settings?

  //scaling-related stuff
  bits = 0;
}

void FormPlot2D::setSpectra(Pixie::SpectraSet& new_set) {
  mySpectra = &new_set;

  ui->comboChose2d->clear();
  for (auto &q: mySpectra->spectra(2, -1))
    ui->comboChose2d->addItem(QString::fromStdString(q->name()));

  zoom_2d = 0;
  name_2d.clear();
  //replot?
}


FormPlot2D::~FormPlot2D()
{
  delete ui;
}

void FormPlot2D::on_comboChose2d_activated(const QString &arg1)
{
  mySpectra->activate();
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

void FormPlot2D::replot_markers() {
  ui->coincPlot->clearItems();

  make_marker(ext_marker);
  make_marker(x_marker);
  make_marker(y_marker);

  ui->coincPlot->replot();
}

void FormPlot2D::make_marker(Marker &marker) {
  if (!marker.visible)
    return;

  QPen pen;
  if (my_marker.themes.count(current_gradient_))
    pen = QPen(my_marker.themes[current_gradient_]);
  else
    pen = QPen(my_marker.default_pen);

  QCPItemStraightLine *one_line;

  //horizontal
  ui->coincPlot->addItem(one_line);
  if (marker.calibrated && (calib_y_.units_ != "channels")) {
    one_line = new QCPItemStraightLine(ui->coincPlot);
    one_line->setPen(pen);
    one_line->point1->setCoords(0, marker.energy);
    one_line->point2->setCoords(1, marker.energy);
    ui->coincPlot->addItem(one_line);
  }
  else if (!marker.calibrated && (calib_y_.units_ == "channels")) {
    one_line = new QCPItemStraightLine(ui->coincPlot);
    one_line->setPen(pen);
    one_line->point1->setCoords(0, marker.channel);
    one_line->point2->setCoords(1, marker.channel);
    ui->coincPlot->addItem(one_line);
  }

  //vertical
  if (marker.calibrated && (calib_x_.units_ != "channels")) {
    one_line = new QCPItemStraightLine(ui->coincPlot);
    one_line->setPen(pen);
    one_line->point1->setCoords(marker.energy, 0);
    one_line->point2->setCoords(marker.energy, 1);
    ui->coincPlot->addItem(one_line);
  }
  else if (!marker.calibrated && (calib_x_.units_ == "channels")) {
    one_line = new QCPItemStraightLine(ui->coincPlot);
    one_line->setPen(pen);
    one_line->point1->setCoords(marker.channel, 0);
    one_line->point2->setCoords(marker.channel, 1);
    ui->coincPlot->addItem(one_line);
  }
}

void FormPlot2D::update_plot() {

  this->setCursor(Qt::WaitCursor);
  CustomTimer guiside(true);

  bool new_data = mySpectra->new_data();
  bool rescale2d = ((zoom_2d != ui->sliderZoom2d->value()) || (name_2d != ui->comboChose2d->currentText()));

  if (rescale2d || new_data) {

    Pixie::Spectrum::Spectrum* some_spectrum = mySpectra->by_name(ui->comboChose2d->currentText().toStdString());
    uint32_t adjrange;

    ui->pushDetails->setEnabled((some_spectrum != nullptr));

    if ((some_spectrum != nullptr)
        && some_spectrum->total_count()
        && (some_spectrum->dimensions() == 2)
        && (adjrange = static_cast<uint32_t>(some_spectrum->resolution() * (ui->sliderZoom2d->value() / 100.0)))
        )
    {
      if (rescale2d) {
        ui->coincPlot->clearGraphs();
        colorMap->data()->setSize(adjrange, adjrange);

        int newbits = some_spectrum->bits();
        if (bits != newbits) {
          bits = newbits;
          calibrate_markers();
          PL_DBG << "new plot is " << bits << " bits";
        }

        Pixie::Detector detector_x_ = some_spectrum->get_detector(0);
        colorMap->keyAxis()->setLabel(QString::fromStdString(detector_x_.name_) + " (" + QString::fromStdString(detector_x_.name_) + ")");
        ui->labelCoDet1->setText(QString::fromStdString(detector_x_.name_) + "(" + QString::fromStdString(detector_x_.name_) + "):");
        if (detector_x_.energy_calibrations_.has_a(Pixie::Calibration(bits)))
          calib_x_ = detector_x_.energy_calibrations_.get(Pixie::Calibration(bits));
        else
          calib_x_ = detector_x_.highest_res_calib();

        Pixie::Detector detector_y_ = some_spectrum->get_detector(1);
        colorMap->valueAxis()->setLabel(QString::fromStdString(detector_y_.name_) + " (" + QString::fromStdString(calib_y_.units_) + ")");
        ui->labelCoDet2->setText(QString::fromStdString(detector_y_.name_) + "(" + QString::fromStdString(calib_y_.units_) + "):");
        if (detector_y_.energy_calibrations_.has_a(Pixie::Calibration(bits)))
          calib_y_ = detector_y_.energy_calibrations_.get(Pixie::Calibration(bits));
        else
          calib_y_ = detector_y_.highest_res_calib();

        PL_DBG << "calibrations for axes are " << calib_x_.bits_ << " & " << calib_y_.bits_ << " bits";

        colorMap->data()->setRange(QCPRange(0, calib_x_.transform(adjrange - 1, bits)),
                                   QCPRange(0, calib_y_.transform(adjrange - 1, bits)));

        PL_INFO << "range maxes at " << calib_x_.transform(adjrange - 1, bits)<< ", " << calib_y_.transform(adjrange - 1, bits);

        ui->coincPlot->rescaleAxes();
        //ui->coincPlot->xAxis->setRange(colorMap->keyAxis()->range());
        //ui->coincPlot->yAxis->setRange(colorMap->valueAxis()->range());
      }

      std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data =
          std::move(some_spectrum->get_spectrum({{0, adjrange}, {0, adjrange}}));
      for (auto it : *spectrum_data)
        colorMap->data()->setCell(it.first[0], it.first[1], it.second);

      colorMap->rescaleDataRange(true);

      name_2d = ui->comboChose2d->currentText();
      zoom_2d = ui->sliderZoom2d->value();

      ui->coincPlot->updateGeometry();
    } else {
      ui->coincPlot->clearGraphs();
      colorMap->clearData();
      colorMap->keyAxis()->setLabel("");
      colorMap->valueAxis()->setLabel("");
    }
    replot_markers();
    ui->coincPlot->replot();
  }

  PL_DBG << "2d plotting " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}

void FormPlot2D::plot_2d_mouse_upon(double x, double y) {
  ui->labelCoEn0->setText(QString::number(calib_x_.transform(x, bits)));
  ui->labelCoEn1->setText(QString::number(calib_y_.transform(y, bits)));
}

void FormPlot2D::plot_2d_mouse_clicked(double x, double y, QMouseEvent *event) {
  PL_INFO << "2D markers requested at " << x << " & " << y;

  x_marker.channel = x;
  x_marker.bits = bits;
  x_marker.calibrated = false;

  y_marker.channel = y;
  y_marker.bits = bits;
  y_marker.calibrated = false;

  calibrate_markers();

  bool visible = (event->button() == Qt::LeftButton);
  x_marker.visible = visible;
  y_marker.visible = visible;
  ext_marker.visible = ext_marker.visible & visible;

  replot_markers();

  emit markers_set(x_marker, y_marker);
}

void FormPlot2D::calibrate_markers() {
  if (!x_marker.calibrated) {
    x_marker.energy = calib_x_.transform(x_marker.channel, x_marker.bits);
    x_marker.calibrated = (calib_x_.units_ != "channels");
  }

  if (!y_marker.calibrated) {
    y_marker.energy = calib_y_.transform(y_marker.channel, y_marker.bits);
    y_marker.calibrated = (calib_y_.units_ != "channels");
  }

  if (!ext_marker.calibrated) {
    ext_marker.energy = calib_x_.transform(ext_marker.channel, x_marker.bits);
    ext_marker.calibrated = ((calib_x_.units_ != "channels") && (calib_y_.units_ != "channels"));
  }
}


void FormPlot2D::set_marker(Marker n) {
  ext_marker = n;
  calibrate_markers();
  if (!ext_marker.visible) {
    x_marker.visible = false;
    y_marker.visible = false;
  }
  replot_markers();
}

void FormPlot2D::on_pushResetScales_clicked()
{
  this->setCursor(Qt::WaitCursor);
  ui->coincPlot->rescaleAxes();
  ui->coincPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}

void FormPlot2D::reset_content() {
    colorMap->clearData();
    x_marker.visible = false;
    y_marker.visible = false;
    ext_marker.visible = false;
    replot_markers();
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
  Pixie::Spectrum::Spectrum* someSpectrum = mySpectra->by_name(ui->comboChose2d->currentText().toStdString());
  if (someSpectrum == nullptr)
    return;

  dialog_spectrum* newSpecDia = new dialog_spectrum(*someSpectrum, this);
  connect(newSpecDia, SIGNAL(finished(bool)), this, SLOT(spectrumDetailsClosed(bool)));
  newSpecDia->exec();
}

void FormPlot2D::spectrumDetailsClosed(bool changed) {
  if (changed) {
    //replot?
  }
}

void FormPlot2D::on_sliderZoom2d_valueChanged(int value)
{
  mySpectra->activate();
}
