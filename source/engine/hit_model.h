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

#include <vector>
#include <map>
#include "digitized_value.h"
#include "time_stamp.h"

#include "xmlable.h"

#include "json.hpp"
using namespace nlohmann;

namespace Qpx {

struct HitModel
{
public:
  TimeStamp                       timebase;
  std::vector<DigitizedVal>       values;
  std::vector<std::string>        idx_to_name;
  std::map<std::string, size_t>   name_to_idx;
  size_t                          tracelength {0};

  void add_value(const std::string& name, uint16_t bits);
  std::string to_string() const;

  void from_xml(const pugi::xml_node &);
  void to_xml(pugi::xml_node &) const;
  std::string xml_element_name() const {return "HitModel";}
};

void to_json(json& j, const HitModel& t);
void from_json(const json& j, HitModel& t);


}
