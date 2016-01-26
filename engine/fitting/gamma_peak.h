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
 *      Gamma::Peak
 *
 ******************************************************************************/

#ifndef GAMMA_PEAK_H
#define GAMMA_PEAK_H

#include <vector>
#include <set>
#include "polynomial.h"
#include "gaussian.h"
#include "hypermet.h"
#include "detector.h"
#include "sum4.h"

namespace Gamma {

class Peak {
public:
  Peak()
      : subpeak (false)
      , center(0)
      , energy(0)
      , height(0)
      , fwhm_sum4(0)
      , fwhm_gaussian (0)
      , hwhm_L (0)
      , hwhm_R (0)
      , fwhm_theoretical(0)
      , lim_L (0)
      , lim_R (0)
      , intersects_L (false)
      , intersects_R (false)
      , selected (false)
      , flagged (false)
      , area_net_(0.0)
      , area_gross_(0.0)
      , area_bckg_(0.0)
      , area_gauss_(0.0)
      , live_seconds_(0.0)
      , cts_per_sec_net_(0.0)
      , cts_per_sec_gauss_(0.0)
      , area_best_(0.0)
      , cps_best_(0.0)
      , intensity_theoretical_(0.0)
      , efficiency_relative_(0.0)
  {}

  Peak(const std::vector<double> &x, const std::vector<double> &y, uint32_t L, uint32_t R,
       Calibration cali_nrg = Calibration(), Calibration cali_fwhm = Calibration(), double live_seconds = 0.0, uint16_t sum4edge_samples = 3);
  
  void construct(Calibration cali_nrg, Calibration cali_fwhm);

  std::vector<double> x_, y_, y_baseline_, y_fullfit_gaussian_, y_fullfit_hypermet_, y_residues_g_;

  std::vector<double> hr_x_, hr_baseline_, hr_fullfit_gaussian_, hr_fullfit_hypermet_;

  SUM4 sum4_;
  Gaussian gaussian_;
  Hypermet hypermet_;

  double center, energy, height, fwhm_sum4, fwhm_gaussian, hwhm_L, hwhm_R,
      fwhm_theoretical, lim_L, lim_R;
  bool intersects_L, intersects_R;
  bool selected, flagged;

  double area_net_, area_gross_, area_bckg_, area_gauss_;
  double live_seconds_, cts_per_sec_net_, cts_per_sec_gauss_;
  double area_best_, cps_best_;
  double intensity_theoretical_, efficiency_relative_;
  
  bool subpeak;

  static bool by_center (const Peak& a, const Peak& b)
  {
    return (a.center < b.center);
  }

  bool operator<(const Peak &other) const {
    return (center < other.center);
  }

  bool operator==(const Peak &other) const {
    return (center == other.center);
  }

  bool operator>(const Peak &other) const {
    return (center > other.center);
  }
  
};

}

#endif
