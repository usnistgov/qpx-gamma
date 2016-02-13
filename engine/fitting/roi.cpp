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

  fits_.clear();
  finder_.clear();
  hr_x.clear();
  hr_background.clear();
  hr_back_steps.clear();
  hr_fullfit.clear();

  if (x.size() != y.size())
    return;

  if ((min >= x.size()) || (max >= x.size()))
    return;

  std::vector<double> x_local, y_local;
  for (uint16_t i = min; i < max; ++i) {
    x_local.push_back(x[i]);
    y_local.push_back(y[i]);
  }

  finder_.setData(x_local, y_local);
  init_background();

  render();
}

void ROI::auto_fit(boost::atomic<bool>& interruptor) {


  peaks_.clear();
  finder_.y_resid_ = finder_.y_;
  finder_.find_peaks(3, 3.0);  //assumes default params!!!

  if (finder_.filtered.empty())
    return;

  if ((LB_.width() == 0) || (RB_.width() == 0))
    init_background();
  std::vector<double> y_nobkg = remove_background();

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
      Hypermet hyp(gaussian.height_, gaussian.center_, gaussian.hwhm_ / sqrt(log(2)));

      Peak fitted;
      fitted.hypermet_ = hyp;
      fitted.center = hyp.center_.val;
      peaks_[hyp.center_.val] = fitted;
    }
  }

  //    PL_DBG << "preliminary peaks " << peaks_.size();
  rebuild();
  save_current_fit();
  iterative_fit(interruptor);

}

void ROI::iterative_fit(boost::atomic<bool>& interruptor) {
  if (!cal_fwhm_.valid() || peaks_.empty())
    return;

  double prev_rsq = peaks_.begin()->second.hypermet_.rsq_;
  PL_DBG << "    initial rsq = " << prev_rsq;

  int MAX_ITER = 5;

  for (int i=0; i < MAX_ITER; ++i) {
    ROI new_fit = *this;

    if (!new_fit.add_from_resid(-1)) {
      PL_DBG << "    failed add from resid";
      break;
    }
    double new_rsq = new_fit.peaks_.begin()->second.hypermet_.rsq_;
    PL_DBG << "    new rsq = " << new_rsq;
    if (new_rsq <= prev_rsq) {
      PL_DBG << "    not improved. reject refit";
      break;
    }
    new_fit.save_current_fit();
    prev_rsq = new_rsq;
    *this = new_fit;

    if (interruptor.load()) {
      PL_DBG << "    fit ROI interrupter by client";
      break;
    }
  }

}

bool ROI::add_from_resid(int32_t centroid_hint) {
  if (finder_.filtered.empty())
    return false;

  int target_peak = 0;
  if (centroid_hint == -1) {
    double biggest = 0;
    for (int j=0; j < finder_.filtered.size(); ++j) {
      std::vector<double> x_pk = std::vector<double>(finder_.x_.begin() + finder_.lefts[j],
                                                     finder_.x_.begin() + finder_.rights[j] + 1);
      std::vector<double> y_pk = std::vector<double>(finder_.y_resid_.begin() + finder_.lefts[j],
                                                     finder_.y_resid_.begin() + finder_.rights[j] + 1);
      Gaussian gaussian(x_pk, y_pk);
      if (
          (gaussian.height_ > 0) &&
          (gaussian.hwhm_ > 0) &&
          (finder_.x_[finder_.lefts[j]] < gaussian.center_) &&
          (gaussian.center_ < finder_.x_[finder_.rights[j]]) &&
          (gaussian.area() > biggest)
        )
      {
        target_peak = j;
        biggest = gaussian.area();
      }
    }
//    PL_DBG << "    biggest potential add at " << finder_.x_[finder_.filtered[target_peak]] << " with area=" << biggest;
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
//    PL_DBG << "<ROI> new peak accepted";
    Hypermet hyp(gaussian.height_, gaussian.center_, gaussian.hwhm_ / sqrt(log(2)));

    Peak fitted;
    fitted.hypermet_ = hyp;
    fitted.center = hyp.center_.val;
    peaks_[hyp.center_.val] = fitted;

    rebuild();    
    return true;
  }
  else
    return false;
}


bool ROI::contains(double bin) {
  return (peaks_.count(bin) > 0);
}

