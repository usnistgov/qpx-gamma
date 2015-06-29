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
 *      FormAnalysis1D -
 *
 ******************************************************************************/

#include "form_analysis_1d.h"
#include "widget_detectors.h"
#include "ui_form_analysis_1d.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormAnalysis1D::FormAnalysis1D(QSettings &settings, XMLableDB<Gamma::Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormAnalysis1D),
  detectors_(newDetDB),
  settings_(settings)
{
  ui->setupUi(this);

  others_have_det_ = false;
  DB_has_detector_ = false;

  loadSettings();

  connect(ui->plotSpectrum, SIGNAL(peaks_changed()), this, SLOT(update_peaks()));
  connect(ui->plotSpectrum, SIGNAL(peaks_selected()), this, SLOT(update_peak_choice()));
  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));

  my_energy_calibration_ = new FormEnergyCalibration(settings_, detectors_);
  ui->tabs->addTab(my_energy_calibration_, "Energy calibration");
  connect(my_energy_calibration_, SIGNAL(detectorsChanged()), this, SLOT(detectorsUpdated()));
  connect(my_energy_calibration_, SIGNAL(peaks_chosen(std::set<double>)), this, SLOT(update_peak_choice(std::set<double>)));
  ui->tabs->setCurrentWidget(my_energy_calibration_);

  my_fwhm_calibration_ = new FormFwhmCalibration(settings_, detectors_);
  ui->tabs->addTab(my_fwhm_calibration_, "FWHM calibration");
  connect(my_fwhm_calibration_, SIGNAL(detectorsChanged()), this, SLOT(detectorsUpdated()));
  connect(my_fwhm_calibration_, SIGNAL(peaks_chosen(std::set<double>)), this, SLOT(update_peak_choice(std::set<double>)));
  ui->tabs->setCurrentWidget(my_fwhm_calibration_);
}

FormAnalysis1D::~FormAnalysis1D()
{
  delete ui;
}

void FormAnalysis1D::closeEvent(QCloseEvent *event) {
  /*  if (!ui->isotopes->save_close()) {
    event->ignore();
    return;
  }*/

  saveSettings();
  event->accept();
}

void FormAnalysis1D::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

  ui->plotSpectrum->loadSettings(settings_);
}

void FormAnalysis1D::saveSettings() {
  ui->plotSpectrum->saveSettings(settings_);
}

void FormAnalysis1D::clear() {
  ui->plotSpectrum->setSpectrum(nullptr);
  current_spectrum_.clear();
  detector_ = Gamma::Detector();
}


void FormAnalysis1D::setSpectrum(Pixie::SpectraSet *newset, QString name) {
  clear();
  spectra_ = newset;
  if (!spectra_) {
    ui->plotSpectrum->setSpectrum(nullptr);
    return;
  }

  current_spectrum_ = name;
  Pixie::Spectrum::Spectrum *spectrum = spectra_->by_name(current_spectrum_.toStdString());
  ui->plotSpectrum->setSpectrum(spectrum);
  my_energy_calibration_->setSpectrum(spectrum);
  my_fwhm_calibration_->setSpectrum(spectrum);
  toggle_radio();
}

void FormAnalysis1D::update_spectrum() {
  ui->plotSpectrum->update_spectrum();
}

void FormAnalysis1D::update_peaks() {
  std::vector<Gamma::Peak> pks = ui->plotSpectrum->peaks();
  my_energy_calibration_->update_peaks(pks);
  my_fwhm_calibration_->update_peaks(pks);
}

void FormAnalysis1D::update_peak_choice(std::set<double> pks) {
  ui->plotSpectrum->select_peaks(pks);
}

void FormAnalysis1D::update_peak_choice() {
  std::set<double> pks = ui->plotSpectrum->selected_peaks();
  my_energy_calibration_->update_peak_selection(pks);
  my_fwhm_calibration_->update_peak_selection(pks);
}

void FormAnalysis1D::toggle_radio() {

  Pixie::Spectrum::Spectrum* spectrum;

  for (auto &q : spectra_->spectra()) {
    if (q && (q->name() != current_spectrum_.toStdString())) {
      for (auto &p : q->get_detectors())
        if (p.shallow_equals(detector_))
          others_have_det_ = true;
    }
  }

  if (detectors_.has_a(detector_))
    DB_has_detector_ = true;

  //tell children
}
