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

void ThreadFitter::set_data(const Gamma::Fitter &data)
{
  if (running_.load()) {
    PL_WARN << "Fitter busy";
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
    PL_WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kFit;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::add_peak(uint32_t L, uint32_t R) {
  if (running_.load()) {
    PL_WARN << "Fitter busy";
    return;
  }
  QMutexLocker locker(&mutex_);
  terminating_.store(false);
  action_ = kAddPeak;
  left_ = L;
  right_ = R;
  if (!isRunning())
    start(HighPriority);
}

void ThreadFitter::refit(double target_ROI) {
  if (running_.load()) {
    PL_WARN << "Fitter busy";
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
    PL_WARN << "Fitter busy";
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


void ThreadFitter::remove_peaks(std::set<double> chosen_peaks) {
  if (running_.load()) {
    PL_WARN << "Fitter busy";
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
      for (auto &q : fitter_.regions_) {
        PL_DBG << "<Fitter> Fitting region " << current << " of " << fitter_.regions_.size() << " at E=" << q.first;
        q.second.auto_fit(interruptor_);
        fitter_.remap_region(q.second);
        current++;
        emit fit_updated(fitter_);
        if ((action_ == kStop) || terminating_.load())
          break;
      }
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kRefit) {
      if (fitter_.regions_.count(target_ROI_)) {
        fitter_.regions_[target_ROI_].rebuild();
        if (fitter_.regions_[target_ROI_].settings_.resid_auto)
          fitter_.remap_region(fitter_.regions_[target_ROI_]);
      }
      emit fit_updated(fitter_);
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kAddPeak) {
      fitter_.add_peak(left_, right_, interruptor_);
      emit fit_updated(fitter_);
      emit fitting_done();
      action_ = kIdle;
    } else if (action_ == kAdjustROI) {
      if (fitter_.regions_.count(target_ROI_))
        fitter_.adj_bounds(fitter_.regions_[target_ROI_], left_, right_, interruptor_);
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


