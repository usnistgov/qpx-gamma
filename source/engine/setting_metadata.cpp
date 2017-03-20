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

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "setting_metadata.h"
#include "qpx_util.h"

namespace Qpx {

SettingType to_type(const std::string &type)
{
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

std::string to_string(SettingType type)
{
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
  s.id_ = id_;
  s.setting_type = setting_type;
  return s;
}

bool SettingMeta::meaningful() const
{
  return (operator!=(this->stripped()));
}

std::string SettingMeta::debug(std::string prepend) const
{
  std::string ret = to_string(setting_type);
  if (!name.empty())
    ret += "(" + name + ")";
  if (address != -1)
    ret += " " + std::to_string(address);
  if (writable)
    ret += " wr";
  if ((setting_type == SettingType::command) && !visible)
    ret += " invis";
  if ((setting_type == SettingType::stem) && saveworthy)
    ret += " savew";
  if ((setting_type == SettingType::binary) || (setting_type == SettingType::pattern))
    ret += " wsize=" + std::to_string(maximum);
  if (is_numeric())
  {
    std::stringstream ss;
    ss << " [" << minimum << "\uFF1A" << step << "\uFF1A" << maximum << "]";
    ret += ss.str();
  }
  if (!unit.empty())
    ret += " unit=" + unit;
  if (!description.empty())
    ret += " \"" + description + "\"";

  if (!flags.empty()) {
    std::string flgs;
    for (auto &q : flags)
      flgs += q + " ";
    boost::algorithm::trim_if(flgs, boost::algorithm::is_any_of("\r\n\t "));
    if (!flgs.empty())
      ret += " flags=\"" + flgs + "\"";
  }

  if (int_menu_items.size())
  {
    ret += "\n" + prepend + " ";
    for (auto &i : int_menu_items)
      ret += std::to_string(i.first) + "=\"" + i.second + "\"  ";
  }

  return ret;
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

bool SettingMeta::is_numeric() const
{
  return ((setting_type == SettingType::integer)
          || (setting_type == SettingType::floating)
          || (setting_type == SettingType::floating_precise));
}


//XMLable

void SettingMeta::populate_menu(const pugi::xml_node &node,
                                const std::string &key_name,
                                const std::string &value_name)
{
  int_menu_items.clear();
  for (pugi::xml_node child : node.children())
  {
    int32_t setting_key = child.attribute(key_name.c_str()).as_int(0);
    std::string setting_val(child.attribute(value_name.c_str()).as_string("no description"));
    int_menu_items[setting_key] = setting_val;
  }
}

void SettingMeta::menu_to_node(pugi::xml_node &node,
                               const std::string &element_name,
                               const std::string &key_name,
                               const std::string &value_name) const
{
  if (int_menu_items.empty())
    return;
  for (auto &q : int_menu_items)
  {
    pugi::xml_node child = node.append_child();
    child.set_name(element_name.c_str());
    child.append_attribute(key_name.c_str()).set_value(q.first);
    child.append_attribute(value_name.c_str()).set_value(q.second.c_str());
  }
}

void SettingMeta::from_xml(const pugi::xml_node &node)
{
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

void SettingMeta::to_xml(pugi::xml_node &node) const
{
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

  if (is_numeric())
  {
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


void to_json(json& j, const SettingMeta &s)
{
  j["id"] = s.id_;
  j["type"] = to_string(s.setting_type);
  j["address"] = s.address;
  j["max_indices"] = s.max_indices;
  j["writable"] = s.writable;

  if (!s.name.empty())
    j["name"] = s.name;
  if (!s.unit.empty())
    j["unit"] = s.unit;
  if (!s.description.empty())
    j["description"] = s.description;

  if (s.setting_type == SettingType::command)
    j["visible"] = s.visible;
  if (s.setting_type == SettingType::stem)
    j["saveworthy"] = s.saveworthy;

  if (s.is_numeric())
  {
    j["step"] = s.step;
    j["minimum"] = s.minimum;
    j["maximum"] = s.maximum;
  }
  else if ((s.setting_type == SettingType::binary) ||
           (s.setting_type == SettingType::pattern))
    j["word_size"] = s.maximum;

  if ((s.setting_type == SettingType::binary) ||
      (s.setting_type == SettingType::indicator) ||
      (s.setting_type == SettingType::int_menu) ||
      (s.setting_type == SettingType::stem))
    for (auto &q : s.int_menu_items)
     j["items"].push_back({{"val", q.first}, {"meaning", q.second}});

  if (!s.flags.empty())
    j["flags"] = s.flags;
}

void from_json(const json& j, SettingMeta &s)
{
  s.id_          = j["id"];
  s.setting_type = to_type(j["type"]);
  s.address      = j["address"];
  s.max_indices  = j["max_indices"];
  s.writable     = j["writable"];

  if (j.count("name"))
    s.name = j["name"];
  if (j.count("unit"))
    s.unit = j["unit"];
  if (j.count("description"))
    s.description = j["description"];

  if (j.count("visible"))
    s.visible = j["visible"];
  if (j.count("saveworthy"))
    s.saveworthy = j["saveworthy"];

  if (s.is_numeric())
  {
    s.step = j["step"];
    s.minimum = j["minimum"];
    s.maximum = j["maximum"];
  }
  else if (j.count("word_size"))
    s.maximum = j["word_size"];

  if (j.count("items"))
    for (auto it : j["items"])
      s.int_menu_items[it["val"]] = it["meaning"];

  if (j.count("flags"))
    for (auto it : j["flags"])
      s.flags.insert(it.get<std::string>());
}

}


