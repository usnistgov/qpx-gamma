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
 *      Setting exactly what it says
 *
 ******************************************************************************/

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "generic_setting.h"
#include "qpx_util.h"
#include "custom_logger.h"

namespace Qpx {

SettingType to_type(const std::string &type) {
  if (type == "boolean")
    return SettingType::boolean;
  else if (type == "integer")
    return SettingType::integer;
  else if (type == "binary")
    return SettingType::binary;
  else if (type == "pattern")
    return SettingType::pattern;
  else if (type == "floating")
    return SettingType::floating;
  else if (type == "floating_precise")
    return SettingType::floating_precise;
  else if (type == "text")
    return SettingType::text;
  else if (type == "color")
    return SettingType::color;
  else if (type == "time")
    return SettingType::time;
  else if (type == "time_duration")
    return SettingType::time_duration;
  else if (type == "detector")
    return SettingType::detector;
  else if (type == "file_path")
    return SettingType::file_path;
  else if (type == "dir_path")
    return SettingType::dir_path;
  else if (type == "int_menu")
    return SettingType::int_menu;
  else if (type == "command")
    return SettingType::command;
  else if (type == "stem")
    return SettingType::stem;
  else if (type == "indicator")
    return SettingType::indicator;
  else
    return SettingType::none;
}

std::string to_string(SettingType type) {
  if (type == SettingType::boolean)
    return "boolean";
  else if (type == SettingType::integer)
    return "integer";
  else if (type == SettingType::binary)
    return "binary";
  else if (type == SettingType::pattern)
    return "pattern";
  else if (type == SettingType::floating)
    return "floating";
  else if (type == SettingType::floating_precise)
    return "floating_precise";
  else if (type == SettingType::text)
    return "text";
  else if (type == SettingType::color)
    return "color";
  else if (type == SettingType::detector)
    return "detector";
  else if (type == SettingType::time)
    return "time";
  else if (type == SettingType::time_duration)
    return "time_duration";
  else if (type == SettingType::file_path)
    return "file_path";
  else if (type == SettingType::dir_path)
    return "dir_path";
  else if (type == SettingType::command)
    return "command";
  else if (type == SettingType::int_menu)
    return "int_menu";
  else if (type == SettingType::indicator)
    return "indicator";
  else if (type == SettingType::stem)
    return "stem";
  else
    return "";
}


SettingMeta::SettingMeta()
  : setting_type(SettingType::none)
  , writable(false)
  , visible(true)
  , saveworthy(true)
  , address(-1)
  , minimum(std::numeric_limits<double>::min())
  , maximum(std::numeric_limits<double>::max())
  , step(1)
  , max_indices(0)
{}


SettingMeta::SettingMeta(const pugi::xml_node &node)
  : SettingMeta()
{
  this->from_xml(node);
}

bool SettingMeta::shallow_equals(const SettingMeta& other) const
{
  return (id_ == other.id_);
}

bool SettingMeta::operator!= (const SettingMeta& other) const
{
  return !operator==(other);
}

bool SettingMeta::operator== (const SettingMeta& other) const
{
  if (id_ != other.id_) return false;
  if (name != other.name) return false;
  if (unit != other.unit) return false;
  if (minimum != other.minimum) return false;
  if (maximum != other.maximum) return false;
  if (step != other.step) return false;
  if (writable != other.writable) return false;
  if (description != other.description) return false;
  if (address != other.address) return false;
  if (int_menu_items != other.int_menu_items) return false;
  if (flags != other.flags) return false;
  return true;
}

SettingMeta SettingMeta::stripped() const
{
  SettingMeta s;
  s.id_ == id_;
  s.setting_type = setting_type;
  return s;
}

bool SettingMeta::meaningful() const
{
  return (operator!=(this->stripped()));
}


void SettingMeta::from_xml(const pugi::xml_node &node) {
  setting_type = to_type(std::string(node.attribute("type").value()));
  if (setting_type == SettingType::none)
    return; //must have non0 type

  id_ = std::string(node.attribute("id").value());
  name = std::string(node.attribute("name").value());
  unit = std::string(node.attribute("unit").value());
  description = std::string(node.attribute("description").value());
  writable = node.attribute("writable").as_bool();
  visible = node.attribute("visible").as_bool(1);
  saveworthy = node.attribute("saveworthy").as_bool();
  address = node.attribute("address").as_llong(-1);
  max_indices = node.attribute("max_indices").as_int(0);
  step = node.attribute("step").as_double();
  minimum = node.attribute("minimum").as_double();
  maximum = node.attribute("maximum").as_double();
  if (!node.attribute("word_size").empty())
    maximum = node.attribute("word_size").as_double();

  if (setting_type == SettingType::binary)
    populate_menu(node, "bit", "description");

  if (setting_type == SettingType::indicator)
    populate_menu(node, "state", "description");

  if (setting_type == SettingType::int_menu)
    populate_menu(node, "item_value", "item_text");

  if (setting_type == SettingType::stem)
    populate_menu(node, "address", "id");

  if (!node.attribute("flags").empty()) {
    std::string flgs = std::string(node.attribute("flags").value());
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, flgs, boost::algorithm::is_any_of("\r\n\t "));
    for (auto &q : tokens) {
      boost::algorithm::trim_if(q, boost::algorithm::is_any_of("\r\n\t "));
      if (!q.empty())
        flags.insert(q);
    }
  }

}

