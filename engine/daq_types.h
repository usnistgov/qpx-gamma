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
 *      Types for organizing data aquired from device
 *        Qpx::Hit        single energy event with coincidence flags
 *        Qpx::StatsUdate metadata for one spill (memory chunk)
 *        Qpx::RunInfo    metadata for the whole run
 *        Qpx::Spill      bundles all data and metadata for a list run
 *        Qpx::ListData   bundles hits in vector and run metadata
 *
 ******************************************************************************/

#ifndef DAQ_TYPES
#define DAQ_TYPES

#include <array>
#include <bitset>
#include <boost/date_time.hpp>
#include "generic_setting.h"
#include "detector.h"
#include "custom_logger.h"
#include "xmlable.h"

namespace Qpx {

struct TimeStamp {
  uint64_t time;

  TimeStamp() {
    time = 0;
  }

  double operator-(const TimeStamp other) const {
    return (static_cast<double>(time) - static_cast<double>(other.time));
  }
  bool operator<(const TimeStamp other) const {return (time < other.time);}
  bool operator>(const TimeStamp other) const {return (time > other.time);}
  bool operator==(const TimeStamp other) const {return (time == other.time);}
  bool operator!=(const TimeStamp other) const {return (time != other.time);}

};

struct Hit : public XMLable {

  int16_t channel;
  TimeStamp timestamp;

  uint16_t energy,
           XIA_PSA,
           user_PSA;

  std::vector<uint16_t> trace;
  
  inline Hit() {
    channel = -1;
    energy = 0;
    XIA_PSA = 0;
    user_PSA = 0;
  }

  boost::posix_time::time_duration to_posix_time();

  void from_xml(const pugi::xml_node &) override {}
  void to_xml(pugi::xml_node &) const override;


  std::string to_string() const;
  std::string xml_element_name() const override {return "Hit";}

  bool operator<(const Hit other) const {return (timestamp < other.timestamp);}
  bool operator>(const Hit other) const {return (timestamp > other.timestamp);}
  bool operator==(const Hit other) const {
    if (channel != other.channel) return false;
    if (timestamp != other.timestamp) return false;
    if (energy != other.energy) return false;
    if (XIA_PSA != other.XIA_PSA) return false;
    if (user_PSA != other.user_PSA) return false;
    if (trace != other.trace) return false;
    return true;
  }
  bool operator!=(const Hit other) const { return !operator==(other); }
  bool shallow_equals(const Hit& other) const {return (timestamp == other.timestamp);}
};

struct Event {
  TimeStamp lower_time, upper_time;
  double window;
  std::map<int16_t, Hit> hit;

  bool in_window(const TimeStamp& ts) const;
  void addHit(const Hit &newhit);
  std::string to_string() const;

  inline Event() {
    window = 0.0;
  }

  inline Event(const Hit &newhit, double win) {
    lower_time = upper_time = newhit.timestamp;
    hit[newhit.channel] = newhit;
    window = win;
  }
};

enum StatsType {
  start, running, stop
};

struct StatsUpdate : public XMLable {
  StatsType stats_type;
  int16_t channel;
//  uint64_t spill_number;
  uint64_t events_in_spill;

  //per module
  double total_time,
    event_rate,
    fast_peaks,
    live_time,
    ftdt,
    sfdt;

  boost::posix_time::ptime lab_time;  //timestamp at end of spill
  
  inline StatsUpdate()
      : stats_type(StatsType::running)
      , channel(-1)
//      , spill_number(0)
      , total_time(0.0)
      , event_rate(0.0)
      , events_in_spill(0)
      , fast_peaks(0.0)
      , live_time(0.0)
      , ftdt(0.0)
      , sfdt(0.0)
  {}

  StatsUpdate operator-(const StatsUpdate) const;
  StatsUpdate operator+(const StatsUpdate) const;
  bool operator<(const StatsUpdate other) const {return (lab_time < other.lab_time);}
  bool operator==(const StatsUpdate other) const;
  bool operator!=(const StatsUpdate other) const {return !operator ==(other);}
  bool shallow_equals(const StatsUpdate& other) const {return ((lab_time == other.lab_time) && (channel == other.channel));}

  void from_xml(const pugi::xml_node &) override;
  void to_xml(pugi::xml_node &) const override;

  std::string xml_element_name() const override {return "StatsUpdate";}

};

struct RunInfo : public XMLable {
  Gamma::Setting state;
  std::vector<Gamma::Detector> detectors;
  uint64_t total_events;
  boost::posix_time::ptime time_start, time_stop;

  inline RunInfo()
      : total_events(0)
      //      , time_start(boost::posix_time::second_clock::local_time())
      //      , time_stop(boost::posix_time::second_clock::local_time())
  {}

  // to convert Pixie time to lab time
  double time_scale_factor() const;

  void from_xml(const pugi::xml_node &) override;
  void to_xml(pugi::xml_node &, bool with_settings = true) const;
  void to_xml(pugi::xml_node &node) const override {to_xml(node, true);}

  std::string xml_element_name() const override {return "RunInfo";}
  bool shallow_equals(const RunInfo& other) const {return (time_start == other.time_start);}
  bool operator!= (const RunInfo& other) const {return !(operator==(other));}
  bool operator== (const RunInfo& other) const;

};


struct Spill {
  inline Spill()/*: spill_number(0)*/ {}
  bool operator==(const Spill other) const {
    if (stats != other.stats)
      return false;
    if (run != other.run)
      return false;
    if (data != other.data)
      return false;
    if (hits.size() != other.hits.size())
      return false;
    return true;
  }
  bool operator!=(const Spill other) const {return !operator ==(other);}

  //bool operator<(const Spill other) const {return (spill_number < other.spill_number);}

  //uint64_t spill_number;

  std::vector<uint32_t>  data;  //as is from device, unparsed
  std::list<Qpx::Hit>    hits;  //parsed
  std::list<StatsUpdate> stats;
  RunInfo                run; //make this not pointer
};

struct ListData {
  std::vector<Qpx::Hit> hits;
  Qpx::RunInfo run;
};

}

#endif
