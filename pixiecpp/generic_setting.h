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
 *      Pixie::Setting exactly what it says
 *
 ******************************************************************************/

#ifndef PIXIE_GENERIC_SETTING
#define PIXIE_GENERIC_SETTING

#include <vector>
#include <string>
#include <boost/lexical_cast.hpp>
#include "tinyxml2.h"
#include "xmlable.h"

namespace Pixie {

enum class NodeType : int {none = 0, setting = 1, stem = 2};
enum class SettingType : int {boolean = 0, integer = 1, floating = 2, text = 3, detector = 4};

struct Setting;

struct Setting : public XMLable {

  NodeType          node_type;

  //if type !none
  uint32_t          index;
  bool              writable;
  uint16_t          address;
  std::string       name, description;

  //if type is setting
  SettingType       setting_type;
  int64_t           value_int;
  std::string       value_text;
  double            value;
  double            minimum, maximum, step;
  std::string       unit;

  //if type is stem
  XMLableDB<Setting> branches;


  
  Setting() : writable(false), branches("setting_branches"), address(0), index(0),
      node_type(NodeType::none), setting_type(SettingType::boolean), 
      value(0), value_int(0), minimum(0), maximum(1), step(1) {}
  
  Setting(tinyxml2::XMLElement* element) : Setting() {this->from_xml(element);}
  
  Setting(std::string nm) : Setting() {name = nm;}

  std::string xml_element_name() const {return "setting";}

  bool shallow_equals(const Setting& other) const {
    return ((name == other.name)
            && (index == other.index)
            && (node_type == other.node_type)
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
    if (node_type != other.node_type) return false;
    if (setting_type != other.setting_type) return false;
    if (index != other.index) return false;
    if (value_text != other.value_text) return false;
    if (writable != other.writable) return false;
    if (description != other.description) return false;
    if (address != other.address) return false;
    return true;
  }
  
  void from_xml(tinyxml2::XMLElement* element) {
    if (element->Attribute("name"))
      name = std::string(element->Attribute("name"));
    if (element->Attribute("index"))
      value = boost::lexical_cast<uint32_t>(element->Attribute("index"));
    if (element->Attribute("writable"))
      writable = true;
    if (element->Attribute("address"))
      value = boost::lexical_cast<uint16_t>(element->Attribute("address"));
    if (element->Attribute("description"))
      description = std::string(element->Attribute("description"));

    if (element->Attribute("type")) {
      node_type = NodeType::setting;
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
    } else {
      tinyxml2::XMLElement* stemData = element->FirstChildElement(branches.xml_element_name().c_str());
      if (stemData != NULL)
        branches.from_xml(stemData);
    }
  }

  void to_xml(tinyxml2::XMLPrinter& printer) const {
    if (node_type == NodeType::none)
      return;
    
    printer.OpenElement("Setting");
    printer.PushAttribute("name", name.c_str());
    printer.PushAttribute("index", std::to_string(index).c_str());
    if (writable)
      printer.PushAttribute("writable", "");
    printer.PushAttribute("address", std::to_string(address).c_str());

    if (node_type == NodeType::setting) {
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
      }
      if ((setting_type == SettingType::integer) ||
          (setting_type == SettingType::floating)) {
        printer.PushAttribute("minimum", std::to_string(minimum).c_str());
        printer.PushAttribute("maximum", std::to_string(maximum).c_str());
        printer.PushAttribute("step", std::to_string(step).c_str());
        printer.PushAttribute("unit", unit.c_str());
      }
    }

    if (!description.empty())
      printer.PushAttribute("description", description.c_str());

    if (node_type == NodeType::stem) {
      if (!branches.empty())
        branches.to_xml(printer);
    }
    
    printer.CloseElement();
  }

};

}

#endif
