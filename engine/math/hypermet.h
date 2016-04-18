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
#include "fit_settings.h"
#include "gaussian.h"
#include "val_uncert.h"


class Hypermet {
public:
  Hypermet() :
    Hypermet(Gaussian(), FitSettings())
  {}


  Hypermet(Gaussian gauss, FitSettings settings);

  Hypermet(const std::vector<double> &x, const std::vector<double> &y,
           Gaussian gauss,
           FitSettings settings);

  static std::vector<Hypermet> fit_multi(const std::vector<double> &x,
                                         const std::vector<double> &y,
                                         std::vector<Hypermet> old,
                                         PolyBounded &background,
                                         FitSettings settings
                                         );

  std::string to_string() const;

  double eval_peak(double);
  double eval_step_tail(double);

  std::vector<double> peak(std::vector<double> x);
  std::vector<double> step_tail(std::vector<double> x);
  ValUncert area() const;

  FitParam center_, height_, width_,
           Lskew_amplitude, Lskew_slope,
           Rskew_amplitude, Rskew_slope,
           tail_amplitude, tail_slope,
           step_amplitude;

  double rsq_;

  static std::string fityk_definition();
  bool extract_params(fityk::Fityk*, fityk::Func*);
};

#endif
