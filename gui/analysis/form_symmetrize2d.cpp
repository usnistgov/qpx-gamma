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
 *      FormSymmetrize2D -
 *
 ******************************************************************************/

#include "form_symmetrize2d.h"
#include "ui_form_symmetrize2d.h"
#include "daq_sink_factory.h"
#include "gamma_fitter.h"
#include "qt_util.h"
#include "manip2d.h"
#include <QInputDialog>
#include "qpx_util.h"

FormSymmetrize2D::FormSymmetrize2D(QSettings &settings, XMLableDB<Qpx::Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormSymmetrize2D),
  detectors_(newDetDB),
  settings_(settings),
  my_gain_calibration_(nullptr),
  gate_x(nullptr),
  gate_y(nullptr)
{
  ui->setupUi(this);

  initialized = false;

  loadSettings();

  ui->plotSpectrum->setFit(&fit_data_);
  ui->plotSpectrum2->setFit(&fit_data_2_);

  my_gain_calibration_ = new FormGainCalibration(settings_, detectors_, fit_data_, fit_data_2_, this);
  my_gain_calibration_->hide();
  //my_gain_calibration_->blockSignals(true);
  connect(my_gain_calibration_, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));
  connect(my_gain_calibration_, SIGNAL(update_detector()), this, SLOT(apply_gain_calibration()));
  connect(my_gain_calibration_, SIGNAL(symmetrize_requested()), this, SLOT(symmetrize()));
  ui->tabs->addTab(my_gain_calibration_, "Gain calibration");

  connect(ui->plotSpectrum, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));
  connect(ui->plotSpectrum2, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));

  tempx = Qpx::SinkFactory::getInstance().create_prototype("1D");
//  tempx.visible = true;
  tempx.name = "tempx";
  Qpx::Setting pattern;
  pattern = tempx.attributes.branches.get(Qpx::Setting("pattern_coinc"));
  pattern.value_pattern.set_gates(std::vector<bool>({1,1}));
  pattern.value_pattern.set_theshold(2);
  tempx.attributes.branches.replace(pattern);
  pattern = tempx.attributes.branches.get(Qpx::Setting("pattern_add"));
  pattern.value_pattern.set_gates(std::vector<bool>({1,0}));
  pattern.value_pattern.set_theshold(1);
  tempx.attributes.branches.replace(pattern);

  tempy = Qpx::SinkFactory::getInstance().create_prototype("1D");
//  tempy.visible = true;
  tempy.name = "tempy";


  res = 0;

  style_fit.default_pen = QPen(Qt::blue, 0);
  style_pts.default_pen = QPen(Qt::darkBlue, 7);

  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
}

void FormSymmetrize2D::configure_UI() {

  ui->plotSpectrum->setVisible(true);
  ui->plotSpectrum->blockSignals(false);

  ui->plotSpectrum2->setVisible(true);
  ui->plotSpectrum2->blockSignals(false);

  make_gated_spectra();
}



void FormSymmetrize2D::setSpectrum(Qpx::Project *newset, QString name) {
  clear();
  spectra_ = newset;
  current_spectrum_ = name;
}

void FormSymmetrize2D::reset() {
  initialized = false;
//  while (ui->tabs->count())
//    ui->tabs->removeTab(0);
}

