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
 *      Types for organizing data aquired from Pixie
 *        Qpx::Hit        single energy event with coincidence flags
 *        Qpx::StatsUdate metadata for one spill (memory chunk from Pixie)
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

void Hit::to_xml(tinyxml2::XMLPrinter &printer, bool with_pattern) const {
  printer.OpenElement("Hit");

  printer.PushAttribute("channel", std::to_string(channel).c_str());
  printer.PushAttribute("energy", std::to_string(energy).c_str());
  printer.PushAttribute("XIA_PSA", std::to_string(XIA_PSA).c_str());
  printer.PushAttribute("user_PSA", std::to_string(user_PSA).c_str());

  printer.CloseElement(); //Hit
}

void Hit::from_xml(tinyxml2::XMLElement* root) {
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
  if (channel != other.channel)
    return false;
  if (total_time != other.total_time)
    return false;
  if (events_in_spill != other.events_in_spill)
    return false;
  if (event_rate != other.event_rate)
    return false;
  if (spill_number != other.spill_number)
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
  return answer;
  //event rate?
  //spill number?
  //labtime?
  //channel?
}

void StatsUpdate::to_xml(tinyxml2::XMLPrinter &printer) const {
  printer.OpenElement("StatsUpdate");
  printer.PushAttribute("channel", std::to_string(channel).c_str());
  printer.PushAttribute("lab_time", boost::posix_time::to_iso_extended_string(lab_time).c_str());
  printer.PushAttribute("spill_number", std::to_string(spill_number).c_str());
  printer.PushAttribute("events_in_spill", std::to_string(events_in_spill).c_str());
  printer.PushAttribute("total_time", std::to_string(total_time).c_str());
  printer.PushAttribute("event_rate", std::to_string(event_rate).c_str());
  printer.PushAttribute("fast_peaks", std::to_string(fast_peaks).c_str());
  printer.PushAttribute("live_time", std::to_string(live_time).c_str());
  printer.PushAttribute("ftdt", std::to_string(ftdt).c_str());
  printer.PushAttribute("sfdt", std::to_string(sfdt).c_str());
  printer.CloseElement(); //StatsUpdate
}

void StatsUpdate::from_xml(tinyxml2::XMLElement* root) {
  boost::posix_time::time_input_facet *tif = new boost::posix_time::time_input_facet;
  tif->set_iso_extended_format();
  std::stringstream iss;

  if (root->Attribute("lab_time")) {
    iss << root->Attribute("lab_time");
    iss.imbue(std::locale(std::locale::classic(), tif));
    iss >> lab_time;
  }

  if (root->Attribute("channel") != nullptr)
    channel = boost::lexical_cast<int>(root->Attribute("channel"));
  if (root->Attribute("spill_number"))
    spill_number = boost::lexical_cast<uint64_t>(root->Attribute("spill_number"));
  if (root->Attribute("events_in_spill"))
    events_in_spill = boost::lexical_cast<uint64_t>(root->Attribute("events_in_spill"));
  if (root->Attribute("total_time"))
    total_time = boost::lexical_cast<double>(root->Attribute("total_time"));
  if (root->Attribute("event_rate"))
    event_rate = boost::lexical_cast<double>(root->Attribute("event_rate"));
  if (root->Attribute("fast_peaks"))
    fast_peaks = boost::lexical_cast<double>(root->Attribute("fast_peaks"));
  if (root->Attribute("live_time"))
    live_time = boost::lexical_cast<double>(root->Attribute("live_time"));
  if (root->Attribute("ftdt"))
    ftdt = boost::lexical_cast<double>(root->Attribute("ftdt"));
  if (root->Attribute("sfdt"))
    sfdt = boost::lexical_cast<double>(root->Attribute("sfdt"));
}



// to convert Pixie time to lab time
double RunInfo::time_scale_factor() const {
  double tot = state.get_setting(Gamma::Setting("QpxSettings/Pixie-4/System/module/TOTAL_TIME", 23, Gamma::SettingType::floating, 0)).value;
  if (time_stop.is_not_a_date_time() ||
      time_start.is_not_a_date_time() ||
      (tot == 0.0) ||
      (tot == -1))
    return 1.0;
  else
    return (time_stop - time_start).total_microseconds() /
        (1000000 * tot);
}

void RunInfo::to_xml(tinyxml2::XMLPrinter &printer, bool with_settings) const {
  printer.OpenElement("RunInfo");
  printer.PushAttribute("time_start", boost::posix_time::to_iso_extended_string(time_start).c_str());
  printer.PushAttribute("time_stop", boost::posix_time::to_iso_extended_string(time_stop).c_str());
  printer.PushAttribute("total_events", std::to_string(total_events).c_str());
  if (with_settings) {
    state.to_xml(printer);
    //if (!detectors.empty())

  }
  printer.CloseElement();
}

void RunInfo::from_xml(tinyxml2::XMLElement* elem) {
  boost::posix_time::time_input_facet *tif = new boost::posix_time::time_input_facet;
  tif->set_iso_extended_format();
  std::stringstream iss;

  if (elem->Attribute("time_start")) {
    iss << elem->Attribute("time_start");
    iss.imbue(std::locale(std::locale::classic(), tif));
    iss >> time_start;
  }
  if (elem->Attribute("time_stop")) {
    iss << elem->Attribute("time_stop");
    iss.imbue(std::locale(std::locale::classic(), tif));
    iss >> time_stop;
  }
  if (elem->Attribute("total_events"))
    total_events = boost::lexical_cast<uint64_t>(elem->Attribute("total_events"));

  if ((elem = elem->FirstChildElement("Setting")) != nullptr) {
    state = Gamma::Setting(elem);
  }
}

}
