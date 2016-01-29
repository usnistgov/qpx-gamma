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

  for (uint16_t i = min; i < max; ++i) {
    x_.push_back(x[i]);
    y_.push_back(y[i]);
  }

  finder_.setData(x_, y_); //assumes default threshold parameters!!!!
  std::vector<double> y_nobkg = make_background();

//  PL_DBG << "ROI finder found " << finder_.filtered.size();

  std::list<Gaussian> prelim_peaks;

  if (peaks_.empty() && finder_.filtered.size()) {
    for (int i=0; i < finder_.filtered.size(); ++i) {
//      PL_DBG << "<ROI> finding peak " << finder_.x_[finder_.lefts[i]] << "-" << finder_.x_[finder_.rights[i]];

      std::vector<double> x_pk = std::vector<double>(finder_.x_.begin() + finder_.lefts[i], finder_.x_.begin() + finder_.rights[i] + 1);
      std::vector<double> y_pk = std::vector<double>(y_nobkg.begin() + finder_.lefts[i], y_nobkg.begin() + finder_.rights[i] + 1);

      Gaussian gaussian(x_pk, y_pk);
      prelim_peaks.push_back(gaussian); //not used


      if (
          (gaussian.height_ > 0) &&
          (gaussian.hwhm_ > 0) &&
          (finder_.x_[finder_.lefts[i]] < gaussian.center_) &&
          (gaussian.center_ < finder_.x_[finder_.rights[i]])
         )
      {
//        PL_DBG << "I like this peak at " << gaussian.center_ << " fw " << gaussian.hwhm_ * 2;
        Hypermet hyp;
        hyp.center_ = gaussian.center_;
        hyp.height_ = gaussian.height_;
        hyp.width_ = gaussian.hwhm_ / sqrt(log(2));

        Peak fitted;
        fitted.hypermet_ = hyp;
        fitted.construct(cal_nrg_, live_seconds_);

        peaks_.insert(fitted);
      }
    }

//    PL_DBG << "preliminary peaks " << peaks_.size();
    PL_DBG << "<ROI> Fitting new ROI " << min << "-" << max
           << " with " << peaks_.size() << " peaks";

    rebuild();

    if (cal_fwhm_.valid()) {
      int MAX_ITER = 3;

      for (int i=0; i < MAX_ITER; ++i) {
        PL_DBG << "Adding from residues iteration " << i;
        if (!add_from_resid(-1))
          break;
      }
    }


  }
}

bool ROI::add_from_resid(uint16_t centroid_hint) {
  if (finder_.filtered.empty())
    return false;

  int target_peak = 0;
  if (centroid_hint == -1) {
    for (int j=0; j < finder_.filtered.size(); ++j)
      if (y_resid_[finder_.filtered[j]] > y_resid_[target_peak])
        target_peak = j;
  } else {
    double diff = abs(x_[finder_.filtered[target_peak]] - centroid_hint);
    for (int j=0; j < finder_.filtered.size(); ++j)
      if (abs(x_[finder_.filtered[j]] - centroid_hint) < diff) {
        target_peak = j;
        diff = abs(x_[finder_.filtered[j]] - centroid_hint);
      }
  }

  std::vector<double> x_pk = std::vector<double>(finder_.x_.begin() + finder_.lefts[target_peak],
                                                 finder_.x_.begin() + finder_.rights[target_peak] + 1);
  std::vector<double> y_pk = std::vector<double>(finder_.y_.begin() + finder_.lefts[target_peak],
                                                 finder_.y_.begin() + finder_.rights[target_peak] + 1);
  Gaussian gaussian(x_pk, y_pk);

  if (
      (gaussian.height_ > 0) &&
      (gaussian.hwhm_ > 0) &&
      (finder_.x_[finder_.lefts[target_peak]] < gaussian.center_) &&
      (gaussian.center_ < finder_.x_[finder_.rights[target_peak]])
      )
  {
    Hypermet hyp;
    hyp.center_ = gaussian.center_;
    hyp.height_ = gaussian.height_;
    hyp.width_ = gaussian.hwhm_ / sqrt(log(2));

    Peak fitted;
    fitted.hypermet_ = hyp;
    fitted.construct(cal_nrg_, live_seconds_);

    peaks_.insert(fitted);

    rebuild();
    return true;
  }
  else
    return false;
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

void ROI::add_peak(const std::vector<double> &x, const std::vector<double> &y,
                   uint16_t L, uint16_t R)
{
  if (x.size() != y.size())
    return;

  if ((L >= x.size()) || (R >= x.size()))
    return;

  Peak fitted;
  uint16_t center_prelim = (L+R) /2; //assume down the middle

  if (overlaps(L) && overlaps(R)) {

    add_from_resid(center_prelim);

  } else if (!x_.empty()) {
    uint32_t min = std::min(static_cast<double>(L), x_[0]);
    uint32_t max = std::max(static_cast<double>(R), x_[x_.size() - 1]);
    PL_DBG << "<ROI> adding on exterior " << min << " " << max;
    x_.clear(); y_.clear();
    for (uint32_t i = min; i <= max; ++i) {
      x_.push_back(x[i]);
      y_.push_back(y[i]);
    }
    finder_.setData(x_, y_); //assumes default threshold parameters!!!!

    std::vector<double> y_nobkg = make_background();

    Gaussian gaussian(finder_.x_, y_nobkg);
    Hypermet hyp(finder_.x_, y_nobkg, gaussian.height_, gaussian.center_, gaussian.hwhm_ / sqrt(log(2)));

    fitted.hypermet_ = hyp;
    fitted.construct(cal_nrg_, live_seconds_);

    if (
        (fitted.height > 0) &&
  //          (fitted.fwhm_sum4 > 0) &&
        (fitted.fwhm_hyp > 0) &&
        (L < fitted.center) &&
        (fitted.center < R)
       )
    {
  //    PL_DBG << "I like this peak at " << fitted.center << " fw " << fitted.fwhm_hyp;
      peaks_.insert(fitted);
      rebuild();
    }

  } else {
    PL_DBG << "<ROI> cannot add to empty ROI";
  }

}

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
  for (auto &q : pks)
    if (remove_peak(q))
      found = true;

  if (found)
    rebuild();
}

