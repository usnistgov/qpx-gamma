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

  moving.appearance.themes["light"] = QPen(Qt::darkGray, 2);
  moving.appearance.themes["dark"] = QPen(Qt::white, 2);

  markx.appearance.themes["light"] = QPen(Qt::darkRed, 1);
  markx.appearance.themes["dark"] = QPen(Qt::yellow, 1);
  marky = markx;

  connect(ui->mcaPlot, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->mcaPlot, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));

  connect(ui->spectraWidget, SIGNAL(contextRequested()), this, SLOT(spectrumDetails()));
  connect(ui->spectraWidget, SIGNAL(stateChanged()), this, SLOT(spectraLooksChanged()));

  bits = 0;
}

FormPlot1D::~FormPlot1D()
{
  delete ui;
}

void FormPlot1D::setSpectra(Pixie::SpectraSet& new_set) {
  mySpectra = &new_set;

  std::set<uint32_t> resolutions = mySpectra->resolutions(1);
  ui->comboResolution->clear();
  for (auto &q: resolutions)
    ui->comboResolution->addItem(QString::number(q) + " bit", QVariant(q));

}

void FormPlot1D::spectraLooksChanged() {
  spectrumDetails();
  mySpectra->activate();
}

void FormPlot1D::spectrumDetails()
{
  ui->pushShowAll->setEnabled(ui->spectraWidget->available_count());
  ui->pushHideAll->setEnabled(ui->spectraWidget->available_count());

  QString id = ui->spectraWidget->selected();
  Pixie::Spectrum::Spectrum* someSpectrum = mySpectra->by_name(id.toStdString());

  Pixie::Spectrum::Metadata md;
  if (someSpectrum)
    md = someSpectrum->metadata();

  if (id.isEmpty() || (someSpectrum == nullptr)) {
    ui->labelSpectrumInfo->setText("Left-click on spectrum above to see statistics, right click to toggle visibility");
    ui->pushAnalyse->setEnabled(false);
    ui->pushFullInfo->setEnabled(false);
    return;
  }

  std::string type = someSpectrum->type();
  double real = md.real_time.total_milliseconds() * 0.001;
  double live = md.live_time.total_milliseconds() * 0.001;
  double dead = 100;
  double rate = 0;
  Gamma::Detector det = Gamma::Detector();
  if (!md.detectors.empty())
    det = md.detectors[0];

  QString detstr("Detector: ");
  detstr += QString::fromStdString(det.name_);
  if (det.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits)))
    detstr += " [ENRG]";
  else if (det.highest_res_calib().units_ != "channels")
    detstr += " (enrg)";
  if (det.fwhm_calibration_.valid())
    detstr += " [FWHM]";

  if (real > 0) {
    dead = (real - live) * 100.0 / real;
    rate = md.total_count.convert_to<double>() / real;
  }

  QString infoText =
      "<nobr>" + id + "(" + QString::fromStdString(type) + ")</nobr><br/>"
      "<nobr>" + detstr + "</nobr><br/>"
      "<nobr>Count: " + QString::number(md.total_count.convert_to<double>()) + "</nobr><br/>"
      "<nobr>Rate: " + QString::number(rate) + "cps</nobr><br/>"
      "<nobr>Live:  " + QString::number(live) + "s</nobr><br/>"
      "<nobr>Real:  " + QString::number(real) + "s</nobr><br/>"
      "<nobr>Dead:  " + QString::number(dead) + "%</nobr><br/>";

  ui->labelSpectrumInfo->setText(infoText);
  ui->pushAnalyse->setEnabled(true);
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
  int newbits = ui->comboResolution->currentData().toInt();
  ui->spectraWidget->setQpxSpectra(*mySpectra, 1, newbits);
  if (bits != newbits) {
    bits = newbits;
    moving.shift(bits);
    markx.shift(bits);
    marky.shift(bits);
  }

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

  std::map<double, double> minima, maxima;

  calib_ = Gamma::Calibration();

  ui->mcaPlot->clearGraphs();
  for (auto &q: mySpectra->spectra(1, bits)) {
    Pixie::Spectrum::Metadata md;
    if (q)
      md = q->metadata();

    if (md.visible && (md.resolution > 0) && (md.total_count > 0)) {

      QVector<double> y(md.resolution);

      std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data =
          std::move(q->get_spectrum({{0, y.size()}}));
      std::vector<double> energies = q->energies(0);

      Gamma::Detector detector = Gamma::Detector();
      if (!md.detectors.empty())
       detector = md.detectors[0];
      Gamma::Calibration temp_calib;
      if (detector.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits)))
        temp_calib = detector.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
      else
        temp_calib = detector.highest_res_calib();
      if (temp_calib.bits_)
        calib_ = temp_calib;

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

      AppearanceProfile profile;
      profile.default_pen = QPen(QColor::fromRgba(md.appearance), 1);
      ui->mcaPlot->addGraph(QVector<double>::fromStdVector(energies), y, profile);

    }
  }
  if (!calib_.bits_)
    calib_.bits_ = bits;

  ui->mcaPlot->use_calibrated(calib_.units_ != "channels");
  ui->mcaPlot->setLabels(QString::fromStdString(calib_.units_), "count");
  ui->mcaPlot->setYBounds(minima, maxima);

  calibrate_markers();
  replot_markers();

  std::string new_label = boost::algorithm::trim_copy(mySpectra->status());
  ui->mcaPlot->setTitle(QString::fromStdString(new_label));

  spectrumDetails();

  PL_DBG << "<Plot1D> plotting took " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}

