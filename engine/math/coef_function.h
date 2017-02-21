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
#include "xmlable.h"

class CoefFunction : public XMLable
{
public:
  CoefFunction();
  CoefFunction(std::vector<double> coeffs, double uncert, double rsq);

  FitParam xoffset() const;
  double chi2() const;
  size_t coeff_count() const;
  std::vector<int> powers() const;
  FitParam get_coeff(int degree) const;
  std::map<int, FitParam> get_coeffs() const;
  std::vector<double> coeffs_consecutive() const;

  void set_xoffset(const FitParam& o);
  void set_chi2(double);
  void set_coeff(int degree, const FitParam& p);
  void set_coeff(int degree, const UncertainDouble& p);
  void add_coeff(int degree, double lbound, double ubound);
  void add_coeff(int degree, double lbound, double ubound, double initial);

  double eval_inverse(double y, double e = 0.2) const;
  std::vector<double> eval_array(const std::vector<double> &x) const;

  //XMLable
  void to_xml(pugi::xml_node &node) const override;
  void from_xml(const pugi::xml_node &node) override;
  std::string xml_element_name() const override {return this->type();}

  //TO IMPLEMENT IN CHILDREN
  virtual std::string type() const = 0;
  virtual std::string to_string() const = 0;
  virtual std::string to_UTF8(int precision = -1, bool with_rsq = false) const = 0;
  virtual std::string to_markup(int precision = -1, bool with_rsq = false) const = 0;

  virtual double eval(double x) const = 0;
  virtual double derivative(double x) const = 0;

protected:
  std::map<int, FitParam> coeffs_;
  FitParam xoffset_ {"xoffset", 0};
  double chi2_ {0};

};

#endif
