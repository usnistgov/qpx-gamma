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

#include "spill.h"
#include <boost/algorithm/string.hpp>
#include "qpx_util.h"

namespace Qpx {

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
  ss << "Stats::";
  if (stats_type == StatsType::start)
    ss << "START(";
  else if (stats_type == StatsType::stop)
    ss << "STOP(";
  else
    ss << "RUN(";
  ss << "ch" << source_channel;
  ss << "|labtm" << boost::posix_time::to_iso_extended_string(lab_time);
  ss << ")";
  return ss.str();
}

void StatsUpdate::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  std::string type = "run";
  if (stats_type == StatsType::start)
    type = "start";
  else if (stats_type == StatsType::stop)
    type = "stop";

  node.append_attribute("type").set_value(type.c_str());

  node.append_attribute("channel").set_value(std::to_string(source_channel).c_str());
  node.append_attribute("lab_time").set_value(boost::posix_time::to_iso_extended_string(lab_time).c_str());
  node.append_attribute("timebase_mult").set_value(std::to_string(model_hit.timestamp.timebase_multiplier).c_str());
  node.append_attribute("timebase_div").set_value(std::to_string(model_hit.timestamp.timebase_divider).c_str());
  node.append_attribute("energy_bits").set_value(std::to_string(model_hit.energy.bits()).c_str());

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
  items.clear();

  if (std::string(node.name()) != xml_element_name())
    return;

  if (node.attribute("lab_time"))
    lab_time = from_iso_extended(node.attribute("lab_time").value());

  std::string type(node.attribute("type").value());
  if (type == "start")
    stats_type = StatsType::start;
  else if (type == "stop")
    stats_type = StatsType::stop;
  else
    stats_type = StatsType::running;

  source_channel = node.attribute("channel").as_int();
  model_hit.energy = DigitizedVal(0, node.attribute("energy_bits").as_int());
  model_hit.timestamp.timebase_multiplier = node.attribute("timebase_mult").as_double();
  model_hit.timestamp.timebase_divider    = node.attribute("timebase_div").as_double();

  if (node.child("Items")) {
    for (auto &i : node.child("Items").attributes()) {
      std::string item_name(i.name());
      PreciseFloat value(i.value());
      items[item_name] = value;
    }
  }
}

bool Spill::operator==(const Spill other) const {
  if (time != other.time)
    return false;
  if (stats != other.stats)
    return false;
  if (detectors != other.detectors)
    return false;
  if (state != other.state)
    return false;
  if (data != other.data)
    return false;
  if (hits.size() != other.hits.size())
    return false;
  return true;
}

bool Spill::empty()
{
  if (!stats.empty())
    return false;
  if (!detectors.empty())
    return false;
  if (state != Setting())
    return false;
  if (!data.empty())
    return false;
  if (!hits.empty())
    return false;
  return true;
}

void Spill::to_xml(pugi::xml_node &root, bool with_settings) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());
  node.append_attribute("time").set_value(boost::posix_time::to_iso_extended_string(time).c_str());
  node.append_attribute("bytes_raw_data").set_value(std::to_string(data.size() * sizeof(uint32_t)).c_str());
  node.append_attribute("number_of_hits").set_value(std::to_string(hits.size()).c_str());

  if (stats.size()) {
    pugi::xml_node statsnode = node.append_child("Stats");
    for (auto &s : stats)
      s.second.to_xml(statsnode);
  }

  if (with_settings) {
    if (!state.branches.empty())
      state.to_xml(node);
    if (!detectors.empty()) {
      pugi::xml_node child = node.append_child("Detectors");
      std::vector<Qpx::Detector> dets = detectors;
      for (auto q : dets) {
        q.settings_.strip_metadata();
//        q.settings_ = Setting();
        q.to_xml(child);
      }
    }
  }
}

void Spill::from_xml(const pugi::xml_node &node) {
  if (std::string(node.name()) != xml_element_name())
    return;

  data.clear();
  hits.clear();

  if (node.attribute("time"))
    time = from_iso_extended(node.attribute("time").value());

  if (node.attribute("number_of_hits"))
    hits.resize(node.attribute("number_of_hits").as_uint());

  if (node.child(state.xml_element_name().c_str()))
    state.from_xml(node.child(state.xml_element_name().c_str()));

  if (node.child("Stats")) {
    stats.clear();
    for (auto &q : node.child("Stats").children()) {
      StatsUpdate st;
      st.from_xml(q);
      stats[st.source_channel] = st;
    }
  }

  if (node.child("Detectors")) {
    detectors.clear();
    for (auto &q : node.child("Detectors").children()) {
      Qpx::Detector det;
      det.from_xml(q);
      detectors.push_back(det);
    }
  }

}

}
