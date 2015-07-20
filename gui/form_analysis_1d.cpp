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

  connect(ui->plotSpectrum, SIGNAL(peaks_changed(std::vector<Gamma::Peak>,bool)), this, SLOT(update_peaks(std::vector<Gamma::Peak>,bool)));
  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));

  my_energy_calibration_ = new FormEnergyCalibration(settings_, detectors_);
  ui->tabs->addTab(my_energy_calibration_, "Energy calibration");
  connect(my_energy_calibration_, SIGNAL(detectorsChanged()), this, SLOT(detectorsUpdated()));
  connect(my_energy_calibration_, SIGNAL(peaks_changed(std::vector<Gamma::Peak>,bool)), this, SLOT(update_peaks_from_nrg(std::vector<Gamma::Peak>,bool)));
  connect(my_energy_calibration_, SIGNAL(update_detector(bool,bool)), this, SLOT(update_detector(bool,bool)));
  ui->tabs->setCurrentWidget(my_energy_calibration_);

  my_fwhm_calibration_ = new FormFwhmCalibration(settings_, detectors_);
  ui->tabs->addTab(my_fwhm_calibration_, "FWHM calibration");
  connect(my_fwhm_calibration_, SIGNAL(detectorsChanged()), this, SLOT(detectorsUpdated()));
  connect(my_fwhm_calibration_, SIGNAL(update_detector(bool,bool)), this, SLOT(update_detector(bool,bool)));
  connect(my_fwhm_calibration_, SIGNAL(peaks_changed(std::vector<Gamma::Peak>, bool)), this, SLOT(update_peaks_from_fwhm(std::vector<Gamma::Peak>, bool)));
  ui->tabs->setCurrentWidget(my_fwhm_calibration_);
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

  if (spectrum) {
    int bits = spectrum->bits();
    for (std::size_t i=0; i < spectrum->add_pattern().size(); i++) {
      if (spectrum->add_pattern()[i] == 1) {
        detector_ = spectrum->get_detectors()[i];

        //what if multiple available?

        if (detector_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits))) {
          nrg_calibration_ = detector_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
          PL_INFO << "<Analysis> Energy calibration drawn from detector \"" << detector_.name_ << "\"";
        } else
          PL_INFO << "<Analysis> No existing calibration for this resolution";

        if (detector_.fwhm_calibrations_.has_a(Gamma::Calibration("FWHM", bits))) {
          fwhm_calibration_ = detector_.fwhm_calibrations_.get(Gamma::Calibration("FWHM", bits));
          PL_INFO << "<Analysis> FWHM calibration drawn from detector \"" << detector_.name_ << "\"";
        } else
          PL_INFO << "<Analysis> No existing FWHM calibration for this resolution";

      }
    }

    my_energy_calibration_->clear();
    my_energy_calibration_->setData(nrg_calibration_, bits);

    my_fwhm_calibration_->clear();
    my_fwhm_calibration_->setData(fwhm_calibration_, bits);

  }
}

void FormAnalysis1D::update_detector(bool in_spectra, bool in_DB) {
  nrg_calibration_ = my_energy_calibration_->get_new_calibration();
  fwhm_calibration_ = my_fwhm_calibration_->get_new_calibration();

  if (in_DB) {
    Gamma::Detector modified = Gamma::Detector();
    if (!detectors_.has_a(detector_)) {
      bool ok;
      QString text = QInputDialog::getText(this, "New Detector",
                                           "Detector name:", QLineEdit::Normal,
                                           QString::fromStdString(detector_.name_),
                                           &ok);
      if (ok && !text.isEmpty()) {
        modified = detector_;
        modified.name_ = text.toStdString();
        if (detectors_.has_a(modified)) {
          QMessageBox::warning(this, "Already exists", "Detector " + text + " already exists", QMessageBox::Ok);
          modified = Gamma::Detector();
        } else {
          //change name in spectrum or spectra?
        }
      }
    } else
      modified = detectors_.get(detector_);

    if (modified != Gamma::Detector())
    {
      PL_INFO << "   applying new calibrations for " << modified.name_ << " in detector database";
      modified.energy_calibrations_.replace(nrg_calibration_);
      modified.fwhm_calibrations_.replace(fwhm_calibration_);
      detectors_.replace(modified);
      emit detectorsChanged();
    }
  }

  if (in_spectra) {
    for (auto &q : spectra_->spectra()) {
      if (q == nullptr)
        continue;
        std::vector<Gamma::Detector> detectors = q->get_detectors();
        for (auto &p : detectors) {
          if (p.shallow_equals(detector_)) {
            PL_INFO << "   applying new calibrations for " << detector_.name_ << " on " << q->name();
            p.energy_calibrations_.replace(nrg_calibration_);
            p.fwhm_calibrations_.replace(fwhm_calibration_);
          }
        }
        q->set_detectors(detectors);
    }

    std::vector<Gamma::Detector> detectors = spectra_->runInfo().p4_state.get_detectors();
    for (auto &p : detectors) {
      if (p.shallow_equals(detector_)) {
        PL_INFO << "   applying new calibrations for " << detector_.name_ << " in spectra set " << spectra_->status();
        p.energy_calibrations_.replace(nrg_calibration_);
        p.fwhm_calibrations_.replace(fwhm_calibration_);
      }
    }
    Pixie::RunInfo ri = spectra_->runInfo();
    for (int i=0; i < detectors.size(); ++i)
      ri.p4_state.set_detector(Pixie::Channel(i), detectors[i]);
    spectra_->setRunInfo(ri);
  }

  my_energy_calibration_->setData(nrg_calibration_, nrg_calibration_.bits_);
  my_fwhm_calibration_->setData(fwhm_calibration_, fwhm_calibration_.bits_);

}


void FormAnalysis1D::update_spectrum() {
  if (this->isVisible())
    ui->plotSpectrum->update_spectrum();
}

void FormAnalysis1D::update_peaks(std::vector<Gamma::Peak> pks, bool content_changed) {
  my_energy_calibration_->update_peaks(pks);
  my_fwhm_calibration_->update_peaks(pks);
}

void FormAnalysis1D::update_peaks_from_fwhm(std::vector<Gamma::Peak> pks, bool content_changed) {
  my_energy_calibration_->update_peaks(pks);
  ui->plotSpectrum->set_peaks(pks, content_changed);
}

void FormAnalysis1D::update_peaks_from_nrg(std::vector<Gamma::Peak> pks, bool content_changed) {
  my_fwhm_calibration_->update_peaks(pks);
  ui->plotSpectrum->set_peaks(pks, content_changed);
}
