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
 *      Gamma::Calibration defines calibration with units and math model
 *
 ******************************************************************************/

#include <list>
#include <iostream>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "calibration.h"
#include "custom_logger.h"
#include "xylib.h"

#include "polynomial.h"
#include "polylog.h"
#include "log_inverse.h"
#include "effit.h"
#include "sqrt_poly.h"

namespace Gamma {

Calibration::Calibration() {
  calib_date_ = boost::posix_time::microsec_clock::local_time();
  type_ = "Energy";
  units_ = "channels";
  model_ = CalibrationModel::polynomial;
  bits_ = 0;
}

bool Calibration::valid() const {
  return (coefficients_.size() > 0);
}

double Calibration::transform(double chan) const {
  if (coefficients_.empty())
    return chan;
  
  if (bits_ && (model_ == CalibrationModel::polynomial))
    return Polynomial(coefficients_).eval(chan);
  else if (bits_ && (model_ == CalibrationModel::sqrt_poly))
    return SqrtPoly(coefficients_).eval(chan);
  else if (bits_ && (model_ == CalibrationModel::polylog))
    return PolyLog(coefficients_).evaluate(chan);
  else if (bits_ && (model_ == CalibrationModel::loginverse))
    return LogInverse(coefficients_).evaluate(chan);
  else if (bits_ && (model_ == CalibrationModel::effit))
    return Effit(coefficients_).evaluate(chan);
  else
    return chan;
}

double Calibration::inverse_transform(double energy) const {
  if (coefficients_.empty())
    return energy;

  if (bits_ && (model_ == CalibrationModel::polynomial))
    return Polynomial(coefficients_).eval_inverse(energy);
//  else if (bits_ && (model_ == CalibrationModel::polylog))
//    return PolyLog(coefficients_).inverse_evaluate(energy);
  else
    return energy;
}


double Calibration::transform(double chan, uint16_t bits) const {
  if (coefficients_.empty() || !bits_ || !bits)
    return chan;
  
  if (bits > bits_)
    chan = chan / pow(2, bits - bits_);
  if (bits < bits_)
    chan = chan * pow(2, bits_ - bits);

  double re = transform(chan);

  return re;
}

double Calibration::inverse_transform(double energy, uint16_t bits) const {
  if (coefficients_.empty() || !bits_ || !bits)
    return energy; //NaN?

  double bin = inverse_transform(energy);

  if (bits > bits_)
    bin = bin / pow(2, bits - bits_);
  if (bits < bits_)
    bin = bin * pow(2, bits_ - bits);

  return bin;
}

std::string Calibration::fancy_equation(int precision, bool with_rsq) {
  if (bits_ && (model_ == CalibrationModel::polynomial))
    return Polynomial(coefficients_).to_UTF8(precision, with_rsq);
  else if (bits_ && (model_ == CalibrationModel::sqrt_poly))
    return SqrtPoly(coefficients_).to_UTF8(precision, with_rsq);
  else if (bits_ && (model_ == CalibrationModel::polylog))
    return PolyLog(coefficients_).to_UTF8(precision, with_rsq);
  else if (bits_ && (model_ == CalibrationModel::loginverse))
    return LogInverse(coefficients_).to_UTF8(precision, with_rsq);
  else if (bits_ && (model_ == CalibrationModel::effit))
    return Effit(coefficients_).to_UTF8(precision, with_rsq);
  else
    return "N/A"; 
}

std::vector<double> Calibration::transform(std::vector<double> chans, uint16_t bits) const {
  std::vector<double> results;
  for (auto &q : chans)
    results.push_back(transform(q, bits));
  return results;
}

std::string Calibration::coef_to_string() const{
  std::stringstream dss;
  dss.str(std::string());
  for (auto &q : coefficients_) {
    dss << q << " ";
  }
  return boost::algorithm::trim_copy(dss.str());
}

void Calibration::coef_from_string(std::string coefs) {
  std::stringstream dss(boost::algorithm::trim_copy(coefs));

  std::string tempstr; std::list<double> templist; double coef;
  while (dss.rdbuf()->in_avail()) {
    dss >> coef;
    templist.push_back(coef);
  }

  coefficients_.resize(templist.size());
  int i=0;
  for (auto &q: templist) {
    coefficients_[i] = q;
    i++;
  }
}

std::string Calibration::to_string()
{
  std::string result;
  result += "[Calibration:" + type_ + "]";
  result += " bits=" + std::to_string(bits_);
  result += " units=" + units_;
  result += " date=" + to_iso_string(calib_date_);
  result += " coeffs=" + coef_to_string();
  return result;
}


void Calibration::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_attribute("Type").set_value(type_.c_str());
  if (type_ == "Energy")
    node.append_attribute("EnergyUnits").set_value(units_.c_str());
  else if (type_ == "Gain")
    node.append_attribute("To").set_value(to_.c_str());
  if (bits_ > 0)
    node.append_attribute("ResolutionBits").set_value(std::to_string(bits_).c_str());

  node.append_child("CalibrationCreationDate");
  node.last_child().append_child(pugi::node_pcdata).set_value(to_iso_extended_string(calib_date_).c_str());

  std::string  model_str = "undefined";
  if (model_ == CalibrationModel::polynomial)
    model_str = "Polynomial";
  else if (model_ == CalibrationModel::sqrt_poly)
    model_str = "SqrtPoly";
  else if (model_ == CalibrationModel::polylog)
    model_str = "PolyLog";
  else if (model_ == CalibrationModel::loginverse)
    model_str = "LogInverse";
  else if (model_ == CalibrationModel::effit)
    model_str = "Effit";
  node.append_child("Equation").append_attribute("Model").set_value(model_str.c_str());

  if ((model_ != CalibrationModel::none) && (coefficients_.size()))
    node.last_child().append_child("Coefficients").append_child(pugi::node_pcdata).set_value(coef_to_string().c_str());
 }


void Calibration::from_xml(const pugi::xml_node &node) {
  boost::posix_time::time_input_facet *tif = new boost::posix_time::time_input_facet;
  tif->set_iso_extended_format();
  std::stringstream iss;

  type_ = std::string(node.attribute("Type").value());

  if (type_ == "Energy")
    units_ = std::string(node.attribute("EnergyUnits").value());
  else if (type_ == "Gain")
    to_ = std::string(node.attribute("To").value());

  bits_ = node.attribute("ResolutionBits").as_int();

  iss << std::string(node.child_value("CalibrationCreationDate"));
  iss.imbue(std::locale(std::locale::classic(), tif));
  iss >> calib_date_;

  std::string model_str = std::string(node.child("Equation").attribute("Model").value());
  if (model_str == "Polynomial")
    model_ = CalibrationModel::polynomial;
  else if (model_str == "SqrtPoly")
    model_ = CalibrationModel::sqrt_poly;
  else if (model_str == "PolyLog")
    model_ = CalibrationModel::polylog;
  else if (model_str == "LogInverse")
    model_ = CalibrationModel::loginverse;
  else if (model_str == "Effit")
    model_ = CalibrationModel::effit;
  coef_from_string(std::string(node.child("Equation").child_value("Coefficients")));

}



}
