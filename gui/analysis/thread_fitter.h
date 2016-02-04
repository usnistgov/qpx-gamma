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

enum FitterAction {kFit, kStop, kIdle, kAddPeak, kRemovePeaks};

class ThreadFitter : public QThread
{
  Q_OBJECT
public:
  explicit ThreadFitter(QObject *parent = 0);
  void terminate();

  void begin();
  void set_data(Gamma::Fitter &data);

  void fit_peaks();
  void stop_work();
  void add_peak(uint32_t L, uint32_t R);
  void remove_peaks(std::set<double> chosen_peaks);

signals:
  void fit_updated(Gamma::Fitter data);
  void fitting_done();

protected:
  void run();

private:
  Gamma::Fitter fitter_;

  QMutex mutex_;
  FitterAction action_;

  uint32_t left_, right_;
  std::set<double> chosen_peaks_;

  boost::atomic<bool> running_;
  boost::atomic<bool> terminating_;
};

#endif
