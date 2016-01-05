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
 *      LogInverse -
 *
 ******************************************************************************/

#ifndef LOG_INVERSE_H
#define LOG_INVERSE_H

#include <vector>
#include <iostream>
#include <numeric>


class LogInverse {
public:
  LogInverse() : degree_(-1), xoffset_(0.0), rsq(-1) {}
  LogInverse(std::vector<double> coeffs, double center = 0);

  LogInverse(std::vector<double> &x, std::vector<double> &y,
             uint16_t degree, double center=0);
  LogInverse(std::vector<double> &x, std::vector<double> &y,
             std::vector<uint16_t> &degrees, double center=0);

  void fit(std::vector<double> &x, std::vector<double> &y,
      std::vector<uint16_t> &degrees, double center=0);

  //LogInverse derivative();

  std::string to_string();
  std::string to_UTF8(int precision = -1, bool with_rsq = false);
  std::string to_markup();
  std::string coef_to_string() const;
  void coef_from_string(std::string);

  double evaluate(double x);
//  double inverse_evaluate(double y, double e = 0.2);
  std::vector<double> evaluate_array(std::vector<double> x);
  
  std::vector<double> coeffs_;
  double xoffset_;
  int degree_;
  double rsq;

  std::string to_str_precision(double, int); //move this somewhere else
};

#endif