void FormSymmetrize2D::make_gated_spectra() {
  std::shared_ptr<Qpx::Sink> source_spectrum = spectra_->by_name(current_spectrum_.toStdString());
  if (source_spectrum == nullptr)
    return;

  this->setCursor(Qt::WaitCursor);

  Qpx::Metadata md = source_spectrum->metadata();

  //  PL_DBG << "Coincidence gate x[" << xmin_ << "-" << xmax_ << "]   y[" << ymin_ << "-" << ymax_ << "]";

  if ((md.total_count > 0) && (md.dimensions() == 2))
  {
    uint32_t adjrange = pow(2,md.bits) - 1;

    tempx.bits = md.bits;
    tempx.name = detector1_.name_ + "[" + to_str_precision(nrg_calibration2_.transform(0), 0) + "," + to_str_precision(nrg_calibration2_.transform(adjrange), 0) + "]";
    tempy.bits = md.bits;
    tempy.name = detector2_.name_ + "[" + to_str_precision(nrg_calibration1_.transform(0), 0) + "," + to_str_precision(nrg_calibration1_.transform(adjrange), 0) + "]";
    Qpx::Setting pattern;
    pattern = tempy.attributes.branches.get(Qpx::Setting("pattern_coinc"));
    pattern.value_pattern.set_gates(std::vector<bool>({1,1}));
    pattern.value_pattern.set_theshold(2);
    tempy.attributes.branches.replace(pattern);
    pattern = tempy.attributes.branches.get(Qpx::Setting("pattern_add"));
    pattern.value_pattern.set_gates(std::vector<bool>({1,0}));
    pattern.value_pattern.set_theshold(1);
    tempy.attributes.branches.replace(pattern);

    gate_x = Qpx::SinkFactory::getInstance().create_from_prototype(tempx);

    //if?
    Qpx::slice_rectangular(source_spectrum, gate_x, {{0, adjrange}, {0, adjrange}});

    fit_data_.clear();
    fit_data_.setData(gate_x);
    ui->plotSpectrum->update_spectrum();

    gate_y = Qpx::SinkFactory::getInstance().create_from_prototype(tempy);

    Qpx::slice_rectangular(source_spectrum, gate_y, {{0, adjrange}, {0, adjrange}});

    fit_data_2_.clear();
    fit_data_2_.setData(gate_y);
    ui->plotSpectrum2->update_spectrum();


//    ui->plotMatrix->refresh();
  }
  this->setCursor(Qt::ArrowCursor);
}

void FormSymmetrize2D::initialize() {
  if (initialized)
    return;

  if (spectra_) {

    PL_DBG << "<Analysis2D> initializing to " << current_spectrum_.toStdString();
    std::shared_ptr<Qpx::Sink>spectrum = spectra_->by_name(current_spectrum_.toStdString());

    if (spectrum && spectrum->bits()) {
      Qpx::Metadata md = spectrum->metadata();
      res = pow(2,md.bits);

      detector1_ = Qpx::Detector();
      detector2_ = Qpx::Detector();

      if (md.detectors.size() > 1) {
        detector1_ = md.detectors[0];
        detector2_ = md.detectors[1];
      } else if (md.detectors.size() == 1) {
        detector2_ = detector1_ = md.detectors[0];
        //HACK!!!
      }

      PL_DBG << "det1 " << detector1_.name_;
      PL_DBG << "det2 " << detector2_.name_;

      if (detector1_.energy_calibrations_.has_a(Qpx::Calibration("Energy", md.bits)))
        nrg_calibration1_ = detector1_.energy_calibrations_.get(Qpx::Calibration("Energy", md.bits));

      if (detector2_.energy_calibrations_.has_a(Qpx::Calibration("Energy", md.bits)))
        nrg_calibration2_ = detector2_.energy_calibrations_.get(Qpx::Calibration("Energy", md.bits));

      bool symmetrized = (md.attributes.branches.get(Qpx::Setting("symmetrized")).value_int != 0);
    }
  }

  initialized = true;
  configure_UI();
}

void FormSymmetrize2D::showEvent( QShowEvent* event ) {
  QWidget::showEvent(event);

  QTimer::singleShot(50, this, SLOT(initialize()));
}

void FormSymmetrize2D::update_peaks(bool content_changed) {
  //update sums
  if (content_changed) {
    ui->plotSpectrum2->update_data();
    ui->plotSpectrum->update_data();
  }

  if (my_gain_calibration_)
    my_gain_calibration_->newSpectrum();
}

void FormSymmetrize2D::update_gates(Marker xx, Marker yy) {
  make_gated_spectra();
}

