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
 *      Gaussian
 *
 ******************************************************************************/

#ifndef GAUSSIAN_H
#define GAUSSIAN_H

#include <vector>
#include <iostream>
#include <numeric>
#include "fityk.h"


class Gaussian {
public:
  Gaussian() : height_(0), hwhm_(0), center_(0), rsq(-1) {}
  Gaussian(const std::vector<double> &x, const std::vector<double> &y);

  static std::vector<Gaussian> fit_multi(const std::vector<double> &x, const std::vector<double> &y, const std::vector<Gaussian> &old);

  double evaluate(double x);
  std::vector<double> evaluate_array(std::vector<double> x);
  double area();
  
  double height_, hwhm_, center_;
  double rsq;

  static std::string fityk_definition();
  bool extract_params(fityk::Func*);
};


#endif

