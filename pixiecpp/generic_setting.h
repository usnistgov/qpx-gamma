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

struct Setting : public XMLable {

  
  std::string name, unit, description;
  double value;
  bool writable;

  Setting() : writable(false), value(0.0) {}
  Setting(tinyxml2::XMLElement* element) : Setting() {this->from_xml(element);}
  Setting(std::string nm) : Setting() {name = nm;}

  std::string xml_element_name() const {return "Setting";}

  bool shallow_equals(const Setting& other) const {return (name == other.name);}
  bool operator!= (const Setting& other) const {return !operator==(other);}
  bool operator== (const Setting& other) const {
    if (name != other.name) return false;
    if (unit != other.unit) return false; //assume other type info same
    if (value != other.value) return false;
    if (writable != other.writable) return false;
    if (description != other.description) return false;
    return true;
  }
  
  void from_xml(tinyxml2::XMLElement* element) {
    if (element->Attribute("name"))
      name = std::string(element->Attribute("name"));
    if (element->Attribute("value"))
      value = boost::lexical_cast<double>(element->Attribute("value"));
    if (element->Attribute("unit"))
      unit = std::string(element->Attribute("unit"));
    if (element->Attribute("writable"))
      writable = (std::string(element->Attribute("writable")) == "1");
    if (element->Attribute("description"))
      description = std::string(element->Attribute("description"));
  }

  void to_xml(tinyxml2::XMLPrinter& printer) const { 
    printer.OpenElement("Setting");
    printer.PushAttribute("name", name.c_str());
    printer.PushAttribute("value", std::to_string(value).c_str());
    printer.PushAttribute("unit", unit.c_str());
    printer.PushAttribute("writable", std::to_string(static_cast<int>(writable)).c_str());
    printer.PushAttribute("description", description.c_str());
    printer.CloseElement();
  }

};

}

#endif
