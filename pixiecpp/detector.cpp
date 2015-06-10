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
 *      Pixie::Calibration defines calibration with units and math model
 *      Pixie::Detector defines detector with name, calibration, DSP parameters
 *
 ******************************************************************************/

#include <list>
#include <iostream>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "detector.h"
#include "custom_logger.h"
#include "xylib.h"

namespace Pixie {

Calibration::Calibration() {
  calib_date_ = boost::posix_time::microsec_clock::local_time();
  type_ = "Energy";
  units_ = "channels";
  model_ = CalibrationModel::polynomial;
  bits_ = 0;
  coefficients_.resize(2);
  coefficients_[0] = 0.0;
  coefficients_[1] = 1.0;
}

double Calibration::transform(double chan) const {
  if (bits_ && (model_ == CalibrationModel::polynomial))
    return polynomial(chan);
  else
    return chan;
}

double Calibration::transform(double chan, uint16_t bits) const {
  if (!bits_ || !bits)
    return chan;
  
//  PL_DBG << "will shift " << chan << " from " << bits << " to " << bits_;

  if (bits > bits_)
    chan = chan / pow(2, bits - bits_);
  if (bits < bits_)
    chan = chan * pow(2, bits_ - bits);

  double re = transform(chan);
//  PL_DBG << "chan " << chan << " -> energy " << re;

  return re;
}

std::vector<double> Calibration::transform(std::vector<double> chans) const {
  if (bits_ && (model_ == CalibrationModel::polynomial)) {
    std::vector<double> results;
    for (auto &q : chans)
      results.push_back(polynomial(q));
    return results;
  } else
    return chans;
}

double Calibration::polynomial(double chan) const {
  double temp = 0;
  for (std::size_t i=0; i < coefficients_.size(); i++)
    temp += (coefficients_[i] * pow(chan, i));
  return temp;
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


void Calibration::to_xml(tinyxml2::XMLPrinter& printer) const {

  printer.OpenElement("Calibration");
  printer.PushAttribute("Type", type_.c_str());
  if (type_ == "Energy")
    printer.PushAttribute("EnergyUnits", units_.c_str());
  if (bits_ > 0)
    printer.PushAttribute("ResolutionBits", std::to_string(bits_).c_str());

  printer.OpenElement("CalibrationCreationDate");
  printer.PushText(to_iso_extended_string(calib_date_).c_str());
  printer.CloseElement();
      
  printer.OpenElement("Equation");
  std::string  model_str = "undefined";
  if (model_ == CalibrationModel::polynomial)
    model_str = "Polynomial";
  printer.PushAttribute("Model", model_str.c_str());

  if ((model_ != CalibrationModel::none) && (coefficients_.size())){
    printer.OpenElement("Coefficients");
    printer.PushText(coef_to_string().c_str());
    printer.CloseElement(); // Coeff
  }
  printer.CloseElement(); // Equation
  printer.CloseElement(); //Calibration
}

void Calibration::from_xml(tinyxml2::XMLElement* root) {
  boost::posix_time::time_input_facet *tif = new boost::posix_time::time_input_facet;
  tif->set_iso_extended_format();
  std::stringstream iss;
  
  type_ = std::string(root->Attribute("Type"));
  if (type_ == "Energy")
    units_ = std::string(root->Attribute("EnergyUnits"));
  if (root->Attribute("ResolutionBits"))
    bits_ = boost::lexical_cast<short>(root->Attribute("ResolutionBits"));
  
  tinyxml2::XMLElement* DateData = root->FirstChildElement("CalibrationCreationDate");
  if (DateData != NULL) {
    iss << std::string(DateData->GetText());
    iss.imbue(std::locale(std::locale::classic(), tif));
    iss >> calib_date_;
  }

  tinyxml2::XMLElement* EqnData = root->FirstChildElement("Equation");
  if (EqnData != NULL) {
    std::string model_str = std::string(EqnData->Attribute("Model"));
    if (model_str == "Polynomial")
      model_ = CalibrationModel::polynomial;
  }

  tinyxml2::XMLElement* CoefData = EqnData->FirstChildElement("Coefficients");
  if ((CoefData != NULL) && (CoefData->GetText() !=  NULL)) {
    coef_from_string(std::string(CoefData->GetText()));
  }
  
}

////////////////////
// Detector ////////
////////////////////

Calibration Detector::highest_res_calib() {
  Calibration result;
  for (int i=0; i<energy_calibrations_.size(); ++i)
    if (energy_calibrations_.get(i).bits_ >= result.bits_)
      result = energy_calibrations_.get(i);
  return result;
}

void Detector::to_xml(tinyxml2::XMLPrinter& printer) const {

  printer.OpenElement("Detector");

  printer.OpenElement("Name");
  printer.PushText(name_.c_str());
  printer.CloseElement();
      
  printer.OpenElement("Type");
  printer.PushText(type_.c_str());
  printer.CloseElement();

  energy_calibrations_.to_xml(printer);

  printer.OpenElement("Optimization"); 
  for (std::size_t j = 0; j < setting_values_.size(); j++) {
    if (!setting_names_[j].empty()) {
      printer.OpenElement("Setting");
      printer.PushAttribute("key", std::to_string(j).c_str());
      printer.PushAttribute("name", setting_names_[j].c_str());
      printer.PushAttribute("value",
                            std::to_string(setting_values_[j]).c_str());
      printer.CloseElement();
    }
  }
  printer.CloseElement(); //Optimization

  printer.CloseElement(); //Detector
}

void Detector::from_xml(tinyxml2::XMLElement* root) {
  tinyxml2::XMLElement* el;
  if (el = root->FirstChildElement("Name"))
    name_ = std::string(el->GetText());
  
  if (el = root->FirstChildElement("Type"))
    type_ = std::string(el->GetText());

  if (el = root->FirstChildElement(Calibration().xml_element_name().c_str())) {
    Calibration newCali;  //this branch for bckwds compatibility with n42 calib entries
    newCali.from_xml(el);
    energy_calibrations_.add(newCali);
  } else if (el = root->FirstChildElement(energy_calibrations_.xml_element_name().c_str())) {
    energy_calibrations_.from_xml(el);
  }

  tinyxml2::XMLElement* OptiData = root->FirstChildElement("Optimization");
  if (OptiData == NULL) return;

  setting_names_.resize(43); //hardcoded for p4
  setting_values_.resize(43);  
  tinyxml2::XMLElement* SettingElement = OptiData->FirstChildElement();
  while (SettingElement != NULL) {
    if (std::string(SettingElement->Name()) == "Setting") {
      int thisKey =  boost::lexical_cast<short>(SettingElement->Attribute("key"));
      setting_names_[thisKey]  = std::string(SettingElement->Attribute("name"));
      setting_values_[thisKey] = boost::lexical_cast<double>(SettingElement->Attribute("value"));
    }
    SettingElement = dynamic_cast<tinyxml2::XMLElement*>(SettingElement->NextSibling());
  }  
}

}
