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
#include "fityk.h"
#include "custom_logger.h"

Polynomial::Polynomial(std::vector<double> coeffs, double xoff) {
  xoffset_ = xoff;
  coeffs_ = coeffs;
  int deg = -1;
  for (size_t i = 0; i < coeffs.size(); i++)
    if (coeffs[i])
      deg = i;
  degree_ = deg;
}

Polynomial::Polynomial(std::vector<double> &x, std::vector<double> &y, uint16_t degree, double center) {
  degree_ = 0; xoffset_ = center;
  if (x.size() != y.size())
    return;

  std::vector<double> x_c, sigma;
  sigma.resize(x.size(), 1);
  
  for (auto &q : x)
    x_c.push_back(q - center);

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);

  f->load_data(0, x_c, y, sigma);
  degree_ = degree;

  std::string definition = "define MyPoly(a0=~0";
  std::vector<std::string> varnames(1, "0");
  for (int i=1; i<=degree_; ++i) {
    std::stringstream ss;
    ss << i;
    varnames.push_back(ss.str());
    definition += ",a" + ss.str() + "=~0";
  }
  definition += ") = a0";
  for (int i=1; i<=degree_; ++i)
    definition += " + a" + varnames[i] + "*x^" + varnames[i];
  
  f->execute(definition);
  f->execute("guess MyPoly");
  f->execute("fit");
  fityk::Func* lastfn = f->all_functions().back();
  coeffs_.resize(degree_+1);
  for (int i=0; i<=degree_; ++i)
    coeffs_[i] = lastfn->get_param_value("a" + varnames[i]);
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

std::ostream &operator<<(std::ostream &out, Polynomial d) {
  if (d.degree_ >=0 )
    out << d.coeffs_[0];
  for (int i=1; i <= d.degree_; i++)
    out << " + " << d.coeffs_[i] << "x^" << i;
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

std::ostream &operator<<(std::ostream &out, Gaussian d) {
  out << "Gaussian center=" << d.center_ << " height="  << d.height_ << " hwhm=" << d.hwhm_;
}


Peak::Peak(const std::vector<double> &x,const std::vector<double> &y, int min, int max) {
  if (
          (x.size() == y.size())
          &&
          (min > -1) && (min < x.size())
          &&
          (max > -1) && (max < x.size())
      )
  {
    for (int32_t i = min; i < (max+1); ++i) {
      x_.push_back(x[i]);
      y_.push_back(y[i]);
    }
    filled_y_.resize(x_.size());
    y_nobase_.resize(x_.size());
    y_fullfit_.resize(x_.size());

    rough_ = Gaussian(x_, y_);
//    PL_DBG << "Preliminary peak " << rough_;

    if ((rough_.center_ <= min)
        || (rough_.height_ <= 0)
        || (rough_.center_ > static_cast<double>(max))
        || (rough_.hwhm_ <= 0)) {
      err = 1;
//      PL_DBG << "Preliminary peak has unfeasable values";
      return;
    }
    
    for (size_t i = 0; i < y_.size(); ++i)
      filled_y_[i] = y_[i] - rough_.evaluate(x_[i]);

    int32_t buffer = 10;
    int32_t center = static_cast<size_t>(floor(rough_.center_ - x_[0]));

    int32_t first0 = static_cast<int32_t>(center);
    int32_t last0 = static_cast<int32_t>(center);

    for(int32_t i = center; i >= 0; i--)
      if (filled_y_[i] <= 0.0)
        first0 = i;

    for(size_t i = center; i < y_.size(); i++)
      if (filled_y_[i] <= 0.0)
        last0 = i;

    first0 -= buffer;
    last0  += buffer;

//    PL_DBG << "Peak likely between " << first0 << " and " << last0;
    
    if ((first0 <= 0) || ((last0+1) >= y_.size())) {
//      PL_DBG << "Cannot discern baseline at least on one side of peak";
      err = 2;
      return;
    }
    
    double avg_left=0, avg_right=0;

    for (int32_t i = 0; i <= first0; i++)
      avg_left+=y_[i];
    for (int32_t i = last0; i < static_cast<int32_t>(y_.size()); i++)
      avg_right+=y_[i];
    
    avg_left /= first0;
    avg_right /= (y_.size() - last0);

//    PL_DBG << "left baseline = " << avg_left << ", right_baseline = " << avg_right;

    if ((avg_left > rough_.height_) || (avg_right > rough_.height_)) {
//      PL_DBG << "Baseline above peak at least on one side";
      err = 3;
      return;
    }
    
    double slope = (avg_right - avg_left) / (last0 - first0);
    for (int32_t i = first0; i <= last0; i++)
      filled_y_[i] = avg_left + ((i - first0) * slope);

    
    for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i)
      y_nobase_[i] = y_[i] - filled_y_[i];

    
    refined_ = Gaussian(x_, y_nobase_);
    PL_DBG << "Preliminary peak " << rough_;
    PL_DBG << "Refined peak " << refined_;

    for (int32_t i = 0; i < static_cast<int32_t>(y_.size()); ++i)
      y_fullfit_[i] = filled_y_[i] + refined_.evaluate(x_[i]);

    double devi = rough_.center_ - refined_.center_;

    PL_DBG << "Center of refined gaussian fit deviates from that of rough fit by " << devi  << " chan";

    if (devi > (max-min))
      refined_ = Gaussian();
  }
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
    PL_ERR << "Fytik threw exception a";
    success = false;
  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    PL_ERR << "Fytik threw exception b";
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
  }

  delete f;
}

void UtilXY::mov_avg(uint16_t window) {
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
}


void UtilXY::find_peaks(int min_width, int max_width, uint16_t avg_window) {
  if (y_.size() < 3)
    return;

  mov_avg(avg_window);

  std::vector<int> temp_peaks;
  for (std::size_t i=1; (i+1) < y_avg_.size(); i++) {
    if ((y_avg_[i] > y_avg_[i-1]) && (y_avg_[i] > y_avg_[i+1]))
      temp_peaks.push_back(i);
  }
  std::vector<int> peaks;
  for (int q = 0; q < y_avg_.size(); q++) {
    bool left = true, right = true;
    for (int d = 1; d <= min_width; d++) {
      if (((q-d) >= 0) && (y_avg_[q-d] >= y_avg_[q-(d-1)]))
        left = false;
      if (((q+d) < y_avg_.size()) && (y_avg_[q+d] >= y_avg_[q+(d-1)]))
        right = false;
    }
    if (left && right)
      peaks.push_back(q);
  }

  int err1=0, err2=0, err3=0;
  peaks_.clear();
  for (auto &q : peaks) {
    //PL_DBG << "fitting peak at x=" << q;
    int xmin = q - max_width / 2;
    int xmax = q + max_width / 2;
    if (xmin < 0) xmin = 0;
    if (xmax >= x_.size()) xmax = x_.size() - 1;
    Peak fitted = Peak(x_, y_, xmin, xmax);
    if (fitted.err == 1)
      err1++;
    else if (fitted.err == 2)
      err2++;
    else if (fitted.err == 3)
      err3++;

    if (fitted.refined_.height_ > 0)
      peaks_.push_back(fitted);
  }

  PL_INFO << "Preliminary search found " << temp_peaks.size() << " potential peaks";
  PL_INFO << "After minimum width filter: " << peaks.size();
  PL_INFO << "Unfeasible prelim fit: " << err1;
  PL_INFO << "Indescernible baselines: " << err2;
  PL_INFO << "Baselines above peak height: " << err3;
  PL_INFO << "Fitted peaks: " << peaks_.size();
}
