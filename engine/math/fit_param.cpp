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
#include "qpx_util.h"

std::string FitParam::fityk_name(int function_num) const {
  std::string ret  = "$" + name_ + "_";
  if (function_num > -1)
    ret += boost::lexical_cast<std::string>(function_num);
  return ret;
}

std::string FitParam::def_bounds() const {
  std::string ret = "~";
  ret += boost::lexical_cast<std::string>(value.value()) +
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
//  DBG << "<FitParam> " << command;
  return f->calculate_expr(command);
}

bool FitParam::extract(fityk::Fityk* f, fityk::Func* func)
{
  try {
    value = UncertainDouble::from_double(func->get_param_value(name_),
                                         get_err(f, func->name));
//    DBG << "<FitParam> " << name_ << " = " << val << " +/- " << uncert;
  } catch (...) {
    return false;
  }
  return true;
}

std::string FitParam::to_string() const {
  std::string ret = name_ + " = " + value.to_string() +
      " [" + boost::lexical_cast<std::string>(lbound) +
       ":" + boost::lexical_cast<std::string>(ubound) + "]";
  return ret;
}

//std::string FitParam::val_uncert(uint16_t precision) const
//{
//  std::stringstream ss;
//  ss << std::setprecision( precision ) << val << " \u00B1 " << uncert;
//  return ss.str();
//}

//double FitParam::err() const
//{
//  double ret = std::numeric_limits<double>::infinity();
//  if (val != 0)
//    ret = uncert / val * 100.0;
//  return ret;
//}

//std::string FitParam::err(uint16_t precision) const
//{
//  std::stringstream ss;
//  ss << std::setprecision( precision ) << err() << "%";
//  return ss.str();
//}


FitParam FitParam::enforce_policy() {
  FitParam ret = *this;
  if (!ret.enabled) {
    ret.ubound = ret.lbound;
    ret.lbound = 0;
    ret.value.setValue(ret.lbound);
  } else if (ret.fixed) {
    ret.ubound = ret.value.value() + ret.lbound  * 0.01;
    ret.lbound = ret.value.value() - ret.lbound  * 0.01;
  }
  return ret;
}

FitParam FitParam::operator^(const FitParam &other) const
{
  double avg = (value.value() + other.value.value()) * 0.5;
  double min = avg;
  double max = avg;
  if (std::isfinite(value.uncertainty())) {
    min = std::min(avg, value.value() - 0.5 * value.uncertainty());
    max = std::max(avg, value.value() + 0.5 * value.uncertainty());
  }
  if (std::isfinite(other.value.uncertainty())) {
    min = std::min(avg, other.value.uncertainty() - 0.5 * other.value.uncertainty());
    max = std::max(avg, other.value.uncertainty() + 0.5 * other.value.uncertainty());
  }
  FitParam ret;
  ret.value = UncertainDouble::from_double(avg, max - min);
}

bool FitParam::operator%(const FitParam &other) const
{
  return value.almost(other.value);
}


void FitParam::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_attribute("name").set_value(name_.c_str());
  node.append_attribute("enabled").set_value(enabled);
  node.append_attribute("fixed").set_value(fixed);
  node.append_attribute("lbound").set_value(to_max_precision(lbound).c_str());
  node.append_attribute("ubound").set_value(to_max_precision(ubound).c_str());
  value.to_xml(node);
}

void FitParam::from_xml(const pugi::xml_node &node) {
  name_ = std::string(node.attribute("name").value());
  enabled = node.attribute("enabled").as_bool(true);
  fixed = node.attribute("fixed").as_bool(false);
  lbound = node.attribute("lbound").as_double(std::numeric_limits<double>::min());
  ubound = node.attribute("ubound").as_double(std::numeric_limits<double>::max());
  if (node.child(value.xml_element_name().c_str()))
    value.from_xml(node.child(value.xml_element_name().c_str()));
}