void SettingMeta::to_xml(pugi::xml_node &node) const {
  if (setting_type == SettingType::none)
    return; //must have non0 type

  pugi::xml_node child = node.append_child();
  child.set_name(xml_element_name().c_str());
  child.append_attribute("id").set_value(id_.c_str());

  child.append_attribute("type").set_value(to_string(setting_type).c_str());

  if (!name.empty())
    child.append_attribute("name").set_value(name.c_str());
  child.append_attribute("address").set_value(std::to_string(address).c_str());
  child.append_attribute("max_indices").set_value(max_indices);
  child.append_attribute("writable").set_value(writable);

  if (setting_type == SettingType::command)
    child.append_attribute("visible").set_value(visible);

  if (setting_type == SettingType::stem)
    child.append_attribute("saveworthy").set_value(saveworthy);

  if ((setting_type == SettingType::binary) || (setting_type == SettingType::pattern))
    child.append_attribute("word_size").set_value(maximum);

  if ((setting_type == SettingType::integer)
      || (setting_type == SettingType::floating)
      || (setting_type == SettingType::floating_precise)) {
    child.append_attribute("step").set_value(step);
    child.append_attribute("minimum").set_value(minimum);
    child.append_attribute("maximum").set_value(maximum);
  }

  if (!unit.empty())
    child.append_attribute("unit").set_value(unit.c_str());
  if (!description.empty())
    child.append_attribute("description").set_value(description.c_str());

  if (setting_type == SettingType::binary)
    menu_to_node(child, "flag", "bit", "description");

  if (setting_type == SettingType::indicator)
    menu_to_node(child, "state", "state", "description");

  if (setting_type == SettingType::int_menu)
    menu_to_node(child, "menu_item", "item_value", "item_text");

  if (setting_type == SettingType::stem)
    menu_to_node(child, "branch", "address", "id");

  if (!flags.empty()) {
    std::string flgs;
    for (auto &q : flags)
      flgs += q + " ";
    boost::algorithm::trim_if(flgs, boost::algorithm::is_any_of("\r\n\t "));
    if (!flgs.empty())
      child.append_attribute("flags").set_value(flgs.c_str());
  }
}

std::string SettingMeta::value_range() const
{
  if (is_numeric())
  {
    std::stringstream ss;
    ss << "[" << minimum << " \uFF1A " << step << " \uFF1A " << maximum << "]";
    return ss.str();
  }
  else if ((setting_type == SettingType::binary) || (setting_type == SettingType::pattern))
  {
    std::stringstream ss;
    ss << to_string(setting_type) << maximum;
    return ss.str();
  }
  else
    return to_string(setting_type);
}

