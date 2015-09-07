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

#ifndef PIXIE_DAQ_TYPES
#define PIXIE_DAQ_TYPES

#include <vector>
#include <array>
#include <bitset>
#include <boost/date_time.hpp>
#include "px_settings.h"
#include "custom_logger.h"

namespace Pixie {

const int kNumChans = 4;

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

struct Hit{

  int16_t module, channel;
  TimeStamp timestamp;

  uint16_t run_type,
           energy,
           XIA_PSA,
           user_PSA;

  std::vector<uint16_t> trace;
  
  inline Hit() {
    module = -1;
    channel = -1;
    run_type = 0;
    energy = 0;
    XIA_PSA = 0;
    user_PSA = 0;
  }

  boost::posix_time::time_duration to_posix_time();
  void from_xml(tinyxml2::XMLElement*);
  void to_xml(tinyxml2::XMLPrinter&, bool with_pattern = true) const;
  std::string to_string() const;

  bool operator<(const Hit other) const {return (timestamp < other.timestamp);}
  bool operator>(const Hit other) const {return (timestamp > other.timestamp);}
  bool operator==(const Hit other) const {
    if (module != other.module) return false;
    if (channel != other.channel) return false;
    if (timestamp != other.timestamp) return false;
    if (run_type != other.run_type) return false;
    if (energy != other.energy) return false;
    if (XIA_PSA != other.XIA_PSA) return false;
    if (user_PSA != other.user_PSA) return false;
    return true;
  }
  bool operator!=(const Hit other) const { return !operator==(other); }
};

struct Event {
  TimeStamp lower_time, upper_time;
  double window;
  std::vector<Hit> hit;

  bool in_window(const TimeStamp& ts) const;
  void addHit(const Hit &newhit);
  std::string to_string() const;

  inline Event() {
    hit.resize(kNumChans);
    window = 0.0;
  }

  inline Event(const Hit &newhit, double win) {
    hit.resize(kNumChans);
    lower_time = upper_time = newhit.timestamp;
    hit[newhit.channel] = newhit;
    window = win;
  }
};

struct StatsUpdate {
  uint64_t spill_number;
  uint64_t events_in_spill;

  //per module
  double total_time,
         event_rate;

  //per channel
  std::vector<double> fast_peaks,
                      live_time,
                      ftdt,
                      sfdt;

  boost::posix_time::ptime lab_time;  //timestamp at end of spill
  
  inline StatsUpdate()
    : spill_number(0)
    , total_time(0.0)
    , event_rate(0.0)
    , events_in_spill(0)
  {
    fast_peaks.resize(kNumChans, 0.0);
    live_time.resize(kNumChans, 0.0);
    ftdt.resize(kNumChans, 0.0);
    sfdt.resize(kNumChans, 0.0);
  }

  inline void eat_stats(Settings& source) {
    for (int i=0; i < kNumChans; i++) {
      fast_peaks[i] = source.get_chan("FAST_PEAKS", Channel(i));
      live_time[i]  = source.get_chan("LIVE_TIME", Channel(i));
      ftdt[i]       = source.get_chan("FTDT", Channel(i));
      sfdt[i]       = source.get_chan("SFDT", Channel(i));
    }
    event_rate = source.get_mod("EVENT_RATE");
    total_time = source.get_mod("TOTAL_TIME");
  }

  StatsUpdate operator-(const StatsUpdate) const;
  StatsUpdate operator+(const StatsUpdate) const;
  bool operator<(const StatsUpdate other) const {return (spill_number < other.spill_number);}
  bool operator==(const StatsUpdate other) const;
  void from_xml(tinyxml2::XMLElement*);
  void to_xml(tinyxml2::XMLPrinter&) const;
};

struct RunInfo {
  Settings p4_state;
  uint64_t total_events;
  boost::posix_time::ptime time_start, time_stop;

  inline RunInfo(): total_events(0),
      time_start(boost::posix_time::second_clock::local_time()),
      time_stop(boost::posix_time::second_clock::local_time()) {
  }

  // to convert Pixie time to lab time
  double time_scale_factor() const;
  void from_xml(tinyxml2::XMLElement*);
  void to_xml(tinyxml2::XMLPrinter&, bool with_settings = true)  const;
};


struct Spill {
  inline Spill(): stats(nullptr), run(nullptr) {}
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

  std::vector<uint32_t> data;  //as is from Pixie, unparsed
  std::list<Pixie::Hit> hits;  //parsed
  StatsUpdate* stats;
  RunInfo*     run;
};

struct ListData {
  std::vector<Pixie::Hit> hits;
  Pixie::RunInfo run;
};

}

#endif
