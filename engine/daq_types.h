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
#include <unordered_map>
#include <boost/date_time.hpp>
#include "generic_setting.h"
#include "detector.h"
#include "custom_logger.h"
#include "xmlable.h"

namespace Qpx {

struct TimeStamp {
  uint64_t time_native;
  double timebase_multiplier;
  double timebase_divider;

  TimeStamp() {
    time_native = 0;
    timebase_multiplier = 0;
    timebase_divider = 1;
  }

  double to_nanosec() const;
  std::string to_string() const;

  double operator-(const TimeStamp other) const {
    return (to_nanosec() - other.to_nanosec());
  }

  bool same_base(const TimeStamp other) const {
    return ((timebase_divider == other.timebase_divider) && (timebase_multiplier == other.timebase_multiplier));
  }

  bool operator<(const TimeStamp other) const;
  bool operator>(const TimeStamp other) const;
  bool operator<=(const TimeStamp other) const;
  bool operator>=(const TimeStamp other) const;
  bool operator==(const TimeStamp other) const;
  bool operator!=(const TimeStamp other) const;

};

class DigitizedVal {
public:

  DigitizedVal()   {
    val_ = 0;
    bits_ = 16;
  }

  DigitizedVal(uint16_t v, uint16_t b) {
    val_ = v;
    bits_ = b;
  }

  void set_val(uint16_t v) {
    val_ = v;
  }

  std::string to_string() const;

  uint16_t bits() const { return bits_; }

  uint16_t val(uint16_t bits) const {
    if (bits == bits_)
      return val_;
    else if (bits < bits_)
      return val_ >> (bits_ - bits);
    else
      return val_ << (bits - bits_);
  }

  bool operator==(const DigitizedVal other) const {
    if (val_ != other.val_) return false;
    if (bits_ != other.bits_) return false;
    return true;
  }
  bool operator!=(const DigitizedVal other) const { return !operator==(other); }

private:
  uint16_t  val_;
  uint16_t  bits_;
};

struct Hit {

  int16_t       source_channel;
  TimeStamp     timestamp;
  DigitizedVal  energy;
  std::unordered_map<std::string, uint16_t> extras;
  std::vector<uint16_t> trace;
  
  inline Hit() {
    source_channel = -1;
  }

  std::string to_string() const;

  void write_bin(std::ofstream &outfile, bool extras = false) const;
  void read_bin(std::ifstream &infile, std::map<int16_t, Hit> &model_hits, bool extras = false);

  bool operator<(const Hit other) const {return (timestamp < other.timestamp);}
  bool operator>(const Hit other) const {return (timestamp > other.timestamp);}
  bool operator==(const Hit other) const {
    if (source_channel != other.source_channel) return false;
    if (timestamp != other.timestamp) return false;
    if (energy != other.energy) return false;
    if (extras != other.extras) return false;
    if (trace != other.trace) return false;
    return true;
  }
  bool operator!=(const Hit other) const { return !operator==(other); }
};

struct Event {
  TimeStamp              lower_time;
  double                 window_ns;
  std::map<int16_t, Hit> hits;

  bool in_window(const Hit& h) const;
  bool past_due(const Hit& h) const;
  bool antecedent(const Hit& h) const;
  bool addHit(const Hit &newhit);

  std::string to_string() const;

  inline Event() {
    window_ns = 0.0;
  }

  inline Event(const Hit &newhit, double win) {
    lower_time = newhit.timestamp;
    hits[newhit.source_channel] = newhit;
    window_ns = win;
  }
};

enum StatsType {
  start, running, stop
};

struct StatsUpdate : public XMLable {
  StatsType stats_type;
  int16_t   source_channel;
  Hit       model_hit;

  uint64_t  events_in_spill;

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
      , source_channel(-1)
      , total_time(0.0)
      , event_rate(0.0)
      , events_in_spill(0)
      , fast_peaks(0.0)
      , live_time(0.0)
      , ftdt(0.0)
      , sfdt(0.0)
  {}

  std::string to_string() const;

  StatsUpdate operator-(const StatsUpdate) const;
  StatsUpdate operator+(const StatsUpdate) const;
  bool operator<(const StatsUpdate other) const {return (lab_time < other.lab_time);}
  bool operator==(const StatsUpdate other) const;
  bool operator!=(const StatsUpdate other) const {return !operator ==(other);}
  bool shallow_equals(const StatsUpdate& other) const {
    return ((lab_time == other.lab_time) && (source_channel == other.source_channel));
  }

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