void SettingMeta::populate_menu(const pugi::xml_node &node, const std::string &key_name, const std::string &value_name) {
  int_menu_items.clear();
  for (pugi::xml_node child : node.children()) {
    int32_t setting_key = child.attribute(key_name.c_str()).as_int(0);
    std::string setting_val(child.attribute(value_name.c_str()).as_string("no description"));
    int_menu_items[setting_key] = setting_val;
  }
}

void SettingMeta::menu_to_node(pugi::xml_node &node, const std::string &element_name,
                               const std::string &key_name, const std::string &value_name) const
{
  if (int_menu_items.empty())
    return;
  for (auto &q : int_menu_items) {
    pugi::xml_node child = node.append_child();
    child.set_name(element_name.c_str());
    child.append_attribute(key_name.c_str()).set_value(q.first);
    child.append_attribute(value_name.c_str()).set_value(q.second.c_str());
  }
}

bool SettingMeta::is_numeric() const
{
  return ((setting_type == SettingType::integer)
          || (setting_type == SettingType::floating)
          || (setting_type == SettingType::floating_precise));
}



Setting::Setting()
  : branches("branches")
  , value_dbl(0.0)
  , value_int(0)
  , value_precise(0)
{}

Setting::Setting(const pugi::xml_node &node)
  : Setting()
{
  if (node.name() == xml_element_name())
    this->from_xml(node);
}

Setting::Setting(std::string id)
  : Setting()
{
  id_ = id;
  metadata.id_ = id;
}

Setting::Setting(SettingMeta meta)
  : Setting()
{
  id_ = meta.id_;
  metadata = meta;
}

Setting::operator bool() const
{
  return ((*this) != Setting());
}

bool Setting::compare(const Setting &other, Match flags) const {
  if ((flags & Match::id) && (id_ != other.id_))
    return false;
  if (flags & Match::indices) {
    bool mt = indices.empty() && other.indices.empty();
    for (auto &q : other.indices) {
      if (indices.count(q) > 0) {
        mt = true;
        break;
      }
    }
    if (!mt)
      return false;
  }
  if ((flags & Match::name) && (other.id_ != metadata.name))
    return false;
  if ((flags & Match::address) && (metadata.address != other.metadata.address))
    return false;
  return true;
}

bool Setting::shallow_equals(const Setting& other) const
{
  return compare(other, Match::id);
}

bool Setting::operator!= (const Setting& other) const
{
  return !operator==(other);
}

bool Setting::operator== (const Setting& other) const
{
  if (id_            != other.id_) return false;
  if (indices        != other.indices) return false;
  if (value_dbl      != other.value_dbl) return false;
  if (value_int      != other.value_int) return false;
  if (value_text     != other.value_text) return false;
  if (value_time     != other.value_time) return false;
  if (value_duration != other.value_duration) return false;
  if (value_precise  != other.value_precise) return false;
  if (value_pattern  != other.value_pattern) return false;
  if (branches       != other.branches) return false;
  //    if (metadata != other.metadata) return false;
  return true;
}


std::string Setting::indices_to_string(bool showblanks) const
{
  if (!showblanks && indices.empty())
    return "";

  int max = metadata.max_indices;
  bool valid = false;
  std::string ret;

  if (!indices.empty() || (showblanks && (max > 0))) {
    int have = 0;
    ret = "{ ";
    //better dealing with index = -1
    for (auto &q : indices)
    {
      ret += std::to_string(q) + " ";
      if (q >= 0) {
        valid = true;
        have++;
      }
    }
    if (showblanks) {
    while (have < max) {
      valid = true;
      ret += "- ";
      have++;
    }
    }

    ret += "}";
  }
  if (valid)
    return ret;
  else
    return "";
}


