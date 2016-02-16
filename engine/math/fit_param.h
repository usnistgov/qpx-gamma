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

class FitParam {

public:
  FitParam() :
    FitParam("", 0)
  {}

  FitParam(std::string name, double v) :
    FitParam(name, v,
             std::numeric_limits<double>::min(),
             std::numeric_limits<double>::max())
  {}

  FitParam(std::string name, double v, double lower, double upper) :
    name_(name),
    val(v),
    uncert(std::numeric_limits<double>::infinity()),
    lbound(lower),
    ubound(upper),
    enabled(true),
    fixed(false)
  {}

  FitParam enforce_policy();

  std::string name() const {return name_;}
  std::string fityk_name(int function_num) const;

  std::string def_bounds() const;
  std::string def_var(int function_num = -1) const;
  bool extract(fityk::Fityk* f, fityk::Func* func);

  std::string to_string() const;

  double val, uncert, lbound, ubound;
  bool enabled, fixed;

private:
  std::string name_;

  double get_err(fityk::Fityk* f,
                 std::string funcname);

};

#endif
