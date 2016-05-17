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
 *      ThreadFitter - thread for preparation of spectra to be plotted
 *
 ******************************************************************************/

#include "thread_fitter.h"
#include "custom_timer.h"

ThreadFitter::ThreadFitter(QObject *parent) :
  QThread(parent),
  terminating_(false),
  running_(false)
{
  action_ = kIdle;
  start(HighPriority);
}

void ThreadFitter::terminate() {
  terminating_.store(true);
  wait();
}

void ThreadFitter::begin() {
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::set_data(const Qpx::Fitter &data)
{
  if (running_.load()) {
    WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  fitter_ = data;
  action_ = kIdle;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::fit_peaks() {
  if (running_.load()) {
    WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kFit;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::add_peak(double L, double R) {
  if (running_.load()) {
    WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kAddPeak;
  LL = L;
  RR = R;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::refit(double target_ROI) {
  if (running_.load()) {
    WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kRefit;
  target_ = target_ROI;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::adjust_SUM4(double target_peak, double left, double right)
{
  if (running_.load()) {
    WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kAdjustSUM4;
  target_ = target_peak;
  LL = left;
  RR = right;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::adjust_LB(double target_ROI, double L, double R) {
  if (running_.load()) {
    WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kAdjustLB;
  target_ = target_ROI;
  LL = L;
  RR = R;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::adjust_RB(double target_ROI, double L, double R) {
  if (running_.load()) {
    WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kAdjustRB;
  target_ = target_ROI;
  LL = L;
  RR = R;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::merge_regions(double L, double R) {
  if (running_.load()) {
    WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kMergeRegions;
  LL = L;
  RR = R;
  if (!isRunning())
    start(HighPriority);
}


void ThreadFitter::remove_peaks(std::set<double> chosen_peaks) {
  if (running_.load()) {
    WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kRemovePeaks;
  chosen_peaks_ = chosen_peaks;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::stop_work() {
  QMutexLocker locker(&mutex_);
  action_ = kStop; //not thread safe
  interruptor_.store(true);
}

void ThreadFitter::run() {

  while (!terminating_.load()) {
    if (action_ != kIdle) {
      running_.store(true);
      interruptor_.store(false);
    }

    if (action_ == kFit) {
      int current = 1;
      CustomTimer total_timer(true);
      std::shared_ptr<CustomTimer> timer(new CustomTimer(true));
      for (auto &q : fitter_.regions_) {
//        DBG << "Fitting " << q.second.L();
        q.second.auto_fit(interruptor_);
        current++;
        if (timer->s() > 2) {
          emit fit_updated(fitter_);
          timer = std::shared_ptr<CustomTimer>(new CustomTimer(true));
          DBG << "<Fitter> " << current << " of " << fitter_.regions_.size() << " regions completed";
        }
        if ((action_ == kStop) || terminating_.load())
          break;
      }
      DBG << "<Fitter> Fitting spectrum was on average " << total_timer.s() / double(fitter_.peaks().size())
          << " s/peak";
      emit fit_updated(fitter_);
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kRefit) {
      if (fitter_.regions_.count(target_)) {
        fitter_.regions_[target_].rebuild();
        if (fitter_.regions_[target_].fit_settings().resid_auto)
          fitter_.regions_[target_].iterative_fit(interruptor_);
      }
      emit fit_updated(fitter_);
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kAddPeak) {
      fitter_.add_peak(LL, RR, interruptor_);
      emit fit_updated(fitter_);
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kAdjustSUM4) {
      if (fitter_.adjust_sum4(target_, LL, RR))
      {
        //emit newselection somehow?
        emit fit_updated(fitter_);
      }
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kAdjustLB) {
      if (fitter_.adj_LB(target_, LL, RR, interruptor_))
        emit fit_updated(fitter_);
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kAdjustRB) {
      if (fitter_.adj_RB(target_, LL, RR, interruptor_))
        emit fit_updated(fitter_);
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kMergeRegions) {
      if (fitter_.merge_regions(LL, RR, interruptor_))
        emit fit_updated(fitter_);
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kRemovePeaks) {
      fitter_.remove_peaks(chosen_peaks_);
      emit fit_updated(fitter_);
      emit fitting_done();
      action_ = kIdle;
    } else {
      QThread::sleep(2);
    }
    running_.store(false);
  }
}


