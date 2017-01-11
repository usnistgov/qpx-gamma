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
 *      Qpx::Detector defines detector with name, calibration, DSP parameters
 *
 ******************************************************************************/

#include <list>
#include <iostream>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "detector.h"
#include "custom_logger.h"

namespace Qpx {

Detector::Detector()
//      : energy_calibrations_("Calibrations")
//      , gain_match_calibrations_("GainMatchCalibrations")
//      , settings_("Optimization")
//      , fwhm_calibration_("FWHM", 0)
//      , efficiency_calibration_("Efficiency", 0)
{
  settings_.metadata.setting_type = SettingType::stem;
}

Detector::Detector(std::string name)
  : Detector()
{
  name_ = name;
}

bool Detector::operator== (const Detector& other) const
{
  return ((name_ == other.name_) &&
          (type_ == other.type_) &&
          (settings_ == other.settings_) &&
          (energy_calibrations_ == other.energy_calibrations_) &&
          (gain_match_calibrations_ == other.gain_match_calibrations_) &&
          (fwhm_calibration_ == other.fwhm_calibration_) &&
          (efficiency_calibration_ == other.efficiency_calibration_));
}

bool Detector::operator!= (const Detector& other) const
{
  return !operator==(other);
}

Calibration Detector::best_calib(int bits) const
{
  if (energy_calibrations_.has_a(Calibration("Energy", bits)))
     return energy_calibrations_.get(Calibration("Energy", bits));
  else
    return highest_res_calib();
}

Calibration Detector::highest_res_calib() const
{
  Calibration result;
  for (size_t i=0; i<energy_calibrations_.size(); ++i)
    if (energy_calibrations_.get(i).bits_ >= result.bits_)
      result = energy_calibrations_.get(i);
  return result;
}

Calibration Detector::get_gain_match(uint16_t bits, std::string todet) const
{
  Calibration result = Calibration("Gain", bits);
  for (size_t i=0; i< gain_match_calibrations_.size(); ++i) {
    Calibration k = gain_match_calibrations_.get(i);
    if ((k.bits_ == result.bits_) &&
        (k.to_   == todet))
      return k;
  }
  return result;
}

void Detector::to_xml(pugi::xml_node &node) const
{
  to_xml_options(node, true);
}

void Detector::to_xml_options(pugi::xml_node &root, bool options) const
{
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_child("Name").append_child(pugi::node_pcdata).set_value(name_.c_str());
  node.append_child("Type").append_child(pugi::node_pcdata).set_value(type_.c_str());

  if (energy_calibrations_.size())
    energy_calibrations_.to_xml(node);

  if (gain_match_calibrations_.size())
    gain_match_calibrations_.to_xml(node);

  if (fwhm_calibration_.valid())
    fwhm_calibration_.to_xml(node);

  if (efficiency_calibration_.valid())
    efficiency_calibration_.to_xml(node);

  if (options && !settings_.branches.empty())
    settings_.to_xml(node);
}

void Detector::from_xml(const pugi::xml_node &node)
{
  if (std::string(node.name()) != xml_element_name())
    return;

  name_ = std::string(node.child_value("Name"));
  type_ = std::string(node.child_value("Type"));

  Calibration newCali;
  for (auto &q : node.children()) {
    std::string nodename(q.name());
    if (nodename == newCali.xml_element_name()) {
      newCali.from_xml(q);
      if (newCali.type_ == "FWHM")
        fwhm_calibration_ = newCali;
      else if (newCali.type_ == "Efficiency")
        efficiency_calibration_ = newCali;
      else if (newCali.type_ == "Energy")
        energy_calibrations_.add(newCali);  //backwards compatibility with n42 calib entries
    }
    else if  (nodename == energy_calibrations_.xml_element_name())
      energy_calibrations_.from_xml(q);
    else if  (nodename == gain_match_calibrations_.xml_element_name())
      gain_match_calibrations_.from_xml(q);
    else if  (nodename == settings_.xml_element_name())
      settings_.from_xml(q);
  }
}

std::string Detector::debug(std::string prepend) const
{
  std::stringstream ss;
  ss << name_ << "(" << type_ << ")\n";
  if (!energy_calibrations_.empty())
  {
    ss << prepend << k_branch_mid << "Energy calibrations\n";
    for (size_t i=0; i < energy_calibrations_.size(); ++i)
    {
      if ((i+1) == energy_calibrations_.size())
        ss << prepend << k_branch_pre << k_branch_end << energy_calibrations_.get(i).to_string() << "\n";
      else
        ss << prepend << k_branch_pre << k_branch_mid << energy_calibrations_.get(i).to_string() << "\n";
    }
  }

  if (!gain_match_calibrations_.empty())
  {
    ss << prepend << k_branch_mid << "Gain match calibrations\n";
    for (size_t i=0; i < gain_match_calibrations_.size(); ++i)
    {
      if ((i+1) == energy_calibrations_.size())
        ss << prepend << k_branch_pre << k_branch_end << gain_match_calibrations_.get(i).to_string() << "\n";
      else
        ss << prepend << k_branch_pre << k_branch_mid << gain_match_calibrations_.get(i).to_string() << "\n";
    }
  }

  if (fwhm_calibration_.valid())
    ss << prepend << k_branch_mid << fwhm_calibration_.to_string() << "\n";
  if (efficiency_calibration_.valid())
    ss << prepend << k_branch_mid << efficiency_calibration_.to_string() << "\n";
  ss << prepend << k_branch_end << settings_.debug(prepend + "  ");

  return ss.str();
}

}
