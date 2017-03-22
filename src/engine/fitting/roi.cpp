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

ROI::ROI(const Finder &parentfinder, double min, double max)
{
  finder_.settings_ = parentfinder.settings_;
  set_data(parentfinder, min, max);
}

double ROI::ID() const
{
  return left_bin();
}

double ROI::left_bin() const
{
  if (finder_.x_.empty())
    return -1;
  else
    return finder_.x_.front();
}

double ROI::right_bin() const
{
  if (finder_.x_.empty())
    return -1;
  else
    return finder_.x_.back();
}

double ROI::left_nrg() const
{
  if (hr_x_nrg.empty())
    return std::numeric_limits<double>::quiet_NaN();
  else
    return hr_x_nrg.front();
}

double ROI::right_nrg() const
{
  if (hr_x_nrg.empty())
    return std::numeric_limits<double>::quiet_NaN();
  else
    return hr_x_nrg.back();
}

double ROI::width() const
{
  if (finder_.x_.empty())
    return 0;
  else
    return right_bin() - left_bin() + 1;
}

void ROI::set_data(const Finder &parentfinder, double l, double r)
{
  if (!finder_.cloneRange(parentfinder, l, r))
  {
    finder_.clear();
    return;
  }

  hr_x.clear();
  hr_background.clear();
  hr_back_steps.clear();
  hr_fullfit.clear();
  init_edges();
  init_background();
  render();
}

bool ROI::refit(OptimizerPtr optimizer, boost::atomic<bool>& interruptor)
{
  if (peaks_.empty())
    return auto_fit(optimizer, interruptor);

  if (!rebuild(optimizer, interruptor))
    return false;

  save_current_fit("Refit");
  return true;
}


bool ROI::auto_fit(OptimizerPtr optimizer, boost::atomic<bool>& interruptor)
{
  peaks_.clear();
  finder_.y_resid_ = finder_.y_;
  finder_.find_peaks();  //assumes default params!!!

  if (finder_.filtered.empty())
    return false;

  if ((LB_.width() == 0) || (RB_.width() == 0))
  {
    init_edges();
    init_background();
  }

  if (!finder_.settings_.sum4_only)
  {
    std::vector<double> y_nobkg = remove_background();

    for (size_t i=0; i < finder_.filtered.size(); ++i)
    {
      std::vector<double> x_pk = std::vector<double>(finder_.x_.begin() + finder_.lefts[i], finder_.x_.begin() + finder_.rights[i] + 1);
      std::vector<double> y_pk = std::vector<double>(y_nobkg.begin() + finder_.lefts[i], y_nobkg.begin() + finder_.rights[i] + 1);

      Gaussian gaussian;
      optimizer->fit(gaussian, x_pk, y_pk);

      if (
          gaussian.height().value().finite() && (gaussian.height().value().value() > 0) &&
          gaussian.hwhm().value().finite() && (gaussian.hwhm().value().value() > 0) &&
          (finder_.x_[finder().lefts[i]] < gaussian.center().value().value()) &&
          (gaussian.center().value().value() < finder_.x_[finder_.rights[i]])
          )
      {
        Peak fitted(Hypermet(gaussian, finder_.settings_), SUM4(), finder_.settings_);
        peaks_[fitted.center().value()] = fitted;
      }
    }
    if (peaks_.empty())
      finder_.settings_.sum4_only = true;
  }

  if (!rebuild(optimizer, interruptor))
    return false;

  save_current_fit("Autofit");

  if (finder_.settings_.resid_auto)
    iterative_fit(optimizer, interruptor);

  return true;
}

void ROI::iterative_fit(OptimizerPtr optimizer, boost::atomic<bool>& interruptor)
{
  if (!finder_.settings_.cali_fwhm_.valid() || peaks_.empty())
    return;

  double prev_rsq = peaks_.begin()->second.hypermet().chi2();

  for (int i=0; i < finder_.settings_.resid_max_iterations; ++i)
  {
    ROI new_fit = *this;

    DBG << "Attempting add from resid with " << peaks_.size() << " peaks";

    if (!new_fit.add_from_resid(optimizer, interruptor, -1)) {
      //      DBG << "    failed add from resid";
      break;
    }
    double new_rsq = new_fit.peaks_.begin()->second.hypermet().chi2();
    double improvement = (prev_rsq - new_rsq) / prev_rsq * 100;
    DBG << "X2 new=" << new_rsq << " previous=" << prev_rsq << " improved by " << improvement;

    if ((new_rsq > prev_rsq) || std::isnan(new_rsq)) {
      DBG << " not improved. reject new refit";
      break;
    }

    new_fit.save_current_fit("Iterative " + std::to_string(new_fit.peaks().size()));
    prev_rsq = new_rsq;
    *this = new_fit;

    if (interruptor.load())
      break;
  }
}

