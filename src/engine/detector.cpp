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

#include "detector.h"

namespace Qpx {

Detector::Detector()
{
  settings_.metadata.setting_type = SettingType::stem;
}

Detector::Detector(std::string name)
  : Detector()
{
  name_ = name;
}

std::string Detector::name() const
{
  return name_;
}

std::string Detector::type() const
{
  return type_;
}

std::list<Setting> Detector::optimizations() const
{
  return settings_.branches.my_data_;
}

void Detector::set_name(const std::string& n)
{
  name_ = n;
}

void Detector::set_type(const std::string& t)
{
  type_ = t;
}

void Detector::add_optimizations(const std::list<Setting>& l, bool writable_only)
{
  for (auto s : l)
  {
    if (writable_only && !s.metadata.writable)
      continue;
    s.indices.clear();
    settings_.branches.replace(s);
  }
}

Setting Detector::get_setting(std::string id) const
{
  return settings_.get_setting(Setting(id), Qpx::Match::id);
}

void Detector::clear_optimizations()
{
  settings_ = Setting("Optimizations");
  settings_.metadata.setting_type = SettingType::stem;
}

void Detector::set_energy_calibration(const Calibration& c)
{
  energy_calibrations_.replace(c);
}

void Detector::set_gain_calibration(const Calibration& c)
{
  gain_match_calibrations_.replace(c);
}

void Detector::set_resolution_calibration(const Calibration& c)
{
  fwhm_calibration_ = c;
}

void Detector::set_efficiency_calibration(const Calibration& c)
{
  efficiency_calibration_ = c;
}

XMLableDB<Calibration> Detector::energy_calibrations() const
{
  return energy_calibrations_;
}

XMLableDB<Calibration> Detector::gain_calibrations() const
{
  return gain_match_calibrations_;
}

void Detector::remove_gain_calibration(size_t pos)
{
  gain_match_calibrations_.remove(pos);
}

void Detector::remove_energy_calibration(size_t pos)
{
  energy_calibrations_.remove(pos);
}

Calibration Detector::resolution() const
{
  return fwhm_calibration_;
}

Calibration Detector::efficiency() const
{
  return efficiency_calibration_;
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

bool Detector::has_energy_calib(int bits) const
{
  return (energy_calibrations_.has_a(Calibration(bits)));
}

Calibration Detector::get_energy_calib(int bits) const
{
  if (has_energy_calib(bits))
     return energy_calibrations_.get(Calibration(bits));
  else
    return Calibration();
}

Calibration Detector::best_calib(int bits) const
{
  if (has_energy_calib(bits))
     return energy_calibrations_.get(Calibration(bits));
  else
    return highest_res_calib();
}

Calibration Detector::highest_res_calib() const
{
  Calibration result;
  for (size_t i=0; i<energy_calibrations_.size(); ++i)
    if (energy_calibrations_.get(i).bits() >= result.bits())
      result = energy_calibrations_.get(i);
  return result;
}

Calibration Detector::get_gain_match(uint16_t bits, std::string todet) const
{
  Calibration result = Calibration(bits);
  for (size_t i=0; i< gain_match_calibrations_.size(); ++i)
  {
    Calibration k = gain_match_calibrations_.get(i);
    if ((k.bits() == result.bits()) &&
        (k.to()   == todet))
      return k;
  }
  return result;
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
        ss << prepend << k_branch_pre << k_branch_end << energy_calibrations_.get(i).debug() << "\n";
      else
        ss << prepend << k_branch_pre << k_branch_mid << energy_calibrations_.get(i).debug() << "\n";
    }
  }

  if (!gain_match_calibrations_.empty())
  {
    ss << prepend << k_branch_mid << "Gain match calibrations\n";
    for (size_t i=0; i < gain_match_calibrations_.size(); ++i)
    {
      if ((i+1) == energy_calibrations_.size())
        ss << prepend << k_branch_pre << k_branch_end << gain_match_calibrations_.get(i).debug() << "\n";
      else
        ss << prepend << k_branch_pre << k_branch_mid << gain_match_calibrations_.get(i).debug() << "\n";
    }
  }

  if (fwhm_calibration_.valid())
    ss << prepend << k_branch_mid << fwhm_calibration_.debug() << "\n";
  if (efficiency_calibration_.valid())
    ss << prepend << k_branch_mid << efficiency_calibration_.debug() << "\n";
  ss << prepend << k_branch_end << settings_.debug(prepend + "  ");

  return ss.str();
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
    if (nodename == newCali.xml_element_name())
    {
      std::string type = std::string(q.attribute("Type").value());
      newCali.from_xml(q);
      if (type == "FWHM")
        fwhm_calibration_ = newCali;
      else if (type == "Efficiency")
        efficiency_calibration_ = newCali;
      else if (type == "Energy")
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

void to_json(json& j, const Detector &s)
{
  j = s.to_json(true);
}

json Detector::to_json(bool options) const
{
  json j;

  j["name"] = name_;
  j["type"] = type_;

  if (energy_calibrations_.size())
    j["energy_calibrations"] = energy_calibrations_;

  if (gain_match_calibrations_.size())
    j["gain_match_calibrations"] = gain_match_calibrations_;

  if (fwhm_calibration_.valid())
    j["fwhm_calibration"] = fwhm_calibration_;

  if (efficiency_calibration_.valid())
    j["efficiency_calibration"] = efficiency_calibration_;

  if (options && !settings_.branches.empty())
    j["optimizations"] = settings_;

  return j;
}

void from_json(const json& j, Detector &s)
{
  s.name_ = j["name"];
  s.type_ = j["type"];

  if (j.count("fwhm_calibration"))
    s.fwhm_calibration_ = j["fwhm_calibration"];
  if (j.count("efficiency_calibration"))
    s.efficiency_calibration_ = j["efficiency_calibration"];
  if (j.count("energy_calibrations"))
    s.energy_calibrations_ = j["energy_calibrations"];
  if (j.count("gain_match_calibrations"))
    s.gain_match_calibrations_ = j["gain_match_calibrations"];
  if (j.count("optimizations"))
    s.settings_ = j["optimizations"];
}

}
