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

#include "gamma_peak.h"

namespace Gamma {

Peak::Peak(const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> &y_baseline, Calibration cali)
  : Peak()
{
  fit(x, y, y_baseline, cali);
}


void Peak::fit(const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> &y_baseline, Calibration cali) {
  if (
      (x.size() == y.size())
      &&
      (y_baseline.size() == y.size())
      )
  {
    x_ = x;
    y_ = y;
    y_baseline_ = y_baseline;
    std::vector<double> nobase(x_.size());
    
    for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i)
      nobase[i] = y_[i] - y_baseline_[i];
    
    gaussian_ = Gaussian(x_, nobase);
    pseudovoigt_ = SplitPseudoVoigt(x_, nobase);

    y_fullfit_gaussian_.resize(x_.size());
    y_fullfit_pseudovoigt_.resize(x_.size());
    for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i) {
      y_fullfit_gaussian_[i] = y_baseline_[i] + gaussian_.evaluate(x_[i]);
      y_fullfit_pseudovoigt_[i] = y_baseline_[i] + pseudovoigt_.evaluate(x_[i]);
    }

    center = gaussian_.center_;
    energy = cali.transform(center);
    height = gaussian_.height_;

    double L, R;

    L = gaussian_.center_ - gaussian_.hwhm_;
    R = gaussian_.center_ + gaussian_.hwhm_;
    fwhm_gaussian = cali.transform(R) - cali.transform(L);
    if ((gaussian_.hwhm_*2) >= x.size())
      fwhm_gaussian = 0;

    L = pseudovoigt_.center_ - pseudovoigt_.hwhm_l;
    R = pseudovoigt_.center_ + pseudovoigt_.hwhm_r;
    fwhm_pseudovoigt = cali.transform(R) - cali.transform(L);
    if ((pseudovoigt_.hwhm_l + pseudovoigt_.hwhm_r) >= x.size())
      fwhm_pseudovoigt = 0;

  }
}

}
