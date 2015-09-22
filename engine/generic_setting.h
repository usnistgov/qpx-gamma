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
#include <set>
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
                              file_path = 12,
                              binary = 13,
                              command = 14
                             };

struct Setting;

struct Setting : public XMLable {

  //if type !none
  int32_t            index;
  std::set<int32_t>  indices;
  bool               writable;
  uint16_t           address;
  std::string        name, description;

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
  Setting(std::string nm, uint16_t addy, SettingType tp, int32_t idx) : Setting() {name = nm; address = addy; setting_type = tp; index = idx;}

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
    if (indices != other.indices) return false;
    if (value_text != other.value_text) return false;
    if (writable != other.writable) return false;
    if (description != other.description) return false;
    if (address != other.address) return false;
    if (branches != other.branches) return false;
    if (int_menu_items != other.int_menu_items) return false;
    return true;
  }
  
  void from_xml(tinyxml2::XMLElement* element);
  void to_xml(tinyxml2::XMLPrinter& printer) const;

  Gamma::Setting get_setting(Gamma::Setting address, bool precise_index = false) const;
  bool retrieve_one_setting(Gamma::Setting&, const Gamma::Setting&, bool precise_index = false) const;

};

}

#endif
