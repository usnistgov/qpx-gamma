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

FormAnalysis1D::FormAnalysis1D(QSettings &settings, XMLableDB<Qpx::Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormAnalysis1D),
  detectors_(newDetDB),
  settings_(settings)
{
  ui->setupUi(this);

  loadSettings();

  ui->plotSpectrum->setFit(&fit_data_);

  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));

  form_energy_calibration_ = new FormEnergyCalibration(settings_, detectors_, fit_data_);
  ui->tabs->addTab(form_energy_calibration_, "Energy calibration");
  connect(form_energy_calibration_, SIGNAL(detectorsChanged()), this, SLOT(detectorsUpdated()));
  connect(form_energy_calibration_, SIGNAL(update_detector()), this, SLOT(update_detector_calibs()));

  form_fwhm_calibration_ = new FormFwhmCalibration(settings_, detectors_, fit_data_);
  ui->tabs->addTab(form_fwhm_calibration_, "FWHM calibration");
  connect(form_fwhm_calibration_, SIGNAL(detectorsChanged()), this, SLOT(detectorsUpdated()));
  connect(form_fwhm_calibration_, SIGNAL(update_detector()), this, SLOT(update_detector_calibs()));

  form_fit_results_ = new FormFitResults(settings_, fit_data_);
  ui->tabs->addTab(form_fit_results_, "Peak integration");
  connect(form_fit_results_, SIGNAL(save_peaks_request()), this, SLOT(save_report()));


  connect(form_energy_calibration_, SIGNAL(selection_changed(std::set<double>)), form_fwhm_calibration_, SLOT(update_selection(std::set<double>)));
  connect(form_energy_calibration_, SIGNAL(selection_changed(std::set<double>)), form_fit_results_, SLOT(update_selection(std::set<double>)));
  connect(form_energy_calibration_, SIGNAL(selection_changed(std::set<double>)), ui->plotSpectrum, SLOT(set_selected_peaks(std::set<double>)));

  connect(form_fwhm_calibration_, SIGNAL(selection_changed(std::set<double>)), form_energy_calibration_, SLOT(update_selection(std::set<double>)));
  connect(form_fwhm_calibration_, SIGNAL(selection_changed(std::set<double>)), form_fit_results_, SLOT(update_selection(std::set<double>)));
  connect(form_fwhm_calibration_, SIGNAL(selection_changed(std::set<double>)), ui->plotSpectrum, SLOT(set_selected_peaks(std::set<double>)));

  connect(form_fit_results_, SIGNAL(selection_changed(std::set<double>)), form_fwhm_calibration_, SLOT(update_selection(std::set<double>)));
  connect(form_fit_results_, SIGNAL(selection_changed(std::set<double>)), form_energy_calibration_, SLOT(update_selection(std::set<double>)));
  connect(form_fit_results_, SIGNAL(selection_changed(std::set<double>)), ui->plotSpectrum, SLOT(set_selected_peaks(std::set<double>)));

  connect(ui->plotSpectrum, SIGNAL(peak_selection_changed(std::set<double>)), form_fwhm_calibration_, SLOT(update_selection(std::set<double>)));
  connect(ui->plotSpectrum, SIGNAL(peak_selection_changed(std::set<double>)), form_energy_calibration_, SLOT(update_selection(std::set<double>)));
  connect(ui->plotSpectrum, SIGNAL(peak_selection_changed(std::set<double>)), form_fit_results_, SLOT(update_selection(std::set<double>)));

  connect(ui->plotSpectrum, SIGNAL(data_changed()), form_energy_calibration_, SLOT(update_data()));
  connect(ui->plotSpectrum, SIGNAL(data_changed()), form_fwhm_calibration_, SLOT(update_data()));
  connect(ui->plotSpectrum, SIGNAL(data_changed()), form_fit_results_, SLOT(update_data()));

  connect(form_energy_calibration_, SIGNAL(change_peaks(std::vector<Qpx::Peak>)), ui->plotSpectrum, SLOT(replace_peaks(std::vector<Qpx::Peak>)));

  connect(form_energy_calibration_, SIGNAL(new_fit()), this, SLOT(update_fits()));
  connect(form_fwhm_calibration_, SIGNAL(new_fit()), this, SLOT(update_fits()));

  ui->tabs->setCurrentWidget(form_energy_calibration_);
}

FormAnalysis1D::~FormAnalysis1D()
{
  delete ui;
}

void FormAnalysis1D::closeEvent(QCloseEvent *event) {
  if (!form_energy_calibration_->save_close()) {
    event->ignore();
    return;
  }

  if (!form_fwhm_calibration_->save_close()) {
    event->ignore();
    return;
  }

  ui->plotSpectrum->saveSettings(settings_);

  PL_DBG << "closing analysis1d";
  saveSettings();
  event->accept();
}

void FormAnalysis1D::loadSettings() {
  settings_.beginGroup("Program");
  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();

  ui->plotSpectrum->loadSettings(settings_);
}

void FormAnalysis1D::saveSettings() {
//  settings_.beginGroup("Program");
//  settings_.setValue("save_directory", data_directory_);
//  settings_.endGroup();

  ui->plotSpectrum->saveSettings(settings_);
}

void FormAnalysis1D::clear() {
  fit_data_.clear();
  ui->plotSpectrum->update_spectrum();
  form_energy_calibration_->clear();
  form_fwhm_calibration_->clear();
  form_fit_results_->clear();
  new_energy_calibration_ = Qpx::Calibration();
  new_fwhm_calibration_ = Qpx::Calibration();
}


