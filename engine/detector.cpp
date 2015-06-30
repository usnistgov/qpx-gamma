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
 *      Gamma::Detector defines detector with name, calibration, DSP parameters
 *
 ******************************************************************************/

#include <list>
#include <iostream>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "detector.h"
#include "custom_logger.h"

namespace Gamma {

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


Calibration Detector::highest_res_fwhm() {
  Calibration result;
  for (int i=0; i<fwhm_calibrations_.size(); ++i)
    if (fwhm_calibrations_.get(i).bits_ >= result.bits_)
      result = fwhm_calibrations_.get(i);
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

  if (energy_calibrations_.size())
    energy_calibrations_.to_xml(printer);
  
  if (fwhm_calibrations_.size())
    fwhm_calibrations_.to_xml(printer);

  if (setting_values_.size()) {
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
  }

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

  if (el = root->FirstChildElement(fwhm_calibrations_.xml_element_name().c_str())) {
    fwhm_calibrations_.from_xml(el);
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
