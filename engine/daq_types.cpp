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

#include "daq_types.h"
#include <boost/algorithm/string.hpp>

namespace Qpx {

boost::posix_time::time_duration Hit::to_posix_time() {
  //converts Pixie ticks to posix duration
  //this is not working well...
  /*uint64_t result = evt_time_lo;
  result += evt_time_hi * 65536;      //pow(2,16)
  result += buf_time_hi * 4294967296; //pow(2,32)
  result = result * 1000 / 75;
  boost::posix_time::time_duration answer =
      boost::posix_time::seconds(result / 1000000000) +
      boost::posix_time::milliseconds((result % 1000000000) / 1000000) +
      boost::posix_time::microseconds((result % 1000000) / 1000);
  //        + boost::posix_time::nanoseconds(result % 1000);*/
  boost::posix_time::time_duration answer = boost::posix_time::microseconds(timestamp.time / 75);
  return answer;
}

void Hit::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());
  node.append_attribute("channel").set_value(std::to_string(channel).c_str());
  node.append_attribute("energy").set_value(std::to_string(energy).c_str());
  node.append_attribute("XIA_PSA").set_value(std::to_string(XIA_PSA).c_str());
  node.append_attribute("user_PSA").set_value(std::to_string(user_PSA).c_str());
}

std::string Hit::to_string() const {
  std::stringstream ss;
  ss << "[ch" << channel << "|t" << timestamp.time << "|e" << energy << "]";
  return ss.str();
}

bool Event::in_window(const TimeStamp &ts) const {
  if ((ts.time == lower_time.time) || (ts.time == upper_time.time))
    return true;
  if ((ts.time > lower_time.time) && ((ts.time - lower_time.time) < window))
    return true;
  if ((ts.time < upper_time.time) && ((upper_time.time - ts.time) < window))
    return true;
  return false;
}

void Event::addHit(const Hit &newhit) {
  if (lower_time.time > newhit.timestamp.time)
    lower_time = newhit.timestamp;
  if (upper_time.time < newhit.timestamp.time)
    upper_time = newhit.timestamp;
  hit[newhit.channel] = newhit;
}

std::string Event::to_string() const {
  std::stringstream ss;
  ss << "EVT[t" << lower_time.time << ":" << upper_time.time << "w" << window << "]";
  return ss.str();
}

// difference across all variables
// except rate wouldn't make sense
// and timestamp would require duration output type
StatsUpdate StatsUpdate::operator-(const StatsUpdate other) const {
  StatsUpdate answer;
  answer.fast_peaks = fast_peaks - other.fast_peaks;
  answer.live_time  = live_time - other.live_time;
  answer.ftdt       = ftdt - other.ftdt;
  answer.sfdt       = sfdt - other.sfdt;
  answer.total_time = total_time - other.total_time;
  answer.events_in_spill = events_in_spill - other.events_in_spill;
  //event rate?
  //spill number?
  //labtime?
  //channel?
  return answer;
}

bool StatsUpdate::operator==(const StatsUpdate other) const {
  if (stats_type != other.stats_type)
    return false;
  if (channel != other.channel)
    return false;
  if (total_time != other.total_time)
    return false;
  if (events_in_spill != other.events_in_spill)
    return false;
  if (event_rate != other.event_rate)
    return false;
//  if (spill_number != other.spill_number)
//    return false;
  if (fast_peaks != other.fast_peaks)
    return false;
  return true;
}

// stacks two, adding up all variables
// except rate wouldn't make sense, neither does timestamp
StatsUpdate StatsUpdate::operator+(const StatsUpdate other) const {
  StatsUpdate answer;
  answer.fast_peaks = fast_peaks + other.fast_peaks;
  answer.live_time  = live_time + other.live_time;
  answer.ftdt       = ftdt + other.ftdt;
  answer.sfdt       = sfdt + other.sfdt;
  answer.total_time = total_time + other.total_time;
  answer.events_in_spill = events_in_spill + other.events_in_spill;
  //event rate?
  //labtime?
  //channel?
  return answer;
}

void StatsUpdate::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());
  node.append_attribute("channel").set_value(std::to_string(channel).c_str());
  node.append_attribute("lab_time").set_value(boost::posix_time::to_iso_extended_string(lab_time).c_str());
