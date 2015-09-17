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

#ifndef PIXIE_GENERIC_SETTING
#define PIXIE_GENERIC_SETTING

#include <vector>
#include <string>
#include <map>
#include <boost/lexical_cast.hpp>
#include "tinyxml2.h"
#include "xmlable.h"

namespace Gamma {

enum class SettingType : int {none = 0,
                              stem = 1,
                              boolean = 2,
                              integer = 3,
                              floating = 4,
                              floating_precise = 5,
                              text = 6,
                              int_menu = 7,
                              detector = 8,
                              time = 9,
                              time_duration = 10,
                              pattern = 11,
                              file_path = 12
                             };

struct Setting;

struct Setting : public XMLable {

  //if type !none
  int32_t           index;
  bool              writable;
  uint16_t          address;
  std::string       name, description;

  //if type is setting
  SettingType       setting_type;
  int64_t           value_int;
  std::string       value_text;
  double            value;
  double            minimum, maximum, step;
  std::string       unit; //or extension if file
  std::map<int32_t, std::string> int_menu_items;

  //if type is stem
  XMLableDB<Setting> branches;


  
  Setting() : writable(false), branches("branches"), address(0), index(-1),
    setting_type(SettingType::none), value(0), value_int(0), minimum(0), maximum(1), step(1) {}
  
  Setting(tinyxml2::XMLElement* element) : Setting() {this->from_xml(element);}
  
  Setting(std::string nm) : Setting() {name = nm;}

  std::string xml_element_name() const {return "Setting";}

  bool shallow_equals(const Setting& other) const {
    return ((name == other.name)
            && (address == other.address)
            && (setting_type == other.setting_type));
  }
  
  bool operator!= (const Setting& other) const {return !operator==(other);}
  bool operator== (const Setting& other) const {
    if (name != other.name) return false;
    if (unit != other.unit) return false;
    if (value != other.value) return false;
    if (value_int != other.value_int) return false;
    if (minimum != other.minimum) return false;
    if (maximum != other.maximum) return false;
    if (step != other.step) return false;
    if (setting_type != other.setting_type) return false;
    if (index != other.index) return false;
    if (value_text != other.value_text) return false;
    if (writable != other.writable) return false;
    if (description != other.description) return false;
    if (address != other.address) return false;
    if (branches != other.branches) return false;
    if (int_menu_items != other.int_menu_items) return false;
    return true;
  }
  
  void from_xml(tinyxml2::XMLElement* element) {
    if (element->Attribute("name"))
      name = std::string(element->Attribute("name"));
    if (element->Attribute("index"))
      index = boost::lexical_cast<uint32_t>(element->Attribute("index"));
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
      } else if (temp_str == "floating") {
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
        if (element->Attribute("unit"))
          unit = std::string(element->Attribute("unit"));
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
      } else if (temp_str == "stem") {
        setting_type = SettingType::stem;
        tinyxml2::XMLElement* stemData = element->FirstChildElement(branches.xml_element_name().c_str());
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
        if (element->Attribute("unit"))
          unit = std::string(element->Attribute("unit"));
      }
    }
  }

  void to_xml(tinyxml2::XMLPrinter& printer) const {
    
    printer.OpenElement("Setting");
    printer.PushAttribute("name", name.c_str());
    printer.PushAttribute("index", std::to_string(index).c_str());
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
    } else if (setting_type == SettingType::floating) {
      printer.PushAttribute("type", "floating");
      printer.PushAttribute("value", std::to_string(value).c_str());
    } else if (setting_type == SettingType::text) {
      printer.PushAttribute("type", "text");
      printer.PushAttribute("value", value_text.c_str());
    } else if (setting_type == SettingType::detector) {
      printer.PushAttribute("type", "detector");
      printer.PushAttribute("value", value_text.c_str());
    } else if (setting_type == SettingType::file_path) {
      printer.PushAttribute("type", "file_path");
      printer.PushAttribute("value", value_text.c_str());
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
      if (!branches.empty())
        branches.to_xml(printer);
    }

    if ((setting_type == SettingType::integer) ||
        (setting_type == SettingType::floating)) {
      printer.PushAttribute("minimum", std::to_string(minimum).c_str());
      printer.PushAttribute("maximum", std::to_string(maximum).c_str());
      printer.PushAttribute("step", std::to_string(step).c_str());
    }
    if (unit.size())
      printer.PushAttribute("unit", unit.c_str());


    if (!description.empty())
      printer.PushAttribute("description", description.c_str());

    printer.CloseElement();
  }

};

}

#endif
