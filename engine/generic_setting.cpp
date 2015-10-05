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
#include "tinyxml2.h"
#include "xmlable.h"
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

  if (setting_type == SettingType::int_menu)
    populate_menu(node, "item_value", "item_text");

  if (setting_type == SettingType::stem)
    populate_menu(node, "address", "id");
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

  if (setting_type == SettingType::int_menu)
    menu_to_node(child, "menu_item", "item_value", "item_text");

  if (setting_type == SettingType::stem)
    menu_to_node(child, "branch", "address", "id");
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

void SettingMeta::from_xml(tinyxml2::XMLElement* element) {
}


void SettingMeta::to_xml(tinyxml2::XMLPrinter& printer) const {
}

void Setting::from_xml(const pugi::xml_node &node) {
  metadata.setting_type = to_type(std::string(node.attribute("type").value()));

  if (metadata.setting_type == SettingType::none)
    return;

  id_ = std::string(node.attribute("id").value());
  index = node.attribute("index").as_int(-1);

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
  child.append_attribute("index").set_value(index);

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
           (metadata.setting_type == SettingType::pattern))
    child.append_attribute("value").set_value(std::to_string(value_int).c_str());

  else if (metadata.setting_type == SettingType::floating)
    child.append_attribute("value").set_value(value_dbl);

  else if ((metadata.setting_type == SettingType::text) ||
           (metadata.setting_type == SettingType::detector) ||
           (metadata.setting_type == SettingType::file_path) ||
           (metadata.setting_type == SettingType::dir_path))
    child.append_attribute("value").set_value(value_text.c_str());

  else if (metadata.setting_type == SettingType::stem)
    for (auto &q : branches.my_data_)
      q.to_xml(child);

  if (metadata.meaningful())
    metadata.to_xml(child);
}


void Setting::from_xml(tinyxml2::XMLElement* element) {
  if (element->Attribute("id"))
    id_ = std::string(element->Attribute("id"));
  PL_DBG << "loaded one " << id_;
  if (element->Attribute("index")) {
    index = boost::lexical_cast<int32_t>(element->Attribute("index"));
  }
  if (element->Attribute("indices")) {
    std::stringstream indices_stream;
    indices_stream.str(element->Attribute("indices"));
    std::string numero;
    indices.clear();
    while (indices_stream.rdbuf()->in_avail()) {
      indices_stream >> numero;
      indices.insert(boost::lexical_cast<int32_t>(numero));
    }
  }

  if (element->Attribute("type")) {
    std::string temp_str(element->Attribute("type"));
    if (temp_str == "boolean") {
      metadata.setting_type = SettingType::boolean;
      if (element->Attribute("value")) {
        std::string temp_str2(element->Attribute("value"));
        if (temp_str2 == "true")
          value_int = 1;
        else
          value_int = 0;
      }
    } else if (temp_str == "integer") {
      metadata.setting_type = SettingType::integer;
      if (element->Attribute("value"))
        value_int = boost::lexical_cast<int64_t>(element->Attribute("value"));
    } else if (temp_str == "binary") {
      metadata.setting_type = SettingType::binary;
      if (element->Attribute("value"))
        value_int = boost::lexical_cast<int64_t>(element->Attribute("value"));
    } else if (temp_str == "pattern") {
      metadata.setting_type = SettingType::pattern;
      if (element->Attribute("value"))
        value_int = boost::lexical_cast<int64_t>(element->Attribute("value"));
    } else if (temp_str == "floating") {
      metadata.setting_type = SettingType::floating;
      if (element->Attribute("value"))
        value_dbl = boost::lexical_cast<double>(element->Attribute("value"));
    } else if (temp_str == "text") {
      metadata.setting_type = SettingType::text;
      if (element->Attribute("value"))
        value_text = std::string(element->Attribute("value"));
    } else if (temp_str == "detector") {
      metadata.setting_type = SettingType::detector;
      if (element->Attribute("value"))
        value_text = std::string(element->Attribute("value"));
    } else if (temp_str == "file_path") {
      metadata.setting_type = SettingType::file_path;
      if (element->Attribute("value"))
        value_text = std::string(element->Attribute("value"));
    } else if (temp_str == "int_menu") {
      metadata.setting_type = SettingType::int_menu;
      if (element->Attribute("value"))
        value_int = boost::lexical_cast<int64_t>(element->Attribute("value"));
    } else if (temp_str == "command") {
      metadata.setting_type = SettingType::command;
    } else if (temp_str == "stem") {
      metadata.setting_type = SettingType::stem;
      branches.clear();
      tinyxml2::XMLElement* stemData = element->FirstChildElement(xml_element_name().c_str());
      while (stemData != NULL) {
        Setting newset(stemData);
        PL_DBG << "loaded " << newset.id_;
        if (newset != Setting())
          branches.add_a(newset);
        stemData = dynamic_cast<tinyxml2::XMLElement*>(stemData->NextSibling());
      }
    }
  }
}

