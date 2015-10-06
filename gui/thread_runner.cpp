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
 *      ThreadRunner - thread for issuing commands to daq outside of gui thread
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
  engine_(Qpx::Engine::getInstance()),
  terminating_(false),
  running_(false)
{
  spectra_ = nullptr;
  interruptor_ = nullptr;
  action_ = kNone;
  fake_chans_ = {0,1};
  file_ = "";
  source_res_ = 0;
  dest_res_ = 0;
  xdt_ = 0.0;
  match_conditions_ = Gamma::Match::id;
  start(HighPriority);
}

ThreadRunner::~ThreadRunner()
{}

void ThreadRunner::terminate() {
//  PL_INFO << "runner thread termination requested";
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
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  interruptor_ = &interruptor;
  timeout_ = timeout;
  action_ = kList;
  if (!isRunning())
    start(HighPriority);
}


void ThreadRunner::do_run(Qpx::SpectraSet &spectra, boost::atomic<bool> &interruptor, uint64_t timeout)
{
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  spectra_ = &spectra;
  interruptor_ = &interruptor;
  timeout_ = timeout;
  action_ = kMCA;
  if (!isRunning())
    start(HighPriority);
}

void ThreadRunner::do_fake(Qpx::SpectraSet &spectra, boost::atomic<bool> &interruptor, QString file, std::array<int,2> chans, int source_res, int dest_res, int timeout) {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  spectra_ = &spectra;
  interruptor_ = &interruptor;
  file_ = file;
  fake_chans_ = chans;
  source_res_ = source_res;
  dest_res_ = dest_res;
  timeout_ = timeout;
  action_ = kSimulate;
  if (!isRunning())
    start(HighPriority);
}

void ThreadRunner::do_from_list(Qpx::SpectraSet &spectra, boost::atomic<bool> &interruptor, QString file) {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  spectra_ = &spectra;
  interruptor_ = &interruptor;
  file_ = file;
  action_ = kFromList;
  if (!isRunning())
    start(HighPriority);
}

void ThreadRunner::do_initialize(QString file) {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kInitialize;
  file_ = file;
  if (!isRunning())
    start(HighPriority);
}


void ThreadRunner::do_boot() {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kBoot;
  if (!isRunning())
    start(HighPriority);
}

void ThreadRunner::do_shutdown() {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kShutdown;
  if (!isRunning())
    start(HighPriority);
}

void ThreadRunner::do_execute_command(const Gamma::Setting &tree) {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  tree_ = tree;
  action_ = kExecuteCommand;
  if (!isRunning())
    start(HighPriority);
}

void ThreadRunner::do_push_settings(const Gamma::Setting &tree) {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  tree_ = tree;
  action_ = kPushSettings;
  if (!isRunning())
    start(HighPriority);
}

void ThreadRunner::do_set_setting(const Gamma::Setting &item, Gamma::Match match) {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  tree_ = item;
  match_conditions_ = match;
  action_ = kSetSetting;
  if (!isRunning())
    start(HighPriority);
}

void ThreadRunner::do_set_detector(int chan, Gamma::Detector det) {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  chan_ = chan;
  det_ = det;
  action_ = kSetDetector;
  if (!isRunning())
    start(HighPriority);
}

void ThreadRunner::do_set_detectors(std::map<int, Gamma::Detector> dets) {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  detectors_ = dets;
  action_ = kSetDetectors;
  if (!isRunning())
    start(HighPriority);
}


void ThreadRunner::do_optimize() {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kOptimize;
  if (!isRunning())
    start(HighPriority);
}

void ThreadRunner::do_oscil(double xdt) {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  xdt_ = xdt;
  action_ = kOscil;
  if (!isRunning())
    start(HighPriority);
}

void ThreadRunner::do_refresh_settings() {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kSettingsRefresh;
  if (!isRunning())
    start(HighPriority);
}

