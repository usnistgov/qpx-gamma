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
 *      Gain matching interface
 *
 ******************************************************************************/

#include "form_gain_match.h"
#include "ui_form_gain_match.h"
#include "spectrum1D.h"
#include "custom_logger.h"
#include "fityk.h"


FormGainMatch::FormGainMatch(ThreadRunner& thread, QSettings& settings, XMLableDB<Gamma::Detector>& detectors, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormGainMatch),
  gm_runner_thread_(thread),
  settings_(settings),
  detectors_(detectors),
  gm_spectra_(),
  gm_interruptor_(false),
  gm_plot_thread_(gm_spectra_),
  my_run_(false)
{
  ui->setupUi(this);

  loadSettings();
  connect(&gm_runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));

  if (Qpx::Spectrum::Template *temp = Qpx::Spectrum::Factory::getInstance().create_template("1D")) {
    reference_   = *temp;
    optimizing_  = *temp;
    delete temp;
  }

  current_pass = 0;

  reference_.name_ = "Reference";
  reference_.visible = true;
  reference_.appearance = QColor(Qt::darkCyan).rgba();

  optimizing_.name_ = "Optimizing";
  optimizing_.visible = true;
  optimizing_.appearance = QColor(Qt::red).rgba();

  moving_.appearance.themes["light"] = QPen(Qt::darkBlue, 2);
  moving_.appearance.themes["dark"] = QPen(Qt::blue, 2);

  a.appearance.themes["light"] = QPen(Qt::darkBlue, 1);
  a.appearance.themes["dark"] = QPen(Qt::blue, 1);

  QColor translu(Qt::blue);
  translu.setAlpha(32);
  b.appearance.themes["light"] = QPen(translu, 1);
  translu.setAlpha(64);
  b.appearance.themes["dark"] = QPen(translu, 1);

  ap_reference_.themes["light"] = QPen(Qt::darkCyan, 0);
  ap_reference_.themes["dark"] = QPen(Qt::cyan, 0);

  ap_optimized_.themes["light"] = QPen(Qt::darkMagenta, 0);
  ap_optimized_.themes["dark"] = QPen(Qt::magenta, 0);

  marker_ref_.appearance = ap_reference_;
  marker_ref_.visible = true;

  marker_opt_.appearance = ap_optimized_;
  marker_opt_.visible = true;


  connect(&gm_plot_thread_, SIGNAL(plot_ready()), this, SLOT(update_plots()));

  connect(ui->plot, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->plot, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));
  connect(this, SIGNAL(restart_run()), this, SLOT(do_run()));
  connect(this, SIGNAL(post_proc()), this, SLOT(do_post_processing()));

  gm_plot_thread_.start();
}

FormGainMatch::~FormGainMatch()
{
  delete ui;
}

void FormGainMatch::loadSettings() {
  settings_.beginGroup("GainMatching");
  ui->spinBits->setValue(settings_.value("bits", 14).toInt());
  ui->spinRefChan->setValue(settings_.value("reference_channel", 0).toInt());
  ui->spinOptChan->setValue(settings_.value("optimized_channel", 1).toInt());
  ui->spinSampleMin->setValue(settings_.value("minimum_sample", 180).toInt());
  ui->spinSampleMax->setValue(settings_.value("maximum_sample", 1050).toInt());
  ui->spinMaxPasses->setValue(settings_.value("maximum_passes", 30).toInt());
  ui->spinPeakWindow->setValue(settings_.value("peak_window", 180).toInt());
  ui->doubleSpinDeltaV->setValue(settings_.value("delta_V", 0.000300).toDouble());
  settings_.endGroup();
}

void FormGainMatch::saveSettings() {
  settings_.beginGroup("GainMatching");
  settings_.setValue("bits", ui->spinBits->value());
  settings_.setValue("reference_channel", ui->spinRefChan->value());
  settings_.setValue("optimized_channel", ui->spinOptChan->value());
  settings_.setValue("minimum_sample", ui->spinSampleMin->value());
  settings_.setValue("maximum_sample", ui->spinSampleMax->value());
  settings_.setValue("maximum_passes", ui->spinMaxPasses->value());
  settings_.setValue("peak_window", ui->spinPeakWindow->value());
  settings_.setValue("delta_V", ui->doubleSpinDeltaV->value());
  settings_.endGroup();
}

void FormGainMatch::closeEvent(QCloseEvent *event) {
  if (my_run_ && gm_runner_thread_.running()) {
    int reply = QMessageBox::warning(this, "Ongoing data acquisition",
                                     "Terminate?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      gm_runner_thread_.terminate();
      gm_runner_thread_.wait();
    } else {
      event->ignore();
      return;
    }
  } else {
    gm_runner_thread_.terminate();
    gm_runner_thread_.wait();
  }


  if (!gm_spectra_.empty()) {
    int reply = QMessageBox::warning(this, "Gain matching data still open",
                                     "Discard?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      gm_spectra_.clear();
    } else {
      event->ignore();
      return;
    }
  }

  gm_spectra_.terminate();
  gm_plot_thread_.wait();
  saveSettings();

  event->accept();
}

