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
 *      Types for organizing data aquired from Device
 *        Qpx::Hit        single energy event with coincidence flags
 *        Qpx::StatsUdate metadata for one spill (memory chunk)
 *        Qpx::RunInfo    metadata for the whole run
 *        Qpx::Spill      bundles all data and metadata for a list run
 *        Qpx::ListData   bundles hits in vector and run metadata
 *
 ******************************************************************************/

#include "stats_update.h"
#include "qpx_util.h"

namespace Qpx {

StatsType StatsUpdate::type_from_str(std::string type)
{
  if (type == "start")
    return StatsType::start;
  else if (type == "stop")
    return StatsType::stop;
  else
    return StatsType::running;
}

std::string StatsUpdate::type_to_str(StatsType type)
{
  if (type == StatsType::start)
    return "start";
  else if (type == StatsType::stop)
    return "stop";
  else
    return "running";
}

// difference across all variables
// except rate wouldn't make sense
// and timestamp would require duration output type
StatsUpdate StatsUpdate::operator-(const StatsUpdate other) const {
  StatsUpdate answer;
  if (source_channel != other.source_channel)
    return answer;

  //labtime?
  for (auto &i : items)
    if (other.items.count(i.first))
      answer.items[i.first] = i.second - other.items.at(i.first);
  return answer;
}

bool StatsUpdate::operator==(const StatsUpdate other) const {
  if (stats_type != other.stats_type)
    return false;
  if (source_channel != other.source_channel)
    return false;
  if (items != other.items)
    return false;
  return true;
}

// stacks two, adding up all variables
// except timestamp wouldn't make sense
StatsUpdate StatsUpdate::operator+(const StatsUpdate other) const {
  StatsUpdate answer;
  if (source_channel != other.source_channel)
    return answer;

  //labtime?
  for (auto &i : items)
    if (other.items.count(i.first))
      answer.items[i.first] = i.second + other.items.at(i.first);
  return answer;
}

std::string StatsUpdate::to_string() const
{
  std::stringstream ss;
  ss << "Stats::" << type_to_str(stats_type) << "("
     << "ch" << source_channel
     << "@" << boost::posix_time::to_iso_extended_string(lab_time)
     << ")";
  return ss.str();
}

void StatsUpdate::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_attribute("type").set_value(type_to_str(stats_type).c_str());
  node.append_attribute("channel").set_value(std::to_string(source_channel).c_str());
  node.append_attribute("lab_time").set_value(boost::posix_time::to_iso_extended_string(lab_time).c_str());

  model_hit.to_xml(node);

  if (items.size()) {
     pugi::xml_node its = node.append_child("Items");
     for (auto &i : items) {
       std::stringstream ss;
       ss << std::setprecision(std::numeric_limits<PreciseFloat>::max_digits10) << i.second;
       its.append_attribute(i.first.c_str()).set_value(ss.str().c_str());
     }
  }
}


void StatsUpdate::from_xml(const pugi::xml_node &node) {
  *this = StatsUpdate();

  if (std::string(node.name()) != xml_element_name())
    return;

  if (node.attribute("lab_time"))
    lab_time = from_iso_extended(node.attribute("lab_time").value());

  if (node.attribute("type").value())
    stats_type = type_from_str(std::string(node.attribute("type").value()));

  source_channel = node.attribute("channel").as_int();
  if (node.child(model_hit.xml_element_name().c_str()))
    model_hit.from_xml(node.child(model_hit.xml_element_name().c_str()));

  if (node.child("Items")) {
    for (auto &i : node.child("Items").attributes()) {
      std::string item_name(i.name());
      PreciseFloat value = std::numeric_limits<long double>::quiet_NaN();

      std::string str(i.value());
      try { value = std::stold(str); }
      catch(...) {}

      items[item_name] = value;
    }
  }
}

void to_json(json& j, const StatsUpdate& s)
{
  j["type"] = s.type_to_str(s.stats_type);
  j["channel"] = s.source_channel;
  j["lab_time"] = boost::posix_time::to_iso_extended_string(s.lab_time);
  j["hit_model"] = s.model_hit;
  for (auto &i : s.items)
  {
    std::stringstream ss;
    ss << std::setprecision(std::numeric_limits<PreciseFloat>::max_digits10) << i.second;
    j["items"][i.first] = ss.str();
  }
}

void from_json(const json& j, StatsUpdate& s)
{
  s.stats_type = s.type_from_str(j["type"]);
  s.source_channel = j["channel"];
  s.lab_time = from_iso_extended(j["lab_time"]);
  s.model_hit = j["hit_model"];

  if (j.count("items"))
  {
    auto o = j["items"];
    for (json::iterator it = o.begin(); it != o.end(); ++it)
    {
      std::string name = it.key();
      PreciseFloat value = std::numeric_limits<long double>::quiet_NaN();
      try { value = std::stold(it.value().get<std::string>()); }
      catch(...) {}
      s.items[name] = value;
    }
  }
}



}
