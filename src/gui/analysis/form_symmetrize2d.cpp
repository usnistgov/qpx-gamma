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
#include "consumer_factory.h"
#include "fitter.h"
#include "qt_util.h"
#include "manip2d.h"
#include <QInputDialog>
#include "qpx_util.h"
#include <QSettings>

using namespace Qpx;

FormSymmetrize2D::FormSymmetrize2D(XMLableDB<Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormSymmetrize2D),
  detectors_(newDetDB),
  my_gain_calibration_(nullptr),
  gate_x(nullptr),
  gate_y(nullptr),
  current_spectrum_(0)
{
  ui->setupUi(this);

  initialized = false;

  loadSettings();

  ui->plotSpectrum->setFit(&fit_data_);
  ui->plotSpectrum2->setFit(&fit_data_2_);

  my_gain_calibration_ = new FormGainCalibration(detectors_, fit_data_, fit_data_2_, this);
  my_gain_calibration_->hide();
  //my_gain_calibration_->blockSignals(true);
//  connect(my_gain_calibration_, SIGNAL(peaks_changed()), this, SLOT(update_peaks()));
//  connect(my_gain_calibration_, SIGNAL(peak_selection_changed(std::set<double>))
//          this, SLOT(peak_selection_changed(std::set<double>)));
  connect(my_gain_calibration_, SIGNAL(update_detector()), this, SLOT(apply_gain_calibration()));
  connect(my_gain_calibration_, SIGNAL(symmetrize_requested()), this, SLOT(symmetrize()));
  ui->tabs->addTab(my_gain_calibration_, "Gain calibration");

  connect(ui->plotSpectrum, SIGNAL(peaks_changed()), this, SLOT(update_peaks()));
  connect(ui->plotSpectrum, SIGNAL(peak_selection_changed(std::set<double>)),
          this, SLOT(peak_selection_changed(std::set<double>)));
  connect(ui->plotSpectrum2, SIGNAL(peaks_changed()), this, SLOT(update_peaks()));
  connect(ui->plotSpectrum2, SIGNAL(peak_selection_changed(std::set<double>)),
          this, SLOT(peak_selection_changed(std::set<double>)));

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

void FormSymmetrize2D::update_spectrum()
{
  configure_UI();
}

void FormSymmetrize2D::setSpectrum(Project *newset, int64_t idx) {
  clear();
  spectra_ = newset;
  current_spectrum_ = idx;
}

void FormSymmetrize2D::reset() {
  initialized = false;
//  while (ui->tabs->count())
//    ui->tabs->removeTab(0);
}

void FormSymmetrize2D::make_gated_spectra() {
  SinkPtr source_spectrum = spectra_->get_sink(current_spectrum_);
  if (!source_spectrum)
    return;

  this->setCursor(Qt::WaitCursor);

  ConsumerMetadata md = source_spectrum->metadata();

  //  DBG << "Coincidence gate x[" << xmin_ << "-" << xmax_ << "]   y[" << ymin_ << "-" << ymax_ << "]";

  if (/*(md.total_count > 0) &&*/ (md.dimensions() == 2))
  {
    DBG << "All good";
    uint16_t bits = md.get_attribute("resolution").value_int;
    uint32_t adjrange = pow(2,bits) - 1;

    gate_x = slice_rectangular(source_spectrum, {{0, adjrange}, {0, adjrange}}, true);

    DBG << "gx " << gate_x->dimensions();

    fit_data_.clear();
    fit_data_.setData(gate_x);
    ui->plotSpectrum->update_spectrum();

    gate_y = slice_rectangular(source_spectrum, {{0, adjrange}, {0, adjrange}}, false);

    DBG << "gy " << gate_y->dimensions();

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

  if (spectra_)
  {
    SinkPtr spectrum = spectra_->get_sink(current_spectrum_);

    if (spectrum) {
      ConsumerMetadata md = spectrum->metadata();
      uint16_t bits = md.get_attribute("resolution").value_int;
      res = pow(2,bits);

      detector1_ = Detector();
      detector2_ = Detector();

      if (md.detectors.size() > 1) {
        detector1_ = md.detectors[0];
        detector2_ = md.detectors[1];
      } else if (md.detectors.size() == 1) {
        detector2_ = detector1_ = md.detectors[0];
        //HACK!!!
      }

      DBG << "det1 " << detector1_.name();
      DBG << "det2 " << detector2_.name();

      nrg_calibration1_ = detector1_.best_calib(bits);
      nrg_calibration2_ = detector2_.best_calib(bits);

      bool symmetrized = (md.get_attribute("symmetrized").value_int != 0);
    }
  }

  initialized = true;
  configure_UI();
}

void FormSymmetrize2D::showEvent( QShowEvent* event ) {
  QWidget::showEvent(event);

  QTimer::singleShot(50, this, SLOT(initialize()));
}

void FormSymmetrize2D::update_peaks() {
  //update sums
//  if (content_changed) {
//    ui->plotSpectrum2->update_data();
//    ui->plotSpectrum->update_data();
//  }

  if (my_gain_calibration_)
    my_gain_calibration_->newSpectrum();
}

void FormSymmetrize2D::peak_selection_changed(std::set<double> selected_peaks)
{
}

//void FormSymmetrize2D::update_gates(Marker xx, Marker yy) {
//  make_gated_spectra();
//}

void FormSymmetrize2D::on_pushAddGatedSpectra_clicked()
{
  this->setCursor(Qt::WaitCursor);
  bool success = false;

  if (gate_x /*&& (gate_x->metadata().total_count > 0)*/) {
    Setting app = gate_x->metadata().get_attribute("appearance");
    app.value_text = generateColor().name(QColor::HexArgb).toStdString();
    gate_x->set_attribute(app);

    spectra_->add_sink(gate_x);
    success = true;
  }

  if (gate_y /*&& (gate_y->metadata().total_count > 0)*/) {
    Setting app = gate_y->metadata().get_attribute("appearance");
    app.value_text = generateColor().name(QColor::HexArgb).toStdString();
    gate_y->set_attribute(app);

    spectra_->add_sink(gate_y);
    success = true;
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

  SinkPtr source_spectrum = spectra_->get_sink(current_spectrum_);
  if (!source_spectrum) {
    this->setCursor(Qt::ArrowCursor);
    return;
  }

  //gain_match_cali_ = fit_data_2_.detector_.get_gain_match(fit_data_2_.metadata_.bits, fit_data_.detector_.name_);
  //compare with source and replace?

  SinkPtr destination = make_symmetrized(source_spectrum);

  if (destination) {
    Setting name = destination->metadata().get_attribute("name");
    name.value_text += "-sym";
    destination->set_attribute(name);
    int64_t idx = spectra_->add_sink(destination);
    if (idx) {
      setSpectrum(spectra_, idx);
      reset();
      initialize();
      emit spectraChanged();
    } else
      WARN << "<FormSymmetrize2D> could not add symmetrized spectrum to project " << name.value_text;
  } else {
    WARN << "<FormSymmetrize2D> could not symmetrize";
    this->setCursor(Qt::ArrowCursor);
    return;
  }

  this->setCursor(Qt::ArrowCursor);
}

void FormSymmetrize2D::apply_gain_calibration()
{
  gain_match_cali_ = fit_data_2_.detector_.get_gain_match(fit_data_2_.settings().bits_, detector1_.name());

  std::string msg_text("Propagating gain match calibration ");
  msg_text += detector2_.name() + "->" +
      gain_match_cali_.to() + " (" + std::to_string(gain_match_cali_.bits()) +
      " bits) to all spectra in current project: "
      + spectra_->identity();

  std::string question_text("Do you also want to save this calibration to ");
  question_text += detector2_.name() + " in detector database?";

  QMessageBox msgBox;
  msgBox.setText(QString::fromStdString(msg_text));
  msgBox.setInformativeText(QString::fromStdString(question_text));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Abort);
  msgBox.setDefaultButton(QMessageBox::No);
  msgBox.setIcon(QMessageBox::Question);
  int ret = msgBox.exec();

  Detector modified;

  if (ret == QMessageBox::Yes) {
    if (!detectors_.has_a(detector2_)) {
      bool ok;
      QString text = QInputDialog::getText(this, "New Detector",
                                           "Detector name:", QLineEdit::Normal,
                                           QString::fromStdString(detector2_.name()),
                                           &ok);
      if (!ok)
        return;

      if (!text.isEmpty()) {
        modified = detector2_;
        modified.set_name(text.toStdString());
        if (detectors_.has_a(modified)) {
          QMessageBox::warning(this, "Already exists", "Detector " + text + " already exists. Will not save to database.", QMessageBox::Ok);
          modified = Detector();
        }
      }
    } else
      modified = detectors_.get(detector2_);

    if (modified != Detector())
    {
      LINFO << "   applying new gain_match calibrations for " << modified.name() << " in detector database";
      modified.set_gain_calibration(gain_match_cali_);
      detectors_.replace(modified);
      emit detectorsChanged();
    }
  }

  if (ret != QMessageBox::Abort) {
    for (auto &q : spectra_->get_sinks()) {
      if (!q.second)
        continue;
      ConsumerMetadata md = q.second->metadata();
      for (auto &p : md.detectors) {
        if (p.shallow_equals(detector2_)) {
          LINFO << "   applying new calibrations for " << detector2_.name()
                << " on " << q.second->metadata().get_attribute("name").value_text;

          p.set_gain_calibration(gain_match_cali_);
        }
      }
      q.second->set_detectors(md.detectors);
    }

//    std::set<Spill> spills = spectra_->spills();
//    Spill sp;
//    if (spills.size())
//      sp = *spills.rbegin();

//    for (auto &p : sp.detectors) {
//      if (p.shallow_equals(detector2_)) {
//        LINFO << "   applying new calibrations for " << detector2_.name_ << " in current project " << spectra_->identity();
//        p.gain_match_calibrations_.replace(gain_match_cali_);
//      }
//    }
//    spectra_->add_spill(&sp);
  }
}

void FormSymmetrize2D::closeEvent(QCloseEvent *event) {
  QSettings settings_;


  ui->plotSpectrum->saveSettings(settings_);
  ui->plotSpectrum2->saveSettings(settings_);

  saveSettings();
  event->accept();
}

void FormSymmetrize2D::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();

  ui->plotSpectrum->loadSettings(settings_);
  ui->plotSpectrum2->loadSettings(settings_);
}

void FormSymmetrize2D::saveSettings() {
  QSettings settings_;

  ui->plotSpectrum->saveSettings(settings_);
}

void FormSymmetrize2D::clear() {
  res = 0;

  current_spectrum_ = 0;
  detector1_ = Detector();
  nrg_calibration1_ = Calibration();

  detector2_ = Detector();
  nrg_calibration2_ = Calibration();

  if (my_gain_calibration_->isVisible())
    my_gain_calibration_->clear();
}

FormSymmetrize2D::~FormSymmetrize2D()
{
  delete ui;
}