bool ROI::add_from_resid(OptimizerPtr optimizer, boost::atomic<bool>& interruptor, int32_t centroid_hint)
{
  if (finder_.filtered.empty())
    return false;

  int target_peak = -1;
  if (centroid_hint == -1) {
    double biggest = 0;
    for (size_t j=0; j < finder_.filtered.size(); ++j)
    {
      std::vector<double> x_pk = std::vector<double>(finder_.x_.begin() + finder_.lefts[j],
                                                     finder_.x_.begin() + finder_.rights[j] + 1);
      std::vector<double> y_pk = std::vector<double>(finder_.y_resid_.begin() + finder_.lefts[j],
                                                     finder_.y_resid_.begin() + finder_.rights[j] + 1);
      Gaussian gaussian;
      optimizer->fit(gaussian, x_pk, y_pk);

      bool too_close = false;

      double lateral_slack = finder_.settings_.resid_too_close * gaussian.hwhm().value().value() * 2;
      for (auto &p : peaks_)
      {
        if ((p.second.center().value() > (gaussian.center().value().value() - lateral_slack))
            && (p.second.center().value() < (gaussian.center().value().value() + lateral_slack)))
          too_close = true;
      }

      //      if (too_close)
      //        DBG << "Too close at " << settings_.cali_nrg_.transform(gaussian.center_, settings_.bits_);

      if ( !too_close &&
           gaussian.height().value().finite() && (gaussian.height().value().value() > 0) &&
           gaussian.hwhm().value().finite() && (gaussian.hwhm().value().value() > 0) &&
           (finder_.x_[finder_.lefts[j]] < gaussian.center().value().value()) &&
           (gaussian.center().value().value() < finder_.x_[finder_.rights[j]]) &&
           (gaussian.height().value().value() > finder_.settings_.resid_min_amplitude) &&
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
    double diff = std::abs(finder_.x_[finder_.filtered[target_peak]] - centroid_hint);
    for (size_t j=0; j < finder_.filtered.size(); ++j)
      if (std::abs(finder_.x_[finder_.filtered[j]] - centroid_hint) < diff) {
        target_peak = j;
        diff = std::abs(finder_.x_[finder_.filtered[j]] - centroid_hint);
      }
  }

  if (target_peak == -1)
    return false;

  std::vector<double> x_pk = std::vector<double>(finder_.x_.begin() + finder_.lefts[target_peak],
                                                 finder_.x_.begin() + finder_.rights[target_peak] + 1);
  std::vector<double> y_pk = std::vector<double>(finder_.y_resid_.begin() + finder_.lefts[target_peak],
                                                 finder_.y_resid_.begin() + finder_.rights[target_peak] + 1);
  Gaussian gaussian;
  optimizer->fit(gaussian, x_pk, y_pk);

  if (
      gaussian.height().value().finite() && (gaussian.height().value().value() > 0) &&
      gaussian.hwhm().value().finite() && (gaussian.hwhm().value().value() > 0) &&
      (finder_.x_[finder_.lefts[target_peak]] < gaussian.center().value().value()) &&
      (gaussian.center().value().value() < finder_.x_[finder_.rights[target_peak]])
      )
  {
    Peak fitted(Hypermet(gaussian, finder_.settings_), SUM4(), finder_.settings_);
    peaks_[fitted.center().value()] = fitted;

    rebuild(optimizer, interruptor);
    return true;
  }
  else
    return false;
}


bool ROI::contains(double peakID) const
{
  return (peaks_.count(peakID) > 0);
}

Peak ROI::peak(double peakID) const
{
  if (contains(peakID))
    return peaks_.at(peakID);
  else
    return Peak();
}


bool ROI::overlaps(double bin) const
{
  if (!width())
    return false;
  return ((bin >= left_bin()) && (bin <= right_bin()));
}

bool ROI::overlaps(double Lbin, double Rbin) const
{
  if (finder_.x_.empty())
    return false;
  if (overlaps(Lbin) || overlaps(Rbin))
    return true;
  if ((Lbin <= left_bin()) && (Rbin >= right_bin()))
    return true;
  return false;
}

bool ROI::overlaps(const ROI& other) const
{
  if (!other.width())
    return false;
  return overlaps(other.left_bin(), other.right_bin());
}

size_t ROI::peak_count() const
{
  return peaks_.size();
}

const std::map<double, Peak> &ROI::peaks() const
{
  return peaks_;
}


bool ROI::adjust_sum4(double &peakID, double left, double right)
{
  if (!contains(peakID))
    return false;

  Qpx::Peak pk = peaks_.at(peakID);
  Qpx::SUM4 new_sum4(left, right, finder_, LB_, RB_);
  pk = Qpx::Peak(pk.hypermet(), new_sum4, finder_.settings_);
  remove_peak(peakID);
  peakID = pk.center().value();
  peaks_[peakID] = pk;
  render();
  save_current_fit("SUM4 adjusted on " + pk.energy().to_string());
  return true;
}


bool ROI::replace_hypermet(double &peakID, Hypermet hyp)
{
  if (!contains(peakID))
    return false;

  Qpx::Peak pk = peaks_.at(peakID);
  pk = Qpx::Peak(hyp, pk.sum4(), finder_.settings_);
  remove_peak(peakID);
  peakID = pk.center().value();
  peaks_[peakID] = pk;
  //set rsq to 0 for all peaks?

  render();
  save_current_fit("Hypermet adjusted on " + pk.energy().to_string());
  return true;
}

bool ROI::override_energy(double peakID, double energy)
{
  if (!contains(peakID))
    return false;

   peaks_[peakID].override_energy(energy);

   render();
   save_current_fit("Peak energy override " + peaks_.at(peakID).center().to_string()
                    + "->" + peaks_.at(peakID).energy().to_string());
   return true;
}


bool ROI::add_peak(const Finder &parentfinder,
                   double left, double right,
                   OptimizerPtr optimizer,
                   boost::atomic<bool>& interruptor)
{
  uint16_t center_prelim = (left+right) * 0.5; //assume down the middle

  if (overlaps(left) && overlaps(right)) {
    ROI new_fit = *this;

    if (!finder_.settings_.sum4_only && new_fit.add_from_resid(optimizer, interruptor, center_prelim))
    {
      *this = new_fit;
      save_current_fit("Added from residuals");
      return true;
    }
    else
    {
      Peak fitted(Hypermet(), SUM4(left, right, finder_, LB_, RB_), finder_.settings_);
      peaks_[fitted.center().value()] = fitted;
      render();
      save_current_fit("Manually added " + fitted.energy().to_string());
      return true;
    }
  }
  else if (width()) //changing region bounds
  {
    if (!finder_.x_.empty())
    {
      left  = std::min(left, left_bin());
      right = std::max(right, right_bin());
    }
    if (!finder_.cloneRange(parentfinder, left, right))
      return false;

    init_edges();
    init_background();
    finder_.y_resid_ = remove_background();
    render();
    finder_.find_peaks();  //assumes default params!!!

    ROI new_fit = *this;
    if (finder_.settings_.sum4_only)
    {
      Peak fitted(Hypermet(), SUM4(left, right, finder_, LB_, RB_), finder_.settings_);
      peaks_[fitted.center().value()] = fitted;
      render();
      save_current_fit("Manually added " + fitted.energy().to_string());
      return true;
    }
    else if (new_fit.add_from_resid(optimizer, interruptor, finder_.find_index(center_prelim)))
    {
      *this = new_fit;
      save_current_fit("Added from residuals");
      return true;
    }
    else
      return auto_fit(optimizer, interruptor);
  }

  DBG << "<ROI> cannot add to empty ROI";
  return false;
}

bool ROI::remove_peaks(const std::set<double> &pks, OptimizerPtr optimizer, boost::atomic<bool>& interruptor)
{
  bool found = false;
  for (auto &q : pks)
    if (remove_peak(q))
      found = true;

  if (!found)
    return false;

  if (peaks_.size() && !rebuild(optimizer, interruptor))
    return false;

  render();
  save_current_fit("Peaks removed");
  return true;
}

bool ROI::remove_peak(double bin)
{
  if (peaks_.count(bin)) {
    UncertainDouble en = peaks_.at(bin).energy();
    peaks_.erase(bin);
    return true;
  }
  return false;
}

bool ROI::override_settings(const FitSettings &fs, boost::atomic<bool>& interruptor)
{
  finder_.settings_ = fs;
  finder_.settings_.overriden = true; //do this in fitter if different?
  save_current_fit("Region settings overriden");

  //propagate to peaks

  //render if calibs changed?
  return true;
}


Fit::Fit(const SUM4Edge &lb, const SUM4Edge &rb,
         const Polynomial &backg,
         const std::map<double, Peak> &peaks,
         const Finder &finder,
         std::string descr)
{
  settings_ = finder.settings_;
  background_ = backg;
  LB_ = lb;
  RB_ = rb;
  peaks_ = peaks;

  description.description = descr;
  description.peaknum = peaks_.size();
  if (peaks_.size()) {
    description.rsq = peaks_.begin()->second.hypermet().chi2();
    UncertainDouble tot_gross = UncertainDouble::from_int(0,0);
    UncertainDouble tot_back = UncertainDouble::from_int(0,0);
    for (auto &p : peaks_)
    {
      tot_gross += p.second.sum4().gross_area();
      tot_back  += p.second.sum4().background_area();
      p.second.hr_peak_.clear();
      p.second.hr_fullfit_.clear();
    }
    UncertainDouble tot_net = tot_gross - tot_back;
    description.sum4aggregate = tot_net.error();
  }
}

void ROI::save_current_fit(std::string description)
{
  Fit thisfit(LB_, RB_, background_,
              peaks_, finder_, description);
  fits_.push_back(thisfit);
  current_fit_ = fits_.size() - 1;
}

bool ROI::rebuild(OptimizerPtr optimizer, boost::atomic<bool>& interruptor)
{
  hr_x.clear();
  hr_x_nrg.clear();
  hr_background.clear();
  hr_back_steps.clear();
  hr_fullfit.clear();

  bool hypermet_fit = false;
  for (auto &q : peaks_)
    if (!q.second.hypermet().gaussian_only())
    {
      hypermet_fit = true;
      break;
    }

  bool success = false;
  if (hypermet_fit)
    success = rebuild_as_hypermet(optimizer, interruptor);
  else
    success = rebuild_as_gaussian(optimizer, interruptor);

  if (!success)
    return false;

  render();
  return true;
}

bool ROI::rebuild_as_hypermet(OptimizerPtr optimizer, boost::atomic<bool>& interruptor)
{
  CustomTimer timer(true);

  std::map<double, Peak> new_peaks;

  std::vector<Hypermet> old_hype;
  for (auto &q : peaks_) {
    if (q.second.hypermet().height().value().value())
      old_hype.push_back(q.second.hypermet());
    else if (q.second.sum4().peak_width())
    {
      Peak s4only(Hypermet(),
                  SUM4(q.second.sum4().left(), q.second.sum4().right(),
                       finder_, LB_, RB_),
                  finder_.settings_);
      new_peaks[s4only.center().value()] = s4only;
    }
  }

  if (old_hype.empty())
    return false;

//  std::vector<Hypermet> hype = Hypermet::fit_multi(finder_.x_, finder_.y_,
//                                                   old_hype, background_,
//                                                   finder_.settings_);

  std::vector<Hypermet> hype = optimizer->fit_multiplet(finder_.x_, finder_.y_,
                                                        old_hype, background_,
                                                        finder_.settings_);

  for (size_t i=0; i < hype.size(); ++i) {
    double edge =  hype[i].width().value().value() * sqrt(log(2)) * 3; //use const from settings
    double left = hype[i].center().value().value() - edge;
    double right = hype[i].center().value().value() + edge;
    Peak one(hype[i], SUM4(left, right, finder_, LB_, RB_), finder_.settings_);
    new_peaks[one.center().value()] = one;
  }

  peaks_ = new_peaks;
  return true;
}

bool ROI::rebuild_as_gaussian(OptimizerPtr optimizer, boost::atomic<bool>& interruptor)
{
  CustomTimer timer(true);

  std::map<double, Peak> new_peaks;

  std::vector<Gaussian> old_gauss;
  for (auto &q : peaks_)
  {
    if (q.second.hypermet().height().value().value())
      old_gauss.push_back(q.second.hypermet().gaussian());
    else if (q.second.sum4().peak_width())
    {
      Peak s4only(Hypermet(), SUM4(q.second.sum4().left(),
                                   q.second.sum4().right(),
                                   finder_, LB_, RB_),
                  finder_.settings_);
      //      q.second.construct(settings_);
      new_peaks[s4only.center().value()] = s4only;
    }
  }

  if (old_gauss.empty())
    return false;

  std::vector<Gaussian> gauss = optimizer->fit_multiplet(finder_.x_, finder_.y_,
                                                         old_gauss, background_,
                                                         finder_.settings_);

  for (size_t i=0; i < gauss.size(); ++i)
  {
    double edge =  gauss[i].hwhm().value().value() * 3; //use const from settings
    double left = gauss[i].center().value().value() - edge;
    double right = gauss[i].center().value().value() + edge;
    Peak one(Hypermet(gauss[i], finder_.settings_),
             SUM4(left, right, finder_, LB_, RB_),
             finder_.settings_);
    new_peaks[one.center().value()] = one;
  }

  peaks_ = new_peaks;
  return true;
}


void ROI::render()
{
  hr_x.clear();
  hr_background.clear();
  hr_back_steps.clear();
  hr_fullfit.clear();
  Polynomial sum4back = SUM4::sum4_background(LB_, RB_, finder_);

  for (double i = 0; i < finder_.x_.size(); i += 0.1)
  {
    hr_x.push_back(finder_.x_[0] + i);
    hr_fullfit.push_back(finder_.y_[std::floor(i)]);
  }
  hr_background = background_.eval_array(hr_x);
  hr_sum4_background_ = sum4back.eval_array(hr_x);
  hr_x_nrg = finder_.settings_.cali_nrg_.transform(hr_x, finder_.settings_.bits_);

  std::vector<double> lowres_backsteps = sum4back.eval_array(finder_.x_);
  std::vector<double> lowres_fullfit   = sum4back.eval_array(finder_.x_);

  for (auto &p : peaks_)
    p.second.hr_fullfit_ = p.second.hr_peak_ = hr_fullfit;

  if (!finder_.settings_.sum4_only) {
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

  finder_.reset();
  finder_.setFit(finder_.x_, lowres_fullfit, lowres_backsteps);
}

std::vector<double> ROI::remove_background()
{
  std::vector<double> y_background = background_.eval_array(finder_.x_);

  std::vector<double> y_nobkg(finder_.x_.size());
  for (int32_t i = 0; i < static_cast<int32_t>(finder_.y_.size()); ++i)
    y_nobkg[i] = finder_.y_[i] - y_background[i];

  return y_nobkg;
}

bool ROI::adjust_LB(const Finder &parentfinder, double left, double right,
                     OptimizerPtr optimizer, boost::atomic<bool>& interruptor)
{
  size_t Lidx = parentfinder.find_index(left);
  size_t Ridx = parentfinder.find_index(right);
  if (Lidx >= Ridx)
    return false;

  SUM4Edge edge(parentfinder.x_, parentfinder.y_, Lidx, Ridx);
  if (!edge.width() || (edge.right() >= RB_.left()))
    return false;

  if ((edge.left() != left_bin()) && !finder_.cloneRange(parentfinder, left, right_bin()))
    return false;

  LB_ = edge;
  init_background();
  cull_peaks();
  render();
  rebuild(optimizer, interruptor);
  save_current_fit("Left baseline adjusted");
  return true;
}

bool ROI::adjust_RB(const Finder &parentfinder, double left, double right,
                    OptimizerPtr optimizer, boost::atomic<bool>& interruptor) {
  size_t Lidx = parentfinder.find_index(left);
  size_t Ridx = parentfinder.find_index(right);
  if (Lidx >= Ridx)
    return false;

  SUM4Edge edge(parentfinder.x_, parentfinder.y_, Lidx, Ridx);
  if (!edge.width() || (edge.left() <= LB_.right()))
    return false;

  if ((edge.right() != right_bin()) && !finder_.cloneRange(parentfinder, left_bin(), right))
    return false;

  RB_ = edge;
  init_background();
  cull_peaks();
  render();
  rebuild(optimizer, interruptor);
  save_current_fit("Right baseline adjusted");
  return true;
}

void ROI::init_edges()
{
  init_LB();
  init_RB();
}

void ROI::init_LB()
{
  int32_t LBend = 0;
  uint16_t samples = finder_.settings_.background_edge_samples;

  if ((samples > 0) && (finder_.y_.size() > samples*3))
    LBend += samples;

  LB_ = SUM4Edge(finder_.x_, finder_.y_, 0, LBend);
}

void ROI::init_RB()
{
  int32_t RBstart = finder_.y_.size() - 1;
  uint16_t samples = finder_.settings_.background_edge_samples;
  if ((samples > 0) && (finder_.y_.size() > samples*3))
    RBstart -= samples;

  RB_ = SUM4Edge(finder_.x_, finder_.y_, RBstart, finder_.y_.size() - 1);
}

void ROI::init_background()
{
  if (finder_.x_.empty())
    return;

  background_ = Polynomial();
  auto xoffset = background_.xoffset();

  //by default, linear
  double run = RB_.left() - LB_.right();
  xoffset.preset_bounds(LB_.left(), LB_.left());
  background_.add_coeff(0, LB_.min(), LB_.max(), LB_.average());

  double minslope = 0, maxslope = 0;
  if (LB_.average() < RB_.average())
  {
    run = RB_.right() - LB_.right();
    xoffset.preset_bounds(LB_.right(), LB_.right());
    minslope = (RB_.min() - LB_.max()) / (RB_.right() - LB_.left());
    maxslope = (RB_.max() - LB_.min()) / (RB_.left() - LB_.right());
  }

  if (RB_.average() < LB_.average())
  {
    run = RB_.left() - LB_.left();
    xoffset.preset_bounds(LB_.left(), LB_.left());
    minslope = (RB_.min() - LB_.max()) / (RB_.left() - LB_.right());
    maxslope = (RB_.max() - LB_.min()) / (RB_.right() - LB_.left());
  }
  background_.set_xoffset(xoffset);

  double slope = (RB_.average() - LB_.average()) / (run) ;
  background_.add_coeff(1, minslope, maxslope, slope);
}

size_t ROI::current_fit() const
{
  return current_fit_;
}

size_t ROI::history_size() const
{
  return fits_.size();
}

std::vector<FitDescription> ROI::history() const
{
  std::vector<FitDescription> ret;
  for (auto &f : fits_)
    ret.push_back(f.description);
  return ret;
}


bool ROI::rollback(const Finder &parent_finder, size_t i)
{
  if (i >= fits_.size())
    return false;

  finder_.settings_ = fits_[i].settings_;
  set_data(parent_finder, fits_[i].LB_.left(), fits_[i].RB_.right());
  background_ = fits_[i].background_;
  LB_ = fits_[i].LB_;
  RB_ = fits_[i].RB_;
  peaks_ = fits_[i].peaks_;
  render();

  current_fit_ = i;

  return true;
}

void ROI::cull_peaks()
{
  std::map<double, Peak> peaks;
  for (auto &p : peaks_) {
    if ((p.first > LB_.right()) &&
        (p.first < RB_.left()))
      peaks[p.first] = p.second;
  }
  peaks_ = peaks;
}


void ROI::to_xml(pugi::xml_node &root, const Finder &parent_finder) const
{
  if (fits_.empty())
    return;

  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());
  node.append_attribute("current_fit").set_value(static_cast<unsigned int>(current_fit_));

  size_t chosenfit = current_fit_;

  ROI temp(*this);

  for (size_t i=0; i < temp.fits_.size(); ++i)
  {
    pugi::xml_node fitnode = node.append_child("Fit");
    fitnode.append_attribute("description").set_value(temp.fits_[i].description.description.c_str());
    temp.rollback(parent_finder, i);

    if (finder_.settings_.overriden)
      finder_.settings_.to_xml(fitnode);

    pugi::xml_node Ledge = fitnode.append_child("BackgroundLeft");
    Ledge.append_attribute("left").set_value(temp.LB_.left());
    Ledge.append_attribute("right").set_value(temp.LB_.right());

    pugi::xml_node Redge = fitnode.append_child("BackgroundRight");
    Redge.append_attribute("left").set_value(temp.RB_.left());
    Redge.append_attribute("right").set_value(temp.RB_.right());

    temp.background_.to_xml(fitnode);
    fitnode.last_child().set_name("BackgroundPoly");

    if (temp.peaks_.size()) {
      pugi::xml_node pks = fitnode.append_child("Peaks");
      for (auto &p : temp.peaks_) {
        pugi::xml_node pk = pks.append_child("Peaks");
        if (p.second.sum4().peak_width()) {
          pugi::xml_node s4 = pk.append_child("SUM4");
          s4.append_attribute("left").set_value(p.second.sum4().left());
          s4.append_attribute("right").set_value(p.second.sum4().right());
        }
        if (p.second.hypermet().height().value().value() > 0)
          p.second.hypermet().to_xml(pk);
      }
    }
  }
}


void ROI::from_xml(const pugi::xml_node &root, const Finder &finder)
{
  if (finder.x_.empty() || (finder.x_.size() != finder.y_.size()))
    return;

  int chosenfit = root.attribute("current_fit").as_int();

  for (auto &node : root.children()) {
    std::string description(node.attribute("description").value());

    SUM4Edge LB, RB;
    if (node.child("BackgroundLeft")) {
      double L = node.child("BackgroundLeft").attribute("left").as_double();
      double R = node.child("BackgroundLeft").attribute("right").as_double();
      LB = SUM4Edge(finder.x_, finder.y_, finder.find_index(L), finder.find_index(R));
    }

    if (node.child("BackgroundRight")) {
      double L = node.child("BackgroundRight").attribute("left").as_double();
      double R = node.child("BackgroundRight").attribute("right").as_double();
      RB = SUM4Edge(finder.x_, finder.y_, finder.find_index(L), finder.find_index(R));
    }

    if (!LB.width() || !RB.width())
      return;

    finder_.settings_ = finder.settings_;
    if (node.child(finder_.settings_.xml_element_name().c_str()))
      finder_.settings_.from_xml(node.child(finder_.settings_.xml_element_name().c_str()));

    //validate background and edges?
    set_data(finder, LB.left(), RB.left());

    LB_ = LB;
    RB_ = RB;

    if (node.child("BackgroundPoly"))
      background_.from_xml(node.child("BackgroundPoly"));

    if (node.child("Peaks")) {
      for (auto &pk : node.child("Peaks").children())
      {
        Hypermet hyp;
        if (pk.child("Hypermet"))
          hyp.from_xml(pk.child("Hypermet"));
        SUM4 s4;
        if (pk.child("SUM4"))
        {
          double L = pk.child("SUM4").attribute("left").as_double();
          double R = pk.child("SUM4").attribute("right").as_double();
          s4 = SUM4(L, R, finder_, LB_, RB_);
        }
        Peak newpeak(hyp, s4, finder_.settings_);
        peaks_[newpeak.center().value()] = newpeak;
      }
    }

    render();
    save_current_fit(description);
    peaks_.clear();
  }
  rollback(finder, chosenfit);
}

json ROI::to_json(const Finder &parent_finder) const
{
  json j;

  if (fits_.empty())
    return j;

  j["current_fit"] = current_fit_;

  ROI temp(*this);

  for (size_t i=0; i < temp.fits_.size(); ++i)
  {
    json jj;

    jj["description"] = temp.fits_[i].description.description;
    temp.rollback(parent_finder, i);

    if (finder_.settings_.overriden)
      jj["settings"] = finder_.settings_;

    jj["background_left"] = temp.LB();
    jj["background_right"] = temp.RB();
    jj["background_poly"] = temp.background_;

    for (auto &p : temp.peaks_)
      jj["peaks"].push_back(p.second);

    j["fits"].push_back(jj);
  }

  return j;
}

ROI::ROI(const json& j, const Finder &finder)
{
  if (finder.x_.empty() || (finder.x_.size() != finder.y_.size()))
    return;

  if (j.count("fits"))
  {
    json o = j["fits"];
    for (json::iterator it = o.begin(); it != o.end(); ++it)
    {
      SUM4Edge LB(it.value()["background_left"], finder);
      SUM4Edge RB(it.value()["background_right"], finder);

      if (!LB.width() || !RB.width())
        return;

      if (it.value().count("settings"))
        finder_.settings_ = it.value()["settings"];
      else
        finder_.settings_ = finder.settings_;

      //validate background and edges?
      set_data(finder, LB.left(), RB.left());

      LB_ = LB;
      RB_ = RB;

      if (it.value().count("peaks"))
      {
        json p = it.value()["peaks"];
        for (json::iterator it2 = p.begin(); it2 != p.end(); ++it2)
        {
          Peak newpeak(it2.value(), finder_, LB_, RB_);
          peaks_[newpeak.center().value()] = newpeak;
        }
      }

      background_ = it.value()["background_poly"];
      render();
      save_current_fit(it.value()["description"]);
      peaks_.clear();
    }
  }

  rollback(finder, j["current_fit"]);
}

}
