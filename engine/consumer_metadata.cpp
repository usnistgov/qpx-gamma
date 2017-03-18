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
 *      Qpx::Consumer   generic spectrum type.
 *                  All public methods are thread-safe.
 *                  When deriving override protected methods.
 *
 ******************************************************************************/

#include "consumer_metadata.h"
#include "qpx_util.h"

namespace Qpx {

ConsumerMetadata::ConsumerMetadata()
{
  attributes_.metadata.setting_type = SettingType::stem;
}

ConsumerMetadata::ConsumerMetadata(std::string tp, std::string descr, uint16_t dim,
                   std::list<std::string> itypes, std::list<std::string> otypes)
  : type_(tp)
  , type_description_(descr)
  , dimensions_(dim)
  , input_types_(itypes)
  , output_types_(otypes)
{
  attributes_.metadata.setting_type = SettingType::stem;
}

bool ConsumerMetadata::shallow_equals(const ConsumerMetadata& other) const
{
  return operator ==(other);
}

bool ConsumerMetadata::operator!= (const ConsumerMetadata& other) const
{
  return !operator==(other);
}

bool ConsumerMetadata::operator== (const ConsumerMetadata& other) const
{
  if (type_ != other.type_) return false; //assume other type info same
  if (attributes_ != other.attributes_) return false;
  return true;
}


std::string ConsumerMetadata::debug(std::string prepend) const
{
  std::stringstream ss;
  ss << type_ << " (dim=" << dimensions_ << ")\n";
  //ss << " " << type_description_;
  ss << prepend << k_branch_mid_B << "Detectors:\n";
  for (size_t i=0; i < detectors.size(); ++i)
  {
    if ((i+1) == detectors.size())
      ss << prepend << k_branch_pre_B << k_branch_end_B << i << ": " << detectors.at(i).debug(prepend + k_branch_pre_B + "  ");
    else
      ss << prepend << k_branch_pre_B << k_branch_mid_B << i << ": " << detectors.at(i).debug(prepend + k_branch_pre_B + k_branch_pre_B);
  }
  ss << prepend << k_branch_end_B << attributes_.debug(prepend + "  ");
  return ss.str();
}

Setting ConsumerMetadata::get_attribute(Setting setting) const
{
  return attributes_.get_setting(setting, Match::id | Match::indices);
}

Setting ConsumerMetadata::get_attribute(std::string setting) const
{
  return attributes_.get_setting(Setting(setting), Match::id | Match::indices);
}

Setting ConsumerMetadata::get_attribute(std::string setting, int32_t idx) const
{
  Setting find(setting);
  find.indices.insert(idx);
  return attributes_.get_setting(find, Match::id | Match::indices);
}

void ConsumerMetadata::set_attribute(const Setting &setting)
{
  attributes_.set_setting_r(setting, Match::id | Match::indices);
}

Setting ConsumerMetadata::get_all_attributes() const
{
  return attributes_;
}

Setting ConsumerMetadata::attributes() const
{
  return attributes_;
}

void ConsumerMetadata::set_attributes(const Setting &settings)
{
  if (settings.branches.size())
    for (auto &s : settings.branches.my_data_)
      set_attributes(s);
  else if (attributes_.has(settings, Match::id | Match::indices))
    set_attribute(settings);
}

void ConsumerMetadata::overwrite_all_attributes(Setting settings)
{
  attributes_ = settings;
}

void ConsumerMetadata::disable_presets()
{
  attributes_.enable_if_flag(false, "preset");
}


void ConsumerMetadata::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_attribute("Type").set_value(type_.c_str());

  if (attributes_.branches.size())
    attributes_.to_xml(node);

  if (!detectors.empty())
  {
    pugi::xml_node detnode = node.append_child("Detectors");
    for (auto &d : detectors)
      d.to_xml(detnode);
  }
}

void ConsumerMetadata::from_xml(const pugi::xml_node &node) {
  if (std::string(node.name()) != xml_element_name())
    return;

  type_ = std::string(node.attribute("Type").value());

  if (node.child(attributes_.xml_element_name().c_str()))
    attributes_.from_xml(node.child(attributes_.xml_element_name().c_str()));

  if (node.child("Detectors")) {
    detectors.clear();
    for (auto &q : node.child("Detectors").children()) {
      Qpx::Detector det;
      det.from_xml(q);
      detectors.push_back(det);
    }
  }
}

void to_json(json& j, const ConsumerMetadata &s)
{
  j["type"] = s.type_;

  if (s.attributes_.branches.size())
    j["attributes"] = s.attributes_;

  if (!s.detectors.empty())
    for (auto &d : s.detectors)
      j["detectors"].push_back(d);
}

void from_json(const json& j, ConsumerMetadata &s)
{
  s.type_ = j["type"];

  if (j.count("attributes"))
    s.attributes_ = j["attributes"];

  if (j.count("detectors"))
  {
    auto o = j["detectors"];
    for (json::iterator it = o.begin(); it != o.end(); ++it)
      s.detectors.push_back(it.value());
  }
}

void ConsumerMetadata::set_det_limit(uint16_t limit)
{
  if (limit < 1)
    limit = 1;

  for (auto &a : attributes_.branches.my_data_)
    if (a.metadata.setting_type == Qpx::SettingType::pattern)
      a.value_pattern.resize(limit);
    else if (a.metadata.setting_type == Qpx::SettingType::stem) {
      Setting prototype;
      for (auto &p : a.branches.my_data_)
        if (p.indices.count(-1))
          prototype = p;
      if (prototype.metadata.setting_type == Qpx::SettingType::stem) {
        a.indices.clear();
        a.branches.clear();
        prototype.metadata.visible = false;
        a.branches.add_a(prototype);
        prototype.metadata.visible = true;
        for (int i=0; i < limit; ++i) {
          prototype.indices.clear();
          prototype.indices.insert(i);
          for (auto &p : prototype.branches.my_data_)
            p.indices = prototype.indices;
          a.branches.add_a(prototype);
          //          a.indices.insert(i);
        }
      }
    }
}

bool ConsumerMetadata::chan_relevant(uint16_t chan) const
{
  for (const Setting &s : attributes_.branches.my_data_)
    if ((s.metadata.setting_type == SettingType::pattern) &&
        (s.value_pattern.relevant(chan)))
      return true;
  return false;
}


}
