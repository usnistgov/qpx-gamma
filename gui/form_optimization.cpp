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
 *      Optimization interface
 *
 ******************************************************************************/

#include "gui/form_optimization.h"
#include "ui_form_optimization.h"
#include "spectrum1D.h"
#include "custom_logger.h"
#include "fityk.h"


FormOptimization::FormOptimization(ThreadRunner& thread, QSettings& settings, XMLableDB<Pixie::Detector>& detectors, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormOptimization),
  runner_thread_(thread),
  settings_(settings),
  detectors_(detectors),
  spectra_(),
  interruptor_(false),
  plot_thread_(spectra_),
  pixie_(Pixie::Wrapper::getInstance())
{
  ui->setupUi(this);

  //loadSettings();
  connect(&runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));

  if (Pixie::Spectrum::Template *temp = Pixie::Spectrum::Factory::getInstance().create_template("1D")) {
    reference_   = *temp;
    optimizing_  = *temp;
    delete temp;
  }

  current_pass = 0;
  max_x = 0;
  running = false;

  reference_.name_ = "Reference";
  reference_.visible = true;
  reference_.appearance = QColor(Qt::darkCyan).rgba();

  optimizing_.name_ = "Optimizing";
  optimizing_.visible = true;
  optimizing_.appearance = QColor(Qt::red).rgba();

  moving_.themes["light"] = QPen(Qt::darkBlue, 2);
  moving_.themes["dark"] = QPen(Qt::blue, 2);

  a.themes["light"] = QPen(Qt::darkBlue, 1);
  a.themes["dark"] = QPen(Qt::blue, 1);

  QColor translu(Qt::blue);
  translu.setAlpha(32);
  b.themes["light"] = QPen(translu, 1);
  translu.setAlpha(64);
  b.themes["dark"] = QPen(translu, 1);

  marker_ref_.themes["light"] = QPen(Qt::darkGreen, 1);
  marker_ref_.themes["dark"] = QPen(Qt::green, 1);
  marker_ref_.visible = true;

  marker_opt_.themes["light"] = QPen(Qt::darkMagenta, 1);
  marker_opt_.themes["dark"] = QPen(Qt::magenta, 1);
  marker_opt_.visible = true;

  connect(&plot_thread_, SIGNAL(plot_ready()), this, SLOT(update_plots()));

  connect(ui->plot, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->plot, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));
  connect(this, SIGNAL(restart_run()), this, SLOT(do_run()));
  connect(this, SIGNAL(post_proc()), this, SLOT(do_post_processing()));

  plot_thread_.start();
}

FormOptimization::~FormOptimization()
{
  delete ui;
}


void FormOptimization::closeEvent(QCloseEvent *event) {
  if (runner_thread_.isRunning()) {
    int reply = QMessageBox::warning(this, "Ongoing data acquisition",
                                     "Terminate?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      runner_thread_.terminate();
      runner_thread_.wait();
    } else {
      event->ignore();
      return;
    }
  }

  if (!spectra_.empty()) {
    int reply = QMessageBox::warning(this, "Optimization data still open",
                                     "Discard?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      spectra_.clear();
    } else {
      event->ignore();
      return;
    }
  }

  spectra_.terminate();
  plot_thread_.wait();
//  saveSettings();

  event->accept();
}

void FormOptimization::toggle_push(bool enable, Pixie::LiveStatus live) {
  bool online = (live == Pixie::LiveStatus::online);
  ui->pushMatchGain->setEnabled(enable && online);
  ui->pushTakeOne->setEnabled(enable && online);
}

void FormOptimization::on_pushTakeOne_clicked()
{
  do_run();
  current_pass = 0;
}

