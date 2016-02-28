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
 *        Qpx::StatsUdate metadata for one spill (memory chunk)
 *        Qpx::RunInfo    metadata for the whole run
 *        Qpx::Spill      bundles all data and metadata for a list run
 *        Qpx::ListData   bundles hits in vector and run metadata
 *
 ******************************************************************************/

#ifndef QPX_DAQ_TYPES
#define QPX_DAQ_TYPES

#include <array>
#include <boost/date_time.hpp>
#include "generic_setting.h"
#include "detector.h"
#include "custom_logger.h"
#include "xmlable.h"

namespace Qpx {

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
  Qpx::Setting state;
  std::vector<Qpx::Detector> detectors;
  boost::posix_time::ptime time;

  inline RunInfo()
  {}

  void from_xml(const pugi::xml_node &) override;
  void to_xml(pugi::xml_node &, bool with_settings = true) const;
  void to_xml(pugi::xml_node &node) const override {to_xml(node, true);}

  std::string xml_element_name() const override {return "RunInfo";}
  bool shallow_equals(const RunInfo& other) const {return (time == other.time);}
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
  std::list<Qpx::Hit>    hits;  //as parsed
  std::map<int16_t, StatsUpdate> stats;
  RunInfo                run;
};

struct ListData {
  std::vector<Qpx::Hit> hits;
  Qpx::RunInfo run;
};

}

#endif
