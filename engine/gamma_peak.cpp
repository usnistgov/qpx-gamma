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

Peak::Peak(const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> &y_baseline,
           Calibration cali_nrg, Calibration cali_fwhm, std::vector<Peak> peaks)
  : Peak()
{
  fit(x, y, y_baseline, cali_nrg, cali_fwhm, peaks);
}


void Peak::construct(Calibration cali_nrg, Calibration cali_fwhm) {
  y_fullfit_gaussian_.resize(x_.size());
  y_fullfit_pseudovoigt_.resize(x_.size());
  for (int32_t i = 0; i < static_cast<int32_t>(x_.size()); ++i) {
    y_fullfit_gaussian_[i] = y_baseline_[i] + gaussian_.evaluate(x_[i]);
    y_fullfit_pseudovoigt_[i] = y_baseline_[i] + pseudovoigt_.evaluate(x_[i]);
  }

  center = gaussian_.center_;
  energy = cali_nrg.transform(center);
  height = gaussian_.height_;

  double L, R;

  L = gaussian_.center_ - gaussian_.hwhm_;
  R = gaussian_.center_ + gaussian_.hwhm_;
  fwhm_gaussian = cali_nrg.transform(R) - cali_nrg.transform(L);
  if ((gaussian_.hwhm_*2) >= x_.size())
    fwhm_gaussian = 0;

  L = gaussian_.center_ - pseudovoigt_.hwhm_l;
  R = gaussian_.center_ + pseudovoigt_.hwhm_r;
  hwhm_L = energy - cali_nrg.transform(L);
  hwhm_R = cali_nrg.transform(R) - energy;
  fwhm_pseudovoigt = cali_nrg.transform(R) - cali_nrg.transform(L);
  if ((pseudovoigt_.hwhm_l + pseudovoigt_.hwhm_r) >= x_.size())
    fwhm_pseudovoigt = 0;

  fwhm_theoretical = cali_fwhm.transform(energy); 
}


void Peak::fit(const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> &y_baseline,
               Calibration cali_nrg, Calibration cali_fwhm, std::vector<Peak> peaks) {
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

    if (peaks.empty()) {
      gaussian_ = Gaussian(x_, nobase);
      pseudovoigt_ = SplitPseudoVoigt(x_, nobase);
      construct(cali_nrg, cali_fwhm);
    } else {
 
      std::vector<Gaussian> old_gauss;
      for (auto &q : peaks)
        old_gauss.push_back(q.gaussian_);
      std::vector<Gaussian>       gauss = Gaussian::fit_multi(x_, nobase, old_gauss); 

      std::vector<SplitPseudoVoigt> old_spv;
      for (auto &q : peaks)
        old_spv.push_back(q.pseudovoigt_);
      std::vector<SplitPseudoVoigt> spv = SplitPseudoVoigt::fit_multi(x_, nobase, old_spv);
      if ((gauss.size() == spv.size()) &&
          (gauss.size() == peaks.size())) {
        multiplet = true;
        y_fullfit_gaussian_.resize(x_.size());
        y_fullfit_pseudovoigt_.resize(x_.size());
        for (int32_t i = 0; i < static_cast<int32_t>(x_.size()); ++i) {
          y_fullfit_gaussian_[i] = y_baseline_[i];
          y_fullfit_pseudovoigt_[i] = y_baseline_[i];
        }
        for (int i=0; i < peaks.size(); ++i) {
          Peak one;
          one.subpeak = true;
          one.intersects_L = peaks[i].intersects_L;
          one.intersects_R = peaks[i].intersects_R;          
          one.x_ = x;
          one.y_ = y;
          one.y_baseline_ = y_baseline;
          one.gaussian_ = gauss[i];
          one.pseudovoigt_ = spv[i];
          one.construct(cali_nrg, cali_fwhm);
          subpeaks_.push_back(one);
          for (int32_t j = 0; j < static_cast<int32_t>(x_.size()); ++j) {
            y_fullfit_gaussian_[j] +=  gauss[i].evaluate(x_[j]);
            y_fullfit_pseudovoigt_[j] += spv[i].evaluate(x_[j]);
          }
        }

        center = subpeaks_[0].center;
        energy = subpeaks_[0].energy;
        height = -1;
        fwhm_gaussian = -1;
        hwhm_L = -1;
        hwhm_R = -1;
        fwhm_pseudovoigt = -1;
        fwhm_theoretical = -1; 

        
      }
    }
  }
}


}
