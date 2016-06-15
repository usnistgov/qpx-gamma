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
 *      FitParam -
 *
 ******************************************************************************/

#ifndef FIT_PARAM_H
#define FIT_PARAM_H

#include "fityk.h"
#include <string>
#include <limits>
#include "xmlable.h"
#include "UncertainDouble.h"

class FitParam : public XMLable {

public:
  FitParam() :
    FitParam("", std::numeric_limits<double>::quiet_NaN())
  {}

  FitParam(std::string name, double v) :
    FitParam(name, v,
             std::numeric_limits<double>::min(),
             std::numeric_limits<double>::max())
  {}

  FitParam(std::string name, double v, double lower, double upper) :
    name_(name),
    value(UncertainDouble::from_double(v, std::numeric_limits<double>::quiet_NaN())),
    lbound(lower),
    ubound(upper),
    enabled(true),
    fixed(false)
  {}

  FitParam enforce_policy();

  bool same_bounds_and_policy(const FitParam &other) const;

  FitParam operator ^ (const FitParam &other) const;
  bool operator % (const FitParam &other) const;
  bool operator == (const FitParam &other) const {return value == other.value;}
  bool operator < (const FitParam &other) const {return value < other.value;}

  std::string name() const {return name_;}

  std::string to_string() const;
//  std::string val_uncert(uint16_t precision) const;
//  std::string err(uint16_t precision) const;
//  double err() const;


  void to_xml(pugi::xml_node &node) const override;
  void from_xml(const pugi::xml_node &node) override;
  std::string xml_element_name() const override {return "FitParam";}


  UncertainDouble value;
  double lbound, ubound;
  bool enabled, fixed;

private:
  std::string name_;

//Fityk implementation

public:
  std::string fityk_name(int function_num) const;
  std::string def_bounds() const;
  std::string def_var(int function_num = -1) const;
  bool extract(fityk::Fityk* f, fityk::Func* func);

private:
  double get_err(fityk::Fityk* f,
                 std::string funcname);


// Ceres implementation


};

#endif
