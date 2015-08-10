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

SUM4::SUM4()
    :peak_width(0)
    ,Lw(0)
    ,Rw(0)
    ,Lsum(0)
    ,Rsum(0)
    ,P_area(0)
    ,B_area(0)
    ,B_variance(0)
    ,net_area(0)
    ,sumYnet(0)
    ,CsumYnet(0)
    ,C2sumYnet(0)
    ,net_variance(0)
    ,err(0)
    ,currieLQ(0)
    ,currieLD(0)
    ,currieLC(0)
    ,Lpeak(0)
    ,Rpeak(0)
    ,currie_quality_indicator(-1)
    ,centroid(0)
    ,centroid_var(0)
    ,fwhm(0)
{
  
}

SUM4::SUM4(const std::vector<double> &x,
           const std::vector<double> &y,
           uint32_t left, uint32_t right,
           uint16_t samples)
    :SUM4()
{

  if ((x.size() != y.size())
      || x.empty()
      || (left > right)
      || (left >= x.size())
      || (right >= x.size())
      || (samples >= x.size()))
    return;

  x_ = x;
  y_ = y;

  Lpeak = left;
  Rpeak = right;

  peak_width = right - left + 1;

  LBend = left - 1;
  RBstart = right + 1;

  int32_t LBstart = LBend - samples;
  if (LBstart < 0)
    LBstart = 0;

  int32_t RBend = RBstart + samples;
  if (RBend >= x.size())
    RBend = x.size() - 1;

  Lw = LBend - LBstart + 1;
  Rw = RBend - RBstart + 1;

  for (int i=LBstart; i <= LBend; ++i)
    Lsum += y[i];

  for (int i=RBstart; i <= RBend; ++i)
    Rsum += y[i];

  
  B_area = peak_width * (Lsum / Lw + Rsum / Rw) / 2.0;

  for (int i=left; i <=right; ++i)
    P_area += y[i];

  net_area = P_area - B_area;

  B_variance = pow((peak_width / 2.0), 2) * (Lsum / pow(Lw, 2) + Rsum / pow(Rw, 2));
  net_variance = P_area + B_variance;

  err = 100 * sqrt(net_variance) / net_area;

  
  currieLQ = 50 * (1 + sqrt(1 + B_variance / 12.5));
  currieLD = 2.71 + 4.65 * sqrt(B_variance);
  currieLC = 2.33 * sqrt(B_variance);

  if (net_area > currieLQ)
    currie_quality_indicator = 1;
  else if (net_area > currieLD)
    currie_quality_indicator = 2;
  else if (net_area > currieLC)
    currie_quality_indicator = 3;
  else if (net_area > 0)
    currie_quality_indicator = 4;
  else
    currie_quality_indicator = -1;


  //by default, linear
  double slope = (Rsum / Rw - Lsum / Lw) / (Rpeak - Lpeak);
  double offset = Lsum / Lw - slope * Lpeak;

  for (int32_t i = Lpeak; i <= Rpeak; ++i) {
    bx_.push_back(x_[i]);
    double B_chan  = offset + i*slope; 
    by_.push_back(B_chan);
    double yn = y_[i] - B_chan;
    sumYnet += yn;
    CsumYnet += i * yn;
    C2sumYnet += pow(i, 2) * yn;
  }

  centroid = CsumYnet / sumYnet;
  centroid_var = (C2sumYnet / sumYnet) - pow(centroid, 2);
  fwhm = 2.0 * sqrt(centroid_var * log(4));

}


