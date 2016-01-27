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
 *      Gamma::Finder
 *
 ******************************************************************************/

#include <vector>
#include <cmath>
#include "finder.h"

namespace Gamma {

void Finder::setData(std::vector<double> x, std::vector<double> y)
{
  clear();
  if (x.size() == y.size()) {
    x_ = x;
    y_ = y;
    calc_kon();
    find_peaks();
  }
}

void Finder::clear() {
  x_.clear();
  y_.clear();

  prelim.clear();
  filtered.clear();
  lefts.clear();
  rights.clear();

  x_kon.clear();
  x_conv.clear();
}


void Finder::calc_kon() {
  if (square_width_ < 2)
    square_width_ = 2;

  int shift = square_width_ / 2;

  x_kon.resize(y_.size(), 0);
  x_conv.resize(y_.size(), 0);
  prelim.clear();

  for (int j = square_width_; (j+2*square_width_+1) < y_.size(); ++j) {
    double kon = 0;
    double avg = 0;
    for (int i=j; i <= (j+square_width_+1); ++i) {
      kon += 2*y_[i] - y_[i-square_width_] - y_[i+square_width_];
      avg += y_[i];
    }
    avg = avg / square_width_;
    x_kon[j + shift] = kon;
    x_conv[j + shift] = kon / sqrt(6* square_width_ * avg);

    if (x_conv[j + shift] > threshold_)
      prelim.push_back(j + shift);
  }

}


void Finder::find_peaks(uint16_t width, double thresh) {
  square_width_ = width;
  threshold_ = thresh;
  calc_kon();
  find_peaks();
}

void Finder::find_peaks() {
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
  }
}

uint16_t Finder::find_left(uint16_t chan) {
  if (x_.empty())
    return 0;

  //assume x is monotone increasing

  double edge_threshold = -0.5 * threshold_;

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

  //assume x is monotone increasing

  double edge_threshold = -0.5 * threshold_;

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

  double edge_threshold = -0.5 * threshold_;

  while ((idx > 0) && (x_conv[idx] > 0))
    idx--;
  if (idx > 0)
    idx--;
  while ((idx > 0) && (x_conv[idx] < edge_threshold))
    idx--;

  return idx;
}

uint16_t Finder::right_edge(uint16_t idx) {
  if (x_conv.empty() || idx >= x_conv.size())
    return 0;

  double edge_threshold = -0.5 * threshold_;

  while ((idx < x_conv.size()) && (x_conv[idx] > 0))
    idx++;
  if (idx < x_conv.size())
    idx++;
  while ((idx < x_conv.size()) && (x_conv[idx] < edge_threshold))
    idx++;

  return idx;
}


}
