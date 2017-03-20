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

#include "spill.h"
#include "qpx_util.h"

namespace Qpx {

bool Spill::operator==(const Spill other) const
{
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

std::string Spill::to_string() const
{
  std::string info = boost::posix_time::to_iso_extended_string(time);
  if (detectors.size())
    info += " D" + std::to_string(detectors.size());
  if (stats.size())
    info += " S" + std::to_string(stats.size());
  if (hits.size())
    info += " [" + std::to_string(hits.size()) + "]";
  if (data.size())
    info += " RAW=" + std::to_string(data.size() * sizeof(uint32_t));
  return info;
}

void Spill::to_xml(pugi::xml_node &root, bool with_settings) const
{
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());
  node.append_attribute("time").set_value(boost::posix_time::to_iso_extended_string(time).c_str());
//  node.append_attribute("bytes_raw_data").set_value(std::to_string(data.size() * sizeof(uint32_t)).c_str());
//  node.append_attribute("number_of_hits").set_value(std::to_string(hits.size()).c_str());

  if (stats.size()) {
    pugi::xml_node statsnode = node.append_child("Stats");
    for (auto &s : stats)
      s.second.to_xml(statsnode);
  }

  if (with_settings)
  {
    if (!state.branches.empty())
      state.to_xml(node);
    if (!detectors.empty())
    {
      pugi::xml_node child = node.append_child("Detectors");
      std::vector<Qpx::Detector> dets = detectors;
      for (auto q : dets)
        q.to_xml(child);
    }
  }
}

void Spill::from_xml(const pugi::xml_node &node)
{
  *this = Spill();

  if (std::string(node.name()) != xml_element_name())
    return;

  data.clear();
  hits.clear();

  if (node.attribute("time"))
    time = from_iso_extended(node.attribute("time").value());

//  if (node.attribute("number_of_hits"))
//    hits.resize(node.attribute("number_of_hits").as_uint());

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

void to_json(json& j, const Spill& s)
{
  to_json(j, s, true);
}

void to_json(json& j, const Spill &s, bool with_settings)
{
  j["time"] = boost::posix_time::to_iso_extended_string(s.time);
//  j["bytes_raw_data"] = data.size() * sizeof(uint32_t);
//  j["number_of_hits"] = hits.size();

  for (auto &st : s.stats)
    j["stats"].push_back(st.second);

  if (!with_settings)
    return;

  if (!s.state.branches.empty())
    j["state"] = s.state;

  if (!s.detectors.empty())
    j["detectors"] = s.detectors;
}

void from_json(const json& j, Spill& s)
{
  s.time = from_iso_extended(j["time"].get<std::string>());

  if (j.count("stats"))
    for (auto it : j["stats"])
    {
      StatsUpdate st = it;
      s.stats[st.source_channel] = st;
    }

  if (j.count("state"))
    s.state = j["state"];

  if (j.count("detectors"))
    for (auto it : j["detectors"])
      s.detectors.push_back(it);
}


}
