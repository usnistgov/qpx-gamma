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
#include "polynomial.h"
#include "gaussian.h"
#include "voigt_.h"
#include "detector.h"

namespace Gamma {

class Peak {
public:
  Peak()
    : center(0)
    , energy(0)
    , height(0)
    , fwhm_gaussian (0)
    , fwhm_pseudovoigt (0)
    , selected (false)
  {}

  Peak(const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> &y_baseline, Calibration cali = Calibration());

  std::vector<double> x_, y_, y_baseline_, y_fullfit_gaussian_, y_fullfit_pseudovoigt_;
  Gaussian gaussian_;
  SplitPseudoVoigt pseudovoigt_;

  double center, energy, height, fwhm_gaussian, fwhm_pseudovoigt;
  bool selected;

  static bool by_centroid_gaussian (const Peak& a, const Peak& b)
  {
      return (a.gaussian_.center_ < b.gaussian_.center_);
  }

private:
  void fit(const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> &y_baseline, Calibration cali = Calibration());
};

enum class BaselineType : int {linear = 0, step = 1, step_polynomial = 2};

}

#endif
