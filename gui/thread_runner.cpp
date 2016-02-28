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
  file_ = "";
  xdt_ = 0.0;
  flag_ = false;
  match_conditions_ = Qpx::Match::id;
  idle_refresh_.store(false);
  idle_refresh_frequency_.store(1);
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

void ThreadRunner::set_idle_refresh(bool refresh) {
  idle_refresh_.store(refresh);
}

void ThreadRunner::set_idle_refresh_frequency(int secs) {
  if (secs < 1)
    secs = 1;
  idle_refresh_frequency_.store(secs);
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

void ThreadRunner::do_initialize(QString file, bool boot) {
  if (running_.load()) {
    PL_WARN << "Runner busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  flag_ = boot;
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

void ThreadRunner::do_push_settings(const Qpx::Setting &tree) {
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

void ThreadRunner::do_set_setting(const Qpx::Setting &item, Qpx::Match match) {
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

void ThreadRunner::do_set_detector(int chan, Qpx::Detector det) {
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

void ThreadRunner::do_set_detectors(std::map<int, Qpx::Detector> dets) {
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
      engine_.get_all_settings();
      engine_.save_optimization();
      Qpx::DeviceStatus ds = engine_.status() ^ Qpx::DeviceStatus::can_run; //turn off can_run
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors(), ds);
      interruptor_->store(false);
      engine_.getMca(timeout_, *spectra_, *interruptor_);
      action_ = kSettingsRefresh;
      emit runComplete();
    } else if (action_ == kList) {
      interruptor_->store(false);
      Qpx::DeviceStatus ds = engine_.status() ^ Qpx::DeviceStatus::can_run; //turn off can_run
      Qpx::ListData *newListRun = engine_.getList(timeout_, *interruptor_);
      action_ = kSettingsRefresh;
      emit listComplete(newListRun);
    } else if (action_ == kInitialize) {
      engine_.initialize(file_.toStdString());
      if (flag_) {
        action_ = kBoot;
        flag_ = false;
      } else {
        action_ = kNone;
        emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors(), engine_.status());
      }
    } else if (action_ == kBoot) {
      if (engine_.boot()) {
        engine_.get_all_settings();
        engine_.save_optimization();
      }
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors(), engine_.status());
      emit bootComplete();
    } else if (action_ == kShutdown) {
      if (engine_.die()) {
        engine_.get_all_settings();
        engine_.save_optimization();
      }
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors(), engine_.status());
    } else if (action_ == kOptimize) {
      engine_.load_optimization();
      action_ = kOscil;
    } else if (action_ == kSettingsRefresh) {
      engine_.get_all_settings();
      engine_.save_optimization();
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors(), engine_.status());
    } else if (action_ == kPushSettings) {
      engine_.push_settings(tree_);
      engine_.get_all_settings();
      engine_.save_optimization();
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors(), engine_.status());
    } else if (action_ == kSetSetting) {
      engine_.set_setting(tree_, match_conditions_);
      engine_.get_all_settings();
      engine_.save_optimization();
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors(), engine_.status());
    } else if (action_ == kSetDetector) {
      engine_.set_detector(chan_, det_);
      engine_.write_settings_bulk();
      engine_.get_all_settings();
      engine_.save_optimization();
      action_ = kNone;
      emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors(), engine_.status());
    } else if (action_ == kSetDetectors) {
      for (auto &q : detectors_) {
        engine_.set_detector(q.first, q.second);
        engine_.load_optimization(q.first);
      }
      engine_.write_settings_bulk();

      //XDT?

      //engine_.get_all_settings();
      //engine_.save_optimization();
      action_ = kOscil;
      //emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors(), engine_.status());
    } else if (action_ == kOscil) {
        std::vector<Qpx::Detector> dets = engine_.get_detectors();

        Qpx::Setting set = Qpx::Setting("XDT");
        for (int i=0; i < dets.size(); i++) {
          set.indices.clear();
          set.indices.insert(i);
          set.value_dbl = xdt_;
          engine_.set_setting(set, Qpx::Match::name | Qpx::Match::indices);
        }
        std::vector<Qpx::Trace> traces = engine_.oscilloscope();

        engine_.get_all_settings();
        engine_.save_optimization();

        action_ = kNone;

        emit settingsUpdated(engine_.pull_settings(), engine_.get_detectors(), engine_.status());
        if (!traces.empty())
          emit oscilReadOut(traces);
    } else {
      bool booted = ((engine_.status() & Qpx::DeviceStatus::booted) != 0);
      if (booted && idle_refresh_.load()) {
        action_ = kSettingsRefresh;
        QThread::sleep(idle_refresh_frequency_.load());
        //PL_DBG << "idle runner will refresh settings";
      }
    }
    running_.store(false);
  }
}
