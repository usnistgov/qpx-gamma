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
#include "polynomial.h"
#include "finder.h"
#include <boost/atomic.hpp>

namespace Gamma {

struct Fit {
  Finder finder_;
  std::map<double, Peak> peaks_;
  Polynomial background_;
};


struct ROI {
  ROI(const Calibration &nrg, const Calibration &fw, uint16_t bits)
    : cal_nrg_ (nrg)
    , cal_fwhm_ (fw)
    , bits_(bits)
  {}

  void set_data(const std::vector<double> &x, const std::vector<double> &y,
                uint16_t min, uint16_t max);
  void auto_fit(boost::atomic<bool>& interruptor);

  bool contains(double bin);
  bool overlaps(uint16_t bin);
  bool overlaps(uint16_t Lbin, uint16_t Rbin);

  //  void add_peaks(const std::set<Peak> &pks, const std::vector<double> &x, const std::vector<double> &y);
  void add_peak(const std::vector<double> &x, const std::vector<double> &y, uint16_t L, uint16_t R, boost::atomic<bool>& interruptor);
  bool add_from_resid(int32_t centroid_hint = -1);

  bool remove_peaks(const std::set<double> &pks);

  bool rollback(int i);

  void rebuild();
  void render();
  void save_current_fit();

  //intrinsic
  Calibration cal_nrg_, cal_fwhm_;
  uint16_t bits_;

  std::vector<Fit> fits_;
//  Fit current_fit_;

  //per_fit
  Finder finder_;
  std::map<double, Peak> peaks_;
  Polynomial background_;

  //rendition
  std::vector<double> hr_x, hr_x_nrg,
                      hr_background, hr_back_steps,
                      hr_fullfit;

private:
  std::vector<double> make_background(uint16_t samples = 5);
  void init_background(uint16_t samples = 5);
  bool remove_peak(double bin);
};


}

#endif
