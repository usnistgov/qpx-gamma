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
 *      Voigt type functions
 *
 ******************************************************************************/

#ifndef HYPERMET_H
#define HYPERMET_H

#include <vector>
#include <iostream>
#include <numeric>
#include "fityk.h"


class Hypermet {
public:
  Hypermet() : height_(0), center_(0), width_(0),
    Lshort_height_(0), Lshort_slope_(0.3),
    Rshort_height_(0), Rshort_slope_(0.3),
    Llong_height_(0), Llong_slope_(2.5),
    step_height_(0),
    rsq(-1) {}

  Hypermet(double h, double c, double w) : height_(h), center_(c), width_(w / sqrt(log(2.0))),
    Lshort_height_(0), Lshort_slope_(0.3),
    Rshort_height_(0), Rshort_slope_(0.3),
    Llong_height_(0), Llong_slope_(2.5),
    step_height_(0),
    rsq(-1) {}
  Hypermet(const std::vector<double> &x, const std::vector<double> &y, double h, double c, double w);

  static std::vector<Hypermet> fit_multi(const std::vector<double> &x,
                                         const std::vector<double> &y,
                                         const std::vector<Hypermet> &old,
                                         bool constrain_to_first);

  double evaluate(double x);
  std::vector<double> evaluate_array(std::vector<double> x);
  
  double center_, height_, width_,
        Lshort_height_, Lshort_slope_,
        Rshort_height_, Rshort_slope_,
        Llong_height_, Llong_slope_,
        step_height_;
  double rsq;

  static std::string fityk_definition();
  bool extract_params(fityk::Func*);
};

#endif
