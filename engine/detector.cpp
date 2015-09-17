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

  printer.OpenElement("Detector");

  printer.OpenElement("Name");
  printer.PushText(name_.c_str());
  printer.CloseElement();

  printer.OpenElement("Type");
  printer.PushText(type_.c_str());
  printer.CloseElement();

  if (energy_calibrations_.size())
    energy_calibrations_.to_xml(printer);

  if (gain_match_calibrations_.size())
    gain_match_calibrations_.to_xml(printer);

  if (fwhm_calibration_.bits_)
    fwhm_calibration_.to_xml(printer);

  if (settings_.setting_type == SettingType::stem)
    settings_.to_xml(printer);

  printer.CloseElement(); //Detector
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
    energy_calibrations_.from_xml(el);
  }

  if (el = root->FirstChildElement(gain_match_calibrations_.xml_element_name().c_str())) {
    gain_match_calibrations_.from_xml(el);
  }

  tinyxml2::XMLElement* stemData = root->FirstChildElement(settings_.xml_element_name().c_str());
  if (stemData != NULL) {
    PL_DBG << "loading optimization settings for " << name_;
    settings_.from_xml(stemData);
  }
  if (!settings_.branches.empty())
    PL_DBG << "Loaded optimization " << settings_.branches.my_data_.front().name;
}

}
