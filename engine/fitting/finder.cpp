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
 *      Qpx::Finder
 *
 ******************************************************************************/

#include <vector>
#include <cmath>
#include "finder.h"

namespace Qpx {

void Finder::setData(const std::vector<double> &x, const std::vector<double> &y)
{
  clear();
  if (x.size() == y.size()) {
    x_ = x;
    y_resid_on_background_ = y_resid_ = y_ = y;

    calc_kon();
    find_peaks();
  }
}

void Finder::setFit(const std::vector<double> &y_fit, const std::vector<double> &y_background)
{
  if (y_fit.size() == y_.size()) {
    y_fit_ = y_fit;

    y_resid_ = y_;
    y_resid_on_background_ = y_background;
    for (int i=0; i < y_.size(); ++i) {
      y_resid_[i] = y_[i] - y_fit_[i];
      y_resid_on_background_[i] += y_resid_[i];
    }

    calc_kon();
    find_peaks();
  }
}

void Finder::clear() {
  x_.clear();
  y_.clear();
  y_fit_.clear();
  y_resid_.clear();

  prelim.clear();
  filtered.clear();
  lefts.clear();
  rights.clear();

  x_kon.clear();
  x_conv.clear();
}


void Finder::calc_kon() {
  fw_theoretical_nrg.clear();
  fw_theoretical_bin.clear();
  if (settings_.cali_fwhm_.valid() && settings_.cali_nrg_.valid()) {
    for (int i=0; i < x_.size(); ++i) {
      double nrg = settings_.cali_nrg_.transform(x_[i], settings_.bits_);
      fw_theoretical_nrg.push_back(nrg);
      double fw = settings_.cali_fwhm_.transform(nrg);
      double L = settings_.cali_nrg_.inverse_transform(nrg - fw/2, settings_.bits_);
      double R = settings_.cali_nrg_.inverse_transform(nrg + fw/2, settings_.bits_);
      double wchan = R-L;
      fw_theoretical_bin.push_back(wchan);
    }
  }

  uint16_t width = settings_.KON_width;

  if (width < 2)
    width = 2;

  double sigma = settings_.KON_sigma_spectrum;
  if (y_resid_ != y_) {
//    PL_DBG << "<Finder> Using sigma resid";
    sigma = settings_.KON_sigma_resid;
  }


//  PL_DBG << "<Finder> width " << settings_.KON_width;

  int start = width;
  int end = x_.size() - 1 - 2 * width;
  int shift = width / 2;

  if (!fw_theoretical_bin.empty()) {
    for (int i=0; i < fw_theoretical_bin.size(); ++i)
      if (ceil(fw_theoretical_bin[i]) < i) {
        start = i;
        break;
      }
    for (int i=fw_theoretical_bin.size() -1 ; i >= 0; --i)
      if (2 * ceil(fw_theoretical_bin[i]) + i + 1 < fw_theoretical_bin.size()) {
        end = i;
        break;
      }
  }


  x_kon.resize(y_resid_.size(), 0);
  x_conv.resize(y_resid_.size(), 0);
  prelim.clear();

  for (int j = start; j < end; ++j) {

    if (!fw_theoretical_bin.empty()) {
      width = floor(fw_theoretical_bin[j]);
      shift = width / 2;
    }

    double kon = 0;
    double avg = 0;
    for (int i=j; i <= (j+width+1); ++i) {
      kon += 2*y_resid_[i] - y_resid_[i-width] - y_resid_[i+width];
      avg += y_resid_[i];
    }
    avg = avg / width;
    x_kon[j + shift] = kon;
    x_conv[j + shift] = kon / sqrt(6* width * avg);

    if (x_conv[j + shift] > sigma)
      prelim.push_back(j + shift);
  }

}


void Finder::find_peaks() {
  calc_kon();
  filtered.clear();
  lefts.clear();
  rights.clear();

  if (prelim.empty())
    return;

  //find edges of contiguous peak areas
  lefts.push_back(prelim[0]);
  int prev = prelim[0];
  for (int i=0; i < prelim.size(); ++i) {
    int current = prelim[i];
    if ((current - prev) > 1) {
      rights.push_back(prev);
      lefts.push_back(current);
    }
    prev = current;
  }
  rights.push_back(prev);

  //assume center is bentween edges
  for (int i=0; i < lefts.size(); ++i)
    filtered.push_back((rights[i] + lefts[i])/2);

  for (int i=0; i < filtered.size(); ++i) {
    lefts[i]  = left_edge(lefts[i]);
    rights[i] = right_edge(rights[i]);
//    PL_DBG << "<Finder> Peak " << lefts[i] << "-"  << filtered[i] << "-"  << rights[i];
  }
}

uint16_t Finder::find_left(uint16_t chan) {
  if (x_.empty())
    return 0;

  //assume x is monotone increasing

  double sigma = settings_.KON_sigma_spectrum;
  if (y_resid_ != y_) {
//    PL_DBG << "<Finder> Using sigma resid";
    sigma = settings_.KON_sigma_resid;
  }

  double edge_threshold = -0.5 * sigma;

  if ((chan < x_[0]) || (chan >= x_[x_.size()-1]))
    return 0;

  int i = x_.size()-1;

  while ((i > 0) && (x_[i] > chan))
    i--;

  return x_[left_edge(i)];
}

uint16_t Finder::find_right(uint16_t chan) {
  if (x_.empty())
    return 0;

  double sigma = settings_.KON_sigma_spectrum;
  if (y_resid_ != y_) {
//    PL_DBG << "<Finder> Using sigma resid";
    sigma = settings_.KON_sigma_resid;
  }

  //assume x is monotone increasing

  double edge_threshold = -0.5 * sigma;

  if ((chan < x_[0]) || (chan >= x_[x_.size()-1]))
    return x_.size() - 1;

  int i = 0;

  while ((i < x_.size()) && (x_[i] < chan))
    i++;

  return x_[right_edge(i)];
}


uint16_t Finder::left_edge(uint16_t idx) {
  if (x_conv.empty() || idx >= x_conv.size())
    return 0;

  if (!fw_theoretical_bin.empty()) {
    double width = floor(fw_theoretical_bin[idx]);
    double goal = x_[idx] - width * settings_.ROI_extend_peaks / 2;
    while ((idx > 0) && (x_[idx] > goal))
      idx--;
    return idx;
  }


  double sigma = settings_.KON_sigma_spectrum;
  if (y_resid_ != y_) {
//    PL_DBG << "<Finder> Using sigma resid";
    sigma = settings_.KON_sigma_resid;
  }

  double edge_threshold = -0.5 * sigma;

  while ((idx > 0) && (x_conv[idx] >= 0))
    idx--;
  if (idx > 0)
    idx--;
  while ((idx > 0) && (x_conv[idx] < edge_threshold))
    idx--;

  if (idx < 0)
    idx = 0;

  return idx;
}

uint16_t Finder::right_edge(uint16_t idx) {
  if (x_conv.empty() || idx >= x_conv.size())
    return 0;

  if (!fw_theoretical_bin.empty()) {
    double width = floor(fw_theoretical_bin[idx]);
    double goal = x_[idx] + width * settings_.ROI_extend_peaks / 2;
    while ((idx < x_.size()) && (x_[idx] < goal))
      idx++;
    return idx;
  }

  double sigma = settings_.KON_sigma_spectrum;
  if (y_resid_ != y_) {
//    PL_DBG << "<Finder> Using sigma resid";
    sigma = settings_.KON_sigma_resid;
  }

  double edge_threshold = -0.5 * sigma;

  while ((idx < x_conv.size()) && (x_conv[idx] >= 0))
    idx++;
  if (idx < x_conv.size())
    idx++;
  while ((idx < x_conv.size()) && (x_conv[idx] < edge_threshold))
    idx++;

  if (idx >= x_conv.size())
    idx = x_conv.size() - 1;

  return idx;
}

int32_t Finder::find_index(double chan_val) {
  if (x_.empty())
    return -1;

  if (chan_val <= x_[0])
    return 0;

  if (chan_val >= x_[x_.size()-1])
    return x_.size()-1;

  int32_t i = 0;
  while ((i < x_.size()) && (x_[i] < chan_val))
    i++;

  return i;
}


}
