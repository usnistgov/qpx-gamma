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

  area_hyp = hypermet_.area();
  area_sum4 = sum4_.peak_area;

  area_best = area_hyp;

  //  if (sum4_.currie_quality_indicator == 1)
//  {
//    area_gross_ = sum4_.P_area;
//    area_bckg_ = sum4_.B_area;
//    area_best_ = area_net_;
//  }

  if (live_seconds > 0) {
    cps_hyp  = area_hyp / live_seconds;
    cps_sum4 = area_sum4 / live_seconds;
  }

  cps_best = cps_hyp;
}


}
