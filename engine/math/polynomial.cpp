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
 *      Polynomial - 
 *
 ******************************************************************************/

#include "polynomial.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <boost/algorithm/string.hpp>
#include "fityk.h"
#include "custom_logger.h"

std::string to_str_precision(double x, int n) {
  std::ostringstream ss;
  if (n < 0)
    ss << x;
  else 
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
    rsq = f->get_rsquared(0);
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

Polynomial Polynomial::derivative() {
  Polynomial new_poly;
  new_poly.xoffset_ = xoffset_;
  if (degree_ > 0)
    new_poly.coeffs_.resize(coeffs_.size() - 1, 0);
  for (int i=1; i < coeffs_.size(); ++ i)
    new_poly.coeffs_[i-1] = i * coeffs_[i];
  new_poly.degree_ = degree_ - 1;
  return new_poly;
}

double Polynomial::inverse_evaluate(double y, double e) {
  int i=0;
  double x0=1;
  Polynomial deriv = derivative();
  double x1 = x0 + (y - evaluate(x0)) / (deriv.evaluate(x0));
  while( i<=100 && std::abs(x1-x0) > e)
  {
    x0 = x1;
    x1 = x0 + (y - evaluate(x0)) / (deriv.evaluate(x0));
    i++;
  }

  double x_adjusted = x1 - xoffset_;

  if(std::abs(x1-x0) <= e)
    return x_adjusted;

  else
  {
    PL_WARN <<"Maximum iteration reached in polynomial inverse evaluation";
    return nan("");
  }
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

std::string Polynomial::to_UTF8(int precision, bool with_rsq) {
  std::vector<std::string> superscripts = {
    "\u2070", "\u00B9", "\u00B2",
    "\u00B3", "\u2074", "\u2075",
    "\u2076", "\u2077", "\u2078",
    "\u2079"
  };

  std::string calib_eqn;
  if (degree_ >= 0)
    calib_eqn += to_str_precision(coeffs_[0], precision);
  for (uint16_t i=1; i <= degree_; i++) {
    calib_eqn += std::string(" + ");
    calib_eqn += to_str_precision(coeffs_[i], precision);
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

  if (with_rsq)
    calib_eqn += std::string("   r")
        + superscripts[2]
        + std::string("=")
        + to_str_precision(rsq, 4);
  
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

std::string Polynomial::coef_to_string() const{
  std::stringstream dss;
  dss.str(std::string());
  for (auto &q : coeffs_) {
    dss << q << " ";
  }
  return boost::algorithm::trim_copy(dss.str());
}

void Polynomial::coef_from_string(std::string coefs) {
  std::stringstream dss(boost::algorithm::trim_copy(coefs));

  std::list<double> templist; double coef;
  while (dss.rdbuf()->in_avail()) {
    dss >> coef;
    templist.push_back(coef);
  }

  coeffs_.resize(templist.size());
  int i=0;
  for (auto &q: templist) {
    coeffs_[i] = q;
    i++;
  }
}
