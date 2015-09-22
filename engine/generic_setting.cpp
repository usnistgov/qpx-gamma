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

void Setting::from_xml(tinyxml2::XMLElement* element) {
  if (element->Attribute("name"))
    name = std::string(element->Attribute("name"));
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
  if (element->Attribute("unit"))
    unit = std::string(element->Attribute("unit"));
  if (element->Attribute("writable")) {
    std::string rw = std::string(element->Attribute("writable"));
    if (rw == "true")
      writable = true;
    else
      writable = false;
  }
  if (element->Attribute("address"))
    address = boost::lexical_cast<uint16_t>(element->Attribute("address"));
  if (element->Attribute("description"))
    description = std::string(element->Attribute("description"));

  if (element->Attribute("type")) {
    std::string temp_str(element->Attribute("type"));
    if (temp_str == "boolean") {
      setting_type = SettingType::boolean;
      if (element->Attribute("value")) {
        std::string temp_str2(element->Attribute("value"));
        if (temp_str2 == "true")
          value_int = 1;
        else
          value_int = 0;
      }
    } else if (temp_str == "integer") {
      setting_type = SettingType::integer;
      if (element->Attribute("value"))
        value_int = boost::lexical_cast<int64_t>(element->Attribute("value"));
    } else if (temp_str == "binary") {
      setting_type = SettingType::binary;
      if (element->Attribute("value"))
        value_int = boost::lexical_cast<int64_t>(element->Attribute("value"));
      if (element->Attribute("word_size"))
        maximum = boost::lexical_cast<double>(element->Attribute("word_size"));
      int_menu_items.clear();
      tinyxml2::XMLElement* menu_item = element->FirstChildElement("flag");
      while (menu_item != nullptr) {
        std::string setting_text = "no description";
        int32_t setting_val = 0;
        if (menu_item->Attribute("bit"))
          setting_val = boost::lexical_cast<int32_t>(menu_item->Attribute("bit"));
        if (menu_item->Attribute("description"))
          setting_text = std::string(menu_item->Attribute("description"));
        int_menu_items[setting_val] = setting_text;
        menu_item = dynamic_cast<tinyxml2::XMLElement*>(menu_item->NextSibling());
      }
    } else if (temp_str == "pattern") {
      setting_type = SettingType::pattern;
      if (element->Attribute("value"))
        value_int = boost::lexical_cast<int64_t>(element->Attribute("value"));
      if (element->Attribute("word_size"))
        maximum = boost::lexical_cast<double>(element->Attribute("word_size"));
    }
    else if (temp_str == "floating") {
      setting_type = SettingType::floating;
      if (element->Attribute("value"))
        value = boost::lexical_cast<double>(element->Attribute("value"));
    } else if (temp_str == "text") {
      setting_type = SettingType::text;
      if (element->Attribute("value"))
        value_text = std::string(element->Attribute("value"));
    } else if (temp_str == "detector") {
      setting_type = SettingType::detector;
      if (element->Attribute("value"))
        value_text = std::string(element->Attribute("value"));
    } else if (temp_str == "file_path") {
      setting_type = SettingType::file_path;
      if (element->Attribute("value"))
        value_text = std::string(element->Attribute("value"));
    } else if (temp_str == "int_menu") {
      setting_type = SettingType::int_menu;
      if (element->Attribute("value"))
        value_int = boost::lexical_cast<int64_t>(element->Attribute("value"));
      tinyxml2::XMLElement* menu_item = element->FirstChildElement("menu_item");
      while (menu_item != nullptr) {
        std::string setting_text = "no description";
        int32_t setting_val = 0;
        if (menu_item->Attribute("item_value"))
          setting_val = boost::lexical_cast<int32_t>(menu_item->Attribute("item_value"));
        if (menu_item->Attribute("item_text"))
          setting_text = std::string(menu_item->Attribute("item_text"));
        int_menu_items[setting_val] = setting_text;
        menu_item = dynamic_cast<tinyxml2::XMLElement*>(menu_item->NextSibling());
      }
    } else if (temp_str == "command") {
      setting_type = SettingType::command;
      if (element->Attribute("available"))
        value_int = boost::lexical_cast<int64_t>(element->Attribute("available"));
    } else if (temp_str == "stem") {
      setting_type = SettingType::stem;
      tinyxml2::XMLElement* stemData = element->FirstChildElement(branches.xml_element_name().c_str());
      if (element->Attribute("status")) {
        std::string temp_str2(element->Attribute("status"));
        if (temp_str2 == "open")
          value_int = 2;
        else if (temp_str2 == "closed")
          value_int = 1;
        else
          value_int = 0;
      }
      if (stemData != NULL)
        branches.from_xml(stemData);
    }
    if ((temp_str == "floating") || (temp_str == "integer")) {
      if (element->Attribute("minimum"))
        minimum = boost::lexical_cast<double>(element->Attribute("minimum"));
      if (element->Attribute("maximum"))
        maximum = boost::lexical_cast<double>(element->Attribute("maximum"));
      if (element->Attribute("step"))
        step = boost::lexical_cast<double>(element->Attribute("step"));
    }
  }
}

