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
  QThread(parent),
  devices_(Qpx::Wrapper::getInstance().settings()),
  terminating_(false)
{
  spectra_ = nullptr;
  interruptor_ = nullptr;
  action_ = kNone;
  fake_chans_ = {0,1};
  exact_index_ = false;
  file_ = "";
  source_res_ = 0;
  dest_res_ = 0;
  xdt_ = 0.0;
}

ThreadRunner::~ThreadRunner()
{}

void ThreadRunner::terminate() {
  PL_INFO << "runner thread termination requested";
  if (interruptor_)
    interruptor_->store(true);
  terminating_.store(true);
  wait();
}

bool ThreadRunner::terminating() {
  return terminating_.load();
}


void ThreadRunner::do_list(boost::atomic<bool> &interruptor, uint64_t timeout)
{
  if (!isRunning()) {
    terminating_.store(false);
    QMutexLocker locker(&mutex_);
    interruptor_ = &interruptor;
    timeout_ = timeout;
    action_ = kList;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_list. Runner busy.";
}


void ThreadRunner::do_run(Qpx::SpectraSet &spectra, boost::atomic<bool> &interruptor, uint64_t timeout)
{
  if (!isRunning()) {
    terminating_.store(false);
    QMutexLocker locker(&mutex_);
    spectra_ = &spectra;
    interruptor_ = &interruptor;
    timeout_ = timeout;
    action_ = kMCA;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_run. Runner busy.";
}

void ThreadRunner::do_fake(Qpx::SpectraSet &spectra, boost::atomic<bool> &interruptor, QString file, std::array<int,2> chans, int source_res, int dest_res, int timeout) {
  if (!isRunning()) {
    terminating_.store(false);
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

void ThreadRunner::do_from_list(Qpx::SpectraSet &spectra, boost::atomic<bool> &interruptor, QString file) {
  if (!isRunning()) {
    terminating_.store(false);
    QMutexLocker locker(&mutex_);
    spectra_ = &spectra;
    interruptor_ = &interruptor;
    file_ = file;
    action_ = kFromList;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_from_list. Runner busy.";
}

void ThreadRunner::do_boot() {
  if (!isRunning()) {
    terminating_.store(false);
    action_ = kBoot;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_boot. Runner busy.";
}

void ThreadRunner::do_shutdown() {
  if (!isRunning()) {
    terminating_.store(false);
    action_ = kShutdown;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_shutdown. Runner busy.";
}

void ThreadRunner::do_execute_command(const Gamma::Setting &tree) {
  if (!isRunning()) {
    terminating_.store(false);
    action_ = kExecuteCommand;
    tree_ = tree;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_execute_command. Runner busy.";
}

void ThreadRunner::do_push_settings(const Gamma::Setting &tree) {
  if (!isRunning()) {
    terminating_.store(false);
    action_ = kPushSettings;
    tree_ = tree;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_push_settings. Runner busy.";
}

void ThreadRunner::do_set_setting(const Gamma::Setting &item, bool exact_index) {
  if (!isRunning()) {
    terminating_.store(false);
    action_ = kSetSetting;
    tree_ = item;
    exact_index_ = exact_index;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_set_setting. Runner busy.";
}

void ThreadRunner::do_set_detector(int chan, Gamma::Detector det) {
  if (!isRunning()) {
    terminating_.store(false);
    action_ = kSetDetector;
    chan_ = chan;
    det_ = det;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_set_detector. Runner busy.";
}


void ThreadRunner::do_optimize() {
  if (!isRunning()) {
    terminating_.store(false);
    action_ = kOptimize;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_optimize. Runner busy.";
}

void ThreadRunner::do_oscil(double xdt) {
  if (!isRunning()) {
    action_ = kOscil;
    xdt_ = xdt;
    terminating_.store(false);
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_oscil. Runner busy.";
}

void ThreadRunner::do_refresh_settings() {
  if (!isRunning()) {
    terminating_.store(false);
    action_ = kSettingsRefresh;
    start(HighPriority);
  } else
    PL_WARN << "Cannot do_refresh_settings. Runner busy.";
}

void ThreadRunner::run()
{
  if (action_ == kMCA) {
    interruptor_->store(false);
    devices_.reset_counters_next_run();
    Qpx::Wrapper::getInstance().getMca(timeout_, *spectra_, *interruptor_);
    emit runComplete();
  } else if (action_ == kList) {
    interruptor_->store(false);
    devices_.reset_counters_next_run();
    Qpx::ListData *newListRun = Qpx::Wrapper::getInstance().getList(timeout_, *interruptor_);
    emit listComplete(newListRun);
  } else if (action_ == kSimulate) {
    interruptor_->store(false);
    Qpx::SpectraSet* intermediate = new Qpx::SpectraSet;
    intermediate->read_xml(file_.toStdString(), true);
    Qpx::Simulator mySource = Qpx::Simulator(intermediate, fake_chans_, source_res_, dest_res_);
    delete intermediate;
    if (mySource.valid())
      Qpx::Wrapper::getInstance().getFakeMca(mySource, *spectra_, timeout_, *interruptor_);
    emit runComplete();
  } else if (action_ == kFromList) {
    interruptor_->store(false);
    Qpx::Sorter sorter(file_.toStdString());
    if (sorter.valid())
      Qpx::Wrapper::getInstance().simulateFromList(sorter, *spectra_, *interruptor_);
    emit runComplete();
  } else if (action_ == kBoot) {
    if (devices_.boot()) {
      devices_.get_all_settings();
      devices_.save_optimization();
    }
    emit settingsUpdated(devices_.pull_settings(), devices_.get_detectors());
  } else if (action_ == kShutdown) {
    if (devices_.die()) {
      devices_.get_all_settings();
      devices_.save_optimization();
    }
    emit settingsUpdated(devices_.pull_settings(), devices_.get_detectors());
  } else if (action_ == kOptimize) {
    devices_.load_optimization();
    action_ = kOscil;
  } else if (action_ == kSettingsRefresh) {
    devices_.get_all_settings();
    devices_.save_optimization();
    emit settingsUpdated(devices_.pull_settings(), devices_.get_detectors());
  } else if (action_ == kExecuteCommand) {
    devices_.push_settings(tree_);
    bool success = devices_.execute_command();
    if (success) {
      devices_.get_all_settings();
      devices_.save_optimization();
    }
    emit settingsUpdated(devices_.pull_settings(), devices_.get_detectors());
  } else if (action_ == kPushSettings) {
    devices_.push_settings(tree_);
    devices_.get_all_settings();
    devices_.save_optimization();
    emit settingsUpdated(devices_.pull_settings(), devices_.get_detectors());
  } else if (action_ == kSetSetting) {
    devices_.set_setting(tree_, exact_index_);
    devices_.get_all_settings();
    devices_.save_optimization();
    emit settingsUpdated(devices_.pull_settings(), devices_.get_detectors());
  } else if (action_ == kSetDetector) {
    devices_.set_detector(chan_, det_);
    devices_.load_optimization(chan_);
    devices_.write_settings_bulk();
    devices_.get_all_settings();
    devices_.save_optimization();
    emit settingsUpdated(devices_.pull_settings(), devices_.get_detectors());
  }

  if (action_ == kOscil) {
    std::vector<Gamma::Detector> dets = devices_.get_detectors();

    Gamma::Setting set = Gamma::Setting("QpxSettings/Pixie-4/System/module/channel/XDT", 0, Gamma::SettingType::floating, 0);
    for (int i=0; i < dets.size(); i++) {
      set.index = i;
      set.value = xdt_;
      devices_.set_setting(set);
    }
    std::vector<Qpx::Trace> traces = devices_.oscilloscope();

    devices_.get_all_settings();
    devices_.save_optimization();
    emit settingsUpdated(devices_.pull_settings(), devices_.get_detectors());

    if (!traces.empty())
      emit oscilReadOut(traces);
  }
}
