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
 * Description:
 *      ThreadRunner - thread for issuing commands to Pixie outside of gui thread
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include <boost/thread.hpp>
#include "thread_runner.h"
#include "custom_logger.h"

ThreadRunner::ThreadRunner(QObject *parent) :
  QThread(parent)
{
  spectra_ = nullptr;
  interruptor_ = nullptr;
  action_ = kNone;
  run_type_ = Pixie::RunType::compressed;
  fake_chans_ = {0,1};
  file_ = "";
  source_res_ = 0;
  dest_res_ = 0;
  boot_keepcw_ = true;
  mod_filter_ = 0.0;
  mod_wait_ = 0.0;
}

ThreadRunner::~ThreadRunner()
{}

void ThreadRunner::terminate() {
  PL_INFO << "runner thread termination requested";
  if (interruptor_)
    interruptor_->store(true);
  wait();
}

void ThreadRunner::do_list(boost::atomic<bool> &interruptor, Pixie::RunType type, uint64_t timeout, bool dblbuf)
{
  if (!isRunning()) {
    QMutexLocker locker(&mutex_);
    interruptor_ = &interruptor;
    run_type_ = type;
    timeout_ = timeout;
    dblbuf_ = dblbuf;
    action_ = kList;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_list. Runner busy.";
}


void ThreadRunner::do_run(Pixie::SpectraSet &spectra, boost::atomic<bool> &interruptor, Pixie::RunType type, uint64_t timeout, bool dblbuf)
{
  if (!isRunning()) {
    QMutexLocker locker(&mutex_);
    spectra_ = &spectra;
    interruptor_ = &interruptor;
    run_type_ = type;
    timeout_ = timeout;
    dblbuf_ = dblbuf;
    action_ = kMCA;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_run. Runner busy.";
}

void ThreadRunner::do_fake(Pixie::SpectraSet &spectra, boost::atomic<bool> &interruptor, QString file, std::array<int,2> chans, int source_res, int dest_res, int timeout) {
  if (!isRunning()) {
    QMutexLocker locker(&mutex_);
    spectra_ = &spectra;
    interruptor_ = &interruptor;
    file_ = file;
    fake_chans_ = chans;
    source_res_ = source_res;
    dest_res_ = dest_res;
    timeout_ = timeout;
    action_ = kSimulate;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_fake. Runner busy.";
}

void ThreadRunner::do_boot(bool boot_keepcw, std::vector<std::string> boot_files, std::vector<uint8_t> boot_slots) {
  if (!isRunning()) {
    boot_keepcw_ = boot_keepcw;
    boot_files_ = boot_files;
    boot_slots_ = boot_slots;
    action_ = kBoot;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_boot. Runner busy.";
}

void ThreadRunner::do_offsets() {
  if (!isRunning()) {
    action_ = kOffsets;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_offsets. Runner busy.";
}

void ThreadRunner::do_baselines() {
  if (!isRunning()) {
    action_ = kBaselines;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_baselines. Runner busy.";
}

void ThreadRunner::do_optimize() {
  if (!isRunning()) {
    action_ = kOptimize;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_optimize. Runner busy.";
}

void ThreadRunner::do_oscil() {
  if (!isRunning()) {
    action_ = kOscil;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_oscil. Runner busy.";
}

void ThreadRunner::do_refresh_settings() {
  if (!isRunning()) {
    action_ = kSettingsRefresh;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_refresh_settings. Runner busy.";
}

void ThreadRunner::do_tau() {
  if (!isRunning()) {
    action_ = kTau;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_tau. Runner busy.";
}

void ThreadRunner::do_BLcut() {
  if (!isRunning()) {
    action_ = kBLcut;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_BLcut. Runner busy.";
}


void ThreadRunner::run()
{
  if (action_ == kMCA) {
    interruptor_->store(false);
    Pixie::Wrapper::getInstance().getMca(run_type_, timeout_, *spectra_, *interruptor_, dblbuf_);
    emit runComplete();
  } else if (action_ == kList) {
    interruptor_->store(false);
    Pixie::ListData *newListRun = Pixie::Wrapper::getInstance().getList(run_type_, timeout_, *interruptor_, dblbuf_);
    emit listComplete(newListRun);
  } else if (action_ == kSimulate) {
    interruptor_->store(false);
    Pixie::SpectraSet* intermediate = new Pixie::SpectraSet;
    intermediate->read_xml(file_.toStdString(), true);
    Pixie::Simulator mySource = Pixie::Simulator(intermediate, fake_chans_, source_res_, dest_res_);
    delete intermediate;
    Pixie::Wrapper::getInstance().getFakeMca(mySource, *spectra_, timeout_, *interruptor_);
    emit runComplete();
  } else if (action_ == kBoot) {
    Pixie::Wrapper &myPixie = Pixie::Wrapper::getInstance();
    myPixie.settings().set_boot_files(boot_files_);
    myPixie.settings().set_slots(boot_slots_);
    myPixie.settings().set_sys("OFFLINE_ANALYSIS", 0);
    myPixie.settings().set_sys("KEEP_CW", boot_keepcw_);
    myPixie.settings().set_sys("MAX_NUMBER_MODULES", 7);  //should not be hardcoded
    bool success = myPixie.boot();
    if (myPixie.settings().live() == Pixie::LiveStatus::online) {
      PL_INFO << "Pixie online functions enabled.";
      emit bootComplete(true, true);
    } else {
      PL_INFO << "Attempting boot of offline functions";
      myPixie.settings().set_sys("OFFLINE_ANALYSIS", 1);
      success = myPixie.boot();
      if (success)
        PL_INFO << "Offline boot successful";
      emit bootComplete(success, false);
    }
  } else if (action_ == kOffsets) {
    Pixie::Wrapper::getInstance().control_adjust_offsets();
    action_ = kOscil;
  } else if (action_ == kBaselines) {
    Pixie::Wrapper::getInstance().control_measure_baselines(0);  //hardcoded for module 0
    action_ = kOscil;
  } else if (action_ == kOptimize) {
    Pixie::Wrapper::getInstance().settings().load_optimization();
    action_ = kOscil;
  } else if (action_ == kSettingsRefresh) {
    Pixie::Wrapper::getInstance().settings().get_all_settings();
    emit settingsUpdated();
  } else if (action_ == kTau) {
    Pixie::Wrapper::getInstance().control_find_tau();
    emit settingsUpdated();
  } else if (action_ == kBLcut) {
    Pixie::Wrapper::getInstance().control_compute_BLcut();
    emit settingsUpdated();
  }

  if (action_ == kOscil) {
    Pixie::Hit oscil_traces_ = Pixie::Wrapper::getInstance().getOscil();
    uint32_t trace_length = oscil_traces_.trace[0].size();

    QList<QVector<double>> *plot_data = new QList<QVector<double>>;
    plot_data->push_back(QVector<double>(trace_length));
    std::iota(plot_data->last().begin(), plot_data->last().end(), 0);

    for (int i=0; i < 4; i++) {  //hardcoded for 4 channels
      plot_data->push_back(QVector<double>());
      Pixie::Detector thisdet = Pixie::Wrapper::getInstance().settings().get_detector(Pixie::Channel(i));
      int hi_res = 0;
      for (auto &q : thisdet.energy_calibrations_.my_data_) {
        if (q.bits_ > hi_res)
          hi_res = q.bits_;
      }
      Pixie::Calibration thiscal = thisdet.energy_calibrations_.get(Pixie::Calibration(hi_res));
      int shiftby = 0;
      if (thiscal.bits_)
        shiftby = 16 - thiscal.bits_;
      for (auto it : oscil_traces_.trace[i]) {
        double calibrated = thiscal.transform(it >> shiftby);
        plot_data->last().push_back(calibrated);
      }
    }
    Pixie::Wrapper::getInstance().settings().get_all_settings();
    emit settingsUpdated();
    emit oscilReadOut(plot_data);
  }
}
