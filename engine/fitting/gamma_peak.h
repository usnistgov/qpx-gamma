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
#include "hypermet.h"
#include "detector.h"
#include "sum4.h"

namespace Gamma {

class Peak {
public:
  Peak()
      : center(0)
      , energy(0)
      , height(0)
      , fwhm_sum4(0)
      , fwhm_hyp (0)
      , selected (false)
      , flagged (false)
      , area_net_(0.0)
      , area_gross_(0.0)
      , area_bckg_(0.0)
      , area_hyp_(0.0)
      , cts_per_sec_net_(0.0)
      , cts_per_sec_hyp_(0.0)
      , area_best_(0.0)
      , cps_best_(0.0)
      , intensity_theoretical_(0.0)
      , efficiency_relative_(0.0)
  {}


  void construct(Calibration cali_nrg, double live_seconds, uint16_t bits);

  std::vector<double> hr_peak_, hr_fullfit_;

  SUM4 sum4_;
  Hypermet hypermet_;

  double center, energy, height, fwhm_sum4, fwhm_hyp;
  bool selected, flagged;

  double area_net_, area_gross_, area_bckg_, area_hyp_;
  double cts_per_sec_net_, cts_per_sec_hyp_;
  double area_best_, cps_best_;
  double intensity_theoretical_, efficiency_relative_;
  

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
