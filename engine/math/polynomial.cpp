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
#include "qpx_util.h"

Polynomial::Polynomial(std::vector<double> coeffs, double xoffset)
  :Polynomial()
{
  xoffset_ = xoffset;
  int highest = -1;
  for (int i=0; i < coeffs.size(); ++i)
    if (coeffs[i] != 0)
      highest = i;
  for (int i=0; i <= highest; ++i)
    coeffs_.push_back(coeffs[i]);
}

Polynomial::Polynomial(std::vector<double> &x, std::vector<double> &y, uint16_t degree, double xoffset)
  :Polynomial()
{
  std::vector<uint16_t> degrees;
  for (int i=0; i <= degree; ++i)
    degrees.push_back(i);

  fit(x, y, degrees, xoffset);
}

Polynomial::Polynomial(std::vector<double> &x, std::vector<double> &y,
                       std::vector<uint16_t> &degrees, double xoffset) {
  fit(x, y, degrees, xoffset);
}


void Polynomial::fit(std::vector<double> &x, std::vector<double> &y,
                       std::vector<uint16_t> &degrees, double xoffset) {
  xoffset_ = xoffset;
  coeffs_.clear();

  if (x.size() != y.size())
    return;

  std::vector<double> x_c, sigma;
  sigma.resize(x.size(), 1);
  
  for (auto &q : x)
    x_c.push_back(q - xoffset_);

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);

  f->load_data(0, x_c, y, sigma);

  std::string definition = "define MyPoly(";
  std::vector<std::string> varnames;
  for (int i=0; i < degrees.size(); ++i) {
    std::stringstream ss;
    ss << degrees[i];
    varnames.push_back(ss.str());
    definition += (i==0)?"":",";
    definition += "a" + ss.str() + "=~0";
  }
  definition += ") = ";
  for (int i=0; i<varnames.size(); ++i) {
    definition += (i==0)?"":"+";
    definition += "a" + varnames[i] + "*x^" + varnames[i];
  }


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
    int highest = 0;
    for (auto &q : degrees)
      if (q > highest)
        highest = q;

    fityk::Func* lastfn = f->all_functions().back();
    coeffs_.resize(highest + 1, 0);
    for (auto &q : degrees) {
      std::stringstream ss;
      ss << q;
      coeffs_[q] = lastfn->get_param_value("a" + ss.str());
    }

    rsq = f->get_rsquared(0);
  }

  delete f;
}


double Polynomial::eval(double x) {
  double x_adjusted = x - xoffset_;
  double result = 0.0;
  for (int i=0; i < coeffs_.size(); i++)
    result += coeffs_[i] * pow(x_adjusted, i);
  return result;
}

Polynomial Polynomial::derivative() {
  Polynomial new_poly;
  new_poly.xoffset_ = xoffset_;

  int highest = 0;
  for (int i=0; i < coeffs_.size(); ++i)
    if (coeffs_[i] != 0)
      highest = i;

  if (highest == 0)
    return new_poly;

  new_poly.coeffs_.resize(highest, 0);
  for (int i=1; i <= highest; ++ i)
    new_poly.coeffs_[i-1] = i * coeffs_[i];

  return new_poly;
}

double Polynomial::eval_inverse(double y, double e) {
  int i=0;
  double x0=1;
  Polynomial deriv = derivative();
  double x1 = x0 + (y - eval(x0)) / (deriv.eval(x0));
  while( i<=100 && std::abs(x1-x0) > e)
  {
    x0 = x1;
    x1 = x0 + (y - eval(x0)) / (deriv.eval(x0));
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


std::vector<double> Polynomial::eval(std::vector<double> x) {
  std::vector<double> y;
  for (auto &q : x)
    y.push_back(eval(q));
  return y;
}

std::string Polynomial::to_string() {
  std::stringstream ss;
  for (int i=0; i < coeffs_.size(); i++) {
    if (i > 0)
      ss << " + ";
    ss << coeffs_[i];
    if (i > 0)
      ss << "x";
    if (i > 1)
      ss << "^" << i;
  }
  return ss.str();
}

std::string Polynomial::to_UTF8(int precision, bool with_rsq) {

  std::string calib_eqn;
  for (int i=0; i <= coeffs_.size(); i++) {
    if (i > 0)
      calib_eqn += " + ";
    calib_eqn += to_str_precision(coeffs_[i]);
    if (i > 0)
      calib_eqn += "x";
    if (i > 1)
      calib_eqn += UTF_superscript(i);
  }

  if (with_rsq)
    calib_eqn += std::string("   r")
        + UTF_superscript(2)
        + std::string("=")
        + to_str_precision(rsq, 4);
  
  return calib_eqn;
}

std::string Polynomial::to_markup(int precision) {
  std::string calib_eqn;
  for (int i=0; i <= coeffs_.size(); i++) {
    if (i > 0)
      calib_eqn += " + ";
    calib_eqn += to_str_precision(coeffs_[i], precision);
    if (i > 0)
      calib_eqn += "x";
    if (i > 1)
      calib_eqn += "<sup>" + UTF_superscript(i) + "</sup>";
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
