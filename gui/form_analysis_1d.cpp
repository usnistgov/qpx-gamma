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
#include <QInputDialog>

FormAnalysis1D::FormAnalysis1D(QSettings &settings, XMLableDB<Gamma::Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormAnalysis1D),
  detectors_(newDetDB),
  settings_(settings)
{
  ui->setupUi(this);

  loadSettings();

  ui->plotSpectrum->setFit(&fit_data_);

  connect(ui->plotSpectrum, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));
  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));

  my_energy_calibration_ = new FormEnergyCalibration(settings_, detectors_, fit_data_);
  ui->tabs->addTab(my_energy_calibration_, "Energy calibration");
  connect(my_energy_calibration_, SIGNAL(detectorsChanged()), this, SLOT(detectorsUpdated()));
  connect(my_energy_calibration_, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks_from_nrg(bool)));
  connect(my_energy_calibration_, SIGNAL(update_detector()), this, SLOT(update_detector_calibs()));

  my_fwhm_calibration_ = new FormFwhmCalibration(settings_, detectors_, fit_data_);
  ui->tabs->addTab(my_fwhm_calibration_, "FWHM calibration");
  connect(my_fwhm_calibration_, SIGNAL(detectorsChanged()), this, SLOT(detectorsUpdated()));
  connect(my_fwhm_calibration_, SIGNAL(update_detector()), this, SLOT(update_detector_calibs()));
  connect(my_fwhm_calibration_, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks_from_fwhm(bool)));

  my_peak_fitter_ = new FormPeakFitter(settings_, fit_data_);
  ui->tabs->addTab(my_peak_fitter_, "Peak integration");
  connect(my_peak_fitter_, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks_from_fitter(bool)));

  ui->tabs->setCurrentWidget(my_energy_calibration_);
}

FormAnalysis1D::~FormAnalysis1D()
{
  delete ui;
}

void FormAnalysis1D::closeEvent(QCloseEvent *event) {
  if (!my_energy_calibration_->save_close()) {
    event->ignore();
    return;
  }

  if (!my_fwhm_calibration_->save_close()) {
    event->ignore();
    return;
  }

  ui->plotSpectrum->saveSettings(settings_);

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
  my_energy_calibration_->clear();
  my_fwhm_calibration_->clear();
  my_peak_fitter_->clear();
  current_spectrum_.clear();
  detector_ = Gamma::Detector();
  nrg_calibration_ = Gamma::Calibration();
  fwhm_calibration_ = Gamma::Calibration();
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

  if (spectrum && spectrum->resolution()) {
    int bits = spectrum->bits();
    nrg_calibration_ = Gamma::Calibration("Energy", bits);
    fwhm_calibration_ = Gamma::Calibration("FWHM", bits);
    detector_ = spectrum->get_detector(0);

    if (detector_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits))) {
      nrg_calibration_ = detector_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
      PL_INFO << "<Analysis> Energy calibration drawn from detector \"" << detector_.name_ << "\"   "
              << nrg_calibration_.to_string();
    } else
      PL_INFO << "<Analysis> No existing calibration for this resolution";

    if (detector_.fwhm_calibration_.coefficients_ != std::vector<double>({0, 1})) {
      fwhm_calibration_ = detector_.fwhm_calibration_;
      PL_INFO << "<Analysis> FWHM calibration drawn from detector \"" << detector_.name_ << "\"   "
              << fwhm_calibration_.to_string();
    } else
      PL_INFO << "<Analysis> No existing FWHM calibration for this resolution";


    fit_data_.fwhm_cali_ = fwhm_calibration_;
    fit_data_.nrg_cali_ = nrg_calibration_;

    my_energy_calibration_->setData(nrg_calibration_, bits);
    my_fwhm_calibration_->setData(fwhm_calibration_, bits);

    my_energy_calibration_->update_peaks(true);
    my_fwhm_calibration_->update_peaks(true);
    my_peak_fitter_->update_peaks(true);
  }

  ui->plotSpectrum->setSpectrum(spectrum);
  ui->plotSpectrum->setData(nrg_calibration_, fwhm_calibration_);
}

void FormAnalysis1D::update_spectrum() {
  if (this->isVisible())
    ui->plotSpectrum->update_spectrum();
}

void FormAnalysis1D::update_peaks(bool content_changed) {
  my_energy_calibration_->update_peaks(content_changed);
  my_fwhm_calibration_->update_peaks(content_changed);
  my_peak_fitter_->update_peaks(content_changed);
}

