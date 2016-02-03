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


void Peak::construct(Calibration cali_nrg, double live_seconds, uint16_t bits) {

  center = hypermet_.center_;
  energy = cali_nrg.transform(center, bits);
  height = hypermet_.height_;

//  center = sum4_.centroid;
//  energy = cali_nrg.transform(center, bits);
//  height = gaussian_.height_;

  double L, R;

  L = sum4_.centroid - sum4_.fwhm / 2;
  R = sum4_.centroid + sum4_.fwhm / 2;
  fwhm_sum4 = cali_nrg.transform(R, bits) - cali_nrg.transform(L, bits);

  L = hypermet_.center_ - hypermet_.width_ * sqrt(log(2));
  R = hypermet_.center_ + hypermet_.width_ * sqrt(log(2));
  fwhm_hyp = cali_nrg.transform(R, bits) - cali_nrg.transform(L, bits);

  area_hyp_ = hypermet_.area();
  area_best_ = area_hyp_;
  if (sum4_.fwhm > 0)
  {
    area_gross_ = sum4_.P_area;
    area_bckg_ = sum4_.B_area;
    area_net_ = sum4_.net_area;
    area_best_ = area_net_;
  }

  if (live_seconds > 0) {
    cps_best_ = cts_per_sec_hyp_ = area_hyp_ / live_seconds;
    cts_per_sec_net_ = area_net_ / live_seconds;
    if (cts_per_sec_net_ > 0)
      cps_best_ = cts_per_sec_net_;
  }
}


}
