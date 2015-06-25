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
 *      Gaussian, Polynomial and composite peaks
 *
 ******************************************************************************/

#include "poly_fit.h"

#include <gsl/gsl_multifit.h>
#include <sstream>
#include <iomanip>
#include <numeric>

#include "fityk.h"
#include "custom_logger.h"

std::string to_str_precision(double x, uint16_t n) {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(n) << x;
  return ss.str();
}

Polynomial::Polynomial(std::vector<double> coeffs, double center)
  :Polynomial()
{
  xoffset_ = center;
  coeffs_ = coeffs;
  int deg = -1;
  for (size_t i = 0; i < coeffs.size(); i++)
    if (coeffs[i])
      deg = i;
  degree_ = deg;
}

Polynomial::Polynomial(std::vector<double> &x, std::vector<double> &y, uint16_t degree, double center)
  :Polynomial()
{
  std::vector<uint16_t> degrees;
  for (int i=1; i <= degree; ++i)
    degrees.push_back(i);

  fit(x, y, degrees, center);
}

Polynomial::Polynomial(std::vector<double> &x, std::vector<double> &y,
                       std::vector<uint16_t> &degrees, double center) {
  fit(x, y, degrees, center);
}


void Polynomial::fit(std::vector<double> &x, std::vector<double> &y,
                       std::vector<uint16_t> &degrees, double center) {
  degree_ = -1;

  xoffset_ = center;
  if (x.size() != y.size())
    return;

  std::vector<double> x_c, sigma;
  sigma.resize(x.size(), 1);
  
  for (auto &q : x)
    x_c.push_back(q - center);

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);

  f->load_data(0, x_c, y, sigma);

  std::string definition = "define MyPoly(a0=~0";
  std::vector<std::string> varnames(1, "0");
  for (auto &q : degrees) {
    std::stringstream ss;
    ss << q;
    varnames.push_back(ss.str());
    definition += ",a" + ss.str() + "=~0";
  }
  definition += ") = a0";
  for (int i=1; i<varnames.size(); ++i)
    definition += " + a" + varnames[i] + "*x^" + varnames[i];
  

  bool success = true;

  f->execute(definition);
  try {
    f->execute("guess MyPoly");
  }
  catch ( ... ) {
    success = false;
  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    success = false;
  }

  if (success) {
    degree_ = degrees[degrees.size() - 1];
    fityk::Func* lastfn = f->all_functions().back();
    coeffs_.resize(degree_ + 1, 0);
    coeffs_[0] = lastfn->get_param_value("a0");
    for (auto &q : degrees) {
      std::stringstream ss;
      ss << q;
      coeffs_[q] = lastfn->get_param_value("a" + ss.str());
    }
    double rsqq = f->get_rsquared(0);
    PL_DBG << "rsquared=" << rsqq;
    rsq = rsqq;
  }

  delete f;
}


double Polynomial::evaluate(double x) {
  double x_adjusted = x - xoffset_;
  double result = 0.0;
  for (int i=0; i <= degree_; i++)
    result += coeffs_[i] * pow(x_adjusted, i);
  return result;
}

std::vector<double> Polynomial::evaluate_array(std::vector<double> x) {
  std::vector<double> y;
  for (auto &q : x)
    y.push_back(evaluate(q));
  return y;
}

std::string Polynomial::to_string() {
  std::stringstream ss;
  if (degree_ >=0 )
    ss << coeffs_[0];
  for (int i=1; i <= degree_; i++)
    ss << " + " << coeffs_[i] << "x^" << i;
  return ss.str();
}

std::string Polynomial::to_UTF8() {
  std::vector<std::string> superscripts = {
    "\u2070", "\u00B9", "\u00B2",
    "\u00B3", "\u2074", "\u2075",
    "\u2076", "\u2077", "\u2078",
    "\u2079"
  };

  std::string calib_eqn;
  if (degree_ >= 0)
    calib_eqn += to_str_precision(coeffs_[0], 3);
  for (uint16_t i=1; i <= degree_; i++) {
    calib_eqn += std::string(" + ");
    calib_eqn += to_str_precision(coeffs_[i], 3);
    calib_eqn += std::string("x");
    if ((i > 1) && (i < 10)) {
      calib_eqn += superscripts[i];
    } else if ((i>9) && (i<100)) {
      calib_eqn += superscripts[i / 10];
      calib_eqn += superscripts[i % 10];
    } else if (i>99) {
      calib_eqn += std::string("^");
      calib_eqn += std::to_string(i);
    }
  }
  return calib_eqn;
}

