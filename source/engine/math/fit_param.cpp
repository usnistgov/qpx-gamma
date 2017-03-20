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

FitParam::FitParam()
  : FitParam("", std::numeric_limits<double>::quiet_NaN())
{
}

FitParam::FitParam(std::string name, double v)
  : FitParam(name, v,
             std::numeric_limits<double>::min(),
             std::numeric_limits<double>::max())
{
}

FitParam::FitParam(std::string name, double v, double lower, double upper)
  : name_(name)
  , value_(UncertainDouble::from_double(v, std::numeric_limits<double>::quiet_NaN()))
  , lower_(lower)
  , upper_(upper)
{
}

void FitParam::set_name(std::string n)
{
  name_ = n;
}

void FitParam::set_value(UncertainDouble v)
{
  value_ = v;

  //if out of bounds?
}

void FitParam::set(const FitParam& other)
{
  value_ = other.value_;
  lower_ = other.lower_;
  upper_ = other.upper_;
  enabled_ = other.enabled_;
  fixed_ = other.fixed_;
}

void FitParam::set(double min, double max, double val)
{
  lower_ = min;
  upper_ = max;
  if ((min <= val) && (val <= max))
    value_.setValue(val);
  else
    value_.setValue((min + max)/2.0);
}

void FitParam::preset_bounds(double min, double max)
{
  lower_ = min;
  upper_ = max;
  value_.setValue((min + max)/2.0);
}

void FitParam::constrain(double min, double max)
{
  double l = std::min(min, max);
  double u = std::max(min, max);
  lower_ = std::max(l, lower_);
  upper_ = std::min(u, upper_);
  value_.setValue(std::min(std::max(value_.value(), lower()), upper()));
}


std::string FitParam::name() const
{
  return name_;
}

UncertainDouble FitParam::value() const
{
  return value_;
}

double FitParam::lower() const
{
  return lower_;
}

double FitParam::upper() const
{
  return upper_;
}

bool FitParam::enabled() const
{
  return enabled_;
}

bool FitParam::fixed() const
{
  return fixed_ || implicitly_fixed();
}

bool FitParam::implicitly_fixed() const
{
  return (value_.finite() &&
          (value_.value() == lower_) &&
          (lower_ == upper_));
}

void FitParam::set_enabled(bool e)
{
  enabled_ = e;
}

FitParam FitParam::enforce_policy()
{
  FitParam ret = *this;
  if (!ret.enabled_) {
    ret.upper_ = ret.lower_;
    ret.lower_ = 0;
    ret.value_.setValue(ret.lower_);
  } else if (ret.fixed_) {
    ret.upper_ = ret.value_.value() + ret.lower_  * 0.01;
    ret.lower_ = ret.value_.value() - ret.lower_  * 0.01;
  }
  return ret;
}

std::string FitParam::to_string() const
{
  std::string ret = name_ + " = " + value_.to_string() +
      " [" + boost::lexical_cast<std::string>(lower_) +
      ":" + boost::lexical_cast<std::string>(upper_) + "]";
  return ret;
}

bool FitParam::same_bounds_and_policy(const FitParam &other) const
{
  if (lower_ != other.lower_)
    return false;
  if (upper_ != other.upper_)
    return false;
  if (enabled_ != other.enabled_)
    return false;
  if (fixed_ != other.fixed_)
    return false;
  return true;
}

void FitParam::to_xml(pugi::xml_node &root) const
{
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());
  node.append_attribute("name").set_value(name_.c_str());
  node.append_attribute("enabled_").set_value(enabled_);
  node.append_attribute("fixed_").set_value(fixed_);
  node.append_attribute("lower_").set_value(to_max_precision(lower_).c_str());
  node.append_attribute("upper_").set_value(to_max_precision(upper_).c_str());
  value_.to_xml(node);
}

void FitParam::from_xml(const pugi::xml_node &node)
{
  name_ = std::string(node.attribute("name").value());
  enabled_ = node.attribute("enabled_").as_bool(true);
  fixed_ = node.attribute("fixed_").as_bool(false);
  lower_ = node.attribute("lower_").as_double(std::numeric_limits<double>::min());
  upper_ = node.attribute("upper_").as_double(std::numeric_limits<double>::max());
  if (node.child(value_.xml_element_name().c_str()))
    value_.from_xml(node.child(value_.xml_element_name().c_str()));
}

void to_json(json& j, const FitParam& s)
{
  j["name"] = s.name_;
  j["enabled"] = s.enabled_;
  j["fixed"] = s.fixed_;
  j["lower"] = s.lower_;
  j["upper"] = s.upper_;
  j["value"] = s.value_;
}

void from_json(const json& j, FitParam& s)
{
  s.name_ = j["name"];
  s.enabled_ = j["enabled"];
  s.fixed_ = j["fixed"];
  s.lower_ = j["lower"];
  s.upper_ = j["upper"];
  s.value_ = j["value"];
}

