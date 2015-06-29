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
 *      Gamma::Fitter
 *
 ******************************************************************************/

#include "gamma_fitter.h"
#include <numeric>
#include "custom_logger.h"

namespace Gamma {

std::vector<double> Fitter::make_baseline(uint16_t left, uint16_t right, uint16_t BL_samples, BaselineType type) {
  int32_t  width = right - left;
  double left_avg = local_avg(left, BL_samples);
  double right_avg = local_avg(right, BL_samples);

  if ((width <= 0) || isnan(left_avg) || isnan(right_avg))
    return std::vector<double>();


  //by default, linear
  double slope = (right_avg - left_avg) / static_cast<double>(width);
  std::vector<double> xx(width+1), yy(width+1), baseline(width+1);
  for (int32_t i = 0; i <= width; ++i) {
    xx[i] = x_[left+i];
    yy[i] = y_[left+i];
    baseline[i] = left_avg + (i * slope);
  }

  if (type == BaselineType::linear)
    return baseline;

  //find centroid, then make step baseline
  Peak prelim = Peak(xx, yy, baseline);
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
    return baseline;
  
}

double Fitter::local_avg(uint16_t chan, uint16_t samples) {
  if ((chan >= x_.size()) || (samples < 1))
    return std::numeric_limits<double>::quiet_NaN();
  
  if ((samples % 2) == 0)
    samples++;
  int32_t half  = (samples - 1) / 2;
  int32_t left  = static_cast<int32_t>(chan) - half;
  int32_t right = static_cast<int32_t>(chan) + half;

  if (left < 0)
    left = 0;
  if (right >= x_.size())
    right = x_.size() - 1;

  samples = right - left + 1;
  
  return std::accumulate(y_.begin() + left, y_.begin() + (right+1), 0) / static_cast<double>(samples);
  
}


Fitter::Fitter(const std::vector<double> &x, const std::vector<double> &y, uint16_t min, uint16_t max, uint16_t avg_window) :
  x_(x), y_(y)
{
  if ((y_.size() == x_.size()) && (min < max) && ((max+1) < x.size())) {
    for (int i=min; i<=max; ++i) {
      x_.push_back(x[i]);
      y_.push_back(y[i]);
    }
    set_mov_avg(avg_window);
    deriv();
  }

  if (x_.size() > 0)
    PL_DBG << "x_ [" << x_[0] << ", " << x_[x_.size() - 1] << "]";
}

void Fitter::set_mov_avg(uint16_t window) {
  y_avg_ = y_;

  if ((window % 2) == 0)
    window++;

  if (y_.size() < window)
    return;

  uint16_t half = (window - 1) / 2;

  //assume values under 0 are same as for index 0
  double avg = (half + 1) * y_[0];

  //begin averaging over the first few
  for (int i = 0; i < half; ++i)
    avg += y_[i];

  avg /= window;

  double remove, add;
  for (int i=0; i < y_.size(); i++) {
    if (i < (half+1))
      remove = y_[0] / window;
    else
      remove = y_[i-(half+1)] / window;

    if ((i + half) > y_.size())
      add = y_[y_.size() - 1] / window;
    else
      add = y_[i + half] / window;

    avg = avg - remove + add;

    y_avg_[i] = avg;
  }

  deriv();
}


void Fitter::deriv() {
  if (!y_avg_.size())
    return;

  deriv1.clear();
  deriv2.clear();

  deriv1.push_back(0);
  for (int i=1; i < y_avg_.size(); ++i) {
    deriv1.push_back(y_avg_[i] - y_avg_[i-1]);
  }

  deriv2.push_back(0);
  for (int i=1; i < deriv1.size(); ++i) {
    deriv2.push_back(deriv1[i] - deriv1[i-1]);
  }
}


void Fitter::find_prelim() {
  prelim.clear();

  int was = 0, is = 0;

  for (int i = 0; i < deriv1.size(); ++i) {
    if (deriv1[i] > 0)
      is = 1;
    else if (deriv1[i] < 0)
      is = -1;
    else
      is = 0;

    if ((was == 1) && (is == -1))
      prelim.push_back(i);

    was = is;
  }
}

void Fitter::filter_prelim(uint16_t min_width) {
  filtered.clear();
  lefts.clear();
  rights.clear();

  if ((y_.size() < 3) || !prelim.size())
    return;

  for (auto &q : prelim) {
    if (!q)
      continue;
    uint16_t left = 0, right = 0;
    for (int i=q-1; i >= 0; --i)
      if (deriv1[i] > 0)
        left++;
      else
        break;

    for (int i=q; i < deriv1.size(); ++i)
      if (deriv1[i] < 0)
        right++;
      else
        break;

    if ((left >= min_width) && (right >= min_width)) {
      filtered.push_back(q-1);
      lefts.push_back(q-left-1);
      rights.push_back(q+right-1);
    }
  }
}

void Fitter::refine_edges(double threshl, double threshr) {
  lefts_t.clear();
  rights_t.clear();

  for (int i=0; i<filtered.size(); ++i) {
    uint16_t left = lefts[i],
        right = rights[i];

    for (int j=lefts[i]; j < filtered[i]; ++j) {
      if (deriv1[j] < threshl)
        left = j;
    }

    for (int j=rights[i]; j > filtered[i]; --j) {
      if ((-deriv1[j]) < threshr)
        right = j;
    }

    lefts_t.push_back(left);
    rights_t.push_back(right);
  }
}

uint16_t Fitter::find_left(uint16_t chan, uint16_t grace) {
  if ((chan - grace < 0) || (chan >= x_.size()))
    return 0;

  int i = chan - grace;
  while ((i >= 0) && (deriv1[i] > 0))
    i--;
  return i;

  
}

uint16_t Fitter::find_right(uint16_t chan, uint16_t grace) {
  if ((chan < 0) || (chan + grace >= x_.size()))
    return x_.size() - 1;

  int i = chan + grace;
  while ((i < x_.size()) && (deriv1[i] < 0))
    i++;
  return i;
}

void Fitter::find_peaks(int min_width, Calibration cali) {
  find_prelim();
  filter_prelim(min_width);

  peaks_.clear();
  for (int i=0; i < filtered.size(); ++i) {
    std::vector<double> baseline = make_baseline(lefts[i], rights[i], 3);
    std::vector<double> xx(x_.begin() + lefts[i], x_.begin() + rights[i] + 1);
    std::vector<double> yy(y_.begin() + lefts[i], y_.begin() + rights[i] + 1);
    Peak fitted = Peak(xx, yy, baseline, cali);

    if (
        (fitted.height > 0) &&
        (fitted.fwhm_gaussian > 0) &&
        (fitted.fwhm_pseudovoigt > 0)
       )
    {
      peaks_.push_back(fitted);
    }
  }
  PL_INFO << "Preliminary search found " << prelim.size() << " potential peaks";
  PL_INFO << "After minimum width filter: " << filtered.size();
  PL_INFO << "Fitted peaks: " << peaks_.size();
}

}