void FormSymmetrize2D::on_pushAddGatedSpectra_clicked()
{
  this->setCursor(Qt::WaitCursor);
  bool success = false;

  if ((gate_x != nullptr) && (gate_x->metadata().total_count > 0)) {
    if (spectra_->by_name(gate_x->name()) != nullptr)
      PL_WARN << "Spectrum " << gate_x->name() << " already exists.";
    else
    {
      Qpx::Setting app = gate_x->metadata().attributes.branches.get(Qpx::Setting("appearance"));
      app.value_text = generateColor().name(QColor::HexArgb).toStdString();
      gate_x->set_option(app);

      spectra_->add_spectrum(gate_x);
      gate_x = nullptr;
      success = true;
    }
  }

  if ((gate_y != nullptr) && (gate_y->metadata().total_count > 0)) {
    if (spectra_->by_name(gate_y->name()) != nullptr)
      PL_WARN << "Spectrum " << gate_y->name() << " already exists.";
    else
    {
      Qpx::Setting app = gate_y->metadata().attributes.branches.get(Qpx::Setting("appearance"));
      app.value_text = generateColor().name(QColor::HexArgb).toStdString();
      gate_y->set_option(app);

      spectra_->add_spectrum(gate_y);
      gate_y = nullptr;
      success = true;
    }
  }

  if (success) {
    emit spectraChanged();
  }
  //make_gated_spectra();
  this->setCursor(Qt::ArrowCursor);
}

void FormSymmetrize2D::symmetrize()
{
  this->setCursor(Qt::WaitCursor);

  QString fold_spec_name = current_spectrum_ + "_sym";
  bool ok;
  QString text = QInputDialog::getText(this, "New spectrum name",
                                       "Spectrum name:", QLineEdit::Normal,
                                       fold_spec_name,
                                       &ok);
  if (ok && !text.isEmpty()) {
    if (spectra_->by_name(fold_spec_name.toStdString()) != nullptr) {
      PL_WARN << "Spectrum " << fold_spec_name.toStdString() << " already exists. Aborting symmetrization";
      this->setCursor(Qt::ArrowCursor);
      return;
    }

    std::shared_ptr<Qpx::Sink> source_spectrum = spectra_->by_name(current_spectrum_.toStdString());
    if (source_spectrum == nullptr) {
      this->setCursor(Qt::ArrowCursor);
      return;
    }

    //gain_match_cali_ = fit_data_2_.detector_.get_gain_match(fit_data_2_.metadata_.bits, fit_data_.detector_.name_);
    //compare with source and replace?

    Qpx::Metadata md = source_spectrum->metadata();

    std::vector<uint16_t> chans;
    Qpx::Setting pattern;
    pattern = md.attributes.branches.get(Qpx::Setting("pattern_add"));
    std::vector<bool> gts = pattern.value_pattern.gates();

    for (int i=0; i < gts.size(); ++i) {
      if (gts[i])
        chans.push_back(i);
    }

    if (chans.size() != 2) {
      PL_WARN << "<Analysis2D> " << md.name << " does not have 2 channels in pattern";
      this->setCursor(Qt::ArrowCursor);
      return;
    }

    Qpx::Metadata temp_sym = Qpx::SinkFactory::getInstance().create_prototype(md.type()); //assume 2D?
//    temp_sym.visible = false;
    temp_sym.name = fold_spec_name.toStdString();
    temp_sym.attributes.branches.replace(pattern);
    pattern = md.attributes.branches.get(Qpx::Setting("pattern_coinc"));
    pattern.value_pattern.set_gates(gts);
    pattern.value_pattern.set_theshold(2);
    temp_sym.attributes.branches.replace(pattern);
    temp_sym.bits = md.bits;

    std::shared_ptr<Qpx::Sink> destination = Qpx::SinkFactory::getInstance().create_from_prototype(temp_sym);

    if (destination == nullptr) {
      PL_WARN << "<Analysis2D> " << fold_spec_name.toStdString() << " could not be created";
      this->setCursor(Qt::ArrowCursor);
      return;
    }

    if (Qpx::symmetrize(source_spectrum, destination)) {
      spectra_->add_spectrum(destination);
      setSpectrum(spectra_, fold_spec_name);
      reset();
      initialize();
      emit spectraChanged();
    } else {
      PL_WARN << "<Analysis2D> could not symmetrize " << md.name;
      this->setCursor(Qt::ArrowCursor);
      return;
    }

  }
  this->setCursor(Qt::ArrowCursor);
}

