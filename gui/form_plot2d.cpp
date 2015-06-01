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

  ext_marker = 0;
  marker_x = 0;
  marker_y = 0;
  zoom_2d = 100;
  current_gradient_ = "Hot";
  current_scale_type_ = "Logarithmic";


  for (auto &q : gradients_)
    gradientMenu.addAction(QString::fromStdString(q.first));
  for (auto &q : gradientMenu.actions()) {
    q->setCheckable(true);
    if (q->text().toStdString() == current_gradient_)
      q->setChecked(true);
  }
  ui->toolColors3d->setMenu(&gradientMenu);
  ui->toolColors3d->setPopupMode(QToolButton::InstantPopup);
  connect(ui->toolColors3d, SIGNAL(triggered(QAction*)), this, SLOT(gradientChosen(QAction*)));


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

  connect(ui->coincPlot, SIGNAL(mouse_upon(int,int)), this, SLOT(plot_2d_mouse_upon(int,int)));
  connect(ui->coincPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*)), this, SLOT(plot_2d_mouse_clicked(double,double,QMouseEvent*)));
}

void FormPlot2D::setSpectra(Pixie::SpectraSet& new_set) {
  mySpectra = &new_set;

  ui->comboChose2d->clear();
  for (auto &q: mySpectra->spectra(2, -1))
    ui->comboChose2d->addItem(QString::fromStdString(q->name()));

  //replot?
}


FormPlot2D::~FormPlot2D()
{
  delete ui;
}

void FormPlot2D::on_sliderZoom2d_sliderReleased()
{
  mySpectra->activate();
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
  current_gradient_ = choice->text().toStdString();
  for (auto &q : gradientMenu.actions())
    q->setChecked(false);
  choice->setChecked(true);
  colorMap->setGradient(gradients_[current_gradient_]);
  ui->coincPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}

void FormPlot2D::replot_markers() {
  ui->coincPlot->clearItems();

  if (ext_marker) {
    make_marker(ext_marker, Qt::green, 3, Qt::Vertical);
    make_marker(ext_marker, Qt::green, 3, Qt::Horizontal);

    make_marker(ext_marker, Qt::black, 1, Qt::Vertical);
    make_marker(ext_marker, Qt::black, 1, Qt::Horizontal);
  }

  if (marker_x || marker_y) {
    make_marker(marker_x, Qt::green, 3, Qt::Vertical);
    make_marker(marker_x, Qt::green, 3, Qt::Horizontal);
    make_marker(marker_y, Qt::green, 3, Qt::Vertical);
    make_marker(marker_y, Qt::green, 3, Qt::Horizontal);

    make_marker(marker_x, Qt::black, 1, Qt::Vertical);
    make_marker(marker_x, Qt::black, 1, Qt::Horizontal);
    make_marker(marker_y, Qt::black, 1, Qt::Vertical);
    make_marker(marker_y, Qt::black, 1, Qt::Horizontal);
  }

  ui->coincPlot->replot();
}

void FormPlot2D::make_marker(double coord, QColor color, int thickness, Qt::Orientations orientation) {
  QCPItemStraightLine *one_line = new QCPItemStraightLine(ui->coincPlot);
  one_line->setPen(QPen(color, thickness));
  if (orientation == Qt::Horizontal) {
    one_line->point1->setCoords(0, coord);
    one_line->point2->setCoords(1, coord);
  } else if (orientation == Qt::Vertical) {
    one_line->point1->setCoords(coord, 0);
    one_line->point2->setCoords(coord, 1);
  }
  ui->coincPlot->addItem(one_line);
}

void FormPlot2D::update_plot() {

  this->setCursor(Qt::WaitCursor);
  CustomTimer guiside(true);

  bool new_data = mySpectra->new_data();

  bool rescale2d = ((zoom_2d != ui->sliderZoom2d->value()) || (name_2d != ui->comboChose2d->currentText()));
  bool redraw2d = (new_data || rescale2d);

  if (redraw2d) {

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
      }
      std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data =
          std::move(some_spectrum->get_spectrum({{0, adjrange}, {0, adjrange}}));
      for (auto it : *spectrum_data)
        colorMap->data()->setCell(it.first[0], it.first[1], it.second);

      colorMap->rescaleDataRange(true);


      if (rescale2d) {
        int bits = some_spectrum->bits();
        QVector<Pixie::Detector> axes3d;
        for (std::size_t i=0; i < some_spectrum->add_pattern().size(); i++) {
          if (some_spectrum->add_pattern()[i] == 1) {
            Pixie::Detector this_det = some_spectrum->get_detectors()[i];
            axes3d.push_back(this_det);
          }
        }

        QString det_units, det_name;
        co_energies_x_ = some_spectrum->energies(0);
        co_energies_y_ = some_spectrum->energies(1);

        colorMap->data()->setRange(QCPRange(0, co_energies_x_[adjrange - 1]),
            QCPRange(0, co_energies_y_[adjrange - 1]));

        det_units = QString::fromStdString(axes3d[0].energy_calibrations_.get(Pixie::Calibration(bits)).units_);
        det_name = QString::fromStdString(axes3d[0].name_);
        colorMap->keyAxis()->setLabel(det_units + " (" + det_name + ")");
        ui->labelCoDet1->setText(det_name + "(" + det_units + "):");

        det_units = QString::fromStdString(axes3d[1].energy_calibrations_.get(Pixie::Calibration(bits)).units_);
        det_name = QString::fromStdString(axes3d[1].name_);
        colorMap->valueAxis()->setLabel(det_units + " (" + det_name + ")");
        ui->labelCoDet2->setText(det_name + "(" + det_units + "):");

        ui->coincPlot->rescaleAxes();
        ui->coincPlot->xAxis->setRange(colorMap->keyAxis()->range());
        ui->coincPlot->yAxis->setRange(colorMap->valueAxis()->range());

        name_2d = ui->comboChose2d->currentText();
        zoom_2d = ui->sliderZoom2d->value();

        ui->coincPlot->updateGeometry();
      }
    } else {
      ui->coincPlot->clearGraphs();
      colorMap->clearData();
      colorMap->keyAxis()->setLabel("");
      colorMap->valueAxis()->setLabel("");
    }
    ui->coincPlot->replot();
  }

  PL_DBG << "2d plotting " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}

void FormPlot2D::plot_2d_mouse_upon(int x, int y) {
  double xx = 0.0, yy = 0.0;

  if (x < static_cast<int>(co_energies_x_.size()))
    xx = co_energies_x_[x];
  if (y < static_cast<int>(co_energies_y_.size()))
    yy = co_energies_y_[y];

  ui->labelCoEn0->setText(QString::number(xx));
  ui->labelCoEn1->setText(QString::number(yy));
}

void FormPlot2D::plot_2d_mouse_clicked(double x, double y, QMouseEvent *event) {
  if (event->button() == Qt::RightButton) {
    marker_x = 0; marker_y = 0; ext_marker = 0;
  } else if ((x < co_energies_x_.size()) && (y < co_energies_y_.size())) {
    marker_x = co_energies_x_[x];
    marker_y = co_energies_y_[y];
    PL_INFO << "Markers set at " << marker_x << ", " << marker_y;
  }
  emit markers_set(marker_x, marker_y);
  replot_markers();
}

void FormPlot2D::set_marker(double n) {
  ext_marker = n;
  if (!ext_marker) {
    marker_x = 0;
    marker_y = 0;
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
    marker_x = 0;
    marker_y = 0;
    ext_marker = 0;
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