void FormAnalysis1D::setSpectrum(Qpx::SpectraSet *newset, QString name) {
  form_energy_calibration_->clear();
  form_fwhm_calibration_->clear();
  form_fit_results_->clear();
  spectra_ = newset;
  new_energy_calibration_ = Qpx::Calibration();
  new_fwhm_calibration_ = Qpx::Calibration();

  if (!spectra_) {
    fit_data_.clear();
    ui->plotSpectrum->update_spectrum();
    return;
  }

  Qpx::Spectrum::Spectrum *spectrum = spectra_->by_name(name.toStdString());

  if (spectrum && spectrum->bits()) {
    fit_data_.clear();
    fit_data_.setData(spectrum);

    form_energy_calibration_->newSpectrum();
    form_fwhm_calibration_->newSpectrum();

    form_energy_calibration_->update_data();
    form_fwhm_calibration_->update_data();
    form_fit_results_->update_data();

    new_energy_calibration_ = form_energy_calibration_->get_new_calibration();
    new_fwhm_calibration_ = form_fwhm_calibration_->get_new_calibration();
  }

  ui->plotSpectrum->update_spectrum();
}

void FormAnalysis1D::update_spectrum() {
  if (this->isVisible()) {
    Qpx::Spectrum::Spectrum *spectrum = spectra_->by_name(fit_data_.metadata_.name);
    if (spectrum && spectrum->bits())
      fit_data_.setData(spectrum);
    ui->plotSpectrum->update_spectrum();
  }
}

void FormAnalysis1D::update_fits() {
  PL_DBG << "calib fits updated locally";
  new_energy_calibration_ = form_energy_calibration_->get_new_calibration();
  new_fwhm_calibration_ = form_fwhm_calibration_->get_new_calibration();
}

void FormAnalysis1D::update_detector_calibs()
{
  std::string msg_text("Propagating calibrations ");
  msg_text +=  "<nobr>" + new_energy_calibration_.to_string() + "</nobr><br/>"
               "<nobr>" + new_fwhm_calibration_.to_string() + "</nobr><br/>"
               "<nobr>  to all spectra in current project: </nobr><br/>"
               "<nobr>" + spectra_->identity() + "</nobr>";

  std::string question_text("Do you also want to save this calibration to ");
  question_text += fit_data_.detector_.name_ + " in detector database?";

  QMessageBox msgBox;
  msgBox.setText(QString::fromStdString(msg_text));
  msgBox.setInformativeText(QString::fromStdString(question_text));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Abort);
  msgBox.setDefaultButton(QMessageBox::No);
  msgBox.setIcon(QMessageBox::Question);
  int ret = msgBox.exec();

  Qpx::Detector modified;

  if (ret == QMessageBox::Yes) {
    if (!detectors_.has_a(fit_data_.detector_)) {
      bool ok;
      QString text = QInputDialog::getText(this, "New Detector",
                                           "Detector name:", QLineEdit::Normal,
                                           QString::fromStdString(fit_data_.detector_.name_),
                                           &ok);

      if (!ok)
        return;

      if (!text.isEmpty()) {
        modified = fit_data_.detector_;
        modified.name_ = text.toStdString();
        if (detectors_.has_a(modified)) {
          QMessageBox::warning(this, "Already exists", "Detector " + text + " already exists. Will not save to database.", QMessageBox::Ok);
          modified = Qpx::Detector();
        }
      }
    } else
      modified = detectors_.get(fit_data_.detector_);

    if (modified != Qpx::Detector())
    {
      PL_INFO << "   applying new calibrations for " << modified.name_ << " in detector database";
      modified.energy_calibrations_.replace(new_energy_calibration_);
      modified.fwhm_calibration_ = new_fwhm_calibration_;
      detectors_.replace(modified);
      emit detectorsChanged();
    }
  }

  if (ret != QMessageBox::Abort) {
    for (auto &q : spectra_->spectra()) {
      if (q == nullptr)
        continue;
      Qpx::Spectrum::Metadata md = q->metadata();
      for (auto &p : md.detectors) {
        if (p.shallow_equals(fit_data_.detector_)) {
          PL_INFO << "   applying new calibrations for " << fit_data_.detector_.name_ << " on " << q->name();
          p.energy_calibrations_.replace(new_energy_calibration_);
          p.fwhm_calibration_ = new_fwhm_calibration_;
        }
      }
      q->set_detectors(md.detectors);
    }


    std::set<Qpx::Spill> spills = spectra_->spills();
    Qpx::Spill sp;
    if (spills.size())
      sp = *spills.end();
    for (auto &p : sp.detectors) {
      if (p.shallow_equals(fit_data_.detector_)) {
        PL_INFO << "   applying new calibrations for " << fit_data_.detector_.name_ << " in current project " << spectra_->identity();
        p.energy_calibrations_.replace(new_energy_calibration_);
        p.fwhm_calibration_ = new_fwhm_calibration_;
      }
    }
    spectra_->add_spill(&sp);
    update_spectrum();
  }
}

void FormAnalysis1D::save_report()
{
  QString fileName = CustomSaveFileDialog(this, "Save analysis report",
                                          data_directory_, "Plain text (*.txt)");
  if (validateFile(this, fileName, true)) {
    PL_INFO << "Writing report to " << fileName.toStdString();
    fit_data_.save_report(fileName.toStdString());
  }
}
