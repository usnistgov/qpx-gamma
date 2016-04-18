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
 *      FitykUtil -
 *
 ******************************************************************************/

#include "fit_param.h"
#include <boost/lexical_cast.hpp>
#include "custom_logger.h"
#include <iomanip>
#include <numeric>

std::string FitParam::fityk_name(int function_num) const {
  std::string ret  = "$" + name_ + "_";
  if (function_num > -1)
    ret += boost::lexical_cast<std::string>(function_num);
  return ret;
}

std::string FitParam::def_bounds() const {

  std::string ret = "~";
  ret += boost::lexical_cast<std::string>(val) +
         " [" + boost::lexical_cast<std::string>(lbound) +
          ":" + boost::lexical_cast<std::string>(ubound) + "]";
  return ret;
}

std::string FitParam::def_var(int function_num) const {
  std::string ret  = fityk_name(function_num) + " = " + def_bounds();
  return ret;
}


double FitParam::get_err(fityk::Fityk* f,
                          std::string funcname)
{
  std::string command = "%" + funcname + "." + name_ + ".error";
//  PL_DBG << "<FitParam> " << command;
  return f->calculate_expr(command);
}

bool FitParam::extract(fityk::Fityk* f, fityk::Func* func)
{
  try {
    val = func->get_param_value(name_);
    uncert = get_err(f, func->name);
//    PL_DBG << "<FitParam> " << name_ << " = " << val << " +/- " << uncert;
  } catch (...) {
    return false;
  }
  return true;
}

std::string FitParam::to_string() const {
  std::string ret = name_ + " = " + boost::lexical_cast<std::string>(val) +
      "\u00B1" + boost::lexical_cast<std::string>(uncert) +
      " [" + boost::lexical_cast<std::string>(lbound) +
       ":" + boost::lexical_cast<std::string>(ubound) + "]";
  return ret;
}

std::string FitParam::val_uncert(uint16_t precision) const
{
  std::stringstream ss;
  ss << std::setprecision( precision ) << val << " \u00B1 " << uncert;
  return ss.str();
}

double FitParam::err() const
{
  double ret = std::numeric_limits<double>::infinity();
  if (val != 0)
    ret = uncert / val * 100.0;
  return ret;
}

std::string FitParam::err(uint16_t precision) const
{
  std::stringstream ss;
  ss << std::setprecision( precision ) << err() << "%";
  return ss.str();
}


FitParam FitParam::enforce_policy() {
  FitParam ret = *this;
  if (!ret.enabled) {
    ret.ubound = ret.lbound;
    ret.lbound = 0;
    ret.val = ret.lbound;
  } else if (ret.fixed) {
    ret.ubound = ret.val + ret.lbound  * 0.01;
    ret.lbound = ret.val - ret.lbound  * 0.01;
  }
  return ret;
}

FitParam FitParam::operator^(const FitParam &other) const
{
  double avg = (val + other.val) * 0.5;
  double min = avg;
  double max = avg;
  if (!std::isnan(uncert)) {
    min = std::min(avg, val - 0.5 * uncert);
    max = std::max(avg, val + 0.5 * uncert);
  }
  if (!std::isnan(other.uncert)) {
    min = std::min(avg, other.val - 0.5 * other.uncert);
    max = std::max(avg, other.val + 0.5 * other.uncert);
  }
  FitParam ret;
  ret.val = avg;
  ret.uncert = max - min;
}

bool FitParam::operator%(const FitParam &other) const
{
  bool ret = false;
  if (!std::isnan(uncert) && (std::abs(val - other.val) <= uncert))
    ret = true;
  if (!std::isnan(other.uncert) && (std::abs(val - other.val) <= other.uncert))
    ret = true;
  return ret;
}




