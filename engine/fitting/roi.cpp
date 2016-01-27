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
 *      Gamma::ROI
 *
 ******************************************************************************/

#include "roi.h"

namespace Gamma {

void ROI::set_data(const std::vector<double> &x, const std::vector<double> &y,
                   uint16_t min, uint16_t max) {
  x_.clear();
  y_.clear();
  finder_.clear();

  if (x.size() != y.size())
    return;

  if ((min >= x.size()) || (max >= x.size()))
    return;

  for (uint16_t i = min; i <= max; ++i) {
    x_.push_back(x[i]);
    y_.push_back(y[i]);
  }

//  PL_DBG << "ROI has " << y_.size() << " points";

  finder_.setData(x_, y_); //assumes default threshold parameters!!!!
  make_background();

//  PL_DBG << "finder found " << finder_.filtered.size();

  if (peaks_.empty() && finder_.filtered.size()) {
    for (int i=0; i < finder_.filtered.size(); ++i) {
      Peak fitted = Peak(finder_.x_, y_nobkg_,
                         finder_.x_[finder_.lefts[i]], finder_.x_[finder_.rights[i]],
                         cal_nrg_, cal_fwhm_,
                         live_seconds_, /*sum4edge_samples*/ 3); //assumption

      if (
          (fitted.height > 0) &&
          (fitted.fwhm_sum4 > 0) &&
//          (fitted.fwhm_gaussian > 0) &&
          (finder_.x_[finder_.lefts[i]] < fitted.center) &&
          (fitted.center < finder_.x_[finder_.rights[i]])
         )
      {
        PL_DBG << "I like this peak at " << fitted.center << " fw " << fitted.fwhm_gaussian;
        peaks_.insert(fitted);
      }
    }

    rebuild();
  }
}

bool ROI::contains(double bin) {
  for (auto &q : peaks_)
    if (q.center == bin)
      return true;
  return false;
}

//bool ROI::overlaps(const Peak &pk) {
//  return (overlaps(pk.x_.front()) || overlaps(pk.x_.back()));
//}

bool ROI::overlaps(uint16_t bin) {
  return ((bin >= x_.front()) && (bin <= x_.back()));
}

//void ROI::add_peak(const Peak &pk, const std::vector<double> &x, const std::vector<double> &y)
//{
//  std::set<Peak> pks;
//  pks.insert(pk);
//  add_peaks(pks, x, y);
//}

//void ROI::add_peaks(const std::set<Peak> &pks, const std::vector<double> &x, const std::vector<double> &y)
//{
//  for (auto &q : pks) {
//    peaks_.erase(q);
//    peaks_.insert(q);
//  }

//  int left = std::numeric_limits<int>::max();
//  int right = - std::numeric_limits<int>::max();
//  for (auto &q : peaks_) {
//    int l = q.x_.front();
//    int r = q.x_.back();
    
//    if (l < left)
//      left = l;
//    if (r > right)
//      right = r;
//  }
//  x_ = std::vector<double>(x.begin() + left, x.begin() + right + 1);
//  y_ = std::vector<double>(y.begin() + left, y.begin() + right + 1);
//  finder_.setData(x_, y_);
//  make_background();
//  rebuild();
//}

void ROI::remove_peaks(const std::set<double> &pks) {
  bool found = false;
  for (auto &q : pks) {
    for (auto &p : peaks_) {
      if (p.center == q) {
        peaks_.erase(p);
        found = true;
        break;
      }
    }
  }
  if (found)
    rebuild();
}

void ROI::remove_peak(const Peak &pk) {
  if (peaks_.count(pk)) {
    peaks_.erase(pk);
    rebuild();
  }
}

void ROI::remove_peak(double bin) {
  for (auto &q : peaks_) {
    if (q.center == bin) {
      peaks_.erase(q);
      rebuild();
      break;
    }
  }
}

void ROI::rebuild() {
  y_fullfit_g_.clear();
  y_fullfit_h_.clear();

  if (peaks_.empty())
    return;

  std::vector<Gaussian> gauss;
  for (auto &q : peaks_)
    gauss.push_back(q.gaussian_);

  if (peaks_.size() > 1)
   gauss = Gaussian::fit_multi(x_, y_nobkg_, gauss);

  Gaussian tallest_g;
  for (auto &q : gauss) {
    if (q.height_ > tallest_g.height_)
      tallest_g = q;
  }

//  PL_DBG << "Using tallest peak at " << tallest_g.center_ << " as starting point for Hypermet constraints";

  Hypermet tallest_h;//(x_, y_nobkg_, tallest_g.height_, tallest_g.center_, tallest_g.hwhm_);

  std::vector<Hypermet> old_hype;
//  old_hype.push_back(tallest_h);

  for (int i=0; i < gauss.size(); ++i) {
//    if ((gauss[i].hwhm_ > 0) &&
//        (gauss[i].height_ > 0) && (gauss[i].center_ != tallest_g.center_)) {
      Hypermet hyp(gauss[i].height_, gauss[i].center_, gauss[i].hwhm_);
      old_hype.push_back(hyp);
//    }
  }

  std::vector<Hypermet> hype = old_hype;// Hypermet::fit_multi(x_, y_nobkg_, old_hype, true);

  peaks_.clear();

  y_fullfit_g_ = y_background_;
  y_fullfit_h_ = y_background_;
  y_resid_h_ = y_;

  std::vector<double> hr_baseline_;
  std::vector<double> hr_x_;
  double offset = y_background_[0];
  double slope  = (y_background_[y_background_.size() -1] - y_background_[0]) / (x_[x_.size() -1] - x_[0]);
  for (double i = 0; i < x_.size(); i += 0.25) {
    hr_x_.push_back(x_[0] + i);
    hr_baseline_.push_back(offset + i* slope);
  }

  for (int i=0; i < hype.size(); ++i) {
    Peak one;
    one.subpeak = true;
    one.intersects_L = (i != 0);
    one.intersects_R = ((i+1) < hype.size());
    one.x_ = x_;
    one.y_ = y_;
    one.y_baseline_ = y_background_;
    one.hr_baseline_ = hr_baseline_;
    one.hr_x_ = hr_x_;
    one.gaussian_ = gauss[i];
    one.hypermet_ = hype[i];
    one.live_seconds_ = live_seconds_;
    one.construct(cal_nrg_, cal_fwhm_);

    peaks_.insert(one);

    for (int32_t j = 0; j < static_cast<int32_t>(x_.size()); ++j) {
      y_fullfit_g_[j] +=  gauss[i].evaluate(x_[j]);
      y_fullfit_h_[j] +=  hype[i].evaluate(x_[j]);
      y_resid_h_[j] -= y_fullfit_h_[j];
    }
  }

}

void ROI::make_background() {
  y_background_.clear();

  if (x_.empty())
    return;

  double rise = y_[y_.size()-1] - y_[0];
  double run = x_[x_.size()-1] - x_[0];
  double slope = rise / run;
  double offset = y_[0];

  //by default, linear
  y_background_.resize(y_.size());
  for (int32_t i = 0; i < y_.size() ; ++i)
    y_background_[i] = offset + (i * slope);

  y_nobkg_.resize(x_.size());
  for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i)
    y_nobkg_[i] = y_[i] - y_background_[i];
}

}
