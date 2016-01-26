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


void Peak::construct(Calibration cali_nrg, Calibration cali_fwhm) {
  y_fullfit_gaussian_.resize(x_.size());
  y_fullfit_hypermet_.resize(x_.size());
  y_residues_g_.resize(x_.size());
  for (int32_t i = 0; i < static_cast<int32_t>(x_.size()); ++i) {
    y_fullfit_gaussian_[i] = y_baseline_[i] + gaussian_.evaluate(x_[i]);
    y_fullfit_hypermet_[i] = y_baseline_[i] + hypermet_.evaluate(x_[i]);
    y_residues_g_[i] = y_[i] - y_fullfit_gaussian_[i];
  }


  hr_fullfit_gaussian_.resize(hr_x_.size());
  hr_fullfit_hypermet_.resize(hr_x_.size());
  for (int32_t i = 0; i < static_cast<int32_t>(hr_x_.size()); ++i) {
    hr_fullfit_gaussian_[i] = hr_baseline_[i] + gaussian_.evaluate(hr_x_[i]);
    hr_fullfit_hypermet_[i] = hr_baseline_[i] + hypermet_.evaluate(hr_x_[i]);
  }

  center = gaussian_.center_;
  energy = cali_nrg.transform(center);
  height = gaussian_.height_;

//  center = sum4_.centroid;
//  energy = cali_nrg.transform(center);
//  height = gaussian_.height_;

  double L, R;

  L = sum4_.centroid - sum4_.fwhm / 2;
  R = sum4_.centroid + sum4_.fwhm / 2;
  fwhm_sum4 = cali_nrg.transform(R) - cali_nrg.transform(L);
  if (sum4_.fwhm >= x_.size())
    fwhm_sum4 = 0;

  L = gaussian_.center_ - gaussian_.hwhm_;
  R = gaussian_.center_ + gaussian_.hwhm_;
  hwhm_L = energy - cali_nrg.transform(L);
  hwhm_R = cali_nrg.transform(R) - energy;
  fwhm_gaussian = cali_nrg.transform(R) - cali_nrg.transform(L);
  if ((gaussian_.hwhm_*2) >= x_.size())
    fwhm_gaussian = 0;

  fwhm_theoretical = cali_fwhm.transform(energy);

  area_gauss_ =  gaussian_.height_ * gaussian_.hwhm_ * sqrt(3.141592653589793238462643383279502884 / log(2.0));
  area_best_ = area_gauss_;
  if (!subpeak)
  {
    area_gross_ = sum4_.P_area;
    area_bckg_ = sum4_.B_area;
    area_net_ = sum4_.net_area;
    area_best_ = area_net_;
  }

  if (live_seconds_ > 0) {
    cps_best_ = cts_per_sec_gauss_ = area_gauss_ / live_seconds_;
    cts_per_sec_net_ = area_net_ / live_seconds_;
    if (cts_per_sec_net_ > 0)
      cps_best_ = cts_per_sec_net_;
  }
}

Peak::Peak(const std::vector<double> &x, const std::vector<double> &y, uint32_t L, uint32_t R,
           Calibration cali_nrg, Calibration cali_fwhm, double live_seconds, uint16_t sum4edge_samples)
  : Peak()
{

  int32_t l = x.size() - 1;
  while ((l > 0) && (x[l] > L))
    l--;

  int32_t r = 0;
  while ((r < x.size()) && (x[r] < R))
    r++;

  if (
      (x.size() == y.size())
      &&
      (l < r)
      &&
      (r < x.size())
      )
  {    
    //PL_DBG << "Constructing peak bw channels " << L << "-" << R << "  indices=" << l << "-" << r;
    sum4_ = SUM4(x, y, l, r, sum4edge_samples);

    x_ = std::vector<double>(x.begin() + l, x.begin() + r + 1);
    y_ = std::vector<double>(y.begin() + l, y.begin() + r + 1);
    y_baseline_ = sum4_.by_;
    
    std::vector<double> nobase(x_.size());    
    for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i)
      nobase[i] = y_[i] - y_baseline_[i];

    gaussian_ = Gaussian(x_, nobase);
    if (gaussian_.height_ > 0)
      hypermet_ = Hypermet(x_, nobase, gaussian_.height_, gaussian_.center_, gaussian_.hwhm_);
    live_seconds_ = live_seconds;

    hr_baseline_.clear();
    hr_x_.clear();
    for (double i = x_[0]; i <= x_[x_.size() -1]; i += 0.25) {
      hr_x_.push_back(i);
      hr_baseline_.push_back(sum4_.offset + i* sum4_.slope);
    }

    construct(cali_nrg, cali_fwhm);
  }
}



}
