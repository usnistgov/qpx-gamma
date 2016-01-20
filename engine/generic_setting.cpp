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
 *      Gamma::Setting exactly what it says
 *
 ******************************************************************************/

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "generic_setting.h"

namespace Gamma {

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
  else if (type == "text")
    return SettingType::text;
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
  else if (type == SettingType::text)
    return "text";
  else if (type == SettingType::detector)
    return "detector";
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


void SettingMeta::from_xml(const pugi::xml_node &node) {
  setting_type = to_type(std::string(node.attribute("type").value()));
  if (setting_type == SettingType::none)
    return; //must have non0 type

  id_ = std::string(node.attribute("id").value());
  name = std::string(node.attribute("name").value());
  unit = std::string(node.attribute("unit").value());
  hardware_type = std::string(node.attribute("hardware_type").value());
  description = std::string(node.attribute("description").value());
  writable = node.attribute("writable").as_bool();
  visible = node.attribute("visible").as_bool(1);
  saveworthy = node.attribute("saveworthy").as_bool();
  address = node.attribute("address").as_int();
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

  if (!hardware_type.empty())
    child.append_attribute("hardware_type").set_value(hardware_type.c_str());

  if (!name.empty())
    child.append_attribute("name").set_value(name.c_str());
  child.append_attribute("address").set_value(address);
  child.append_attribute("writable").set_value(writable);

  if (setting_type == SettingType::command)
    child.append_attribute("visible").set_value(visible);

  if (setting_type == SettingType::stem)
    child.append_attribute("saveworthy").set_value(saveworthy);

  if ((setting_type == SettingType::binary) || (setting_type == SettingType::pattern))
    child.append_attribute("word_size").set_value(maximum);

  if ((setting_type == SettingType::integer) || (setting_type == SettingType::floating)) {
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

  if (metadata.setting_type == SettingType::boolean)
    value_int = node.attribute("value").as_bool();

  else if ((metadata.setting_type == SettingType::integer) ||
           (metadata.setting_type == SettingType::binary) ||
           (metadata.setting_type == SettingType::int_menu) ||
           (metadata.setting_type == SettingType::indicator) ||
           (metadata.setting_type == SettingType::pattern))
    value_int = node.attribute("value").as_llong();

  else if (metadata.setting_type == SettingType::floating)
    value_dbl = node.attribute("value").as_double();

  else if ((metadata.setting_type == SettingType::text) ||
           (metadata.setting_type == SettingType::detector) ||
           (metadata.setting_type == SettingType::file_path) ||
           (metadata.setting_type == SettingType::dir_path))
    value_text = std::string(node.attribute("value").value());

  else if (metadata.setting_type == SettingType::stem) {
    branches.clear();
    value_text = std::string(node.attribute("reference").value());
    for (pugi::xml_node child : node.children()) {
      Setting newset(child);
      if (newset != Setting())
        branches.add_a(newset);
    }
  }

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

  if (metadata.setting_type == SettingType::boolean)
    child.append_attribute("value").set_value(static_cast<bool>(value_int));

  else if ((metadata.setting_type == SettingType::integer) ||
           (metadata.setting_type == SettingType::binary) ||
           (metadata.setting_type == SettingType::int_menu) ||
           (metadata.setting_type == SettingType::indicator) ||
           (metadata.setting_type == SettingType::pattern))
    child.append_attribute("value").set_value(std::to_string(value_int).c_str());

  else if (metadata.setting_type == SettingType::floating)
    child.append_attribute("value").set_value(value_dbl);

  else if ((metadata.setting_type == SettingType::text) ||
           (metadata.setting_type == SettingType::detector) ||
           (metadata.setting_type == SettingType::file_path) ||
           (metadata.setting_type == SettingType::dir_path))
    child.append_attribute("value").set_value(value_text.c_str());

  else if (metadata.setting_type == SettingType::stem) {
    if (!value_text.empty())
      child.append_attribute("reference").set_value(value_text.c_str());
    for (auto &q : branches.my_data_)
      q.to_xml(child);
  }

  if (metadata.meaningful())
    metadata.to_xml(child);
}


bool Setting::compare(const Setting &other, Gamma::Match flags) const {
  bool match = true;
  if ((flags & Match::id) && (id_ != other.id_))
    match = false;
  if (flags & Match::indices) {
    match = false;
    for (auto &q : other.indices)
      if (indices.count(q) > 0) {
        match = true;
        break;
      }
  }
  if ((flags & Match::name) && (other.id_ != metadata.name))
    match = false;
  if ((flags & Match::address) && (metadata.address != other.metadata.address))
    match = false;
  return match;
}

bool Setting::push_one_setting(const Gamma::Setting &setting, Gamma::Setting& root, Match flags){
  if (root.compare(setting, flags)) {
      root = setting;
      return true;
  } else if ((root.metadata.setting_type == Gamma::SettingType::stem)
             || (root.metadata.setting_type == Gamma::SettingType::indicator)) {
    for (auto &q : root.branches.my_data_)
      if (push_one_setting(setting, q, flags))
        return true;
  }
  return false;
}


bool Setting::retrieve_one_setting(Gamma::Setting &det, const Gamma::Setting& root, Match flags) const {
  if (root.compare(det, flags)) {
      det = root;
      return true;
  } else if ((root.metadata.setting_type == Gamma::SettingType::stem)
             || (root.metadata.setting_type == Gamma::SettingType::indicator)) {
    for (auto &q : root.branches.my_data_)
      if (retrieve_one_setting(det, q, flags))
        return true;
  }
  return false;
}

Gamma::Setting Setting::get_setting(Gamma::Setting address, Match flags) const {
  Gamma::Setting addy(address);
  if (retrieve_one_setting(addy, *this, flags))
    return addy;
  else
    return Gamma::Setting();
}

void Setting::delete_one_setting(const Gamma::Setting &det, Gamma::Setting& root, Match flags) {
  Gamma::Setting truncated = root;
  truncated.branches.clear();
  for (auto &q : root.branches.my_data_) {
    if (!q.compare(det, flags)) {
      if (q.metadata.setting_type == Gamma::SettingType::stem) {
        delete_one_setting(det, q, flags);
        if (!q.branches.empty())
          truncated.branches.add_a(q);
      } else
        truncated.branches.add_a(q);
    }
  }
  root = truncated;
}

void Setting::del_setting(Gamma::Setting address, Match flags) {
  Gamma::Setting addy(address);
  delete_one_setting(addy, *this, flags);
}


void Setting::enrich(const std::map<std::string, Gamma::SettingMeta> &setting_definitions, bool impose_limits) {
  if (setting_definitions.count(id_) > 0) {
    Gamma::SettingMeta meta = setting_definitions.at(id_);
    metadata = meta;
    if (((meta.setting_type == Gamma::SettingType::indicator) || (meta.setting_type == Gamma::SettingType::binary) ||
        (meta.setting_type == Gamma::SettingType::stem)) && !meta.int_menu_items.empty()) {
      XMLableDB<Gamma::Setting> br = branches;
      branches.clear();
      for (auto &q : meta.int_menu_items) {
        if (setting_definitions.count(q.second) > 0) {
          Gamma::SettingMeta newmeta = setting_definitions.at(q.second);
          bool added = false;
          for (int i=0; i < br.size(); ++i) {
            Gamma::Setting newset = br.get(i);
            if (newset.id_ == newmeta.id_) {
              newset.enrich(setting_definitions, impose_limits);
              branches.add_a(newset);
              added = true;
            }
          }
          if (!added && (q.first != 666)) {
            Gamma::Setting newset = Gamma::Setting(newmeta);
            newset.indices = indices;
            newset.enrich(setting_definitions, impose_limits);
            branches.add(newset);
          }

        }
      }
    } else if (impose_limits) {
      if (meta.setting_type == Gamma::SettingType::integer) {
        if (value_int > meta.maximum)
          value_int = meta.maximum;
        if (value_int < meta.minimum)
          value_int = meta.minimum;
      } else if (meta.setting_type == Gamma::SettingType::floating) {
        if (value_dbl > meta.maximum)
          value_dbl = meta.maximum;
        if (value_dbl < meta.minimum)
          value_dbl = meta.minimum;
      }
    }
  }
}

void Setting::condense() {
  XMLableDB<Gamma::Setting> oldbranches = branches;
  branches.clear();
  for (int i=0; i < oldbranches.size(); ++i) {
    Gamma::Setting setting = oldbranches.get(i);
    if (setting.metadata.setting_type == SettingType::stem) {
      setting.condense();
      branches.add_a(setting);
    } else if (metadata.saveworthy
               && setting.metadata.writable
               && (setting.metadata.setting_type != Gamma::SettingType::command)) {
      branches.add_a(setting);
    }
  }
}

void Setting::cull_invisible() {
  XMLableDB<Gamma::Setting> oldbranches = branches;
  branches.clear();
  for (int i=0; i < oldbranches.size(); ++i) {
    Gamma::Setting setting = oldbranches.get(i);
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
  XMLableDB<Gamma::Setting> oldbranches = branches;
  branches.clear();
  for (int i=0; i < oldbranches.size(); ++i) {
    Gamma::Setting setting = oldbranches.get(i);
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


}