bool ROI::overlaps(uint16_t bin) {
  if (finder_.x_.empty())
    return false;
  return ((bin >= finder_.x_.front()) && (bin <= finder_.x_.back()));
}

bool ROI::overlaps(uint16_t Lbin, uint16_t Rbin) {
  if (finder_.x_.empty())
    return false;
  if (overlaps(Lbin) || overlaps(Rbin))
    return true;
  if ((Lbin <= finder_.x_.front()) && (Rbin >= finder_.x_.back()))
    return true;
  return false;
}

void ROI::adjust_bounds(const std::vector<double> &x, const std::vector<double> &y,
                        uint16_t L, uint16_t R,
                        boost::atomic<bool>& interruptor)
{
  if (x.size() != y.size())
    return;

  if ((L >= x.size()) || (R >= x.size()))
    return;

//  PL_DBG << "Adjusting ROI bounds";

  std::vector<double> x_local, y_local;
  for (uint16_t i = L; i <= R; ++i) {
    x_local.push_back(x[i]);
    y_local.push_back(y[i]);
  }

  finder_.setData(x_local, y_local);
  init_background();

  std::map<double, Peak> peaks;
  for (auto &p : peaks_) {
    if ((p.first > x_local[0]) &&
        (p.first < x_local[x_local.size() - 1]))
    peaks[p.first] = p.second;
  }

//  PL_DBG << "ROI adjustment keeps " << peaks.size() << " of " << peaks_.size() << " existing peaks";
  peaks_ = peaks;
  render();

  rebuild();
  save_current_fit();
  iterative_fit(interruptor);
}



