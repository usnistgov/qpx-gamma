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
      , fwhm_sum4(0)
      , fwhm_hyp (0)
      , intensity_theoretical_(0.0)
      , efficiency_relative_(0.0)
  {}


  void construct(FitSettings fs);

  std::vector<double> hr_peak_, hr_fullfit_;

  SUM4 sum4_;
  Hypermet hypermet_;

  double center, energy, fwhm_sum4, fwhm_hyp;

  FitParam area_sum4, area_hyp, area_best;
  double cps_sum4, cps_hyp, cps_best;

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
