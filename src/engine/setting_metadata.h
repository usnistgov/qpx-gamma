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

#pragma once

#include <string>
#include <map>
#include <set>

#include "xmlable.h"

#include "json.hpp"
using namespace nlohmann;

namespace Qpx {

enum class SettingType {none,
                        stem,       // as branches
                        boolean,    // as int
                        integer,    // as int
                        command,    // as int
                        int_menu,   // as int + branches
                        binary,     // as int + branches
                        indicator,  // as int + branches
                        floating,   // as double
                        floating_precise, // as PreciseFloat
                        text,          // as text
                        color,         // as text
                        file_path,     // as text
                        dir_path,      // as text
                        detector,      // as text DOES NOT SCALE
                        time,          // as ptime
                        time_duration, // as time_duration
                        pattern        // as Pattern
                       };

SettingType to_type(const std::string &type);
std::string to_string(SettingType);


struct SettingMeta : public XMLable
{
  std::string        id_;
  SettingType        setting_type {SettingType::none};

  bool               writable    {false};
  bool               visible     {true};
  bool               saveworthy  {true};
  double             minimum     {std::numeric_limits<double>::min()};
  double             maximum     {std::numeric_limits<double>::max()};
  double             step        {1};
  int16_t            max_indices {0};
  int64_t            address     {-1};
  std::string        name, description;
  std::string        unit;                       //or extension if file
  std::map<int32_t, std::string> int_menu_items; //or intrinsic branches
  std::set<std::string> flags;


  SettingMeta() {}

  SettingMeta stripped() const;
  bool meaningful() const;

  bool is_numeric() const;
  std::string value_range() const;

  std::string debug(std::string prepend = std::string()) const;

  bool shallow_equals(const SettingMeta& other) const;
  bool operator!= (const SettingMeta& other) const;
  bool operator== (const SettingMeta& other) const;

  //XMLable
  SettingMeta(const pugi::xml_node &node);
  void from_xml(const pugi::xml_node &node);
  void to_xml(pugi::xml_node &node) const;
  std::string xml_element_name() const {return "SettingMeta";}

private:
  void populate_menu(const pugi::xml_node &node,
                     const std::string &key_name,
                     const std::string &value_name);
  void menu_to_node(pugi::xml_node &node, const std::string &element_name,
                    const std::string &key_name, const std::string &value_name) const;
};

void to_json(json& j, const SettingMeta &s);
void from_json(const json& j, SettingMeta &s);

}
