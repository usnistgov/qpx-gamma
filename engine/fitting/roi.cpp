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
 *      Qpx::ROI
 *
 ******************************************************************************/

#include "roi.h"
#include "gaussian.h"
#include "custom_logger.h"
#include "custom_timer.h"

namespace Qpx {

void ROI::set_data(const std::vector<double> &x, const std::vector<double> &y,
                   double l, double r) {

  fits_.clear();
  finder_.clear();
  hr_x.clear();
  hr_background.clear();
  hr_back_steps.clear();
  hr_fullfit.clear();

  if (x.size() != y.size())
    return;

  Finder temp;
  temp.setData(x, y);
  int32_t min = temp.find_index(l);
  int32_t max = temp.find_index(r);

  if (min >= x.size())
      min = x.size() - 1;
  if (max >= x.size())
      max = x.size() - 1;

  if (min < 0)
      min = 0;
  if (max < 0)
      max = 0;

  std::vector<double> x_local, y_local;
  for (int32_t i = min; i < max; ++i) {
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
  finder_.find_peaks();  //assumes default params!!!

  if (finder_.filtered.empty())
    return;

  if ((LB_.width() == 0) || (RB_.width() == 0))
    init_background();

  if (!settings_.sum4_only) {

    std::vector<double> y_nobkg = remove_background();

    //  DBG << "ROI finder found " << finder_.filtered.size();


    for (int i=0; i < finder_.filtered.size(); ++i) {
      //      DBG << "<ROI> finding peak " << finder_.x_[finder_.lefts[i]] << "-" << finder_.x_[finder_.rights[i]];

      std::vector<double> x_pk = std::vector<double>(finder_.x_.begin() + finder_.lefts[i], finder_.x_.begin() + finder_.rights[i] + 1);
      std::vector<double> y_pk = std::vector<double>(y_nobkg.begin() + finder_.lefts[i], y_nobkg.begin() + finder_.rights[i] + 1);

      Gaussian gaussian(x_pk, y_pk);

      if (
          gaussian.height_.value.finite() && (gaussian.height_.value.value() > 0) &&
          gaussian.hwhm_.value.finite() && (gaussian.hwhm_.value.value() > 0) &&
          (finder_.x_[finder_.lefts[i]] < gaussian.center_.value.value()) &&
          (gaussian.center_.value.value() < finder_.x_[finder_.rights[i]])
          )
      {
        //        DBG << "I like this peak at " << gaussian.center_ << " fw " << gaussian.hwhm_ * 2;
        Peak fitted(Hypermet(gaussian, settings_), SUM4(), settings_);
//        fitted.center = fitted.hypermet_.center_.value;

        peaks_[fitted.center().value()] = fitted;
      }
    }
    if (peaks_.empty())
      settings_.sum4_only = true;
  }

  //    DBG << "preliminary peaks " << peaks_.size();
  rebuild();

  if (settings_.resid_auto)
    iterative_fit(interruptor);

//  DBG << "ROI fit complete with: ";
//  for (auto &p : peaks_) {
//    DBG << "  " << p.second.hypermet_.to_string();
//  }
}

void ROI::iterative_fit(boost::atomic<bool>& interruptor) {
  if (!settings_.cali_fwhm_.valid() || peaks_.empty())
    return;

  double prev_rsq = peaks_.begin()->second.hypermet().rsq_;
  DBG << "  initial rsq = " << prev_rsq;

  for (int i=0; i < settings_.resid_max_iterations; ++i) {
    ROI new_fit = *this;

    if (!new_fit.add_from_resid(-1)) {
//      DBG << "    failed add from resid";
      break;
    }
    double new_rsq = new_fit.peaks_.begin()->second.hypermet().rsq_;
    if ((new_rsq <= prev_rsq) || std::isnan(new_rsq)) {
      DBG << "    not improved. reject refit";
      break;
    } else
      DBG << "    new rsq = " << new_rsq;

    new_fit.save_current_fit();
    prev_rsq = new_rsq;
    *this = new_fit;

    if (interruptor.load()) {
      DBG << "    fit ROI interrupter by client";
      break;
    }
  }

}

bool ROI::add_from_resid(int32_t centroid_hint) {

  if (finder_.filtered.empty())
    return false;

  int target_peak = -1;
  if (centroid_hint == -1) {
    double biggest = 0;
    for (int j=0; j < finder_.filtered.size(); ++j) {
      std::vector<double> x_pk = std::vector<double>(finder_.x_.begin() + finder_.lefts[j],
                                                     finder_.x_.begin() + finder_.rights[j] + 1);
      std::vector<double> y_pk = std::vector<double>(finder_.y_resid_.begin() + finder_.lefts[j],
                                                     finder_.y_resid_.begin() + finder_.rights[j] + 1);
      Gaussian gaussian(x_pk, y_pk);
      bool too_close = false;

      double lateral_slack = settings_.resid_too_close * gaussian.hwhm_.value.value() * 2;
      for (auto &p : peaks_) {
        if ((p.second.center().value() > (gaussian.center_.value.value() - lateral_slack))
            && (p.second.center().value() < (gaussian.center_.value.value() + lateral_slack)))
          too_close = true;
      }

//      if (too_close)
//        DBG << "Too close at " << settings_.cali_nrg_.transform(gaussian.center_, settings_.bits_);

      if ( !too_close &&
          gaussian.height_.value.finite() && (gaussian.height_.value.value() > 0) &&
          gaussian.hwhm_.value.finite() && (gaussian.hwhm_.value.value() > 0) &&
          (finder_.x_[finder_.lefts[j]] < gaussian.center_.value.value()) &&
          (gaussian.center_.value.value() < finder_.x_[finder_.rights[j]]) &&
          (gaussian.height_.value.value() > settings_.resid_min_amplitude) &&
          (gaussian.area().value() > biggest)
        )
      {
        target_peak = j;
        biggest = gaussian.area().value();
      }
    }
//    DBG << "    biggest potential add at " << finder_.x_[finder_.filtered[target_peak]] << " with area=" << biggest;
  } else {

    //THIS NEVER HAPPENS
    double diff = abs(finder_.x_[finder_.filtered[target_peak]] - centroid_hint);
    for (int j=0; j < finder_.filtered.size(); ++j)
      if (abs(finder_.x_[finder_.filtered[j]] - centroid_hint) < diff) {
        target_peak = j;
        diff = abs(finder_.x_[finder_.filtered[j]] - centroid_hint);
      }
  }

  if (target_peak == -1)
    return false;

  std::vector<double> x_pk = std::vector<double>(finder_.x_.begin() + finder_.lefts[target_peak],
                                                 finder_.x_.begin() + finder_.rights[target_peak] + 1);
  std::vector<double> y_pk = std::vector<double>(finder_.y_resid_.begin() + finder_.lefts[target_peak],
                                                 finder_.y_resid_.begin() + finder_.rights[target_peak] + 1);
  Gaussian gaussian(x_pk, y_pk);

  if (
      gaussian.height_.value.finite() && (gaussian.height_.value.value() > 0) &&
      gaussian.hwhm_.value.finite() && (gaussian.hwhm_.value.value() > 0) &&
      (finder_.x_[finder_.lefts[target_peak]] < gaussian.center_.value.value()) &&
      (gaussian.center_.value.value() < finder_.x_[finder_.rights[target_peak]])
      )
  {
//    DBG << "<ROI> new peak accepted";
    Hypermet hyp(gaussian, settings_);

    Peak fitted(Hypermet(gaussian, settings_), SUM4(), settings_);
//    fitted.center = hyp.center_.value;
    peaks_[fitted.center().value()] = fitted;

    rebuild();    
    return true;
  }
  else
    return false;
}


bool ROI::contains(double bin) {
  return (peaks_.count(bin) > 0);
}

bool ROI::overlaps(double bin) {
  if (finder_.x_.empty())
    return false;
  return ((bin >= finder_.x_.front()) && (bin <= finder_.x_.back()));
}

bool ROI::overlaps(double Lbin, double Rbin) {
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

//  DBG << "Adjusting ROI bounds";

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

//  DBG << "ROI adjustment keeps " << peaks.size() << " of " << peaks_.size() << " existing peaks";
  peaks_ = peaks;
  render();

  rebuild();
//  if (settings_.resid_auto)
//    iterative_fit(interruptor);
}



void ROI::add_peak(const std::vector<double> &x, const std::vector<double> &y,
                   double L, double R,
                   boost::atomic<bool>& interruptor)
{
  if (x.size() != y.size())
    return;

  uint16_t center_prelim = (L+R) /2; //assume down the middle

  if (overlaps(L) && overlaps(R)) {
    ROI new_fit = *this;

    if (!settings_.sum4_only && new_fit.add_from_resid(center_prelim)) {
      *this = new_fit;
      save_current_fit();
    } else {
      Peak fitted(Hypermet(),
                  SUM4(finder_.x_, finder_.y_, finder_.find_index(L), finder_.find_index(R), sum4_background_, LB_, RB_),
                  settings_);
//      fitted.center = fitted.sum4_.centroid();
//      fitted.construct(settings_);
      peaks_[fitted.center().value()] = fitted;
      render();
      save_current_fit();
    }

  } else if (!finder_.x_.empty()) {
    if (!finder_.x_.empty()) {
      L = std::min(L, finder_.x_[0]);
      R = std::max(R, finder_.x_[finder_.x_.size() - 1]);
    }
    uint16_t min = finder_.find_index(L);
    uint16_t max = finder_.find_index(R);
//    DBG << "<ROI> aaaadding on exterior " << min << " " << max << " at around " << center_prelim;
    std::vector<double> x_local, y_local;
    for (uint32_t i = min; i <= max; ++i) {
      x_local.push_back(x[i]);
      y_local.push_back(y[i]);
    }
    finder_.setData(x_local, y_local); //assumes default threshold parameters!!!!


    init_background();
    finder_.y_resid_ = remove_background();
    render();
    finder_.find_peaks();  //assumes default params!!!

    ROI new_fit = *this;
    if (settings_.sum4_only)
    {
      Peak fitted(Hypermet(),
                  SUM4(finder_.x_, finder_.y_, finder_.find_index(L), finder_.find_index(R), sum4_background_, LB_, RB_),
                  settings_);
//      fitted.center = fitted.sum4_.centroid();
//      fitted.construct(settings_);
      peaks_[fitted.center().value()] = fitted;
      render();
      save_current_fit();
    }
    else if (new_fit.add_from_resid(finder_.find_index(center_prelim))) {
      *this = new_fit;
      save_current_fit();
    } else
      auto_fit(interruptor);

  } else {
    DBG << "<ROI> cannot add to empty ROI";
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
  Fit current_fit(finder_);
  current_fit.background_ = background_;
  current_fit.sum4_background_ = sum4_background_;
  current_fit.LB_ = LB_;
  current_fit.RB_ = RB_;
  current_fit.peaks_ = peaks_;
  fits_.push_back(current_fit);
}

void ROI::rebuild() {
  hr_x.clear();
  hr_x_nrg.clear();
  hr_background.clear();
  hr_back_steps.clear();
  hr_fullfit.clear();

  bool hypermet_fit = false;
  for (auto &q : peaks_)
    if (!q.second.hypermet().gaussian_only()) {
      hypermet_fit = true;
      break;
    }

  if (hypermet_fit)
    rebuild_as_hypermet();
  else
    rebuild_as_gaussian();

  render();
  save_current_fit();
}

void ROI::rebuild_as_hypermet()
{
  CustomTimer timer(true);

  std::map<double, Peak> new_peaks;

  std::vector<Hypermet> old_hype;
  for (auto &q : peaks_) {
    if (q.second.hypermet().height_.value.value())
      old_hype.push_back(q.second.hypermet());
    else if (q.second.sum4().peak_width()) {
      Peak s4only(Hypermet(),
                  SUM4(finder_.x_, finder_.y_,
                       finder_.find_index(q.second.sum4().left()),
                       finder_.find_index(q.second.sum4().right()),
                       sum4_background_, LB_, RB_),
                  settings_);
//      q.second.construct(settings_);
      new_peaks[s4only.center().value()] = s4only;
    }
  }

  if (old_hype.size() > 0) {
    std::vector<Hypermet> hype = Hypermet::fit_multi(finder_.x_, finder_.y_,
                                                     old_hype, background_,
                                                     settings_);

    for (int i=0; i < hype.size(); ++i) {
      double edge =  hype[i].width_.value.value() * sqrt(log(2)) * 3; //use const from settings
      uint32_t edgeL = finder_.find_index(hype[i].center_.value.value() - edge);
      uint32_t edgeR = finder_.find_index(hype[i].center_.value.value() + edge);
      Peak one(hype[i],
               SUM4(finder_.x_, finder_.y_, edgeL, edgeR, sum4_background_, LB_, RB_),
               settings_);
//      one.construct(settings_);
      new_peaks[one.center().value()] = one;
    }
  }

  peaks_ = new_peaks;
//  DBG << "Hypermet rebuild took " << timer.s() / double(peaks_.size()) << " s/peak";
}

void ROI::rebuild_as_gaussian()
{
  CustomTimer timer(true);

  std::map<double, Peak> new_peaks = peaks_;

  std::vector<Gaussian> old_gauss;
  for (auto &q : peaks_) {
    if (q.second.hypermet().height_.value.value())
      old_gauss.push_back(q.second.hypermet().gaussian());
    else if (q.second.sum4().peak_width()) {
      Peak s4only(Hypermet(), SUM4(finder_.x_, finder_.y_,
                finder_.find_index(q.second.sum4().left()),
                finder_.find_index(q.second.sum4().right()),
                sum4_background_, LB_, RB_),
                  settings_);
//      q.second.construct(settings_);
      new_peaks[s4only.center().value()] = s4only;
    }
  }

  if (old_gauss.size() > 0) {
    std::vector<Gaussian> gauss = Gaussian::fit_multi(finder_.x_, finder_.y_,
                                                     old_gauss, background_,
                                                     settings_);

    for (size_t i=0; i < gauss.size(); ++i) {
      double edge =  gauss[i].hwhm_.value.value() * 3; //use const from settings
      uint32_t edgeL = finder_.find_index(gauss[i].center_.value.value() - edge);
      uint32_t edgeR = finder_.find_index(gauss[i].center_.value.value() + edge);
      Peak one(Hypermet(gauss[i], settings_),
               SUM4(finder_.x_, finder_.y_, edgeL, edgeR, sum4_background_, LB_, RB_),
               settings_);
      new_peaks[one.center().value()] = one;
    }
  }

  peaks_ = new_peaks;
//  DBG << "Hypermet rebuild took " << timer.s() / double(peaks_.size()) << " s/peak";
}


void ROI::render() {
  hr_x.clear();
  hr_background.clear();
  hr_back_steps.clear();
  hr_fullfit.clear();
  for (double i = 0; i < finder_.x_.size(); i += 0.25)
  {
    hr_x.push_back(finder_.x_[0] + i);
    hr_fullfit.push_back(finder_.y_[std::floor(i)]);
  }
  hr_background = background_.eval_array(hr_x);
  hr_sum4_background_ = sum4_background_.eval_array(hr_x);
  hr_x_nrg = settings_.cali_nrg_.transform(hr_x, settings_.bits_);

  std::vector<double> lowres_backsteps = sum4_background_.eval_array(finder_.x_);
  std::vector<double> lowres_fullfit = sum4_background_.eval_array(finder_.x_);
  for (auto &p : peaks_)
    p.second.hr_fullfit_ = p.second.hr_peak_ = hr_fullfit;

  if (!settings_.sum4_only) {
    hr_fullfit    = hr_background;
    hr_back_steps = hr_background;
    lowres_backsteps = background_.eval_array(finder_.x_);
    lowres_fullfit = background_.eval_array(finder_.x_);
    for (auto &p : peaks_) {
      for (int32_t j = 0; j < static_cast<int32_t>(hr_x.size()); ++j) {
        double step = p.second.hypermet().eval_step_tail(hr_x[j]);
        hr_back_steps[j] += step;
        hr_fullfit[j]    += step + p.second.hypermet().eval_peak(hr_x[j]);
      }

      for (int32_t j = 0; j < static_cast<int32_t>(finder_.x_.size()); ++j) {
        double step = p.second.hypermet().eval_step_tail(finder_.x_[j]);
        lowres_backsteps[j] += step;
        lowres_fullfit[j]   += step + p.second.hypermet().eval_peak(finder_.x_[j]);
      }
    }

    for (auto &p : peaks_) {
      p.second.hr_fullfit_ = hr_back_steps;
      p.second.hr_peak_.resize(hr_back_steps.size());
      for (int32_t j = 0; j < static_cast<int32_t>(hr_x.size()); ++j) {
        p.second.hr_peak_[j]     = p.second.hypermet().eval_peak(hr_x[j]);
        p.second.hr_fullfit_[j] += p.second.hr_peak_[j];
      }
    }
  }

  finder_.setFit(lowres_fullfit, lowres_backsteps);
}

std::vector<double> ROI::remove_background() {
  std::vector<double> y_background = background_.eval_array(finder_.x_);

  std::vector<double> y_nobkg(finder_.x_.size());
  for (int32_t i = 0; i < static_cast<int32_t>(finder_.y_.size()); ++i)
    y_nobkg[i] = finder_.y_[i] - y_background[i];

  return y_nobkg;
}

void ROI::set_LB(SUM4Edge lb) {
  if (lb.width() < 1)
    return;
  LB_ = lb;
  make_sum4_background();
  rebuild();
}

void ROI::set_RB(SUM4Edge rb) {
  if (rb.width() < 1)
    return;
  RB_ = rb;
  make_sum4_background();
  rebuild();
}


void ROI::init_background() {
  if (finder_.x_.empty())
    return;

  background_ = PolyBounded();
  sum4_background_ = PolyBounded();

  int32_t LBend = 0;
  int32_t RBstart = finder_.y_.size() - 1;

  uint16_t samples = settings_.background_edge_samples;

  if ((samples > 0) && (finder_.y_.size() > samples*3)) {
    LBend += samples;
    RBstart -= samples;
  }

  LB_ = SUM4Edge(finder_.x_, finder_.y_, 0, LBend);
  RB_ = SUM4Edge(finder_.x_, finder_.y_, RBstart, finder_.y_.size() - 1);
  make_sum4_background();

  //by default, linear
  double run = RB_.left() - LB_.right();

  background_.xoffset_.value.setValue(LB_.left());
  background_.add_coeff(0, LB_.min(), LB_.max(), LB_.average());

  double minslope = 0, maxslope = 0;
  if (LB_.average() < RB_.average()) {
    run = RB_.right() - LB_.right();
    background_.xoffset_.value.setValue(LB_.right());
    minslope = (RB_.min() - LB_.max()) / (RB_.right() - LB_.left());
    maxslope = (RB_.max() - LB_.min()) / (RB_.left() - LB_.right());
  }


  if (RB_.average() < LB_.average()) {
    run = RB_.left() - LB_.left();
    background_.xoffset_.value.setValue(LB_.left());
    minslope = (RB_.min() - LB_.max()) / (RB_.left() - LB_.right());
    maxslope = (RB_.max() - LB_.min()) / (RB_.right() - LB_.left());
  }

  double slope = (RB_.average() - LB_.average()) / (run) ;
  background_.add_coeff(1, minslope, maxslope, slope);
}

void ROI::make_sum4_background() {
  if (finder_.x_.empty())
    return;
  double run = RB_.left() - LB_.right();
  sum4_background_.xoffset_.value.setValue(LB_.right());
  double s4base = LB_.average();
  double s4slope = (RB_.average() - LB_.average()) / run;
  sum4_background_.add_coeff(0, s4base, s4base, s4base);
  sum4_background_.add_coeff(1, s4slope, s4slope, s4slope);
}


bool ROI::rollback(int i) {
  if ((i < 0) || (i >= fits_.size()))
    return false;

  background_ = fits_[i].background_;
  sum4_background_ = fits_[i].sum4_background_;
  LB_ = fits_[i].LB_;
  RB_ = fits_[i].RB_;
  finder_ = fits_[i].finder_;
  peaks_ = fits_[i].peaks_;

  render();

  return true;
}

void ROI::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  settings_.to_xml(node);

  pugi::xml_node Ledge = node.append_child("BackgroundLeft");
  Ledge.append_attribute("left").set_value(LB_.left());
  Ledge.append_attribute("right").set_value(LB_.right());

  pugi::xml_node Redge = node.append_child("BackgroundRight");
  Redge.append_attribute("left").set_value(RB_.left());
  Redge.append_attribute("right").set_value(RB_.right());

  background_.to_xml(node);
  node.last_child().set_name("BackgroundPoly");

  if (peaks_.size()) {
    pugi::xml_node pks = node.append_child("Peaks");
    for (auto &p : peaks_) {
      pugi::xml_node pk = pks.append_child("Peaks");
      if (p.second.sum4().peak_width()) {
        pugi::xml_node s4 = pk.append_child("SUM4");
        s4.append_attribute("left").set_value(p.second.sum4().left());
        s4.append_attribute("right").set_value(p.second.sum4().right());
      }
      if (p.second.hypermet().height_.value.value() > 0)
        p.second.hypermet().to_xml(pk);
    }
  }
}


void ROI::from_xml(const pugi::xml_node &node, const Finder &finder, const FitSettings &parentsettings)
{
  if (finder.x_.empty() || (finder.x_.size() != finder.y_.size()))
    return;

  settings_ = parentsettings;
  if (node.child(settings_.xml_element_name().c_str()))
    settings_.from_xml(node.child(settings_.xml_element_name().c_str()));

  if (node.child("BackgroundLeft")) {
    double L = node.child("BackgroundLeft").attribute("left").as_double();
    double R = node.child("BackgroundLeft").attribute("right").as_double();
    LB_ = SUM4Edge(finder.x_, finder.y_, finder.find_index(L), finder.find_index(R));
  }

  if (node.child("BackgroundRight")) {
    double L = node.child("BackgroundRight").attribute("left").as_double();
    double R = node.child("BackgroundRight").attribute("right").as_double();
    RB_ = SUM4Edge(finder.x_, finder.y_, finder.find_index(L), finder.find_index(R));
  }

  if (node.child("BackgroundPoly"))
    background_.from_xml(node.child("BackgroundPoly"));

  //validate background and edges?

  int32_t min = finder.find_index(LB_.left());
  int32_t max = finder.find_index(RB_.right());

  if (min >= finder.x_.size())
      min = finder.x_.size() - 1;
  if (max >= finder.x_.size())
      max = finder.x_.size() - 1;

  if (min < 0)
      min = 0;
  if (max < 0)
      max = 0;

  std::vector<double> x_local, y_local;
  for (int32_t i = min; i < max; ++i) {
    x_local.push_back(finder.x_[i]);
    y_local.push_back(finder.y_[i]);
  }

  finder_.setData(x_local, y_local);

  make_sum4_background();

  if (node.child("Peaks")) {
    for (auto &pk : node.child("Peaks").children())
    {
      Hypermet hyp;
      if (pk.child("Hypermet"))
        hyp.from_xml(pk.child("Hypermet"));
      SUM4 s4;
      if (pk.child("SUM4")) {
        double L = pk.child("SUM4").attribute("left").as_double();
        double R = pk.child("SUM4").attribute("right").as_double();
        s4 = SUM4(finder_.x_, finder_.y_,
                  finder_.find_index(L),
                  finder_.find_index(R),
                  sum4_background_, LB_, RB_);
        DBG << "Results in " << s4.left() << " - " << s4.right() << " w" << s4.peak_width();
      }
      Peak newpeak(hyp, s4, settings_);
      peaks_[newpeak.center().value()] = newpeak;
    }
  }

  render();
}


}