double local_avg(const std::vector<double> &x,
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



std::vector<double> make_background(const std::vector<double> &x,
                                    const std::vector<double> &y,
                                    uint16_t left, uint16_t right,
                                    uint16_t samples,
                                    BaselineType type) {
  int32_t  width = right - left;
  double left_avg = local_avg(x, y, left, samples);
  double right_avg = local_avg(x, y, right, samples);

  if ((width <= 0) || isnan(left_avg) || isnan(right_avg))
    return std::vector<double>();


  //by default, linear
  double slope = (right_avg - left_avg) / static_cast<double>(width);
  std::vector<double> xx(width+1), yy(width+1), baseline(width+1);
  for (int32_t i = 0; i <= width; ++i) {
    xx[i] = x[left+i];
    yy[i] = y[left+i];
    baseline[i] = left_avg + (i * slope);
  }

  //if (type == BaselineType::linear)
    return baseline;

  //find centroid, then make step baseline
  /*  Peak prelim = Peak(xx, yy, baseline);
  int32_t center = static_cast<int32_t>(prelim.gaussian_.center_);
  for (int32_t i = 0; i <= width; ++i) {
    if (xx[i] <= center)
      baseline[i] = left_avg;
    else if (xx[i] > center)
      baseline[i] = right_avg;
  }

  if (type == BaselineType::step)
    return baseline;

  //substitute step with poly function
  std::vector<uint16_t> poly_terms({1,3,5,7,9,11,13,15,17,19,21});
  Polynomial poly = Polynomial(xx, baseline, 21, prelim.gaussian_.center_);
  baseline = poly.evaluate_array(xx);

  if (type == BaselineType::step_polynomial)
  return baseline;*/
  
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

  L = sum4_.centroid - sum4_.fwhm / 2;
  R = sum4_.centroid + sum4_.fwhm / 2;
  fwhm_sum4 = cali_nrg.transform(R) - cali_nrg.transform(L);
  if (sum4_.fwhm >= x_.size())
    fwhm_sum4 = 0;

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

  area_gauss_ =  gaussian_.height_ * gaussian_.hwhm_ * sqrt(3.141592653589793238462643383279502884 / log(2.0));
  if (!subpeak)
  {
    area_gross_ = sum4_.P_area;
    area_bckg_ = sum4_.B_area;
    area_net_ = sum4_.net_area;
  }

  if (live_seconds_ > 0) {
    cts_per_sec_net_ = area_net_ / live_seconds_;
    cts_per_sec_gauss_ = area_gauss_ / live_seconds_;
  }
}

Peak::Peak(const std::vector<double> &x, const std::vector<double> &y, uint32_t L, uint32_t R,
           Calibration cali_nrg, Calibration cali_fwhm, double live_seconds)
  : Peak()
{
  if (
      (x.size() == y.size())
      &&
      (L < R)
      &&
      (R < x.size())
      )
  {    
    sum4_ = SUM4(x, y, L, R, 3);

    x_ = std::vector<double>(x.begin() + L, x.begin() + R + 1);
    y_ = std::vector<double>(y.begin() + L, y.begin() + R + 1);
    y_baseline_ = sum4_.by_;
    
    std::vector<double> nobase(x_.size());    
    for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i)
      nobase[i] = y_[i] - y_baseline_[i];

    gaussian_ = Gaussian(x_, nobase);
    pseudovoigt_ = SplitPseudoVoigt(x_, nobase);
    live_seconds_ = live_seconds;
    construct(cali_nrg, cali_fwhm);
  }
}

bool Multiplet::contains(double bin) {
  for (auto &q : peaks_)
    if (q.center == bin)
      return true;
  return false;
}

bool Multiplet::overlaps(const Peak &pk) {
  if ((pk.center >= x_.front()) && (pk.center <= x_.back()))
    return true;
  if ((pk.x_.front() >= x_.front()) && (pk.x_.front() <= x_.back()))
    return true;
  if ((pk.x_.back() >= x_.front()) && (pk.x_.back() <= x_.back()))
    return true;
}


void Multiplet::add_peak(const Peak &pk, const std::vector<double> &x, const std::vector<double> &y)
{
  std::set<Peak> pks;
  pks.insert(pk);
  add_peaks(pks, x, y);
}

void Multiplet::add_peaks(const std::set<Peak> &pks, const std::vector<double> &x, const std::vector<double> &y)
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


void Multiplet::remove_peak(const Peak &pk) {
  if (peaks_.count(pk)) {
    peaks_.erase(pk);
    rebuild();
  }
}

void Multiplet::remove_peak(double bin) {
  for (auto &q : peaks_) {
    if (q.center == bin) {
      peaks_.erase(q);
      rebuild();
      break;
    }
  }
}

void Multiplet::rebuild() {
  y_fullfit_.clear();

  std::vector<double> nobck(x_.size());    
  for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i)
    nobck[i] = y_[i] - y_background_[i];
 
  std::vector<Gaussian> old_gauss;
  for (auto &q : peaks_)
    old_gauss.push_back(q.gaussian_);
  std::vector<Gaussian> gauss = Gaussian::fit_multi(x_, nobck, old_gauss);

  std::vector<SplitPseudoVoigt> old_spv;
  for (auto &q : peaks_)
    old_spv.push_back(q.pseudovoigt_);
  std::vector<SplitPseudoVoigt> spv = SplitPseudoVoigt::fit_multi(x_, nobck, old_spv);

  peaks_.clear();
  if (gauss.size() == spv.size()) {
    y_fullfit_ = y_background_;
    
    for (int i=0; i < gauss.size(); ++i) {
      Peak one;
      one.subpeak = true;
      one.intersects_L = (i != 0);
      one.intersects_R = ((i+1) < gauss.size());          
      one.x_ = x_;
      one.y_ = y_;
      one.y_baseline_ = y_background_;
      one.gaussian_ = gauss[i];
      one.pseudovoigt_ = spv[i];
      one.live_seconds_ = live_seconds_;
      one.construct(cal_nrg_, cal_fwhm_);
      
      peaks_.insert(one);
      
      for (int32_t j = 0; j < static_cast<int32_t>(x_.size()); ++j)
        y_fullfit_[j] +=  gauss[i].evaluate(x_[j]);
    }
  }
}

}