bool ROI::remove_peak(double bin) {
  for (auto &q : peaks_) {
    if (q.center == bin) {
      peaks_.erase(q);
      return true;
    }
  }
  return false;
}

void ROI::rebuild() {
  y_resid_.clear();

  hr_x.clear();
  hr_background.clear();
  hr_back_steps.clear();
  hr_fullfit.clear();

  if (peaks_.empty())
    return;

////  PL_DBG << "Using tallest peak at " << tallest_g.center_ << " as starting point for Hypermet constraints";

//  Hypermet tallest_h(x_, y_nobkg_, tallest_g.height_, tallest_g.center_, tallest_g.hwhm_);

  Hypermet tallest_h;
  for (auto &q : peaks_) {
    if (q.hypermet_.height_ > tallest_h.height_)
      tallest_h = q.hypermet_;
  }

  std::vector<Hypermet> old_hype;
  old_hype.push_back(tallest_h);

  for (auto &q : peaks_)
    if (q.hypermet_.center_ != tallest_h.center_)
      old_hype.push_back(q.hypermet_);

  std::vector<Hypermet> hype = Hypermet::fit_multi(x_, y_, old_hype, true);

  peaks_.clear();

  if (hype.empty())
    return;

  hr_x.clear();
  for (double i = 0; i <= x_.size(); i += 0.25)
    hr_x.push_back(x_[0] + i);

  hr_background = hype.front().poly(hr_x);
  hr_back_steps = hr_background;
  hr_fullfit    = hr_background;

  std::vector<double> lowres_fullfit = hype.front().poly(x_);

  for (int i=0; i < hype.size(); ++i) {
    for (int32_t j = 0; j < static_cast<int32_t>(hr_x.size()); ++j) {
      hr_back_steps[j] += hype[i].eval_step_tail(hr_x[j]);
      hr_fullfit[j]    += hype[i].eval_step_tail(hr_x[j]) + hype[i].eval_peak(hr_x[j]);
    }

    for (int32_t j = 0; j < static_cast<int32_t>(x_.size()); ++j)
      lowres_fullfit[j] += hype[i].eval_step_tail(x_[j]) + hype[i].eval_peak(x_[j]);
  }


  for (int i=0; i < hype.size(); ++i) {
    Peak one;
    one.hr_baseline_h_ = hr_back_steps;
    one.hr_x_ = hr_x;
    one.hypermet_ = hype[i];
    one.construct(cal_nrg_, live_seconds_);
    peaks_.insert(one);
  }

  y_resid_ = y_;
  for (int i=0; i < y_.size(); ++ i)
    y_resid_[i] -= lowres_fullfit[i];

  finder_.setData(x_, y_resid_);
}

std::vector<double> ROI::make_background(uint16_t samples) {
  std::vector<double> y_background_h;


  if (x_.size() < samples) {
    y_background_h.resize(y_.size(), 0);
    return y_background_h;
  }

//  double min_y = y_[0], min_x = x_[0];
//  double max_y = y_[0], max_x = x_[0];
//  for (int i=0; i < y_.size(); ++i) {
//    if (y_[i] < min_y) {
//      min_y = y_[i];
//      min_x = x_[i];
//    }
//    if (y_[i] > max_y) {
//      max_y = y_[i];
//      max_x = x_[i];
//    }
//  }

  double min_L = y_[0];
  for (int i=0; i < samples; ++i)
    min_L = std::min(min_L, y_[i]);

  double min_R = y_[y_.size()-1];
  for (int i=y_.size()-1; i >= y_.size()-samples; --i)
    min_R = std::min(min_R, y_[i]);


  double rise = min_R - min_L;
  double run = x_[x_.size()-1] - x_[0];
  double slope = rise / run;
  double offset = min_L;

  //by default, linear
  y_background_h.resize(y_.size());
  for (int32_t i = 0; i < y_.size() ; ++i)
    y_background_h[i] = offset + (i * slope);

  std::vector<double> y_nobkg(x_.size());
  for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i)
    y_nobkg[i] = y_[i] - y_background_h[i];

  return y_nobkg;
}

}
