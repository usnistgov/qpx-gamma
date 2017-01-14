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

  void fit(const std::vector<double> &x, const std::vector<double> &y);

  static std::vector<Gaussian> fit_multi(const std::vector<double> &x,
                                         const std::vector<double> &y,
                                         std::vector<Gaussian> old,
                                         Polynomial &background,
                                         FitSettings settings
                                         );

  std::string to_string() const;

  double evaluate(double x);
  std::vector<double> evaluate_array(std::vector<double> x);
  UncertainDouble area() const;
  
  FitParam height_, hwhm_, center_;
  double rsq_ {0};


  static std::string root_definition(uint16_t start = 0);
  static std::string root_definition(uint16_t a, uint16_t c, uint16_t w);

  void set_params(TF1* f, uint16_t start = 0) const;
  void set_params(TF1* f, uint16_t a, uint16_t c, uint16_t w) const;
  void get_params(TF1* f, uint16_t start = 0);
  void get_params(TF1* f, uint16_t a, uint16_t c, uint16_t w);
  void fit_root(const std::vector<double> &x, const std::vector<double> &y);
  static std::vector<Gaussian> fit_multi_root(const std::vector<double> &x,
                                              const std::vector<double> &y,
                                              std::vector<Gaussian> old,
                                              Polynomial &background,
                                              FitSettings settings);
  static std::vector<Gaussian> fit_multi_root_commonw(const std::vector<double> &x,
                                                      const std::vector<double> &y,
                                                      std::vector<Gaussian> old,
                                                      Polynomial &background,
                                                      FitSettings settings);

};


#endif

