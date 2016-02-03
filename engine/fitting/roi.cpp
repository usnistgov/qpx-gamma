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
#include "gaussian.h"

namespace Gamma {

void ROI::set_data(const std::vector<double> &x, const std::vector<double> &y,
                   uint16_t min, uint16_t max) {

  std::vector<double> x_local, y_local;
  finder_.clear();

  if (x.size() != y.size())
    return;

  if ((min >= x.size()) || (max >= x.size()))
    return;

  for (uint16_t i = min; i < max; ++i) {
    x_local.push_back(x[i]);
    y_local.push_back(y[i]);
  }

  finder_.setData(x_local, y_local);

  hr_x.clear();
  hr_background.clear();
  hr_back_steps.clear();
  hr_fullfit.clear();
  for (double i = 0; i < finder_.x_.size(); i += 0.25) {
    hr_x.push_back(finder_.x_[0] + i);
    hr_background.push_back(finder_.y_[i]);
    hr_back_steps.push_back(finder_.y_[i]);
    hr_fullfit.push_back(finder_.y_[i]);
  }

  hr_x_nrg = cal_nrg_.transform(hr_x, bits_);
}

void ROI::auto_fit() {
  peaks_.clear();
  finder_.y_resid_ = finder_.y_;
  finder_.find_peaks(3, 3.0);  //assumes default params!!!

  if (finder_.filtered.empty())
    return;

  std::vector<double> y_nobkg = make_background();

  //  PL_DBG << "ROI finder found " << finder_.filtered.size();


  for (int i=0; i < finder_.filtered.size(); ++i) {
    //      PL_DBG << "<ROI> finding peak " << finder_.x_[finder_.lefts[i]] << "-" << finder_.x_[finder_.rights[i]];

    std::vector<double> x_pk = std::vector<double>(finder_.x_.begin() + finder_.lefts[i], finder_.x_.begin() + finder_.rights[i] + 1);
    std::vector<double> y_pk = std::vector<double>(y_nobkg.begin() + finder_.lefts[i], y_nobkg.begin() + finder_.rights[i] + 1);

    Gaussian gaussian(x_pk, y_pk);

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
      fitted.construct(cal_nrg_, live_seconds_, bits_);

      peaks_.insert(fitted);
    }
  }

  //    PL_DBG << "preliminary peaks " << peaks_.size();

  if (cal_fwhm_.valid()) {
//    PL_DBG << "<ROI> Fitting new ROI " << min << "-" << max
//           << " with " << peaks_.size() << " peaks...";
    rebuild();
    int MAX_ITER = 3;

    for (int i=0; i < MAX_ITER; ++i) {
      PL_DBG << "Adding from residues iteration " << i;
      if (!add_from_resid(-1))
        break;
    }
  } else {
//    PL_DBG << "<ROI> Fitting new ROI " << min << "-" << max
//           << " with " << peaks_.size() << " peaks";

    rebuild();
  }

}