void Setting::to_xml(tinyxml2::XMLPrinter& printer) const {

  printer.OpenElement("Setting");
  printer.PushAttribute("name", name.c_str());
  printer.PushAttribute("index", std::to_string(index).c_str());
  if (!indices.empty()) {
    std::stringstream indices_stream;
    for (auto &q : indices)
      indices_stream << q << " ";
    std::string indices_string = boost::algorithm::trim_copy(indices_stream.str());
    if (!indices_string.empty())
      printer.PushAttribute("indices", indices_string.c_str());
  }
  if (writable)
    printer.PushAttribute("writable", "true");
  else
    printer.PushAttribute("writable", "false");
  printer.PushAttribute("address", std::to_string(address).c_str());


  if (setting_type == SettingType::boolean) {
    printer.PushAttribute("type", "boolean");
    if (value_int)
      printer.PushAttribute("value", "true");
    else
      printer.PushAttribute("value", "false");
  } else if (setting_type == SettingType::integer) {
    printer.PushAttribute("type", "integer");
    printer.PushAttribute("value", std::to_string(value_int).c_str());
    printer.PushAttribute("minimum", std::to_string(static_cast<int64_t>(minimum)).c_str());
    printer.PushAttribute("maximum", std::to_string(static_cast<int64_t>(maximum)).c_str());
    printer.PushAttribute("step", std::to_string(static_cast<int64_t>(step)).c_str());
  } else if (setting_type == SettingType::binary) {
    printer.PushAttribute("type", "binary");
    printer.PushAttribute("value", std::to_string(value_int).c_str());
    printer.PushAttribute("word_size", std::to_string(static_cast<int64_t>(maximum)).c_str());
    for (auto &q : int_menu_items) {
      printer.OpenElement("flag");
      printer.PushAttribute("bit", std::to_string(q.first).c_str());
      printer.PushAttribute("description", q.second.c_str());
      printer.CloseElement();
    }
  } else if (setting_type == SettingType::pattern) {
    printer.PushAttribute("type", "pattern");
    printer.PushAttribute("value", std::to_string(value_int).c_str());
    printer.PushAttribute("word_size", std::to_string(static_cast<int64_t>(maximum)).c_str());
  } else if (setting_type == SettingType::floating) {
    printer.PushAttribute("type", "floating");
    printer.PushAttribute("value", std::to_string(value).c_str());
    printer.PushAttribute("minimum", std::to_string(minimum).c_str());
    printer.PushAttribute("maximum", std::to_string(maximum).c_str());
    printer.PushAttribute("step", std::to_string(step).c_str());
  } else if (setting_type == SettingType::text) {
    printer.PushAttribute("type", "text");
    printer.PushAttribute("value", value_text.c_str());
  } else if (setting_type == SettingType::detector) {
    printer.PushAttribute("type", "detector");
    printer.PushAttribute("value", value_text.c_str());
  } else if (setting_type == SettingType::file_path) {
    printer.PushAttribute("type", "file_path");
    printer.PushAttribute("value", value_text.c_str());
  } else if (setting_type == SettingType::command) {
    printer.PushAttribute("type", "command");
    printer.PushAttribute("available", std::to_string(value_int).c_str());
  } else if (setting_type == SettingType::int_menu) {
    printer.PushAttribute("type", "int_menu");
    printer.PushAttribute("value", std::to_string(value_int).c_str());
    for (auto &q : int_menu_items) {
      printer.OpenElement("menu_item");
      printer.PushAttribute("item_value", std::to_string(q.first).c_str());
      printer.PushAttribute("item_text", q.second.c_str());
      printer.CloseElement();
    }
  } else if (setting_type == SettingType::stem) {
    printer.PushAttribute("type", "stem");
    if (value_int == 2)
      printer.PushAttribute("status", "open");
    else if (value_int == 1)
      printer.PushAttribute("status", "closed");
    else
      printer.PushAttribute("status", "dead");
    if (!branches.empty())
      branches.to_xml(printer);
  }

  if (unit.size())
    printer.PushAttribute("unit", unit.c_str());


  if (!description.empty())
    printer.PushAttribute("description", description.c_str());

  printer.CloseElement();
}

bool Setting::retrieve_one_setting(Gamma::Setting &det, const Gamma::Setting& root, bool precise_index) const {
  if ((root.setting_type == Gamma::SettingType::none) || det.name.empty())
    return false;
  if (root.setting_type == Gamma::SettingType::stem) {
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, det.name, boost::algorithm::is_any_of("/"));
    if ((tokens.size() > 1) && (root.name == tokens[0])) {
      Gamma::Setting truncated = det;
      truncated.name.clear();
      for (int i=1; i < tokens.size(); ++i) {
        truncated.name += tokens[i];
        if ((i+1) < tokens.size())
          truncated.name += "/";
      }
      for (auto &q : root.branches.my_data_) {
        if (retrieve_one_setting(truncated, q)) {
          det = truncated;
          return true;
        }
      }
    }
  } else if (det.name == root.name) {
    if ((precise_index && (root.index == det.index)) ||
        (!precise_index && (root.indices.count(det.index) > 0)))
    {
      det.value = root.value;
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

}