std::string Setting::val_to_string() const
{
  std::stringstream ss;
  if (metadata.setting_type == SettingType::boolean) {
    if (value_int != 0)
      ss << "true";
    else
      ss << "false";
  }
  else if ((metadata.setting_type == SettingType::integer) ||
           (metadata.setting_type == SettingType::int_menu) ||
           (metadata.setting_type == SettingType::indicator) )
    ss << std::to_string(value_int);
  else if (metadata.setting_type == SettingType::binary)
    //    ss << itohex64(value_int);
    ss << std::to_string(value_int);
  else if (metadata.setting_type == SettingType::floating)
    ss << std::setprecision(std::numeric_limits<double>::max_digits10) << value_dbl;
  else if (metadata.setting_type == SettingType::floating_precise)
    ss << std::setprecision(std::numeric_limits<PreciseFloat>::max_digits10) << value_precise;
  else if (metadata.setting_type == SettingType::pattern)
    ss << value_pattern.to_string();
  else if ((metadata.setting_type == SettingType::text) ||
           (metadata.setting_type == SettingType::color) ||
           (metadata.setting_type == SettingType::detector) ||
           (metadata.setting_type == SettingType::file_path) ||
           (metadata.setting_type == SettingType::dir_path))
    ss << value_text;
  else if (metadata.setting_type == SettingType::time) {
    if (value_time.is_not_a_date_time())
      ss << "INVALID";
    else
      ss << boost::posix_time::to_iso_extended_string(value_time);
  }
  else if (metadata.setting_type == SettingType::time_duration)
    ss << boost::posix_time::to_simple_string(value_duration);
  return ss.str();
}

std::string Setting::val_to_pretty_string() const {
  std::string ret = val_to_string();
  if ((metadata.setting_type == SettingType::time_duration) && !value_duration.is_not_a_date_time())
    ret = very_simple(value_duration);
  else if ((metadata.setting_type == SettingType::int_menu) && metadata.int_menu_items.count(value_int))
    ret = metadata.int_menu_items.at(value_int);
  else if ((metadata.setting_type == SettingType::indicator) && metadata.int_menu_items.count(value_int))
    ret = get_setting(Setting(metadata.int_menu_items.at(value_int)), Match::id).metadata.name;
  else if (metadata.setting_type == SettingType::binary) {
    int size = metadata.maximum;
    if (size > 32)
      ret = "0x" + itohex64(value_int);
    else if (size > 16)
      ret = "0x" + itohex32(value_int);
    else
      ret = "0x" + itohex16(value_int);
  } else if (metadata.setting_type == SettingType::stem)
    ret = value_text;
  return ret;
}


void Setting::val_from_node(const pugi::xml_node &node)
{
  if (metadata.setting_type == SettingType::boolean)
    value_int = node.attribute("value").as_bool();
  else if ((metadata.setting_type == SettingType::integer) ||
           (metadata.setting_type == SettingType::int_menu) ||
           (metadata.setting_type == SettingType::indicator) )
    value_int = node.attribute("value").as_llong();
  else if (metadata.setting_type == SettingType::pattern)
    value_pattern = Pattern(node.attribute("value").value());
  else if (metadata.setting_type == SettingType::binary)
    value_int = node.attribute("value").as_llong();
  //    ss << itohex64(value_int);
  else if (metadata.setting_type == SettingType::floating)
    value_dbl = node.attribute("value").as_double();
  else if (metadata.setting_type == SettingType::floating_precise)
    value_precise = PreciseFloat(node.attribute("value").value());
  else if ((metadata.setting_type == SettingType::text) ||
           (metadata.setting_type == SettingType::detector) ||
           (metadata.setting_type == SettingType::color) ||
           (metadata.setting_type == SettingType::file_path) ||
           (metadata.setting_type == SettingType::dir_path))
    value_text = std::string(node.attribute("value").value());
  else if (metadata.setting_type == SettingType::time)
    value_time = from_iso_extended(node.attribute("value").value());
  else if (metadata.setting_type == SettingType::time_duration)
    value_duration = boost::posix_time::duration_from_string(node.attribute("value").value());
}

