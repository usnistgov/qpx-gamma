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
 *      CoefFunction -
 *
 ******************************************************************************/

#ifndef COEF_FUNCTION_H
#define COEF_FUNCTION_H

#include <vector>
#include <map>
#include "fit_param.h"

#ifdef FITTER_ROOT_ENABLED
#endif

class CoefFunction {
public:
  CoefFunction();
  CoefFunction(std::vector<double> coeffs, double uncert, double rsq);

  //TO IMPLEMENT IN CHILDREN

  virtual std::string type() const = 0;
  virtual std::string to_string() const = 0;
  virtual std::string to_UTF8(int precision = -1, bool with_rsq = false) = 0;
  virtual std::string to_markup(int precision = -1, bool with_rsq = false) = 0;
  virtual double eval(double x)  = 0;
  virtual double derivative(double x) = 0;

  void add_coeff(int degree, double lbound, double ubound);
  void add_coeff(int degree, double lbound, double ubound, double initial);

  std::vector<double> coeffs();

  std::vector<double> eval_array(const std::vector<double> &x);
  double eval_inverse(double y, double e = 0.2);

  std::map<int, FitParam> coeffs_;
  FitParam xoffset_;
  double rsq_; //chisquared, actually

  void fit(const std::vector<double> &x,
           const std::vector<double> &y,
           const std::vector<double> &x_sigma,
           const std::vector<double> &y_sigma);

// Fityk emplementation
  virtual std::string fityk_definition() = 0;
  bool add_self(fityk::Fityk *f, int function_num = -1) const;
  bool extract_params(fityk::Fityk* f, fityk::Func*);
  void fit_fityk(const std::vector<double> &x,
                 const std::vector<double> &y,
                 const std::vector<double> &y_sigma);

#ifdef FITTER_ROOT_ENABLED
// Root implementation
  void fit_root(const std::vector<double> &x,
                const std::vector<double> &y,
                const std::vector<double> &x_sigma,
                const std::vector<double> &y_sigma);

  virtual std::string root_definition() = 0;

#endif

};

#endif