void FormGainMatch::toggle_push(bool enable, Qpx::DeviceStatus status) {
  bool online = (status & Qpx::DeviceStatus::can_run);
  ui->pushMatchGain->setEnabled(enable && online);

  ui->spinRefChan->setEnabled(enable && online);
  ui->spinOptChan->setEnabled(enable && online);
  ui->spinBits->setEnabled(enable && online);
  ui->spinSampleMax->setEnabled(enable && online);
  ui->spinMaxPasses->setEnabled(enable && online);
}

void FormGainMatch::do_run()
{
  ui->pushStop->setEnabled(true);
  emit toggleIO(false);
  my_run_ = true;

  bits = ui->spinBits->value();

  gm_spectra_.clear();
  reference_.bits = bits;
  reference_.add_pattern.resize(ui->spinRefChan->value() + 1, 0);
  reference_.add_pattern[ui->spinRefChan->value()] = 1;
  reference_.match_pattern.resize(ui->spinRefChan->value() + 1, 0);
  reference_.match_pattern[ui->spinRefChan->value()] = 1;

  optimizing_.bits = bits;
  optimizing_.add_pattern.resize(ui->spinOptChan->value() + 1, 0);
  optimizing_.add_pattern[ui->spinOptChan->value()] = 1;
  optimizing_.match_pattern.resize(ui->spinOptChan->value() + 1, 0);
  optimizing_.match_pattern[ui->spinOptChan->value()] = 1;

  XMLableDB<Qpx::Spectrum::Template> db("SpectrumTemplates");
  db.add(reference_);
  db.add(optimizing_);

  gm_plot_thread_.start();

  gm_spectra_.set_spectra(db);
  ui->plot->clearGraphs();

  x.resize(pow(2,bits), 0.0);
  y_ref.resize(pow(2,bits), 0.0);
  y_opt.resize(pow(2,bits), 0.0);

  minTimer = new CustomTimer(true);
  gm_runner_thread_.do_run(gm_spectra_, gm_interruptor_, ui->spinSampleMax->value());
}

void FormGainMatch::run_completed() {
  if (my_run_) {
    gm_spectra_.clear();
    gm_spectra_.terminate();
    gm_plot_thread_.wait();
    marker_ref_.visible = false;
    marker_opt_.visible = false;
    gauss_ref_.gaussian_.height_ = 0;
    gauss_opt_.gaussian_.height_ = 0;
    //replot_markers();

    y_ref.clear();
    y_opt.clear();

    if ((current_pass < max_passes)  && !gm_runner_thread_.terminating()){
      PL_INFO << "[Gain matching] Passes remaining " << max_passes - current_pass;
      current_pass++;
      emit post_proc();
    } else {
      ui->pushStop->setEnabled(false);
      emit toggleIO(true);
      my_run_ = false;
    }
  }
}

void FormGainMatch::do_post_processing() {
  Gamma::Setting gain_setting("VGAIN", ui->spinOptChan->value());

  Qpx::Engine::getInstance().get_all_settings();
  gain_setting = Qpx::Engine::getInstance().pull_settings().get_setting(gain_setting, Gamma::Match::name | Gamma::Match::indices);
  double old_gain = gain_setting.value_dbl;
  QThread::sleep(2);
  double new_gain = old_gain;
  if (gauss_opt_.gaussian_.center_ < gauss_ref_.gaussian_.center_) {
    PL_INFO << "[Gain matching] increasing gain";
    new_gain += ui->doubleSpinDeltaV->value();
  } else if (gauss_opt_.gaussian_.center_ > gauss_ref_.gaussian_.center_) {
    PL_INFO << "[Gain matching] decreasing gain";
    new_gain -= ui->doubleSpinDeltaV->value();
  } else {
    PL_INFO << "[Gain matching] gain matching complete ";
    ui->pushStop->setEnabled(false);
    emit toggleIO(true);
    return;
  }
  gain_setting.value_dbl = new_gain;
  Qpx::Engine::getInstance().set_setting(gain_setting, Gamma::Match::id | Gamma::Match::indices);
  QThread::sleep(2);
  gain_setting = Qpx::Engine::getInstance().pull_settings().get_setting(gain_setting, Gamma::Match::id | Gamma::Match::indices);
  new_gain = gain_setting.value_dbl;


  PL_INFO << "gain changed from " << std::fixed << std::setprecision(6)
          << old_gain << " to " << new_gain;

  QThread::sleep(2);
  emit restart_run();
}

bool FormGainMatch::find_peaks() {
  if (moving_.visible) {

    fitter_ref_.find_peaks(5);
    fitter_opt_.find_peaks(5);

    if (fitter_ref_.peaks_.size())
      gauss_ref_ = fitter_ref_.peaks_.begin()->second;

    if (fitter_opt_.peaks_.size())
      gauss_opt_ = fitter_opt_.peaks_.begin()->second;

    if (gauss_ref_.gaussian_.height_ && gauss_opt_.gaussian_.height_)
      return true;
  }
  return false;
}

