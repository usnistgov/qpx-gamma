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
 ******************************************************************************/

#include "calibration.h"
#include "qpx_util.h"

#include "polynomial.h"
#include "polylog.h"
#include "log_inverse.h"
#include "effit.h"
#include "sqrt_poly.h"

namespace Qpx {

Calibration::Calibration(uint16_t bbits)
{
  bits_ = bbits;
}

std::string Calibration::model() const
{
  if (function_)
    return function_->type();
  return "undefined";
}

bool Calibration::operator== (const Calibration& other) const
{
  if (units_ != other.units_) return false;
  if (model() != other.model()) return false;
  if (bits_ != other.bits_) return false;
  if (to_ != other.to_) return false;
  return true;
}

uint16_t Calibration::bits() const
{
  return bits_;
}

std::string Calibration::units() const
{
  return units_;
}

std::string Calibration::to() const
{
  return to_;
}

boost::posix_time::ptime Calibration::calib_date() const
{
  return calib_date_;
}

void Calibration::set_units(const std::string& u)
{
  units_ = u;
}

void Calibration::set_to(const std::string& t)
{
  to_ = t;
}

void Calibration::set_function(const std::string& type,
                  const std::vector<double>& coefs)
{
  if (type == Polynomial().type())
    function_ = std::make_shared<Polynomial>(coefs, 0, 0);
  else if (type == PolyLog().type())
    function_ = std::make_shared<PolyLog>(coefs, 0, 0);
  else if (type == Effit().type())
    function_ = std::make_shared<Effit>(coefs, 0, 0);
  else if (type == LogInverse().type())
    function_ = std::make_shared<LogInverse>(coefs, 0, 0);
  else if (type == SqrtPoly().type())
    function_ = std::make_shared<SqrtPoly>(coefs, 0, 0);
  else if (function_)
    function_.reset();
}

void Calibration::set_function(std::shared_ptr<CoefFunction> f)
{
  function_ = f;
}

bool Calibration::operator!= (const Calibration& other) const
{
  return !operator==(other);
}

bool Calibration::valid() const
{
  return (function_.operator bool() && function_->coeff_count());
}

double Calibration::transform(double chan) const
{
  if (valid())
    return function_->eval(chan);
  return chan;
}

double Calibration::inverse_transform(double energy) const
{
  if (valid())
    return function_->eval_inverse(energy);
  return energy;
}

double Calibration::transform(double chan, uint16_t bits) const
{
  if (!bits_ || !bits || !valid())
    return chan;
  if (bits > bits_)
    chan = chan / pow(2, bits - bits_);
  if (bits < bits_)
    chan = chan * pow(2, bits_ - bits);
  return transform(chan);
}

double Calibration::inverse_transform(double energy, uint16_t bits) const
{
  if (!bits_ || !bits || !valid())
    return energy; //NaN?
  double bin = inverse_transform(energy);
  if (bits > bits_)
    bin = bin * pow(2, bits - bits_);
  if (bits < bits_)
    bin = bin / pow(2, bits_ - bits);
  return bin;
}

std::string Calibration::fancy_equation(int precision, bool with_rsq) const
{
  if (valid())
    return function_->to_UTF8(precision, with_rsq);
  return "N/A";
}

std::vector<double> Calibration::transform(const std::vector<double> &chans, uint16_t bits) const
{
  std::vector<double> results;
  for (auto &q : chans)
    results.push_back(transform(q, bits));
  return results;
}

std::string Calibration::coefs_to_string() const
{
  if (!valid())
    return "";
  std::stringstream dss;
  for (auto &q : function_->coeffs_consecutive())
    dss << q << " ";
  return boost::algorithm::trim_copy(dss.str());
}

std::vector<double> Calibration::coefs_from_string(const std::string &coefs)
{
  std::stringstream ss(boost::algorithm::trim_copy(coefs));
  std::vector<double> templist;
  while (ss.rdbuf()->in_avail())
  {
    double coef;
    ss >> coef;
    templist.push_back(coef);
  }
  return templist;
}

std::string Calibration::debug() const
{
  std::string result;
  result += " eqn=" + fancy_equation(4, true);
  result += " bits=" + std::to_string(bits_);
  result += " units=" + units_;
  result += " date=" + to_iso_string(calib_date_);
  return result;
}


void Calibration::to_xml(pugi::xml_node &root) const
{
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  if (units_.size())
    node.append_attribute("EnergyUnits").set_value(units_.c_str());

  if (to_.size())
    node.append_attribute("To").set_value(to_.c_str());

  if (bits_ > 0)
    node.append_attribute("ResolutionBits").set_value(std::to_string(bits_).c_str());

  node.append_child("CalibrationCreationDate");
  node.last_child().append_child(pugi::node_pcdata).set_value(to_iso_extended_string(calib_date_).c_str());

  node.append_child("Equation").append_attribute("Model").set_value(model().c_str());
  if (function_ && function_->coeff_count())
    node.last_child().append_child("Coefficients").append_child(pugi::node_pcdata)
        .set_value(coefs_to_string().c_str());
 }


void Calibration::from_xml(const pugi::xml_node &node)
{
  if (node.attribute("ResolutionBits"))
    bits_ = node.attribute("ResolutionBits").as_int();
  if (node.child_value("CalibrationCreationDate"))
    calib_date_ = from_iso_extended(node.child_value("CalibrationCreationDate"));
  if (node.attribute("EnergyUnits"))
    units_ = std::string(node.attribute("EnergyUnits").value());
  if (node.attribute("To"))
    to_ = std::string(node.attribute("To").value());

  std::string model_str = std::string(node.child("Equation").attribute("Model").value());
  auto coefs = coefs_from_string(std::string(node.child("Equation").child_value("Coefficients")));
  set_function(model_str, coefs);
}

void to_json(json& j, const Calibration &s)
{
  j["calibration_creation_date"] = to_iso_extended_string(s.calib_date_);
  j["bits"] = s.bits_;

  if (s.units_.size())
    j["units"] = s.units_;

  if (s.to_.size())
    j["to"] = s.to_;

  if (s.function_)
  {
    j["function"] = (*s.function_);
    j["function"]["type"] = s.function_->type();
  }
}

void from_json(const json& j, Calibration &s)
{
  s.calib_date_ = from_iso_extended(j["calibration_creation_date"]);
  s.bits_ = j["bits"];

  if (j.count("units"))
    s.units_ = j["units"];
  if (j.count("to"))
    s.to_ = j["to"];

  if (j.count("function"))
  {
    std::string type = j["function"]["type"];
    if (type == Polynomial().type())
      s.function_ = std::make_shared<Polynomial>();
    else if (type == PolyLog().type())
      s.function_ = std::make_shared<PolyLog>();
    else if (type == Effit().type())
      s.function_ = std::make_shared<Effit>();
    else if (type == LogInverse().type())
      s.function_ = std::make_shared<LogInverse>();
    else if (type == SqrtPoly().type())
      s.function_ = std::make_shared<SqrtPoly>();
    from_json(j["function"], *s.function_);
  }
}



}
