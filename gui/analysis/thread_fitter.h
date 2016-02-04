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

#ifndef THREAD_FITTER_H_
#define THREAD_FITTER_H_

#include <QThread>
#include <QMutex>
#include "gamma_fitter.h"

enum FitterAction {kFit, kStop, kIdle};

class ThreadFitter : public QThread
{
    Q_OBJECT
public:
    explicit ThreadFitter(QObject *parent = 0) :
        QThread(parent),
        terminating_(false),
        running_(false)
      {
        action_ = kIdle;
        start(HighPriority);
      }


  void terminate() {
    terminating_.store(true);
    wait();
  }

    void begin() {
      if (!isRunning())
        start(HighPriority);
    }

    void set_data(Gamma::Fitter &data)
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

    void fit_peaks() {
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

    void stop_work() {
      QMutexLocker locker(&mutex_);
      action_ = kStop; //not thread safe
    }

signals:
    void fit_updated(Gamma::Fitter data);
    void fitting_done();

protected:
    void run() {

      while (!terminating_.load()) {
        if (action_ != kIdle)
          running_.store(true);

        if (action_ == kFit) {
          int current = 1;
          for (auto &q : fitter_.regions_) {
            PL_DBG << "<Fitter> Fitting region " << current << " of " << fitter_.regions_.size() << "...";
            q.auto_fit();
            current++;
            fitter_.remap_peaks();
            emit fit_updated(fitter_);
            if ((action_ == kStop) || terminating_.load())
              break;
          }
          emit fitting_done();
          action_ = kIdle;
        } else {
          QThread::sleep(2);
        }
        running_.store(false);
      }
    }

private:
    Gamma::Fitter fitter_;

    QMutex mutex_;
    FitterAction action_;

    boost::atomic<bool> running_;
    boost::atomic<bool> terminating_;
};

#endif