void FormGainMatch::update_plots() {

  std::map<double, double> minima, maxima;

  bool have_data = false;
  bool have_peaks = false;

  if (ui->plot->isVisible()) {
    this->setCursor(Qt::WaitCursor);

    ui->plot->clearGraphs();
    have_data = true;

    for (auto &q: gm_spectra_.by_type("1D")) {
      Qpx::Spectrum::Metadata md;
      if (q)
        md = q->metadata();

      if (md.total_count > 0) {

        std::vector<double> x, y;

        if (md.name == "Reference") {
          fitter_ref_.setData(q);
          y = y_ref = fitter_ref_.y_;
          x = fitter_ref_.x_;
        } else {
          fitter_opt_.setData(q);
          y = y_opt = fitter_opt_.y_;
          x = fitter_opt_.x_;
        }

        AppearanceProfile profile;
        profile.default_pen = QPen(QColor::fromRgba(md.appearance), 1);
        ui->plot->addGraph(QVector<double>::fromStdVector(x),
                           QVector<double>::fromStdVector(y),
                           profile);
      } else
        have_data = false;
    }
    ui->plot->setLabels("channel", "count");

    if (have_data)
      have_peaks = find_peaks();

    if (gauss_ref_.gaussian_.height_) {
      ui->plot->addGraph(QVector<double>::fromStdVector(gauss_ref_.x_), QVector<double>::fromStdVector(gauss_ref_.y_fullfit_pseudovoigt_), ap_reference_);
      ui->plot->addGraph(QVector<double>::fromStdVector(gauss_ref_.x_), QVector<double>::fromStdVector(gauss_ref_.y_baseline_), ap_reference_);
    }

    if (gauss_opt_.gaussian_.height_) {
      ui->plot->addGraph(QVector<double>::fromStdVector(gauss_opt_.x_), QVector<double>::fromStdVector(gauss_opt_.y_fullfit_pseudovoigt_), ap_optimized_);
      ui->plot->addGraph(QVector<double>::fromStdVector(gauss_opt_.x_), QVector<double>::fromStdVector(gauss_opt_.y_baseline_), ap_optimized_);
    }

    std::string new_label = boost::algorithm::trim_copy(gm_spectra_.status());
    ui->plot->setTitle(QString::fromStdString(new_label));
  }

  ui->plot->setYBounds(minima, maxima);
  ui->plot->replot_markers();
  ui->plot->redraw();

  if (have_data && (minTimer != nullptr) && (minTimer->s() > ui->spinSampleMin->value()) && have_peaks) {
    delete minTimer;
    minTimer = nullptr;
    gm_interruptor_.store(true);
  }

  this->setCursor(Qt::ArrowCursor);
}

void FormGainMatch::addMovingMarker(double x) {
  moving_.pos.set_bin(x, fitter_ref_.metadata_.bits, fitter_ref_.nrg_cali_);
  moving_.visible = true;
  a.pos.set_bin(x - ui->spinPeakWindow->value() / 2, fitter_ref_.metadata_.bits, fitter_ref_.nrg_cali_);
  a.visible = true;
  b.pos.set_bin(x + ui->spinPeakWindow->value() / 2, fitter_ref_.metadata_.bits, fitter_ref_.nrg_cali_);
  b.visible = true;
  replot_markers();
}

void FormGainMatch::removeMovingMarker(double x) {
  moving_.visible = false;
  a.visible = false;
  b.visible = false;
  replot_markers();
}


void FormGainMatch::replot_markers() {
  std::list<Marker> markers;

  if (gauss_ref_.gaussian_.height_) {
    marker_ref_.pos.set_bin(gauss_ref_.gaussian_.center_, fitter_ref_.metadata_.bits, fitter_ref_.nrg_cali_);
    marker_ref_.visible = true;
    markers.push_back(marker_ref_);
  }

  if (gauss_opt_.gaussian_.height_) {
    marker_opt_.pos.set_bin(gauss_opt_.gaussian_.center_, fitter_opt_.metadata_.bits, fitter_opt_.nrg_cali_);
    marker_opt_.visible = true;
    markers.push_back(marker_opt_);
  }

  ui->plot->set_markers(markers);
  ui->plot->set_block(a,b);
  ui->plot->replot_markers();
  ui->plot->redraw();
}


void FormGainMatch::on_pushMatchGain_clicked()
{
  current_pass = 0;
  max_passes = ui->spinMaxPasses->value();
  do_run();
}

void FormGainMatch::on_pushStop_clicked()
{
  PL_INFO << "Acquisition interrupted by user";
  max_passes = 0;
  gm_interruptor_.store(true);
}

void FormGainMatch::on_pushSaveOpti_clicked()
{
  Qpx::Engine::getInstance().save_optimization();
  std::vector<Gamma::Detector> dets = Qpx::Engine::getInstance().get_detectors();
  for (auto &q : dets) {
    if (detectors_.has_a(q)) {
      Gamma::Detector modified = detectors_.get(q);
      modified.settings_ = q.settings_;
      detectors_.replace(modified);
    }
  }
  emit optimization_approved();
}