void FormSymmetrize2D::apply_gain_calibration()
{
  gain_match_cali_ = fit_data_2_.detector_.get_gain_match(fit_data_2_.metadata_.bits, detector1_.name_);

  std::string msg_text("Propagating gain match calibration ");
  msg_text += detector2_.name_ + "->" + gain_match_cali_.to_ + " (" + std::to_string(gain_match_cali_.bits_) + " bits) to all spectra in current project: "
      + spectra_->identity();

  std::string question_text("Do you also want to save this calibration to ");
  question_text += detector2_.name_ + " in detector database?";

  QMessageBox msgBox;
  msgBox.setText(QString::fromStdString(msg_text));
  msgBox.setInformativeText(QString::fromStdString(question_text));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Abort);
  msgBox.setDefaultButton(QMessageBox::No);
  msgBox.setIcon(QMessageBox::Question);
  int ret = msgBox.exec();

  Qpx::Detector modified;

  if (ret == QMessageBox::Yes) {
    if (!detectors_.has_a(detector2_)) {
      bool ok;
      QString text = QInputDialog::getText(this, "New Detector",
                                           "Detector name:", QLineEdit::Normal,
                                           QString::fromStdString(detector2_.name_),
                                           &ok);
      if (!ok)
        return;

      if (!text.isEmpty()) {
        modified = detector2_;
        modified.name_ = text.toStdString();
        if (detectors_.has_a(modified)) {
          QMessageBox::warning(this, "Already exists", "Detector " + text + " already exists. Will not save to database.", QMessageBox::Ok);
          modified = Qpx::Detector();
        }
      }
    } else
      modified = detectors_.get(detector2_);

    if (modified != Qpx::Detector())
    {
      PL_INFO << "   applying new gain_match calibrations for " << modified.name_ << " in detector database";
      modified.gain_match_calibrations_.replace(gain_match_cali_);
      detectors_.replace(modified);
      emit detectorsChanged();
    }
  }

  if (ret != QMessageBox::Abort) {
    for (auto &q : spectra_->spectra()) {
      if (q == nullptr)
        continue;
      Qpx::Metadata md = q->metadata();
      for (auto &p : md.detectors) {
        if (p.shallow_equals(detector2_)) {
          PL_INFO << "   applying new calibrations for " << detector2_.name_ << " on " << q->name();
          p.gain_match_calibrations_.replace(gain_match_cali_);
        }
      }
      q->set_detectors(md.detectors);
    }

    std::set<Qpx::Spill> spills = spectra_->spills();
    Qpx::Spill sp;
    if (spills.size())
      sp = *spills.end();

    for (auto &p : sp.detectors) {
      if (p.shallow_equals(detector2_)) {
        PL_INFO << "   applying new calibrations for " << detector2_.name_ << " in current project " << spectra_->identity();
        p.gain_match_calibrations_.replace(gain_match_cali_);
      }
    }
    spectra_->add_spill(&sp);
  }
}

void FormSymmetrize2D::closeEvent(QCloseEvent *event) {
  ui->plotSpectrum->saveSettings(settings_);
  ui->plotSpectrum2->saveSettings(settings_);

  saveSettings();
  event->accept();
}

void FormSymmetrize2D::loadSettings() {
  settings_.beginGroup("Program");
  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();

  ui->plotSpectrum->loadSettings(settings_);
  ui->plotSpectrum2->loadSettings(settings_);
}

void FormSymmetrize2D::saveSettings() {
  ui->plotSpectrum->saveSettings(settings_);
}

void FormSymmetrize2D::clear() {
  res = 0;

  current_spectrum_.clear();
  detector1_ = Qpx::Detector();
  nrg_calibration1_ = Qpx::Calibration();

  detector2_ = Qpx::Detector();
  nrg_calibration2_ = Qpx::Calibration();

  if (my_gain_calibration_->isVisible())
    my_gain_calibration_->clear();
}

FormSymmetrize2D::~FormSymmetrize2D()
{
  delete ui;
}
