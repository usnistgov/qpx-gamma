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
 *      Gamma::ROI
 *
 ******************************************************************************/

#ifndef GAMMA_ROI_H
#define GAMMA_ROI_H

#include "gamma_peak.h"

namespace Gamma {


struct ROI {
  ROI(const Calibration &nrg, const Calibration &fw, double live_seconds = 0.0)
  : cal_nrg_ (nrg)
  , cal_fwhm_ (fw)
  , live_seconds_(live_seconds) 
  {}

  bool contains(double bin);
  bool overlaps(const Peak &);
  void add_peaks(const std::set<Peak> &pks, const std::vector<double> &x, const std::vector<double> &y);
  void add_peak(const Peak &pk, const std::vector<double> &x, const std::vector<double> &y);
  void remove_peak(const Peak &pk);
  void remove_peak(double bin);
  void rebuild();

  std::set<Peak> peaks_;
  std::vector<double> x_, y_,
    y_background_, y_fullfit_g_, y_fullfit_h_, y_resid_h_;
  Calibration cal_nrg_, cal_fwhm_;
  double live_seconds_;

private:
  static double local_avg(const std::vector<double> &x,
                          const std::vector<double> &y,
                          uint16_t chan,
                          uint16_t samples = 1);

  static std::vector<double> make_background(const std::vector<double> &x,
                                             const std::vector<double> &y,
                                             uint16_t left, uint16_t right,
                                             uint16_t samples);

};


}

#endif
