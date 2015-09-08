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
 *        Pixie::Hit        single energy event with coincidence flags
 *        Pixie::StatsUdate metadata for one spill (memory chunk from Pixie)
 *        Pixie::RunInfo    metadata for the whole run
 *        Pixie::Spill      bundles all data and metadata for a list run
 *        Pixie::ListData   bundles hits in vector and run metadata
 *
 ******************************************************************************/

#include "daq_types.h"
#include <boost/algorithm/string.hpp>

namespace Pixie {

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

  printer.PushAttribute("module", std::to_string(module).c_str());
  printer.PushAttribute("channel", std::to_string(channel).c_str());
  printer.PushAttribute("run_type", std::to_string(run_type).c_str());
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
  for (int i=0; i < kNumChans; i++) {
    answer.fast_peaks[i] = fast_peaks[i] - other.fast_peaks[i];
    answer.live_time[i]  = live_time[i] - other.live_time[i];
    answer.ftdt[i]       = ftdt[i] - other.ftdt[i];
    answer.sfdt[i]       = sfdt[i] - other.sfdt[i];
  }
  answer.total_time = total_time - other.total_time;
  answer.events_in_spill = events_in_spill - other.events_in_spill;
  return answer;
}

bool StatsUpdate::operator==(const StatsUpdate other) const {
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
  for (int i=0; i < kNumChans; i++) {
    answer.fast_peaks[i] = fast_peaks[i] + other.fast_peaks[i];
    answer.live_time[i]  = live_time[i] + other.live_time[i];
    answer.ftdt[i]       = ftdt[i] + other.ftdt[i];
    answer.sfdt[i]       = sfdt[i] + other.sfdt[i];
  }
  answer.total_time = total_time + other.total_time;
  answer.events_in_spill = events_in_spill + other.events_in_spill;
  return answer;
}

void StatsUpdate::to_xml(tinyxml2::XMLPrinter &printer) const {
  printer.OpenElement("StatsUpdate");
  printer.PushAttribute("lab_time", boost::posix_time::to_iso_extended_string(lab_time).c_str());
  printer.PushAttribute("spill_number", std::to_string(spill_number).c_str());
  printer.PushAttribute("events_in_spill", std::to_string(events_in_spill).c_str());
  printer.PushAttribute("total_time", std::to_string(total_time).c_str());
  printer.PushAttribute("event_rate", std::to_string(event_rate).c_str());

  for (int i=0; i < kNumChans; i++) {
    printer.OpenElement("ChanStats");
    printer.PushAttribute("chan", std::to_string(i).c_str());
    printer.PushAttribute("fast_peaks", std::to_string(fast_peaks[i]).c_str());
    printer.PushAttribute("live_time", std::to_string(live_time[i]).c_str());
    printer.PushAttribute("ftdt", std::to_string(ftdt[i]).c_str());
    printer.PushAttribute("sfdt", std::to_string(sfdt[i]).c_str());
    printer.CloseElement(); //ChanStats
  }

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

  if (root->Attribute("spill_number"))
    spill_number = boost::lexical_cast<uint64_t>(root->Attribute("spill_number"));
  if (root->Attribute("events_in_spill"))
    events_in_spill = boost::lexical_cast<uint64_t>(root->Attribute("events_in_spill"));
  if (root->Attribute("total_time"))
    total_time = boost::lexical_cast<double>(root->Attribute("total_time"));
  if (root->Attribute("event_rate"))
    event_rate = boost::lexical_cast<double>(root->Attribute("event_rate"));

  tinyxml2::XMLElement* elem = root->FirstChildElement("ChanStats");
  while (elem != nullptr) {
    if (elem->Attribute("chan") != nullptr) {
      int chan = boost::lexical_cast<int>(elem->Attribute("chan"));
      if ((chan >= 0) && (chan < kNumChans)) {
        if (elem->Attribute("fast_peaks"))
          fast_peaks[chan] = boost::lexical_cast<double>(elem->Attribute("fast_peaks"));
        if (elem->Attribute("live_time"))
          live_time[chan] = boost::lexical_cast<double>(elem->Attribute("live_time"));
        if (elem->Attribute("ftdt"))
          ftdt[chan] = boost::lexical_cast<double>(elem->Attribute("ftdt"));
        if (elem->Attribute("sfdt"))
          sfdt[chan] = boost::lexical_cast<double>(elem->Attribute("sfdt"));
      }
      elem = dynamic_cast<tinyxml2::XMLElement*>(elem->NextSibling());
    }
  }

}



// to convert Pixie time to lab time
double RunInfo::time_scale_factor() const {
  if (time_stop.is_not_a_date_time() ||
      time_start.is_not_a_date_time())/* ||
      (p4_state.get_mod("TOTAL_TIME") == 0.0) ||
      (p4_state.get_mod("TOTAL_TIME") == -1))*/
    return 1.0;
  else
    return (time_stop - time_start).total_microseconds() /
        (1000000 /* * p4_state.get_mod("TOTAL_TIME")*/);
}

void RunInfo::to_xml(tinyxml2::XMLPrinter &printer, bool with_settings) const {
  printer.OpenElement("RunInfo");
  printer.PushAttribute("time_start", boost::posix_time::to_iso_extended_string(time_start).c_str());
  printer.PushAttribute("time_stop", boost::posix_time::to_iso_extended_string(time_stop).c_str());
  printer.PushAttribute("total_events", std::to_string(total_events).c_str());
  if (with_settings)
    p4_state.to_xml(printer);
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

  if ((elem = elem->FirstChildElement("PixieSettings")) != nullptr) {
    p4_state = Settings(elem);
  }
}

}
