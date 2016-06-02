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
//#include <boost/algorithm/string.hpp>
#include "qpx_util.h"

namespace Qpx {

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
