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

#include <boost/date_time.hpp>
#include "hit_model.h"
#include "precise_float.h"

#include "xmlable.h"

#include "json.hpp"
using namespace nlohmann;

namespace Qpx {

struct StatsUpdate : public XMLable
{
  enum class Type
  {
    start, running, stop
  };

  static Type type_from_str(std::string);
  static std::string type_to_str(Type);

  Type      stats_type     {Type::running};
  int16_t   source_channel {-1};
  HitModel  model_hit;
  boost::posix_time::ptime lab_time;  //timestamp at end of spill
  std::map<std::string, PreciseFloat> items;
  
  std::string to_string() const;

  StatsUpdate operator-(const StatsUpdate) const;
  StatsUpdate operator+(const StatsUpdate) const;

  bool operator<(const StatsUpdate other) const {return (lab_time < other.lab_time);}
  bool operator==(const StatsUpdate other) const;
  bool operator!=(const StatsUpdate other) const {return !operator ==(other);}
  bool shallow_equals(const StatsUpdate& other) const {
    return ((lab_time == other.lab_time) && (source_channel == other.source_channel));
  }

  //XMLable
  void from_xml(const pugi::xml_node &) override;
  void to_xml(pugi::xml_node &) const override;
  std::string xml_element_name() const override {return "StatsUpdate";}

  friend void to_json(json& j, const StatsUpdate& s);
  friend void from_json(const json& j, StatsUpdate& s);
};

}