void FormOptimization::do_run()
{
  ui->pushStop->setEnabled(true);
  emit toggleIO(false);
  running = true;

  bits = ui->spinBits->value();

  spectra_.clear();
  reference_.bits = bits;
  reference_.add_pattern.resize(4,0);
  reference_.add_pattern[ui->spinRefChan->value()] = 1;
  reference_.match_pattern.resize(4,0);
  reference_.match_pattern[ui->spinRefChan->value()] = 1;

  optimizing_.bits = bits;
  optimizing_.add_pattern.resize(4,0);
  optimizing_.add_pattern[ui->spinOptChan->value()] = 1;
  optimizing_.match_pattern.resize(4,0);
  optimizing_.match_pattern[ui->spinOptChan->value()] = 1;

  XMLableDB<Pixie::Spectrum::Template> db("SpectrumTemplates");
  db.add(reference_);
  db.add(optimizing_);

  plot_thread_.start();

  spectra_.set_spectra(db);
  ui->plot->clearGraphs();

  x.resize(pow(2,bits), 0.0);
  y_ref.resize(pow(2,bits), 0.0);
  y_opt.resize(pow(2,bits), 0.0);

  minTimer = new CustomTimer(true);
  runner_thread_.do_run(spectra_, interruptor_, Pixie::RunType::compressed, ui->spinSampleMax->value(), true);
}

void FormOptimization::run_completed() {
  PL_INFO << "run completed called";

  spectra_.clear();
  spectra_.terminate();
  plot_thread_.wait();
  marker_ref_.visible = false;
  marker_opt_.visible = false;
  gauss_ref_.refined_.height_ = 0;
  gauss_opt_.refined_.height_ = 0;
  //replot_markers();

  y_ref.clear();
  y_opt.clear();

  if (current_pass > 0) {
    current_pass--;
    PL_INFO << "Passes remaining " << current_pass;

    emit post_proc();
  } else {
    ui->pushStop->setEnabled(false);
    emit toggleIO(true);
  }
}

void FormOptimization::do_post_processing() {
  PL_INFO << "do post proc called";

  Pixie::Settings &settings = pixie_.settings();

  double old_gain = settings.get_chan("VGAIN",
                                      Pixie::Channel(ui->spinOptChan->value()),
                                      Pixie::Module::current,
                                      Pixie::LiveStatus::online);
  QThread::sleep(2);
  double new_gain = old_gain;
  if (gauss_opt_.refined_.center_ < gauss_ref_.refined_.center_) {
    PL_INFO << "increasing gain";
    new_gain += ui->doubleSpinDeltaV->value();
  } else if (gauss_opt_.refined_.center_ > gauss_ref_.refined_.center_) {
    PL_INFO << "decreasing gain";
    new_gain -= ui->doubleSpinDeltaV->value();
  } else {
    PL_INFO << "Gain matching complete ";
    ui->pushStop->setEnabled(false);
    emit toggleIO(true);
    return;
  }
  settings.set_chan("VGAIN",new_gain,Pixie::Channel(ui->spinOptChan->value()), Pixie::Module::current);
  QThread::sleep(2);
  new_gain = settings.get_chan("VGAIN",
                               Pixie::Channel(ui->spinOptChan->value()),
                               Pixie::Module::current,
                               Pixie::LiveStatus::online);


  PL_INFO << "gain changed from " << std::fixed << std::setprecision(6)
          << old_gain << " to " << new_gain;

  QThread::sleep(2);
  emit restart_run();
}

bool FormOptimization::find_peaks() {
  PL_INFO << "find peaks called";

  int xmin = moving_.position - ui->spinPeakWindow->value();
  int xmax = moving_.position + ui->spinPeakWindow->value();

  if (xmin < 0) xmin = 0;
  if (xmax >= x.size()) xmax = x.size() - 1;

  if (moving_.visible) {
    gauss_ref_ = Peak(x, y_ref, xmin, xmax);
    gauss_opt_ = Peak(x, y_opt, xmin, xmax);

    replot_markers();

    return true;
  } else
    return false;
}