//  node.append_attribute("spill_number").set_value(std::to_string(spill_number).c_str());
  node.append_attribute("events_in_spill").set_value(std::to_string(events_in_spill).c_str());
  node.append_attribute("total_time").set_value(std::to_string(total_time).c_str());
  node.append_attribute("event_rate").set_value(std::to_string(event_rate).c_str());
  node.append_attribute("fast_peaks").set_value(std::to_string(fast_peaks).c_str());
  node.append_attribute("live_time").set_value(std::to_string(live_time).c_str());
  node.append_attribute("ftdt").set_value(std::to_string(ftdt).c_str());
  node.append_attribute("sfdt").set_value(std::to_string(sfdt).c_str());
}


void StatsUpdate::from_xml(const pugi::xml_node &node) {
  if (std::string(node.name()) != xml_element_name())
    return;

  boost::posix_time::time_input_facet *tif = new boost::posix_time::time_input_facet;
  tif->set_iso_extended_format();
  std::stringstream iss;

  if (node.attribute("lab_time")) {
    iss << node.attribute("lab_time").value();
    iss.imbue(std::locale(std::locale::classic(), tif));
    iss >> lab_time;
  }

  channel = node.attribute("channel").as_int();
//  spill_number = node.attribute("spill_number").as_ullong();
  events_in_spill = node.attribute("events_in_spill").as_ullong();
  total_time = node.attribute("total_time").as_double();
  event_rate = node.attribute("event_rate").as_double();
  fast_peaks = node.attribute("fast_peaks").as_double();
  live_time = node.attribute("live_time").as_double();
  ftdt = node.attribute("ftdt").as_double();
  sfdt = node.attribute("sfdt").as_double();
}

// to convert Pixie time to lab time
double RunInfo::time_scale_factor() const {
  double tot = state.get_setting(Gamma::Setting("Pixie4/System/module/TOTAL_TIME"), Gamma::Match::id).value_dbl;
  //PL_DBG << "total time " << tot;
  if (time_stop.is_not_a_date_time() ||
      time_start.is_not_a_date_time() ||
      (tot <= 0.0))
    return 1.0;
  else
    return (time_stop - time_start).total_microseconds() /
        (1000000 * tot);
}

bool RunInfo::operator== (const RunInfo& other) const {
  if (time_start != other.time_start) return false;
  if (time_stop != other.time_stop) return false;
  if (total_events != other.total_events) return false;
  if (detectors != other.detectors) return false;
  if (state != other.state) return false;
  return true;
}

void RunInfo::to_xml(pugi::xml_node &root, bool with_settings) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());
  node.append_attribute("time_start").set_value(boost::posix_time::to_iso_extended_string(time_start).c_str());
  node.append_attribute("time_stop").set_value(boost::posix_time::to_iso_extended_string(time_stop).c_str());
  node.append_attribute("total_events").set_value(std::to_string(total_events).c_str());
  if (with_settings) {
    state.to_xml(node);
    if (!detectors.empty()) {
      pugi::xml_node child = node.append_child("Detectors");
      for (auto q : detectors)
        q.to_xml(child);
    }
  }
}

void RunInfo::from_xml(const pugi::xml_node &node) {
  if (std::string(node.name()) != xml_element_name())
    return;

  total_events = node.attribute("Name").as_ullong();

  boost::posix_time::time_input_facet *tif = new boost::posix_time::time_input_facet;
  tif->set_iso_extended_format();
  std::stringstream iss;

  if (node.attribute("time_start")) {
    iss << node.attribute("time_start").value();
    iss.imbue(std::locale(std::locale::classic(), tif));
    iss >> time_start;
  }

  iss.str(std::string());
  if (node.attribute("time_stop")) {
    iss << node.attribute("time_stop").value();
    iss.imbue(std::locale(std::locale::classic(), tif));
    iss >> time_stop;
  }

  total_events = node.attribute("total_events").as_ullong();

  state.from_xml(node.child(state.xml_element_name().c_str()));

  if (node.child("Detectors")) {
    detectors.clear();
    for (auto &q : node.child("Detectors").children()) {
      Gamma::Detector det;
      det.from_xml(q);
      detectors.push_back(det);
    }
  }

}

}
