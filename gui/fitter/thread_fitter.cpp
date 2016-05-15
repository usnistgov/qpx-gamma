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
  target_ROI_ = target_ROI;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::adjust_roi_bounds(double target_ROI, uint32_t L, uint32_t R) {
  if (running_.load()) {
    WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kAdjustROI;
  target_ROI_ = target_ROI;
  left_ = L;
  right_ = R;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::adjust_LB(double target_ROI, uint32_t L, uint32_t R) {
  if (running_.load()) {
    WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kAdjustLB;
  target_ROI_ = target_ROI;
  left_ = L;
  right_ = R;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::adjust_RB(double target_ROI, uint32_t L, uint32_t R) {
  if (running_.load()) {
    WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kAdjustRB;
  target_ROI_ = target_ROI;
  left_ = L;
  right_ = R;
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
      if (fitter_.regions_.count(target_ROI_)) {
        fitter_.regions_[target_ROI_].rebuild();
        if (fitter_.regions_[target_ROI_].settings_.resid_auto)
          fitter_.regions_[target_ROI_].iterative_fit(interruptor_);
      }
      emit fit_updated(fitter_);
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kAddPeak) {
      fitter_.add_peak(LL, RR, interruptor_);
      emit fit_updated(fitter_);
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kAdjustROI) {
      if (fitter_.regions_.count(target_ROI_))
        fitter_.adj_bounds(fitter_.regions_[target_ROI_], left_, right_, interruptor_);
      emit fit_updated(fitter_);
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kAdjustLB) {
      if (fitter_.regions_.count(target_ROI_)) {
        Qpx::ROI *parent_region = &fitter_.regions_[target_ROI_];
        Qpx::SUM4Edge edge(parent_region->finder_.x_, parent_region->finder_.y_, left_, right_);
        parent_region->set_LB(edge);
        emit fit_updated(fitter_);
      }
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kAdjustRB) {
      if (fitter_.regions_.count(target_ROI_)) {
        Qpx::ROI *parent_region = &fitter_.regions_[target_ROI_];
        Qpx::SUM4Edge edge(parent_region->finder_.x_, parent_region->finder_.y_, left_, right_);
        parent_region->set_RB(edge);
        emit fit_updated(fitter_);
      }
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