void FormAnalysis1D::update_peaks_from_fwhm(bool content_changed) {
  my_peak_fitter_->update_peaks(content_changed);
  ui->plotSpectrum->update_fit(content_changed);
  my_energy_calibration_->update_peaks(content_changed);
}

void FormAnalysis1D::update_peaks_from_nrg(bool content_changed) {
  my_fwhm_calibration_->update_peaks(content_changed);
  my_peak_fitter_->update_peaks(content_changed);
  ui->plotSpectrum->update_fit(content_changed);
}

void FormAnalysis1D::update_peaks_from_fitter(bool content_changed) {
  my_energy_calibration_->update_peaks(content_changed);
  my_fwhm_calibration_->update_peaks(content_changed);
  ui->plotSpectrum->update_fit(content_changed);
}

void FormAnalysis1D::update_detector_calibs()
{
  nrg_calibration_ = my_energy_calibration_->get_new_calibration();
  fwhm_calibration_ = my_fwhm_calibration_->get_new_calibration();

  std::string msg_text("Propagating calibrations ");
  msg_text +=  "<nobr>" + nrg_calibration_.to_string() + "</nobr><br/>"
               "<nobr>" + fwhm_calibration_.to_string() + "</nobr><br/>"
               "<nobr>  to all spectra in current project: </nobr><br/>"
               "<nobr>" + spectra_->status() + "</nobr>";

  std::string question_text("Do you also want to save this calibration to ");
  question_text += detector_.name_ + " in detector database?";

  QMessageBox msgBox;
  msgBox.setText(QString::fromStdString(msg_text));
  msgBox.setInformativeText(QString::fromStdString(question_text));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Abort);
  msgBox.setDefaultButton(QMessageBox::No);
  msgBox.setIcon(QMessageBox::Question);
  int ret = msgBox.exec();

  Gamma::Detector modified;

  if (ret == QMessageBox::Yes) {
    if (!detectors_.has_a(detector_)) {
      bool ok;
      QString text = QInputDialog::getText(this, "New Detector",
                                           "Detector name:", QLineEdit::Normal,
                                           QString::fromStdString(detector_.name_),
                                           &ok);

      if (!ok)
        return;

      if (!text.isEmpty()) {
        modified = detector_;
        modified.name_ = text.toStdString();
        if (detectors_.has_a(modified)) {
          QMessageBox::warning(this, "Already exists", "Detector " + text + " already exists. Will not save to database.", QMessageBox::Ok);
          modified = Gamma::Detector();
        }
      }
    } else
      modified = detectors_.get(detector_);

    if (modified != Gamma::Detector())
    {
      PL_INFO << "   applying new calibrations for " << modified.name_ << " in detector database";
      modified.energy_calibrations_.replace(nrg_calibration_);
      modified.fwhm_calibration_ = fwhm_calibration_;
      detectors_.replace(modified);
      emit detectorsChanged();
    }
  }

  if (ret != QMessageBox::Abort) {
    for (auto &q : spectra_->spectra()) {
      if (q == nullptr)
        continue;
      std::vector<Gamma::Detector> detectors = q->get_detectors();
      for (auto &p : detectors) {
        if (p.shallow_equals(detector_)) {
          PL_INFO << "   applying new calibrations for " << detector_.name_ << " on " << q->name();
          p.energy_calibrations_.replace(nrg_calibration_);
          p.fwhm_calibration_ = fwhm_calibration_;
        }
      }
      q->set_detectors(detectors);
    }

    std::vector<Gamma::Detector> detectors = spectra_->runInfo().p4_state.get_detectors();
    for (auto &p : detectors) {
      if (p.shallow_equals(detector_)) {
        PL_INFO << "   applying new calibrations for " << detector_.name_ << " in current project " << spectra_->status();
        p.energy_calibrations_.replace(nrg_calibration_);
        p.fwhm_calibration_ = fwhm_calibration_;
      }
    }
    Pixie::RunInfo ri = spectra_->runInfo();
    for (int i=0; i < detectors.size(); ++i)
      ri.p4_state.set_detector(Pixie::Channel(i), detectors[i]);
    spectra_->setRunInfo(ri);

    ui->plotSpectrum->setData(nrg_calibration_, fwhm_calibration_);
    my_energy_calibration_->setData(nrg_calibration_, nrg_calibration_.bits_);
    my_fwhm_calibration_->setData(fwhm_calibration_, fwhm_calibration_.bits_);
  }
}
