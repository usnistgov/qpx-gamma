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

#pragma once

#include <QThread>
#include <QMutex>
#include "fitter.h"

enum FitterAction {kFit, kStop, kIdle, kAddPeak, kRemovePeaks, kRefit,
                  kAdjustLB, kAdjustRB, kOverrideSettingsROI, kMergeRegions};

class ThreadFitter : public QThread
{
  Q_OBJECT
public:
  explicit ThreadFitter(QObject *parent = 0);
  void terminate();

  void begin();
  void set_data(const Qpx::Fitter &data);

  void fit_peaks();
  void stop_work();
  void refit(double target_ROI);
  void add_peak(double L, double R);
  void merge_regions(double L, double R);
  void adjust_LB(double target_ROI, double L, double R);
  void adjust_RB(double target_ROI, double L, double R);
  void override_ROI_settings(double regionID, FitSettings fs);
  void remove_peaks(std::set<double> chosen_peaks);

signals:
  void fit_updated(Qpx::Fitter data);
  void fitting_done();

protected:
  void run();

private:
  Qpx::Fitter fitter_;

  QMutex mutex_;
  FitterAction action_;

  double LL, RR;
  double target_;
  Hypermet hypermet_;
  FitSettings settings_;
  std::set<double> chosen_peaks_;

  boost::atomic<bool> running_;
  boost::atomic<bool> terminating_;
  boost::atomic<bool> interruptor_;

};
