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
 *      Qpx::Peak
 *
 ******************************************************************************/

#include "gamma_peak.h"

namespace Qpx {


void Peak::construct(FitSettings fs) {

  center = hypermet_.center_.val;
  energy = fs.cali_nrg_.transform(center, fs.bits_);

//  center = sum4_.centroid;
//  energy = cali_nrg.transform(center, bits);
//  height = gaussian_.height_;

  double L, R;

  L = sum4_.centroid.val - sum4_.fwhm / 2;
  R = sum4_.centroid.val + sum4_.fwhm / 2;
  fwhm_sum4 = fs.cali_nrg_.transform(R, fs.bits_) - fs.cali_nrg_.transform(L, fs.bits_);

  L = hypermet_.center_.val - hypermet_.width_.val * sqrt(log(2));
  R = hypermet_.center_.val + hypermet_.width_.val * sqrt(log(2));
  fwhm_hyp = fs.cali_nrg_.transform(R, fs.bits_) - fs.cali_nrg_.transform(L, fs.bits_);

  area_hyp = hypermet_.area();
  area_sum4 = sum4_.peak_area;

  area_best = area_sum4;

  //  if (sum4_.currie_quality_indicator == 1)
//  {
//    area_gross_ = sum4_.P_area;
//    area_bckg_ = sum4_.B_area;
//    area_best_ = area_net_;
//  }

  cps_best = cps_hyp = cps_sum4 = 0;

  double live_seconds = fs.live_time.total_milliseconds() * 0.001;

  if (live_seconds > 0) {
    cps_hyp  = area_hyp.val / live_seconds;
    cps_sum4 = area_sum4.val / live_seconds;
    cps_best = cps_hyp;
  }

}


}
