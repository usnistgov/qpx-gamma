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

Calibration Detector::get_gain_match(uint16_t bits, std::string todet)
{
  Calibration result = Calibration("Gain", bits);
  for (int i=0; i< gain_match_calibrations_.size(); ++i) {
    Calibration k = gain_match_calibrations_.get(i);
    if ((k.bits_ == result.bits_) &&
        (k.to_   == todet))
      return k;
  }
  return result;
}

void Detector::to_xml(tinyxml2::XMLPrinter& printer) const {
  to_xml_options(printer, true);
}

void Detector::to_xml(pugi::xml_node &node) const {
  to_xml_options(node, true);
}


void Detector::to_xml_options(tinyxml2::XMLPrinter& printer, bool options) const {
  printer.OpenElement("Detector");

  printer.OpenElement("Name");
  printer.PushText(name_.c_str());
  printer.CloseElement();

  printer.OpenElement("Type");
  printer.PushText(type_.c_str());
  printer.CloseElement();

  if (energy_calibrations_.size())
    XMLableDB<Calibration>(energy_calibrations_).to_xml(printer);

  if (gain_match_calibrations_.size())
    XMLableDB<Calibration>(gain_match_calibrations_).to_xml(printer);

  if (fwhm_calibration_.bits_)
    fwhm_calibration_.to_xml(printer);

  if (options && (settings_.metadata.setting_type == SettingType::stem))
    settings_.to_xml(printer);

  printer.CloseElement(); //Detector
}

void Detector::to_xml_options(pugi::xml_node &root, bool options) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_child("Name").append_child(pugi::node_pcdata).set_value(name_.c_str());
  node.append_child("Type").append_child(pugi::node_pcdata).set_value(type_.c_str());

  if (energy_calibrations_.size())
    energy_calibrations_.to_xml(node);

  if (gain_match_calibrations_.size())
    gain_match_calibrations_.to_xml(node);

  if (fwhm_calibration_.bits_)
    fwhm_calibration_.to_xml(node);

  if (options && (settings_.metadata.setting_type == SettingType::stem))
    settings_.to_xml(node);
}

void Detector::from_xml(tinyxml2::XMLElement* root) {
  tinyxml2::XMLElement* el;
  if (el = root->FirstChildElement("Name"))
    name_ = std::string(el->GetText());
  
  if (el = root->FirstChildElement("Type"))
    type_ = std::string(el->GetText());

  if (el = root->FirstChildElement(Calibration().xml_element_name().c_str())) {
    Calibration newCali; 
    newCali.from_xml(el);
    if (newCali.type_ == "FWHM")
      fwhm_calibration_ = newCali;
    else
      energy_calibrations_.add(newCali);  //this branch for bckwds compatibility with n42 calib entries
  }

  if (el = root->FirstChildElement(energy_calibrations_.xml_element_name().c_str())) {
    XMLableDB<Calibration> ec(energy_calibrations_.xml_element_name());
    ec.from_xml(el);
    energy_calibrations_ = XMLable2DB<Calibration>(ec.xml_element_name());
    energy_calibrations_.my_data_ = ec.my_data_;
  }

  if (el = root->FirstChildElement(gain_match_calibrations_.xml_element_name().c_str())) {
    XMLableDB<Calibration> ec(gain_match_calibrations_.xml_element_name());
    ec.from_xml(el);
    gain_match_calibrations_  = XMLable2DB<Calibration>(ec.xml_element_name());
    gain_match_calibrations_.my_data_ = ec.my_data_;
  }

  tinyxml2::XMLElement* stemData = root->FirstChildElement(settings_.xml_element_name().c_str());
  if (stemData != NULL)
    settings_.from_xml(stemData);
}

void Detector::from_xml(const pugi::xml_node &node) {
  if (std::string(node.name()) != xml_element_name())
    return;

  name_ = std::string(node.child_value("Name"));
  type_ = std::string(node.child_value("Type"));

  Calibration newCali;
  newCali.from_xml(node.child(newCali.xml_element_name().c_str()));
  if (newCali.type_ == "FWHM")
    fwhm_calibration_ = newCali;
  else if (newCali.type_ == "Energy")
    energy_calibrations_.add(newCali);  //backwards compatibility with n42 calib entries

  energy_calibrations_.from_xml(node.child(energy_calibrations_.xml_element_name().c_str()));
  gain_match_calibrations_.from_xml(node.child(gain_match_calibrations_.xml_element_name().c_str()));
  settings_.from_xml(node.child(settings_.xml_element_name().c_str()));
}

}
