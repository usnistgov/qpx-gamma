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

bool ROI::contains(double bin) {
  for (auto &q : peaks_)
    if (q.center == bin)
      return true;
  return false;
}

bool ROI::overlaps(const Peak &pk) {
  if ((pk.center >= x_.front()) && (pk.center <= x_.back()))
    return true;
  if ((pk.x_.front() >= x_.front()) && (pk.x_.front() <= x_.back()))
    return true;
  if ((pk.x_.back() >= x_.front()) && (pk.x_.back() <= x_.back()))
    return true;
  return false;
}


void ROI::add_peak(const Peak &pk, const std::vector<double> &x, const std::vector<double> &y)
{
  std::set<Peak> pks;
  pks.insert(pk);
  add_peaks(pks, x, y);
}

void ROI::add_peaks(const std::set<Peak> &pks, const std::vector<double> &x, const std::vector<double> &y)
{
  for (auto &q : pks) {
    peaks_.erase(q);
    peaks_.insert(q);
  }

  int left = std::numeric_limits<int>::max();
  int right = - std::numeric_limits<int>::max();
  for (auto &q : peaks_) {
    int l = q.x_.front();
    int r = q.x_.back();
    
    if (l < left)
      left = l;
    if (r > right)
      right = r;    
  }
  y_background_ = make_background(x, y, left, right, 3);
  x_ = std::vector<double>(x.begin() + left, x.begin() + right + 1);
  y_ = std::vector<double>(y.begin() + left, y.begin() + right + 1);

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

  std::vector<double> nobck(x_.size());
  for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i)
    nobck[i] = y_[i] - y_background_[i];

  std::vector<Gaussian> old_gauss;
  for (auto &q : peaks_)
    old_gauss.push_back(q.gaussian_);
  std::vector<Gaussian> gauss = Gaussian::fit_multi(x_, nobck, old_gauss);

  if (gauss.size() < 2) {
    PL_DBG << "multiplet build failed";
    return;
  }

  Gaussian tallest_g;
  for (auto &q : gauss) {
    if (q.height_ > tallest_g.height_)
      tallest_g = q;
  }

  PL_DBG << "Using tallest peak at " << tallest_g.center_ << " as starting point for Hypermet constraints";

  Hypermet tallest_h(x_, nobck, tallest_g.height_, tallest_g.center_, tallest_g.hwhm_);

  std::vector<Hypermet> old_hype;
  old_hype.push_back(tallest_h);

  for (int i=0; i < gauss.size(); ++i) {
    if ((gauss[i].hwhm_ > 0) &&
        (gauss[i].height_ > 0) && (gauss[i].center_ != tallest_g.center_)) {
      Hypermet hyp(gauss[i].height_, gauss[i].center_, gauss[i].hwhm_);
      old_hype.push_back(hyp);
    }
  }

  std::vector<Hypermet> hype = Hypermet::fit_multi(x_, nobck, old_hype, true);

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


//void Multiplet::rebuild() {
//  y_fullfit_.clear();

//  std::vector<double> nobck(x_.size());
//  for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i)
//    nobck[i] = y_[i] - y_background_[i];

//  std::vector<Gaussian> old_gauss;
//  for (auto &q : peaks_)
//    old_gauss.push_back(q.gaussian_);
//  std::vector<Gaussian> gauss = Gaussian::fit_multi(x_, nobck, old_gauss);

//  peaks_.clear();

//  y_fullfit_ = y_background_;

//  for (int i=0; i < gauss.size(); ++i) {
//    Peak one;
//    one.subpeak = true;
//    one.intersects_L = (i != 0);
//    one.intersects_R = ((i+1) < gauss.size());
//    one.x_ = x_;
//    one.y_ = y_;
//    one.y_baseline_ = y_background_;
//    one.gaussian_ = gauss[i];
//    one.live_seconds_ = live_seconds_;
//    one.construct(cal_nrg_, cal_fwhm_);

//    peaks_.insert(one);

//    for (int32_t j = 0; j < static_cast<int32_t>(x_.size()); ++j)
//      y_fullfit_[j] +=  gauss[i].evaluate(x_[j]);
//  }

//}

double ROI::local_avg(const std::vector<double> &x,
                 const std::vector<double> &y,
                 uint16_t chan, uint16_t samples) {

  if ((chan >= x.size()) || (samples < 1) || (x.size() != y.size()))
    return std::numeric_limits<double>::quiet_NaN();

  if ((samples % 2) == 0)
    samples++;
  int32_t half  = (samples - 1) / 2;
  int32_t left  = static_cast<int32_t>(chan) - half;
  int32_t right = static_cast<int32_t>(chan) + half;

  if (left < 0)
    left = 0;
  if (right >= x.size())
    right = x.size() - 1;

  samples = right - left + 1;

  return std::accumulate(y.begin() + left, y.begin() + (right+1), 0) / static_cast<double>(samples);
}



std::vector<double> ROI::make_background(const std::vector<double> &x,
                                    const std::vector<double> &y,
                                    uint16_t left, uint16_t right,
                                    uint16_t samples) {
  int32_t  width = right - left;
  double left_avg = local_avg(x, y, left, samples);
  double right_avg = local_avg(x, y, right, samples);

  if ((width <= 0) || isnan(left_avg) || isnan(right_avg))
    return std::vector<double>();

  //by default, linear
  double slope = (right_avg - left_avg) / static_cast<double>(width);
  std::vector<double> baseline(width+1);
  for (int32_t i = 0; i <= width; ++i)
    baseline[i] = left_avg + (i * slope);
  return baseline;
}

}
