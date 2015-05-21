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
  for (int i = 0; i < coeffs.size(); i++)
    if (coeffs[i])
      deg = i;
  degree_ = deg;
}

Polynomial::Polynomial(std::vector<double> &x, std::vector<double> &y, uint16_t degree, double center) {
  degree_ = 0; xoffset_ = center;
  if (x.size() != y.size())
    return;

  std::vector<double> x_c, sigma(x.size(), 1);
  for (auto &q : x)
    x_c.push_back(q - center);

  fityk::Fityk *f = new fityk::Fityk;
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
  for (auto &q : x)
    y.push_back(evaluate(q));
  return y;
}

std::ostream &operator<<(std::ostream &out, Gaussian d) {
  out << "Gaussian center=" << d.center_ << " height="  << d.height_ << " hwhm=" << d.hwhm_;
}


Peak::Peak(std::vector<double> &x, std::vector<double> &y, int min, int max) {
  if (
          (x.size() == y.size())
          &&
          (min > -1) && (min < x.size())
          &&
          (max > -1) && (max < x.size())
      )
  {
    for (int i = min; i < (max+1); ++i) {
      x_.push_back(x[i]);
      y_.push_back(y[i]);
    }
    rough_ = find_g(x_, y_);

    for (int i = 0; i < y_.size(); ++i)
      diff_y_.push_back(y_[i] - rough_.evaluate(x_[i]));

    fill0_linear(3, 20);

    baseline_ = Polynomial(x_, filled_y_, 9, rough_.center_);

    for (int i = 0; i < y_.size(); ++i)
      y_nobase_.push_back(y_[i] - baseline_.evaluate(x_[i]));

    /*
    refined_ = find_g(x_, filled_y_);

    for (int i = 0; i < y_.size(); ++i)
      y_fullfit_.push_back(filled_y_[i] + refined_.evaluate(x_[i]));
    */

    
    refined_ = find_g(x_, y_nobase_);

    for (int i = 0; i < y_.size(); ++i)
      y_fullfit_.push_back(baseline_.evaluate(x_[i]) + refined_.evaluate(x_[i]));
    

  }
}

Gaussian Peak::find_g(std::vector<double> &x, std::vector<double> &y) {
  std::vector<double> sigma(x.size(), 1);
  
  fityk::Fityk *f = new fityk::Fityk;
  f->load_data(0, x, y, sigma);
  f->execute("guess Gaussian");
  f->execute("fit");
  fityk::Func* lastfn = f->all_functions().back();
  Gaussian g = Gaussian(lastfn->get_param_value("center"),
                        lastfn->get_param_value("height"),
                        lastfn->get_param_value("hwhm"));
  delete f;
  return g;
}

void Peak::fill0_linear(int buffer, int sample) {
  filled_y_ = diff_y_;
  
  int center = rough_.center_ - x_[0];

  int first0 = center;
  int last0 = center;

  for(int i = center; i >= 0; --i)
    if (diff_y_[i] <= 0)
      first0 = i;

  for(int i = center; i < diff_y_.size(); ++i)
    if (diff_y_[i] <= 0)
      last0 = i;

  double edge = (last0 - first0) / 2.0;
  double edge_margin = edge * 1.75;
  
  int last_left = center - edge_margin;
  int first_right = center + edge_margin;

  double avg_left=0, avg_right=0;

  if ((last_left > 0) && (first_right+1 < diff_y_.size())) {
    for (int i = 0; i <= last_left; ++i)
      avg_left+=y_[i];
    for (int i = first_right; i < diff_y_.size(); ++i)
      avg_right+=y_[i];
    
    avg_left /= last_left;
    avg_right /= (diff_y_.size()-first_right);
  } else {
    avg_left = diff_y_[0];
    avg_right = diff_y_[diff_y_.size() - 1];
  }

  double slope = (avg_right - avg_left) / (first_right - last_left);
  for (int i = last_left; i <= first_right; i++)
    filled_y_[i] = avg_left + ((i - last_left) * slope);
}

void UtilXY::find_peaks(int min_width, int max_width) {
  if (y_.size() < 3)
    return;
  std::vector<int> temp_peaks;
  for (std::size_t i=1; (i+1) < y_.size(); i++) {
    if ((y_[i] > y_[i-1]) && (y_[i] > y_[i+1]))
      temp_peaks.push_back(i);
  }
  std::vector<int> peaks;
  for (int q = 0; q < y_.size(); q++) {
    bool left = true, right = true;
    for (int d = 1; d <= min_width; d++) {
      if (((q-d) >= 0) && (y_[q-d] >= y_[q-(d-1)]))
        left = false;
      if (((q+d) < y_.size()) && (y_[q+d] >= y_[q+(d-1)]))
        right = false;
    }
    if (left && right)
      peaks.push_back(q);
  }

  peaks_.clear();
  for (auto &q : peaks) {
    PL_DBG << "fitting peak at x=" << q;
    int xmin = q - max_width / 2;
    int xmax = q + max_width / 2;
    if (xmin < 0) xmin = 0;
    if (xmax >= x_.size()) xmax = x_.size() - 1;
    Peak fitted = Peak(x_, y_, xmin, xmax);
    peaks_.push_back(fitted);
  }
}
