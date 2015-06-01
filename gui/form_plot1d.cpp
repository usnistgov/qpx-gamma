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

#include "form_plot1d.h"
#include "ui_form_plot1d.h"
#include "dialog_spectrum.h"
#include "custom_timer.h"

FormPlot1D::FormPlot1D(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormPlot1D)
{
  ui->setupUi(this);

  moving.themes["light"] = QPen(Qt::darkGray, 2);
  moving.themes["dark"] = QPen(Qt::white, 2);

  markx.themes["light"] = QPen(Qt::darkRed, 1);
  markx.themes["dark"] = QPen(Qt::yellow, 1);
  marky = markx;

  connect(ui->mcaPlot, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->mcaPlot, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));

  connect(ui->spectraWidget, SIGNAL(contextRequested()), this, SLOT(spectrumDetails()));
  connect(ui->spectraWidget, SIGNAL(stateChanged()), this, SLOT(spectraLooksChanged()));
}

FormPlot1D::~FormPlot1D()
{
  delete ui;
}

void FormPlot1D::addMovingMarker(double x) {
  moving.position = x;
  moving.visible = true;
  PL_INFO << "Marker at " << x;
  replot_markers();
  emit marker_set(x);
}

void FormPlot1D::removeMovingMarker(double x) {
  moving.visible = false;
  markx.visible = false;
  marky.visible = false;
  emit marker_set(0);
  replot_markers();
}


void FormPlot1D::setSpectra(Pixie::SpectraSet& new_set) {
  mySpectra = &new_set;

  std::list<uint32_t> resolutions = mySpectra->resolutions(1);
  ui->comboResolution->clear();
  for (auto &q: resolutions)
    ui->comboResolution->addItem(QString::number(q) + " bit", QVariant(q));

}

void FormPlot1D::set_markers2d(double x, double y) {
  markx.position = x;
  marky.position = y;
  if (!x && !y)
    removeMovingMarker(0);
  else {
    markx.position = x;
    marky.position = y;
    markx.visible = true;
    marky.visible = true;
    replot_markers();
  }
}

void FormPlot1D::replot_markers() {
  std::list<Marker> markers;

  markers.push_back(moving);
  markers.push_back(markx);
  markers.push_back(marky);

  ui->mcaPlot->set_markers(markers);
  ui->mcaPlot->replot_markers();
  ui->mcaPlot->redraw();
}

void FormPlot1D::spectraLooksChanged() {
  ui->spectraWidget->update_looks();
  spectrumDetails();
  mySpectra->activate();
}

void FormPlot1D::spectrumDetails()
{
  QString id = ui->spectraWidget->selected();
  Pixie::Spectrum::Spectrum* someSpectrum = mySpectra->by_name(id.toStdString());

  if (someSpectrum == nullptr) {
    ui->labelSpectrumInfo->setText("Select spectrum above to see statistics");
    ui->pushCalibrate->setEnabled(false);
    ui->pushFullInfo->setEnabled(false);
    return;
  }

  double real = someSpectrum->real_time().total_milliseconds() * 0.001;
  double live = someSpectrum->live_time().total_milliseconds() * 0.001;
  double dead = 100;
  if (real > 0)
    dead = (real - live) * 100.0 / real;

  QString infoText =
      "<nobr>" + id + "(" + QString::fromStdString(someSpectrum->type()) + ")</nobr><br/>"
      "<nobr>Count: " + QString::number(someSpectrum->total_count()) + "</nobr><br/>"
      "<nobr>Live:  " + QString::number(live) + "s</nobr><br/>"
      "<nobr>Real:  " + QString::number(real) + "s</nobr><br/>"
      "<nobr>Dead:  " + QString::number(dead) + "%</nobr><br/>";

  ui->labelSpectrumInfo->setText(infoText);
  ui->pushCalibrate->setEnabled(true);
  ui->pushFullInfo->setEnabled(true);
}

void FormPlot1D::spectrumDetailsClosed(bool changed) {
  if (changed) {
    ui->spectraWidget->update_looks();
    mySpectra->activate();
  }
}

void FormPlot1D::on_comboResolution_currentIndexChanged(int index)
{
  ui->spectraWidget->setQpxSpectra(*mySpectra, 1, ui->comboResolution->currentData().toInt());
  mySpectra->activate();
}

void FormPlot1D::reset_content() {
  moving.visible = false;
  markx.visible = false;
  marky.visible = false;
  ui->mcaPlot->reset_scales();
  ui->mcaPlot->clearGraphs();
  ui->mcaPlot->clearExtras();
  ui->mcaPlot->replot_markers();
  ui->mcaPlot->rescale();
  ui->mcaPlot->redraw();
}

void FormPlot1D::update_plot() {

  this->setCursor(Qt::WaitCursor);
  CustomTimer guiside(true);

  //bool new_data = mySpectra->new_data();
  std::map<double, double> minima, maxima;

  ui->mcaPlot->clearGraphs();
  for (auto &q: mySpectra->spectra(1, ui->comboResolution->currentData().toInt())) {
    if (q && q->visible() && q->resolution() && q->total_count()) {
      uint32_t res = q->resolution();
      uint32_t app = q->appearance();

      if (res) {

      QVector<double> y(q->resolution());

      std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data =
          std::move(q->get_spectrum({{0, y.size()}}));

      std::vector<double> energies = q->energies(0);
      int i = 0;
      for (auto it : *spectrum_data) {
        double xx = energies[i];
        double yy = it.second;
        y[it.first[0]] = yy;
        if (!minima.count(xx) || (minima[energies[i]] > yy))
          minima[xx] = yy;
        if (!maxima.count(xx) || (maxima[energies[i]] < yy))
          maxima[xx] = yy;
        ++i;
      }

      ui->mcaPlot->addGraph(QVector<double>::fromStdVector(energies), y, QColor::fromRgba(app), 1);
      }
    }
  }
  ui->mcaPlot->setLabels("keV", "count");

  ui->mcaPlot->setYBounds(minima, maxima);
  replot_markers();

  std::string new_label = boost::algorithm::trim_copy(mySpectra->status());
  ui->mcaPlot->setTitle(QString::fromStdString(new_label));

  spectrumDetails();

  PL_DBG << "1D plotting took " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}

void FormPlot1D::on_pushFullInfo_clicked()
{
  Pixie::Spectrum::Spectrum* someSpectrum = mySpectra->by_name(ui->spectraWidget->selected().toStdString());
  if (someSpectrum == nullptr)
    return;

  dialog_spectrum* newSpecDia = new dialog_spectrum(*someSpectrum, this);
  connect(newSpecDia, SIGNAL(finished(bool)), this, SLOT(spectrumDetailsClosed(bool)));
  newSpecDia->exec();
}

void FormPlot1D::on_pushCalibrate_clicked()
{
  emit requestCalibration(ui->spectraWidget->selected());
}