void FormPlot1D::on_pushFullInfo_clicked()
{
  Pixie::Spectrum::Spectrum* someSpectrum = mySpectra->by_name(ui->spectraWidget->selected().toStdString());
  if (someSpectrum == nullptr)
    return;

  dialog_spectrum* newSpecDia = new dialog_spectrum(*someSpectrum, this);
  connect(newSpecDia, SIGNAL(finished(bool)), this, SLOT(spectrumDetailsClosed(bool)));
  connect(newSpecDia, SIGNAL(delete_spectrum()), this, SLOT(spectrumDetailsDelete()));
  newSpecDia->exec();
}

void FormPlot1D::spectrumDetailsDelete()
{
  std::string name = ui->spectraWidget->selected().toStdString();

  PL_INFO << "will delete " << name;
  mySpectra->delete_spectrum(name);

  updateUI();
}

void FormPlot1D::updateUI()
{
  std::set<uint32_t> resolutions = mySpectra->resolutions(1);

  if (resolutions.count(bits) == 1) {
    ui->spectraWidget->setQpxSpectra(*mySpectra, 1, bits);
    mySpectra->activate();
  } else {
    ui->comboResolution->clear();
    for (auto &q: resolutions)
      ui->comboResolution->addItem(QString::number(q) + " bit", QVariant(q));
  }
}

void FormPlot1D::on_pushAnalyse_clicked()
{
  emit requestAnalysis(ui->spectraWidget->selected());
}

void FormPlot1D::addMovingMarker(double x) {
  PL_INFO << "<Plot1D> marker at " << x;
  moving.channel = x;
  moving.bits = bits;
  moving.visible = true;
  if (calib_.units_ != "channels") {
    moving.energy = x;
    moving.energy_valid = true;
    moving.chan_valid = false;
    emit marker_set(moving);
  } else {
    moving.chan_valid = true;
    moving.energy_valid = false;
    calibrate_markers();
    emit marker_set(moving);
  }
  replot_markers();
}

void FormPlot1D::removeMovingMarker(double x) {
  moving.visible = false;
  markx.visible = false;
  marky.visible = false;
  emit marker_set(moving);
  replot_markers();
}

void FormPlot1D::set_markers2d(Marker x, Marker y) {
  x.appearance = markx.appearance;
  y.appearance = marky.appearance;

  markx = x; marky = y;
  markx.shift(bits);
  marky.shift(bits);

  calibrate_markers();

  if (!x.visible && !y.visible)
    moving.visible = false;

  replot_markers();
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


void FormPlot1D::calibrate_markers() {
  if (!markx.energy_valid && markx.chan_valid) {
    markx.energy = calib_.transform(markx.channel, markx.bits);
    markx.energy_valid = (calib_.units_ != "channels");
  }

  if (!marky.energy_valid && marky.chan_valid) {
    marky.energy = calib_.transform(marky.channel, marky.bits);
    marky.energy_valid = (calib_.units_ != "channels");
  }

  if (!moving.energy_valid && moving.chan_valid) {
    moving.energy = calib_.transform(moving.channel, moving.bits);
    moving.energy_valid = (calib_.units_ != "channels");
  }
}

void FormPlot1D::on_pushShowAll_clicked()
{
  ui->spectraWidget->show_all();
}

void FormPlot1D::on_pushHideAll_clicked()
{
  ui->spectraWidget->hide_all();
}

void FormPlot1D::set_scale_type(QString sct) {
  ui->mcaPlot->set_scale_type(sct);
}

void FormPlot1D::set_plot_style(QString stl) {
  ui->mcaPlot->set_plot_style(stl);
}

QString FormPlot1D::scale_type() {
  return ui->mcaPlot->scale_type();
}

QString FormPlot1D::plot_style() {
  return ui->mcaPlot->plot_style();
}