void ROI::add_peak(const std::vector<double> &x, const std::vector<double> &y,
                   uint16_t L, uint16_t R,
                   boost::atomic<bool>& interruptor)
{
  if (x.size() != y.size())
    return;

  if ((L >= x.size()) || (R >= x.size()))
    return;

  uint16_t center_prelim = (L+R) /2; //assume down the middle

  if (overlaps(L) && overlaps(R)) {
    ROI new_fit = *this;

    if (new_fit.add_from_resid(center_prelim)) {
      *this = new_fit;
      save_current_fit();
    }

  } else if (!finder_.x_.empty()) {
    uint32_t min = std::min(static_cast<double>(L), finder_.x_[0]);
    uint32_t max = std::max(static_cast<double>(R), finder_.x_[finder_.x_.size() - 1]);
    PL_DBG << "<ROI> adding on exterior " << min << " " << max << " at around " << center_prelim;
    std::vector<double> x_local, y_local;
    for (uint32_t i = min; i <= max; ++i) {
      x_local.push_back(x[i]);
      y_local.push_back(y[i]);
    }
    finder_.setData(x_local, y_local); //assumes default threshold parameters!!!!

    render();

    init_background();
    finder_.y_resid_ = remove_background();
    finder_.find_peaks(3, 3.0);  //assumes default params!!!

    ROI new_fit = *this;
    if (new_fit.add_from_resid(center_prelim)) {
      *this = new_fit;
      save_current_fit();
    } else
      auto_fit(interruptor);

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

bool ROI::remove_peaks(const std::set<double> &pks) {
  bool found = false;
  for (auto &q : pks)
    if (remove_peak(q))
      found = true;

  if (found) {
    rebuild();
    save_current_fit();
  }

  return found;
}

bool ROI::remove_peak(double bin) {
  if (peaks_.count(bin)) {
    peaks_.erase(bin);
    return true;
  }
  return false;
}

void ROI::save_current_fit() {
  Fit current_fit;
  current_fit.background_ = background_;
  current_fit.finder_ = finder_;
  current_fit.peaks_ = peaks_;
  fits_.push_back(current_fit);
}

void ROI::rebuild() {
  hr_x.clear();
  hr_x_nrg.clear();
  hr_background.clear();
  hr_back_steps.clear();
  hr_fullfit.clear();


  if (peaks_.empty())
    return;

  std::vector<Hypermet> old_hype;
  for (auto &q : peaks_)
    old_hype.push_back(q.second.hypermet_);

  std::vector<Hypermet> hype = Hypermet::fit_multi(finder_.x_, finder_.y_,
                                                   old_hype, background_,
                                                   cal_nrg_, cal_fwhm_);

  peaks_.clear();

  if (hype.empty())
    return;

  for (int i=0; i < hype.size(); ++i) {
    Peak one;
    one.hypermet_ = hype[i];
    one.center = hype[i].center_.val;
    peaks_[one.center] = one;
  }

  render();
}

void ROI::render() {
  hr_x.clear();
  hr_background.clear();
  hr_back_steps.clear();
  hr_fullfit.clear();
  for (double i = 0; i < finder_.x_.size(); i += 0.25)
    hr_x.push_back(finder_.x_[0] + i);
  hr_background = background_.eval(hr_x);
  hr_back_steps = hr_background;
  hr_fullfit    = hr_background;
  hr_x_nrg = cal_nrg_.transform(hr_x, bits_);

  std::vector<double> lowres_backsteps = background_.eval(finder_.x_);
  std::vector<double> lowres_fullfit = background_.eval(finder_.x_);

  for (auto &p : peaks_) {
    for (int32_t j = 0; j < static_cast<int32_t>(hr_x.size()); ++j) {
      double step = p.second.hypermet_.eval_step_tail(hr_x[j]);
      hr_back_steps[j] += step;
      hr_fullfit[j]    += step + p.second.hypermet_.eval_peak(hr_x[j]);
    }

    for (int32_t j = 0; j < static_cast<int32_t>(finder_.x_.size()); ++j) {
      double step = p.second.hypermet_.eval_step_tail(finder_.x_[j]);
      lowres_backsteps[j] += step;
      lowres_fullfit[j]   += step + p.second.hypermet_.eval_peak(finder_.x_[j]);
    }
  }

  for (auto &p : peaks_) {
    p.second.hr_fullfit_ = hr_back_steps;
    p.second.hr_peak_.resize(hr_back_steps.size());
    for (int32_t j = 0; j < static_cast<int32_t>(hr_x.size()); ++j) {
      p.second.hr_peak_[j]     = p.second.hypermet_.eval_peak(hr_x[j]);
      p.second.hr_fullfit_[j] += p.second.hr_peak_[j];
    }
  }

  finder_.setFit(lowres_fullfit, lowres_backsteps);
}

std::vector<double> ROI::remove_background() {
  std::vector<double> y_background = background_.eval(finder_.x_);

  std::vector<double> y_nobkg(finder_.x_.size());
  for (int32_t i = 0; i < static_cast<int32_t>(finder_.y_.size()); ++i)
    y_nobkg[i] = finder_.y_[i] - y_background[i];

  return y_nobkg;
}

void ROI::set_LB(SUM4Edge lb) {
  if (lb.width() < 1)
    return;
  LB_ = lb;
  rebuild();
  save_current_fit();
}

void ROI::set_RB(SUM4Edge rb) {
  if (rb.width() < 1)
    return;
  RB_ = rb;
  rebuild();
  save_current_fit();
}


void ROI::init_background(uint16_t samples) {
  background_ = Polynomial();


  int32_t LBend = 0;
  int32_t RBstart = finder_.y_.size() - 1;

  if ((samples > 0) && (finder_.y_.size() > samples*3)) {
    LBend += samples;
    RBstart -= samples;
  }

  LB_ = SUM4Edge(finder_.y_, 0, LBend);
  RB_ = SUM4Edge(finder_.y_, RBstart, finder_.y_.size() - 1);

  //by default, linear
  double run = RB_.start() - LB_.end();
  double offset = LB_.end();

  if (RB_.average() < LB_.average()) {
    run = RB_.start() - LB_.start();
    offset = LB_.start();
  }

  if (RB_.average() > LB_.average()) {
    run = RB_.end() - LB_.end();
    offset = LB_.end();
  }

  double slope = (RB_.average() - LB_.average()) / (run) ; //mid?
  background_ = Polynomial({LB_.average(), slope}, finder_.x_[offset]);

}

bool ROI::rollback(int i) {
  if ((i < 0) || (i >= fits_.size()))
    return false;

  background_ = fits_[i].background_;
  finder_ = fits_[i].finder_;
  peaks_ = fits_[i].peaks_;

  render();

  return true;
}


}
