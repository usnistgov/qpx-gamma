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
#include "custom_logger.h"

namespace Qpx {


void Peak::construct(FitSettings fs) {

  if (hypermet_.height_.val) {
    center.val = hypermet_.center_.val;
    center.uncert = hypermet_.center_.uncert;
  } else {
    center.val = sum4_.centroid.val;
    center.uncert = sum4_.centroid.uncert;
  }

  if (std::isinf(center.uncert) || std::isnan(center.uncert)) {
    center.uncert = sum4_.centroid.uncert;
//    PL_DBG << "overriding peak center uncert with sum4 for " << center.val << " with " << sum4_.centroid.to_string();
  }

  energy.val = fs.cali_nrg_.transform(center.val, fs.bits_);

  double emin = fs.cali_nrg_.transform(energy.val - 0.5 * center.uncert, fs.bits_);
  double emax = fs.cali_nrg_.transform(energy.val + 0.5 * center.uncert, fs.bits_);
  energy.uncert = emax - emin;


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

  cps_best = cps_hyp = cps_sum4 = 0;

  double live_seconds = fs.live_time.total_milliseconds() * 0.001;

  if (live_seconds > 0) {
    cps_hyp  = area_hyp.val / live_seconds;
    cps_sum4 = area_sum4.val / live_seconds;
    if (hypermet_.height_.val)
      cps_best = cps_hyp;
    else
      cps_best = cps_sum4;
  }

}


}
