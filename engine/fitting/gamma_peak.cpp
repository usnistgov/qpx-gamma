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


void Peak::construct(Calibration cali_nrg, double live_seconds) {

  hr_fullfit_hypermet_.resize(hr_x_.size());
  for (int32_t i = 0; i < static_cast<int32_t>(hr_x_.size()); ++i)
    hr_fullfit_hypermet_[i] = hr_baseline_h_[i] + hypermet_.eval_peak(hr_x_[i]);

  center = hypermet_.center_;
  energy = cali_nrg.transform(center); //bits??
  height = hypermet_.height_;

//  center = sum4_.centroid;
//  energy = cali_nrg.transform(center);
//  height = gaussian_.height_;

  double L, R;

  L = sum4_.centroid - sum4_.fwhm / 2;
  R = sum4_.centroid + sum4_.fwhm / 2;
  fwhm_sum4 = cali_nrg.transform(R) - cali_nrg.transform(L);
//  if (sum4_.fwhm >= x_.size())
//    fwhm_sum4 = 0;

  L = hypermet_.center_ - hypermet_.width_ * 2 * sqrt(log(2));
  R = hypermet_.center_ + hypermet_.width_ * 2 * sqrt(log(2));
  fwhm_hyp = cali_nrg.transform(R) - cali_nrg.transform(L);
//  if ((gaussian_.hwhm_*2) >= x_.size())
//    fwhm_gaussian = 0;

  area_hyp_ = hypermet_.area(); //gaussian_.height_ * gaussian_.hwhm_ * sqrt(M_PI / log(2.0));
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

Peak::Peak(const Hypermet &hype, const SUM4 &sum4,
     const std::vector<double> &hr_x,
     const std::vector<double> &hr_baseline_h,
     Calibration cali_nrg,
     double live_seconds)
{
  if ((hr_x.size() != hr_baseline_h.size())
      || (hr_x.empty()))
    return;

//      sum4_ = SUM4(x, y, l, r, sum4edge_samples);

  hr_x_ = hr_x;
  hr_baseline_h_ = hr_baseline_h;
  hypermet_ = hype;
  sum4_ = sum4;

  construct(cali_nrg, live_seconds);

//  if (gaussian_.hwhm_ > 0) {
//    double L = gaussian_.center_ - 2.5*gaussian_.hwhm_;
//    double R = gaussian_.center_ + 2.5*gaussian_.hwhm_;

//    int32_t l = x.size() - 1;
//    while ((l > 0) && (x[l] > L))
//      l--;

//    int32_t r = 0;
//    while ((r < x.size()) && (x[r] < R))
//      r++;

//  }
}



}