std::string Polynomial::to_markup() {
  std::string calib_eqn;
  if (degree_ >= 0)
    calib_eqn += std::to_string(coeffs_[0]);
  for (uint16_t i=1; i <= degree_; i++) {
    calib_eqn += std::string(" + ");
    calib_eqn += std::to_string(coeffs_[i]);
    calib_eqn += std::string("x");
    if (i > 1) {
      calib_eqn += std::string("<sup>");
      calib_eqn += std::to_string(i);
      calib_eqn += std::string("</sup>");
    }
  }
  return calib_eqn;
}


Gaussian::Gaussian(const std::vector<double> &x, const std::vector<double> &y):
  Gaussian() {
  std::vector<double> sigma;
  sigma.resize(x.size(), 1);

  bool success = true;
  std::vector<fityk::Func*> fns;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);

  try {
    f->execute("guess Gaussian");
  }
  catch ( ... ) {
    //PL_ERR << "Fytik threw exception a";
    success = false;
  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    //PL_ERR << "Fytik threw exception b";
    success = false;
  }

  if (success) {
    std::vector<fityk::Func*> fns = f->all_functions();
    if (fns.size()) {
      fityk::Func* lastfn = fns.back();
      center_ = lastfn->get_param_value("center");
      height_ = lastfn->get_param_value("height");
      hwhm_   = lastfn->get_param_value("hwhm");
    }
    rsq = f->get_rsquared(0);
  }

  delete f;
}

double Gaussian::evaluate(double x) {
  return height_ * exp(-log(2.0)*(pow(((x-center_)/hwhm_),2)));
}

std::vector<double> Gaussian::evaluate_array(std::vector<double> x) {
  std::vector<double> y;
  for (auto &q : x) {
    double val = evaluate(q);
    if (val < 0)
      y.push_back(0);
    else
      y.push_back(val);
  }
  return y;
}

std::string Gaussian::to_string() {
  std::stringstream ss;
  ss << "Gaussian center=" << center_ << " height="  << height_ << " hwhm=" << hwhm_;
  return ss.str();
}



Peak::Peak(const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> &y_baseline) {
  fit(x, y, y_baseline);
}


void Peak::fit(const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> &y_baseline) {
  if (
      (x.size() == y.size())
      &&
      (y_baseline.size() == y.size())
      )
  {
    x_ = x;
    y_ = y;
    y_baseline_ = y_baseline;
    std::vector<double> nobase(x_.size());
    
    for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i)
      nobase[i] = y_[i] - y_baseline_[i];
    
    gaussian_ = Gaussian(x_, nobase);

    y_fullfit_.resize(x_.size());
    for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i)
      y_fullfit_[i] = y_baseline_[i] + gaussian_.evaluate(x_[i]);
  }
}



std::vector<double> UtilXY::make_baseline(uint16_t left, uint16_t right, uint16_t BL_samples, BaselineType type) {
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

double UtilXY::local_avg(uint16_t chan, uint16_t samples) {
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


UtilXY::UtilXY(const std::vector<double> &x, const std::vector<double> &y, uint16_t min, uint16_t max, uint16_t avg_window) :
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

void UtilXY::set_mov_avg(uint16_t window) {
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


void UtilXY::deriv() {
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


void UtilXY::find_prelim() {
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

void UtilXY::filter_prelim(uint16_t min_width) {
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

void UtilXY::refine_edges(double threshl, double threshr) {
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

uint16_t UtilXY::find_left(uint16_t chan, uint16_t grace) {
  if ((chan - grace < 0) || (chan >= x_.size()))
    return 0;

  int i = chan - grace;
  while ((i >= 0) && (deriv1[i] > 0))
    i--;
  return i;

  
}

uint16_t UtilXY::find_right(uint16_t chan, uint16_t grace) {
  if ((chan < 0) || (chan + grace >= x_.size()))
    return x_.size() - 1;

  int i = chan + grace;
  while ((i < x_.size()) && (deriv1[i] < 0))
    i++;
  return i;
}

void UtilXY::find_peaks(int min_width) {
  find_prelim();
  filter_prelim(min_width);

  peaks_.clear();
  for (int i=0; i < filtered.size(); ++i) {
    std::vector<double> baseline = make_baseline(lefts[i], rights[i], 3);
    std::vector<double> xx(x_.begin() + lefts[i], x_.begin() + rights[i] + 1);
    std::vector<double> yy(y_.begin() + lefts[i], y_.begin() + rights[i] + 1);
    Peak fitted = Peak(xx, yy, baseline);

    if (fitted.gaussian_.height_ > 0)
      peaks_.push_back(fitted);
  }

  PL_INFO << "Preliminary search found " << prelim.size() << " potential peaks";
  PL_INFO << "After minimum width filter: " << filtered.size();
  PL_INFO << "Fitted peaks: " << peaks_.size();
}