void Setting::from_xml(const pugi::xml_node &node) {
  metadata.setting_type = to_type(std::string(node.attribute("type").value()));

  if (metadata.setting_type == SettingType::none)
    return;

  id_ = std::string(node.attribute("id").value());

  std::stringstream indices_stream;
  indices_stream.str(node.attribute("indices").value());
  std::string numero;
  indices.clear();
  while (indices_stream.rdbuf()->in_avail()) {
    indices_stream >> numero;
    indices.insert(boost::lexical_cast<int32_t>(numero));
  }

  if (metadata.setting_type == SettingType::stem) {
    branches.clear();
    value_text = std::string(node.attribute("reference").value());
    for (pugi::xml_node child : node.children()) {
      Setting newset(child);
      if (newset != Setting())
        branches.add_a(newset);
    }
  } else
    val_from_node(node);

  if (node.child(metadata.xml_element_name().c_str()))
    metadata.from_xml(node.child(metadata.xml_element_name().c_str()));
}

void Setting::to_xml(pugi::xml_node &node, bool with_metadata) const {
  if (metadata.setting_type == SettingType::none)
    return; //must have non0 type

  pugi::xml_node child = node.append_child();
  child.set_name(xml_element_name().c_str());
  child.append_attribute("id").set_value(id_.c_str());
  child.append_attribute("type").set_value(to_string(metadata.setting_type).c_str());

  if (!indices.empty()) {
    std::stringstream indices_stream;
    for (auto &q : indices)
      indices_stream << q << " ";
    std::string indices_string = boost::algorithm::trim_copy(indices_stream.str());
    if (!indices_string.empty())
      child.append_attribute("indices").set_value(indices_string.c_str());
  }

  if (metadata.setting_type != SettingType::stem)
    child.append_attribute("value").set_value(val_to_string().c_str());
  else if (!value_text.empty())
    child.append_attribute("reference").set_value(value_text.c_str());

  if (metadata.meaningful())
    metadata.to_xml(child);

  if (metadata.setting_type == SettingType::stem)
    for (auto &q : branches.my_data_)
      q.to_xml(child);
}

void Setting::set_value(const Setting &other)
{
  value_int = other.value_int;
  value_text = other.value_text;
  value_dbl = other.value_dbl;
  value_time = other.value_time;
  value_duration = other.value_duration;
  value_precise = other.value_precise;
  value_pattern = other.value_pattern;
}


bool Setting::set_setting_r(const Setting &setting, Match flags){
  if (this->compare(setting, flags)) {
    this->set_value(setting);
    return true;
  } else if ((this->metadata.setting_type == SettingType::stem)
             || (this->metadata.setting_type == SettingType::indicator)) {
    for (auto &q : this->branches.my_data_)
      if (q.set_setting_r(setting, flags))
        return true;
  }
  return false;
}


bool Setting::retrieve_one_setting(Setting &det, const Setting& root, Match flags) const {
  if (root.compare(det, flags)) {
    det = root;
    return true;
  } else if ((root.metadata.setting_type == SettingType::stem)
             || (root.metadata.setting_type == SettingType::indicator)) {
    for (auto &q : root.branches.my_data_)
      if (retrieve_one_setting(det, q, flags))
        return true;
  }
  return false;
}

Setting Setting::get_setting(Setting address, Match flags) const {
  if (retrieve_one_setting(address, *this, flags))
    return address;
  else
    return Setting();
}

std::list<Setting> Setting::find_all(const Setting &setting, Match flags) const
{
  std::list<Setting> result;
  if (metadata.setting_type == Qpx::SettingType::stem)
    for (auto &q : branches.my_data_)
      result.splice(result.end(), q.find_all(setting, flags));
  else if (compare(setting, flags) && (metadata.setting_type != Qpx::SettingType::detector))
    result.push_back(*this);
  return result;
}

