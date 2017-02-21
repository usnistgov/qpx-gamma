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
#include "polynomial.h"
#include "fit_settings.h"

class Gaussian {
public:
  Gaussian();

  std::string to_string() const;

  double evaluate(double x);
  std::vector<double> evaluate_array(std::vector<double> x);
  UncertainDouble area() const;

  const FitParam& center() const {return center_;}
  const FitParam& height() const {return height_;}
  const FitParam& hwhm() const {return hwhm_;}

  void set_center(const UncertainDouble &ncenter);
  void set_height(const UncertainDouble &nheight);
  void set_hwhm(const UncertainDouble &nwidth);

  void set_center(const FitParam &ncenter);
  void set_height(const FitParam &nheight);
  void set_hwhm(const FitParam &nwidth);

  void constrain_center(double min, double max);
  void constrain_height(double min, double max);
  void constrain_hwhm(double min, double max);

  void bound_center(double min, double max);
  void bound_height(double min, double max);
  void bound_hwhm(double min, double max);

  void set_chi2(double);
  
protected:
  FitParam height_ {"height", 0};
  FitParam center_ {"center", 0};
  FitParam hwhm_ {"hwhm", 0};
  double chi2_ {0};
};


#endif

