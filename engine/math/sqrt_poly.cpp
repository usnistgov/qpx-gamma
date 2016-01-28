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
 *      SqrtPoly -
 *
 ******************************************************************************/

#include "sqrt_poly.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <boost/algorithm/string.hpp>
#include "fityk.h"
#include "custom_logger.h"
#include "qpx_util.h"

SqrtPoly::SqrtPoly(std::vector<double> coeffs) //deprecate
  :SqrtPoly()
{
  for (int i=0; i < coeffs.size(); ++i)
    if (coeffs[i] > 0)
      coeffs_[i] = coeffs[i];
}

SqrtPoly::SqrtPoly(std::map<uint16_t, double> coeffs)
  :SqrtPoly()
{
  coeffs_ = coeffs;
}

SqrtPoly::SqrtPoly(std::vector<double> &x, std::vector<double> &y, uint16_t degree)
  :SqrtPoly()
{
  std::vector<uint16_t> degrees;
  for (int i=0; i <= degree; ++i)
    degrees.push_back(i);

  fit(x, y, degrees);
}

SqrtPoly::SqrtPoly(std::vector<double> &x, std::vector<double> &y,
                       std::vector<uint16_t> &degrees) {
  fit(x, y, degrees);
}


void SqrtPoly::fit(std::vector<double> &x, std::vector<double> &y,
                       std::vector<uint16_t> &degrees) {
  if (x.size() != y.size())
    return;

  std::vector<double> yy, sigma;
  sigma.resize(x.size(), 1);
  
  for (auto &q : y)
    yy.push_back(q*q);


  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);

  f->load_data(0, x, yy, sigma);

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
    fityk::Func* lastfn = f->all_functions().back();
    for (auto &q : degrees) {
      std::stringstream ss;
      ss << q;
      coeffs_[q] = lastfn->get_param_value("a" + ss.str());
    }
    rsq = f->get_rsquared(0);
  }

  delete f;
}


double SqrtPoly::eval(double x) {
  double result = 0.0;
  for (auto &q: coeffs_)
    result += q.second * pow(x, q.first);
  result = sqrt(result);
  return exp(result);
}

/*
SqrtPoly SqrtPoly::derivative() {
  SqrtPoly new_poly;
  new_poly.xoffset_ = xoffset_;
  if (degree_ > 0)
    new_poly.coeffs_.resize(coeffs_.size() - 1, 0);
  for (int i=1; i < coeffs_.size(); ++ i)
    new_poly.coeffs_[i-1] = i * coeffs_[i];
  new_poly.degree_ = degree_ - 1;
  return new_poly;
}

double SqrtPoly::inverse_evaluate(double y, double e) {
  int i=0;
  double x0=1;
  SqrtPoly deriv = derivative();
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
    PL_WARN <<"Maximum iteration reached in SqrtPoly inverse evaluation";
    return nan("");
  }
}
*/


std::vector<double> SqrtPoly::eval(std::vector<double> x) {
  std::vector<double> y;
  for (auto &q : x)
    y.push_back(eval(q));
  return y;
}

std::string SqrtPoly::to_string() {
  std::stringstream ss;
  ss << "sqrt(";
  bool first = true;
  for (auto &q: coeffs_) {
    if (!first)
      ss << " + ";
    else
      first = false;
    ss << q.second;
    if (q.first > 0)
      ss << "x";
    if (q.first > 1)
      ss << "^" << q.first;
  }
  ss << ")";
  return ss.str();
}

std::string SqrtPoly::to_UTF8(int precision, bool with_rsq) {

  bool first = true;
  std::string calib_eqn = "sqrt(";
  for (auto &q: coeffs_) {
    if (!first)
      calib_eqn += " + ";
    else
      first = false;
    calib_eqn += to_str_precision(q.second, precision);
    if (q.first > 0)
      calib_eqn += "x";
    if (q.first > 1)
      calib_eqn += UTF_superscript(q.first);
  }

  if (with_rsq)
    calib_eqn += std::string("  r")
        + UTF_superscript(2)
        + std::string("=")
        + to_str_precision(rsq, 4);
  
  calib_eqn += ")";
  return calib_eqn;
}

std::string SqrtPoly::to_markup(int precision) {
  std::string calib_eqn = "sqrt(";
  bool first = true;

  for (auto &q: coeffs_) {
    if (!first)
      calib_eqn += " + ";
    else
      first = false;
    calib_eqn += to_str_precision(q.second, precision);
    if (q.first > 0)
      calib_eqn += "x";
    if (q.first > 1)
      calib_eqn += "<sup>" + std::to_string(q.first) + "</sup>";
  }

  calib_eqn += ")";
  return calib_eqn;
}

std::string SqrtPoly::coef_to_string() const{
  std::stringstream dss;
  dss.str(std::string());
  for (auto &q : coeffs_) {
    dss << q.first << ":" << q.second << " ";
  }
  return boost::algorithm::trim_copy(dss.str());
}

void SqrtPoly::coef_from_string(std::string coefs) {
  std::stringstream dss(boost::algorithm::trim_copy(coefs));

  std::list<double> templist; double coef;
  while (dss.rdbuf()->in_avail()) {
    dss >> coef;
    templist.push_back(coef);
  }

  int i=0;
  for (auto &q: templist) {
    coeffs_[i] = q;
    i++;
  }
}