bool ROI::add_from_resid(uint16_t centroid_hint) {
  if (finder_.filtered.empty())
    return false;

  int target_peak = 0;
  if (centroid_hint == -1) {
    for (int j=0; j < finder_.filtered.size(); ++j)
      if (finder_.y_resid_[finder_.filtered[j]] > finder_.y_resid_[target_peak])
        target_peak = j;
  } else {
    double diff = abs(finder_.x_[finder_.filtered[target_peak]] - centroid_hint);
    for (int j=0; j < finder_.filtered.size(); ++j)
      if (abs(finder_.x_[finder_.filtered[j]] - centroid_hint) < diff) {
        target_peak = j;
        diff = abs(finder_.x_[finder_.filtered[j]] - centroid_hint);
      }
  }

  std::vector<double> x_pk = std::vector<double>(finder_.x_.begin() + finder_.lefts[target_peak],
                                                 finder_.x_.begin() + finder_.rights[target_peak] + 1);
  std::vector<double> y_pk = std::vector<double>(finder_.y_resid_.begin() + finder_.lefts[target_peak],
                                                 finder_.y_resid_.begin() + finder_.rights[target_peak] + 1);
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
    fitted.construct(cal_nrg_, live_seconds_, bits_);

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

bool ROI::overlaps(uint16_t bin) {
  if (finder_.x_.empty())
    return false;
  return ((bin >= finder_.x_.front()) && (bin <= finder_.x_.back()));
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

  } else if (!finder_.x_.empty()) {
    uint32_t min = std::min(static_cast<double>(L), finder_.x_[0]);
    uint32_t max = std::max(static_cast<double>(R), finder_.x_[finder_.x_.size() - 1]);
    PL_DBG << "<ROI> adding on exterior " << min << " " << max;
    std::vector<double> x_local, y_local;
    for (uint32_t i = min; i <= max; ++i) {
      x_local.push_back(x[i]);
      y_local.push_back(y[i]);
    }
    finder_.setData(x_local, y_local); //assumes default threshold parameters!!!!

    std::vector<double> y_nobkg = make_background();

    Gaussian gaussian(finder_.x_, y_nobkg);
    Hypermet hyp(finder_.x_, y_nobkg, gaussian.height_, gaussian.center_, gaussian.hwhm_ / sqrt(log(2)));

    fitted.hypermet_ = hyp;
    fitted.construct(cal_nrg_, live_seconds_, bits_);

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
  hr_background.clear();
  hr_back_steps.clear();
  hr_fullfit.clear();

  if (peaks_.empty())
    return;

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

  std::vector<Hypermet> hype = Hypermet::fit_multi(finder_.x_, finder_.y_, old_hype, true);

  peaks_.clear();

  if (hype.empty())
    return;

  hr_background = hype.front().poly(hr_x);
  hr_back_steps = hr_background;
  hr_fullfit    = hr_background;

  std::vector<double> lowres_fullfit = hype.front().poly(finder_.x_);

  for (int i=0; i < hype.size(); ++i) {
    for (int32_t j = 0; j < static_cast<int32_t>(hr_x.size()); ++j) {
      hr_back_steps[j] += hype[i].eval_step_tail(hr_x[j]);
      hr_fullfit[j]    += hype[i].eval_step_tail(hr_x[j]) + hype[i].eval_peak(hr_x[j]);
    }

    for (int32_t j = 0; j < static_cast<int32_t>(finder_.x_.size()); ++j)
      lowres_fullfit[j] += hype[i].eval_step_tail(finder_.x_[j]) + hype[i].eval_peak(finder_.x_[j]);
  }


  for (int i=0; i < hype.size(); ++i) {
    Peak one;
    one.hr_fullfit_ = hr_back_steps;
    one.hr_peak_.resize(hr_back_steps.size());
    for (int32_t j = 0; j < static_cast<int32_t>(hr_x.size()); ++j) {
      one.hr_peak_[j]     = hype[i].eval_peak(hr_x[j]);
      one.hr_fullfit_[j] += one.hr_peak_[j];
    }

    one.hypermet_ = hype[i];
    one.construct(cal_nrg_, live_seconds_, bits_);
    peaks_.insert(one);
  }

  finder_.setFit(lowres_fullfit);
}

std::vector<double> ROI::make_background(uint16_t samples) {
  std::vector<double> y_background_h;


  if (finder_.x_.size() < samples) {
    y_background_h.resize(finder_.y_.size(), 0);
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

  double min_L = finder_.y_[0];
  for (int i=0; i < samples; ++i)
    min_L = std::min(min_L, finder_.y_[i]);

  double min_R = finder_.y_[finder_.y_.size()-1];
  for (int i=finder_.y_.size()-1; i >= finder_.y_.size()-samples; --i)
    min_R = std::min(min_R, finder_.y_[i]);


  double rise = min_R - min_L;
  double run = finder_.x_[finder_.x_.size()-1] - finder_.x_[0];
  double slope = rise / run;
  double offset = min_L;

  //by default, linear
  y_background_h.resize(finder_.y_.size());
  for (int32_t i = 0; i < finder_.y_.size() ; ++i)
    y_background_h[i] = offset + (i * slope);

  std::vector<double> y_nobkg(finder_.x_.size());
  for (int32_t i = 0; i < static_cast<int32_t>(finder_.y_.size()); ++i)
    y_nobkg[i] = finder_.y_[i] - y_background_h[i];

  return y_nobkg;
}

}