void ThreadRunner::run()
{
  while (!terminating_.load()) {

    if (action_ != kNone)
      running_.store(true);

    if (action_ == kMCA) {
      interruptor_->store(false);
      engine_.getMca(timeout_, *spectra_, *interruptor_);
      action_ = kNone;
      emit runComplete();
    } else if (action_ == kList) {
      interruptor_->store(false);
      Qpx::ListData *newListRun = engine_.getList(timeout_, *interruptor_);
      action_ = kNone;
      emit listComplete(newListRun);
    } else if (action_ == kSimulate) {
      interruptor_->store(false);
      Qpx::SpectraSet* intermediate = new Qpx::SpectraSet;
      intermediate->read_xml(file_.toStdString(), true);
      Qpx::Simulator mySource = Qpx::Simulator(intermediate, fake_chans_, source_res_, dest_res_);
      delete intermediate;
      if (mySource.valid())
        engine_.getFakeMca(mySource, *spectra_, timeout_, *interruptor_);
      action_ = kNone;
      emit runComplete();
    } else if (action_ == kFromList) {
      interruptor_->store(false);
      Qpx::Sorter sorter(file_.toStdString());
      if (sorter.valid())
        engine_.simulateFromList(sorter, *spectra_, *interruptor_);
      action_ = kNone;
      emit runComplete();
    } else if (action_ == kInitialize) {
      engine_.initialize(file_.toStdString());
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors());
    } else if (action_ == kBoot) {
      if (engine_.boot()) {
        engine_.get_all_settings();
        engine_.save_optimization();
      }
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors());
      emit bootComplete();
    } else if (action_ == kShutdown) {
      if (engine_.die()) {
        engine_.get_all_settings();
        engine_.save_optimization();
      }
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors());
    } else if (action_ == kOptimize) {
      engine_.load_optimization();
      action_ = kOscil;
    } else if (action_ == kSettingsRefresh) {
      engine_.get_all_settings();
      engine_.save_optimization();
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors());
    } else if (action_ == kExecuteCommand) {
      engine_.push_settings(tree_);
      bool success = engine_.execute_command();
      if (success) {
        engine_.get_all_settings();
        engine_.save_optimization();
      }
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors());
    } else if (action_ == kPushSettings) {
      engine_.push_settings(tree_);
      engine_.get_all_settings();
      engine_.save_optimization();
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors());
    } else if (action_ == kSetSetting) {
      engine_.set_setting(tree_, match_conditions_);
      engine_.get_all_settings();
      engine_.save_optimization();
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors());
    } else if (action_ == kSetDetector) {
      engine_.set_detector(chan_, det_);
      engine_.load_optimization(chan_);
      engine_.write_settings_bulk();
      engine_.get_all_settings();
      engine_.save_optimization();
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors());
    } else if (action_ == kSetDetectors) {
      for (auto &q : detectors_) {
        engine_.set_detector(q.first, q.second);
        engine_.load_optimization(q.first);
      }
      engine_.write_settings_bulk();
      Gamma::Setting set = Gamma::Setting("Adjust offsets");
      set.value_dbl = 1;
      engine_.set_setting(set, Gamma::Match::name);
      engine_.execute_command();

      //XDT?

      //engine_.get_all_settings();
      //engine_.save_optimization();
      action_ = kOscil;
      //emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors());
    } else if (action_ == kOscil) {
        std::vector<Gamma::Detector> dets = engine_.get_detectors();

        Gamma::Setting set = Gamma::Setting("XDT");
        for (int i=0; i < dets.size(); i++) {
          set.index = i;
          set.value_dbl = xdt_;
          engine_.set_setting(set, Gamma::Match::name | Gamma::Match::indices);
        }
        std::vector<Qpx::Trace> traces = engine_.oscilloscope();

        engine_.get_all_settings();
        engine_.save_optimization();

        action_ = kNone;

        emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors());
        if (!traces.empty())
          emit oscilReadOut(traces);
    } else {
      //PL_DBG << "idling";
      QThread::sleep(0.5);
    }
    running_.store(false);
  }
}
