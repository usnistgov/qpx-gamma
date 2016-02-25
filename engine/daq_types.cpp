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

double TimeStamp::to_nanosec() const {
  if (timebase_divider == 0)
    return 0;
  else
    return time_native * timebase_multiplier / timebase_divider;
}

std::string TimeStamp::to_string() const {
  std::stringstream ss;
  ss << time_native << "x(" << timebase_multiplier << "/" << timebase_divider << ")";
  return ss.str();
}


bool TimeStamp::operator<(const TimeStamp other) const {
  if (same_base((other)))
    return (time_native < other.time_native);
  else
    return (to_nanosec() < other.to_nanosec());
}

bool TimeStamp::operator>(const TimeStamp other) const {
  if (same_base((other)))
    return (time_native > other.time_native);
  else
    return (to_nanosec() > other.to_nanosec());
}

bool TimeStamp::operator<=(const TimeStamp other) const {
  if (same_base((other)))
    return (time_native <= other.time_native);
  else
    return (to_nanosec() <= other.to_nanosec());
}

bool TimeStamp::operator>=(const TimeStamp other) const {
  if (same_base((other)))
    return (time_native >= other.time_native);
  else
    return (to_nanosec() >= other.to_nanosec());
}

bool TimeStamp::operator==(const TimeStamp other) const {
  if (same_base((other)))
    return (time_native == other.time_native);
  else
    return (to_nanosec() == other.to_nanosec());
}

bool TimeStamp::operator!=(const TimeStamp other) const {
  if (same_base((other)))
    return (time_native != other.time_native);
  else
    return (to_nanosec() != other.to_nanosec());
}

std::string DigitizedVal::to_string() const {
  std::stringstream ss;
  ss << val_ << "(" << bits_ << "b)";
  return ss.str();
}


void Hit::write_bin(std::ofstream &outfile, bool extras) const {
  outfile.write((char*)&source_channel, sizeof(source_channel));
  uint16_t time_hy = (timestamp.time_native >> 48) & 0x000000000000FFFF;
  uint16_t time_hi = (timestamp.time_native >> 32) & 0x000000000000FFFF;
  uint16_t time_mi = (timestamp.time_native >> 16) & 0x000000000000FFFF;
  uint16_t time_lo =  timestamp.time_native        & 0x000000000000FFFF;
  outfile.write((char*)&time_hy, sizeof(time_hy));
  outfile.write((char*)&time_hi, sizeof(time_hi));
  outfile.write((char*)&time_mi, sizeof(time_mi));
  outfile.write((char*)&time_lo, sizeof(time_lo));
  uint16_t nrg = energy.val(energy.bits());
  outfile.write((char*)&nrg, sizeof(nrg));
}

void Hit::read_bin(std::ifstream &infile, std::map<int16_t, Hit> &model_hits, bool extras) {
  uint16_t entry[6];
  infile.read(reinterpret_cast<char*>(entry), 12);
  int16_t channel = reinterpret_cast<int16_t&>(entry[0]);
  *this = model_hits[channel];

  source_channel = channel;
  uint64_t time_hy = entry[1];
  uint64_t time_hi = entry[2];
  uint64_t time_mi = entry[3];
  uint64_t time_lo = entry[4];
  uint64_t timestamp = (time_hy << 48) + (time_hi << 32) + (time_mi << 16) + time_lo;
  this->timestamp.time_native = timestamp;
  energy.set_val(entry[5]);
}

std::string Hit::to_string() const {
  std::stringstream ss;
  ss << "[ch" << source_channel << "|t" << timestamp.to_string() << "|e" << energy.to_string() << "]";
  return ss.str();
}

bool Event::in_window(const Hit& h) const {
  return (h.timestamp >= lower_time) && ((h.timestamp - lower_time) <= window_ns);
}

bool Event::past_due(const Hit& h) const {
  return (h.timestamp >= lower_time) && ((h.timestamp - lower_time) > window_ns);
}

bool Event::antecedent(const Hit& h) const {
  return (h.timestamp < lower_time);
}

bool Event::addHit(const Hit &newhit) {
  if (hits.count(newhit.source_channel))
    return false;
  if (lower_time > newhit.timestamp)
    lower_time = newhit.timestamp;
  hits[newhit.source_channel] = newhit;
  return true;
}

std::string Event::to_string() const {
  std::stringstream ss;
  ss << "EVT[t" << lower_time.to_string() << "w" << window_ns << "]";
  for (auto &q : hits)
    ss << " " << q.first << "=" << q.second.to_string();
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
  //labtime?
  //channel?
  return answer;
}

bool StatsUpdate::operator==(const StatsUpdate other) const {
  if (stats_type != other.stats_type)
    return false;
  if (source_channel != other.source_channel)
    return false;
  if (total_time != other.total_time)
    return false;
  if (events_in_spill != other.events_in_spill)
    return false;
  if (event_rate != other.event_rate)
    return false;
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
  ss << "|evts" << events_in_spill;
  ss << "|labtm" << boost::posix_time::to_iso_extended_string(lab_time);
  ss << "|devtm" << total_time;
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
  node.append_attribute("events_in_spill").set_value(std::to_string(events_in_spill).c_str());
  node.append_attribute("timebase_mult").set_value(std::to_string(model_hit.timestamp.timebase_multiplier).c_str());
  node.append_attribute("timebase_div").set_value(std::to_string(model_hit.timestamp.timebase_divider).c_str());
  node.append_attribute("energy_bits").set_value(std::to_string(model_hit.energy.bits()).c_str());

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

  std::string type(node.attribute("type").value());
  if (type == "start")
    stats_type = StatsType::start;
  else if (type == "stop")
    stats_type = StatsType::stop;
  else
    stats_type = StatsType::running;

  source_channel = node.attribute("channel").as_int();
  events_in_spill = node.attribute("events_in_spill").as_ullong();
  model_hit.energy = DigitizedVal(0, node.attribute("energy_bits").as_int());
  model_hit.timestamp.timebase_multiplier = node.attribute("timebase_mult").as_double();
  model_hit.timestamp.timebase_divider    = node.attribute("timebase_div").as_double();

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
