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
 *      Effit -
 *
 ******************************************************************************/

#ifndef EFFIT_H
#define EFFIT_H

#include <vector>
#include <iostream>
#include <numeric>


class Effit {
public:
  Effit() : xoffset_(0.0), rsq(-1), A(0), B(1), C(0), D(0), E(0), F(0), G(20) {}
  Effit(std::vector<double> coeffs, double center = 0);

  Effit(std::vector<double> &x, std::vector<double> &y, double center=0);

  void fit(std::vector<double> &x, std::vector<double> &y, double center=0);

  //Effit derivative();

  std::vector<double> coeffs();

  std::string to_string();
  std::string to_UTF8(int precision = -1, bool with_rsq = false);
  std::string to_markup();
  std::string coef_to_string() const;
  void coef_from_string(std::string);

  double evaluate(double x);
//  double inverse_evaluate(double y, double e = 0.2);
  std::vector<double> evaluate_array(std::vector<double> x);
  
  double xoffset_;
  double rsq;

  double A, B, C, D, E, F, G;
};

#endif
