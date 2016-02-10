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
#include "polynomial.h"
#include "calibration.h"
#include "fit_param.h"


class Hypermet {
public:
  Hypermet() :
    Hypermet(0,0,0)
  {}

  Hypermet(double h, double c, double w) :
    height_("hh", h), center_("cc", c), width_("ww", w),
    Lshort_height_("lshort_h", 1.0e-10, 1.0e-10, 0.75), Lshort_slope_("lshort_s", 0.5, 0.3, 2),
    Rshort_height_("rshort_h", 1.0e-10, 1.0e-10, 0.75), Rshort_slope_("rshort_s", 0.5, 0.3, 2),
    Llong_height_("llong_h", 0, 0, 0.015), Llong_slope_("llong_s", 2.5, 2.5, 50),
    step_height_("step_h", 1.0e-10, 1.0e-10, 0.75),
    rsq_(0) {}

  Hypermet(const std::vector<double> &x, const std::vector<double> &y, double h, double c, double w);

  static std::vector<Hypermet> fit_multi(const std::vector<double> &x,
                                         const std::vector<double> &y,
                                         std::vector<Hypermet> old,
                                         Polynomial &background,
                                         Gamma::Calibration cali_nrg,
                                         Gamma::Calibration cali_fwhm
                                         );

  double eval_peak(double);
  double eval_step_tail(double);

  std::vector<double> peak(std::vector<double> x);
  std::vector<double> step_tail(std::vector<double> x);
  double area();

  FitParam center_, height_, width_,
           Lshort_height_, Lshort_slope_,
           Rshort_height_, Rshort_slope_,
           Llong_height_, Llong_slope_,
           step_height_;

  double rsq_;

  static std::string fityk_definition();
  bool extract_params(fityk::Fityk*, fityk::Func*);
};

#endif