void Setting::to_xml(tinyxml2::XMLPrinter& printer) const {
  if (metadata.setting_type == SettingType::none)
    return;

  printer.OpenElement(this->xml_element_name().c_str());
  printer.PushAttribute("id", id_.c_str());
  //if (index != -1)
  printer.PushAttribute("index", std::to_string(index).c_str());
  if (!indices.empty()) {
    std::stringstream indices_stream;
    for (auto &q : indices)
      indices_stream << q << " ";
    std::string indices_string = boost::algorithm::trim_copy(indices_stream.str());
    if (!indices_string.empty())
      printer.PushAttribute("indices", indices_string.c_str());
  }

  if (metadata.setting_type == SettingType::boolean) {
    printer.PushAttribute("type", "boolean");
    if (value_int)
      printer.PushAttribute("value", "true");
    else
      printer.PushAttribute("value", "false");
  } else if (metadata.setting_type == SettingType::integer) {
    printer.PushAttribute("type", "integer");
    printer.PushAttribute("value", std::to_string(value_int).c_str());
  } else if (metadata.setting_type == SettingType::binary) {
    printer.PushAttribute("type", "binary");
    printer.PushAttribute("value", std::to_string(value_int).c_str());
  } else if (metadata.setting_type == SettingType::pattern) {
    printer.PushAttribute("type", "pattern");
    printer.PushAttribute("value", std::to_string(value_int).c_str());
  } else if (metadata.setting_type == SettingType::floating) {
    printer.PushAttribute("type", "floating");
    printer.PushAttribute("value", std::to_string(value_dbl).c_str());
  } else if (metadata.setting_type == SettingType::text) {
    printer.PushAttribute("type", "text");
    printer.PushAttribute("value", value_text.c_str());
  } else if (metadata.setting_type == SettingType::detector) {
    printer.PushAttribute("type", "detector");
    printer.PushAttribute("value", value_text.c_str());
  } else if (metadata.setting_type == SettingType::file_path) {
    printer.PushAttribute("type", "file_path");
    printer.PushAttribute("value", value_text.c_str());
  } else if (metadata.setting_type == SettingType::command) {
    printer.PushAttribute("type", "command");
  } else if (metadata.setting_type == SettingType::int_menu) {
    printer.PushAttribute("type", "int_menu");
    printer.PushAttribute("value", std::to_string(value_int).c_str());
  } else if (metadata.setting_type == SettingType::stem) {
    printer.PushAttribute("type", "stem");
    for (auto &q : branches.my_data_)
      q.to_xml(printer);
  }
  printer.CloseElement();
}

bool Setting::retrieve_one_setting(Gamma::Setting &det, const Gamma::Setting& root, bool precise_index) const {
  if ((root.metadata.setting_type == Gamma::SettingType::none) || det.id_.empty())
    return false;
  if (root.metadata.setting_type == Gamma::SettingType::stem) {
    Gamma::Setting truncated = det;
    for (auto &q : root.branches.my_data_) {
      if (retrieve_one_setting(truncated, q)) {
        det = truncated;
        return true;
      }
    }
  } else if (det.id_ == root.id_) {
    if ((precise_index && (root.index == det.index)) ||
        (!precise_index && (root.indices.count(det.index) > 0)))
    {
      det.value_dbl = root.value_dbl;
      det.value_int = root.value_int;
      det.value_text = root.value_text;
      return true;
    }
  }
  return false;
}

Gamma::Setting Setting::get_setting(Gamma::Setting address, bool precise_index) const {
  Gamma::Setting addy(address);
  if (retrieve_one_setting(addy, *this, precise_index))
    return addy;
  else
    return Gamma::Setting();
}

void Setting::enrich(const std::map<std::string, Gamma::SettingMeta> &setting_definitions) {
  if (setting_definitions.count(id_) > 0) {
    Gamma::SettingMeta meta = setting_definitions.at(id_);
    if (meta.address == -1) {
      meta.address = index;
    }
    metadata = meta;
    if (meta.setting_type == Gamma::SettingType::stem) {
      XMLable2DB<Gamma::Setting> br = branches;
      branches.clear();
      for (auto &q : meta.int_menu_items) {
        if (setting_definitions.count(q.second) > 0) {
          Gamma::SettingMeta newmeta = setting_definitions.at(q.second);
          bool added = false;
          for (int i=0; i < br.size(); ++i) {
            Gamma::Setting newset = br.get(i);
            if (newset.id_ == newmeta.id_) {
              newset.enrich(setting_definitions);
              branches.add_a(newset);
              added = true;
            }
          }
          if (!added && (q.first != 666)) {
            Gamma::Setting newset = Gamma::Setting(newmeta, index);
            newset.indices = indices;
            newset.enrich(setting_definitions);
            branches.add(newset);
          }

        }
      }
    }
  }
}

void Setting::condense() {
  XMLable2DB<Gamma::Setting> oldbranches = branches;
  branches.clear();
  for (int i=0; i < oldbranches.size(); ++i) {
    Gamma::Setting setting = oldbranches.get(i);
    if (setting.metadata.setting_type == SettingType::stem) {
      setting.condense();
      branches.add_a(setting);
    } else if (metadata.saveworthy && setting.metadata.writable) {
      branches.add_a(setting);
    }
  }
}

void Setting::cull_invisible() {
  XMLable2DB<Gamma::Setting> oldbranches = branches;
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
  XMLable2DB<Gamma::Setting> oldbranches = branches;
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