void Setting::set_all(const std::list<Setting> &settings, Match flags)
{
  if (metadata.setting_type == Qpx::SettingType::stem)
    for (auto &q : branches.my_data_)
      q.set_all(settings, flags);
  else
    for (auto &q : settings)
      if (compare(q, flags) && (metadata.setting_type != Qpx::SettingType::detector))
        set_value(q);
}


bool Setting::has(Setting address, Match flags) const {
  return (retrieve_one_setting(address, *this, flags));
}

void Setting::delete_one_setting(const Setting &det, Setting& root, Match flags) {
  Setting truncated = root;
  truncated.branches.clear();
  for (auto &q : root.branches.my_data_) {
    if (!q.compare(det, flags)) {
      if (q.metadata.setting_type == SettingType::stem) {
        delete_one_setting(det, q, flags);
        if (!q.branches.empty())
          truncated.branches.add_a(q);
      } else
        truncated.branches.add_a(q);
    }
  }
  root = truncated;
}

void Setting::del_setting(Setting address, Match flags) {
  Setting addy(address);
  delete_one_setting(addy, *this, flags);
}


void Setting::enrich(const std::map<std::string, SettingMeta> &setting_definitions, bool impose_limits) {
  if (setting_definitions.count(id_) > 0) {
    SettingMeta meta = setting_definitions.at(id_);
    metadata = meta;
    if (((meta.setting_type == SettingType::indicator) ||
         (meta.setting_type == SettingType::binary) ||
         (meta.setting_type == SettingType::stem)) && !meta.int_menu_items.empty()) {
      XMLableDB<Setting> br = branches;
      branches.clear();
      for (auto &q : meta.int_menu_items) {
        if (setting_definitions.count(q.second) > 0) {
          SettingMeta newmeta = setting_definitions.at(q.second);
          bool added = false;
          for (int i=0; i < br.size(); ++i) {
            Setting newset = br.get(i);
            if (newset.id_ == newmeta.id_) {
              newset.enrich(setting_definitions, impose_limits);
              branches.add_a(newset);
              added = true;
            }
          }
          if (!added && (q.first != 666)) { //hack, eliminate this
            Setting newset = Setting(newmeta);
            newset.indices = indices;
            newset.enrich(setting_definitions, impose_limits);
            branches.add_a(newset);
          }
        }
      }
      for (int i=0; i < br.size(); ++i) {
        if (setting_definitions.count(br.get(i).id_) == 0) {
          Setting newset = br.get(i);
          newset.metadata.visible = false;
          branches.add_a(newset);
        }
      }
    } else if (impose_limits) {
      if (meta.setting_type == SettingType::integer) {
        if (value_int > meta.maximum)
          value_int = meta.maximum;
        if (value_int < meta.minimum)
          value_int = meta.minimum;
      } else if (meta.setting_type == SettingType::floating) {
        if (value_dbl > meta.maximum)
          value_dbl = meta.maximum;
        if (value_dbl < meta.minimum)
          value_dbl = meta.minimum;
      } else if (meta.setting_type == SettingType::floating_precise) {
        if (value_precise > meta.maximum)
          value_precise = meta.maximum;
        if (value_precise < meta.minimum)
          value_precise = meta.minimum;
      } else if (meta.setting_type == SettingType::pattern) {
        if (value_pattern.gates().size() != meta.maximum)
          value_pattern.resize(meta.maximum);
      }
    }
  }
}

void Setting::condense() {
  XMLableDB<Setting> oldbranches = branches;
  branches.clear();
  for (int i=0; i < oldbranches.size(); ++i) {
    Setting setting = oldbranches.get(i);
    if (setting.metadata.setting_type == SettingType::stem) {
      setting.condense();
      branches.add_a(setting);
    } else if (metadata.saveworthy
               && setting.metadata.writable
               && (setting.metadata.setting_type != SettingType::command)) {
      branches.add_a(setting);
    }
  }
}

void Setting::enable_if_flag(bool enable, std::string flag)
{
  if (metadata.flags.count(flag))
    metadata.writable = enable;
  if (metadata.setting_type == SettingType::stem)
    for (auto &q : branches.my_data_)
      q.enable_if_flag(enable, flag);
}

