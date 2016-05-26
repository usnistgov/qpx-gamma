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
 *      ThreadPlotPrep - thread for preparation of spectra to be plotted
 *
 ******************************************************************************/

#ifndef THREAD_PLOT_SIGNAL_H_
#define THREAD_PLOT_SIGNAL_H_

#include <QThread>
#include <QMutex>
#include "project.h"
//#include "custom_logger.h"

class ThreadPlotSignal : public QThread
{
  Q_OBJECT
public:
  explicit ThreadPlotSignal(QObject *parent = 0)
    : QThread(parent)
    , terminating_(false)
    , wait_s_(3)
  {}

  void monitor_source(Qpx::ProjectPtr pj)
  {
    QMutexLocker locker(&mutex_);
    terminate_helper();
    project_ = pj;
    terminating_.store(false);
    if (!isRunning())
      start(HighPriority);
  }

  void terminate_wait()
  {
    QMutexLocker locker(&mutex_);
    terminate_helper();
  }

  void set_wait_time(uint16_t time)
  {
    wait_s_.store(time);
  }

  Qpx::ProjectPtr current_source()
  {
    QMutexLocker locker(&mutex_);
    return project_;
  }

signals:
  void plot_ready();

protected:
  void run() {
//    DBG << "<ThreadPlot> loop starting "
//        << terminating_.load() << " "
//        << (project_.operator bool());

    while (!terminating_.load()
           && project_
           && project_->wait_ready())
    {
      emit plot_ready();
      if (!terminating_.load())
        QThread::sleep(wait_s_.load());
    }
//    DBG << "<ThreadPlot> loop ended";
  }

private:
  QMutex mutex_;
  Qpx::ProjectPtr project_;
  boost::atomic<bool> terminating_;
  boost::atomic<uint16_t> wait_s_;

  void terminate_helper()
  {
    terminating_.store(true);
    if (project_)
      project_->activate();
    wait();
  }

};

#endif