void FormOptimization::update_plots() {

  PL_INFO << "update plots called";

  double maxx = max_x;
  bool have_data = false;

  if (ui->plot->isVisible()) {
    this->setCursor(Qt::WaitCursor);

    ui->plot->clearGraphs();
    have_data = true;

    for (auto &q: spectra_.by_type("1D")) {
      if (q->total_count()) {

        //        PL_INFO << "count=" << q->total_count();
        uint32_t res = pow(2, bits);
        uint32_t app = q->appearance();
        std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data =
            std::move(q->get_spectrum({{0, res}}));

        std::vector<double> y(res);
        for (auto it : *spectrum_data) {
          y[it.first[0]] = it.second;
        }

        if (q->name() == "Reference")
          y_ref = y;
        else
          y_opt = y;

        for (std::size_t i=0; i < res; i++)
          x[i] = i;

        ui->plot->addGraph(QVector<double>::fromStdVector(x),
                           QVector<double>::fromStdVector(y),
                           QColor::fromRgba(app), 1);

        if (maxx < res)
          maxx = res;
      } else
        have_data = false;
    }
    ui->plot->setLabels("channel", "count");

    if (gauss_ref_.refined_.height_) {
      ui->plot->addGraph(QVector<double>::fromStdVector(gauss_ref_.x_), QVector<double>::fromStdVector(gauss_ref_.y_fullfit_), Qt::cyan, 0);
      ui->plot->addGraph(QVector<double>::fromStdVector(gauss_ref_.x_), QVector<double>::fromStdVector(gauss_ref_.baseline_.evaluate_array(gauss_ref_.x_)), Qt::cyan, 0);
    }

    if (gauss_opt_.refined_.height_)
      ui->plot->addGraph(QVector<double>::fromStdVector(gauss_opt_.x_), QVector<double>::fromStdVector(gauss_opt_.y_fullfit_), Qt::magenta, 0);

    if (max_x != maxx) {
      ui->plot->reset_scales();
      max_x = maxx;
    } else
      ui->plot->update_plot();

    std::string new_label = boost::algorithm::trim_copy(spectra_.status());
    ui->plot->setTitle(QString::fromStdString(new_label));
  }

  if (running && have_data && (minTimer->s() > ui->spinSampleMin->value()) && find_peaks()) {
    PL_INFO << "Singular peaks found within range. Moving along...";
    delete minTimer;
    interruptor_.store(true);
    running = false;
  }

  this->setCursor(Qt::ArrowCursor);
}

void FormOptimization::addMovingMarker(double x) {
  moving_.position = x;
  moving_.visible = true;
  a.position = x - ui->spinPeakWindow->value();
  a.visible = true;
  b.position = x + ui->spinPeakWindow->value();
  b.visible = true;
  //  ui->pushAdd->setEnabled(true);
  //  PL_INFO << "Marker at " << moving.position << " originally caliblated to " << old_calibration_.transform(moving.position)
  //          << ", new calibration = " << new_calibration_.transform(moving.position);
  replot_markers();
}

void FormOptimization::removeMovingMarker(double x) {
  moving_.visible = false;
  a.visible = false;
  b.visible = false;
  //  ui->pushAdd->setEnabled(false);
  replot_markers();
}


void FormOptimization::replot_markers() {
  std::list<Marker> markers;

  //markers.push_back(moving);

  if (gauss_ref_.refined_.height_) {
    marker_ref_.position = gauss_ref_.refined_.center_;
    marker_ref_.visible = true;
    markers.push_back(marker_ref_);
  }

  if (gauss_opt_.refined_.height_) {
    marker_opt_.position = gauss_opt_.refined_.center_;
    marker_opt_.visible = true;
    markers.push_back(marker_opt_);
  }

  ui->plot->set_markers(markers);
  ui->plot->set_block(a,b);
}


void FormOptimization::on_pushMatchGain_clicked()
{
  current_pass = ui->spinMaxPasses->value();
  do_run();
}

void FormOptimization::on_pushStop_clicked()
{
  PL_INFO << "Acquisition interrupted by user";
  current_pass = 0;
  interruptor_.store(true);
}

void FormOptimization::on_pushSaveOpti_clicked()
{
    pixie_.settings().save_optimization();
    std::vector<Pixie::Detector> dets = pixie_.settings().get_detectors();
    for (auto &q : dets) {
      if (detectors_.has_a(q)) {
        Pixie::Detector modified = detectors_.get(q);
        modified.setting_values_ = q.setting_values_;
        detectors_.replace(modified);
      }
    }
    emit optimization_approved();
}