void Setting::cull_invisible() {
  XMLableDB<Setting> oldbranches = branches;
  branches.clear();
  for (int i=0; i < oldbranches.size(); ++i) {
    Setting setting = oldbranches.get(i);
    if (setting.metadata.visible)
      if (setting.metadata.setting_type == SettingType::stem) {
        setting.cull_invisible();
        if (!setting.branches.empty())
          branches.add_a(setting);
      } else
        branches.add_a(setting);
  }
}

void Setting::cull_readonly() {
  XMLableDB<Setting> oldbranches = branches;
  branches.clear();
  for (int i=0; i < oldbranches.size(); ++i) {
    Setting setting = oldbranches.get(i);
    if (setting.metadata.setting_type == SettingType::stem) {
      setting.cull_readonly();
      if (!setting.branches.empty())
        branches.add_a(setting);
    } else if (setting.metadata.writable)
      branches.add_a(setting);
  }
}

void Setting::strip_metadata() {
  for (auto &q : branches.my_data_)
    q.strip_metadata();
  metadata = metadata.stripped();
}

std::ostream& operator << (std::ostream &os, const Setting &s) {
  os << s.id_;
  if (s.indices.size()) {
    os << " [ ";
    for (auto &q : s.indices)
      os << q << " ";
    os << "]";
  }
  os << " = " << s.val_to_pretty_string() << std::endl;
  if (s.branches.size())
    os << "{" << std::endl;
  for (auto &q : s.branches.my_data_)
    os << q;
  if (s.branches.size())
    os << "}" << std::endl;
  return os;
}

//For numerics
bool Setting::is_numeric() const
{
  return metadata.is_numeric();
}

double Setting::number()
{
  if (metadata.setting_type == SettingType::integer)
    return value_int;
  else if (metadata.setting_type == SettingType::floating)
    return value_dbl;
  else if (metadata.setting_type == SettingType::floating_precise)
    return value_precise.convert_to<double>();
  else
    return std::numeric_limits<double>::quiet_NaN();
}

void Setting::set_number(double val)
{
  if (val > metadata.maximum)
    val = metadata.maximum;
  else if (val < metadata.minimum)
    val = metadata.minimum;
  if (metadata.setting_type == SettingType::integer)
    value_int = val;
  else if (metadata.setting_type == SettingType::floating)
    value_dbl = val;
  else if (metadata.setting_type == SettingType::floating_precise)
    value_precise = val;
}

// prefix
Setting& Setting::operator++()
{
  if (metadata.setting_type == SettingType::integer)
  {
    value_int += metadata.step;
    if (value_int > metadata.maximum)
      value_int = metadata.maximum;
  }
  else if (metadata.setting_type == SettingType::floating)
  {
    value_dbl += metadata.step;
    if (value_dbl > metadata.maximum)
      value_dbl = metadata.maximum;
  }
  else if (metadata.setting_type == SettingType::floating_precise)
  {
    value_precise += metadata.step;
    if (value_precise > metadata.maximum)
      value_precise = metadata.maximum;
  }
  return *this;
}

Setting& Setting::operator--()
{
  if (metadata.setting_type == SettingType::integer)
  {
    value_int -= metadata.step;
    if (value_int < metadata.minimum)
      value_int = metadata.minimum;
  }
  else if (metadata.setting_type == SettingType::floating)
  {
    value_dbl -= metadata.step;
    if (value_dbl < metadata.minimum)
      value_dbl = metadata.minimum;
  }
  else if (metadata.setting_type == SettingType::floating_precise)
  {
    value_precise -= metadata.step;
    if (value_precise < metadata.minimum)
      value_precise = metadata.minimum;
  }
  return *this;
}

// postfix
Setting Setting::operator++(int)
{
  Setting tmp(*this);
  operator++();
  return tmp;
}

Setting Setting::operator--(int)
{
  Setting tmp(*this);
  operator--();
  return tmp;
}


}


