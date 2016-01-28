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

#ifndef SQRT_POLY_H
#define SQRT_POLY_H

#include <vector>
#include <iostream>
#include <numeric>
#include <map>


class SqrtPoly {
public:
  SqrtPoly() : rsq(-1) {}
  SqrtPoly(std::vector<double> coeffs); //deprecate
  SqrtPoly(std::map<uint16_t, double> coeffs);

  SqrtPoly(std::vector<double> &x, std::vector<double> &y, uint16_t degree);

  SqrtPoly(std::vector<double> &x, std::vector<double> &y, std::vector<uint16_t> &degrees);
  void fit(std::vector<double> &x, std::vector<double> &y, std::vector<uint16_t> &degrees);

  //output
  std::string to_string();
  std::string to_UTF8(int precision = -1, bool with_rsq = false);
  std::string to_markup(int precision = -1);
  std::string coef_to_string() const;
  void coef_from_string(std::string);

  double eval(double x);
  double eval_deriv(double x);
  std::vector<double> eval(std::vector<double> x);

  //  double inverse_evaluate(double y, double e = 0.2);
  
  std::map<uint16_t, double> coeffs_;
  double rsq;

};

#endif
