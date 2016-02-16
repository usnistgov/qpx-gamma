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

#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

#include <vector>
#include <iostream>
#include <numeric>
#include <map>
#include "fityk.h"
#include "fit_param.h"

class Polynomial {
public:
  Polynomial() : rsq_(0), xoffset_(0) {}
  Polynomial(std::vector<double> coeffs, double xoffset = 0, double rsq = 0);
  Polynomial(std::vector<double> &x, std::vector<double> &y, uint16_t degree, double xoffset = 0);
  Polynomial(std::vector<double> &x, std::vector<double> &y, std::vector<uint16_t> &degrees, double xoffset = 0);

  void fit(std::vector<double> &x, std::vector<double> &y, std::vector<uint16_t> &degrees, double xoffset = 0);

  Polynomial derivative();

  std::string to_string(bool with_rsq = false);
  std::string to_UTF8(int precision = -1, bool with_rsq = false);
  std::string to_markup(int precision = -1, bool with_rsq = false);
  std::string coef_to_string() const;
  void coef_from_string(std::string);

  double eval(double x);
  std::vector<double> eval(std::vector<double> x);

  double eval_inverse(double y, double e = 0.2);
  
  std::vector<double> coeffs_;
  double xoffset_;
  double rsq_;


  static std::string fityk_definition(const std::vector<uint16_t> &degrees, double xoffset = 0);
  bool extract_params(fityk::Func*, const std::vector<uint16_t> &degrees);
};

class PolyBounded {
public:
  PolyBounded();
  PolyBounded(Polynomial p);

  void add_coeff(int degree, double lbound, double ubound);
  void add_coeff(int degree, double lbound, double ubound, double initial);

  void fit(std::vector<double> &x, std::vector<double> &y, std::vector<double> &y_sigma);
  bool add_self(fityk::Fityk *f, int function_num = -1) const;

  std::string fityk_definition();
  bool extract_params(fityk::Func*);

  Polynomial to_simple() const;
  std::string to_string() const;

  double eval(double x);
  std::vector<double> eval(std::vector<double> x);

  PolyBounded derivative();
  double eval_inverse(double y, double e = 0.2);

  std::map<int, FitParam> coeffs_;
  FitParam xoffset_;
  double rsq_;
};

#endif
