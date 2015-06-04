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

const int kNumChans = 32;

struct Hit{
  
  //common to whole buffer
  uint16_t run_type,
           module,
           buf_time_hi,
           buf_time_mi,
           buf_time_lo,
           evt_time_hi,
           evt_time_lo;

  //unique to event
  std::bitset<Pixie::kNumChans> pattern;

  //per channel
  std::vector<uint16_t> energy,
                        chan_trig_time,
                        XIA_PSA,
                        user_PSA,
                        chan_real_time;
  std::vector<std::vector<uint16_t>> trace;
  
  inline Hit() {
    chan_trig_time.resize(kNumChans,0);
    energy.resize(kNumChans,0);
    XIA_PSA.resize(kNumChans,0);
    user_PSA.resize(kNumChans,0);
    chan_real_time.resize(kNumChans,0);
    trace.resize(kNumChans);
  }

  boost::posix_time::time_duration to_posix_time() {
    //converts Pixie ticks to posix duration
    //this is not working well...
    uint64_t result = evt_time_lo;
    result += evt_time_hi * 65536;      //pow(2,16)
    result += buf_time_hi * 4294967296; //pow(2,32)
    result = result * 1000 / 75;
    boost::posix_time::time_duration answer =
        boost::posix_time::seconds(result / 1000000000) +
        boost::posix_time::milliseconds((result % 1000000000) / 1000000) +
        boost::posix_time::microseconds((result % 1000000) / 1000);
    //        + boost::posix_time::nanoseconds(result % 1000);
    return answer;
  }
  
};

struct StatsUpdate {
  uint64_t spill_count;

  //per module
  double total_time,
         event_rate;

  //per channel
  std::vector<double> fast_peaks,
                      live_time,
                      ftdt,
                      sfdt;

  boost::posix_time::ptime lab_time;  //timestamp at end of spill
  
  inline StatsUpdate(): spill_count(0), total_time(0.0), event_rate(0.0) {
    fast_peaks.resize(kNumChans, 0.0);
    live_time.resize(kNumChans, 0.0);
    ftdt.resize(kNumChans, 0.0);
    sfdt.resize(kNumChans, 0.0);
  }

  inline void eat_stats(Settings& source) {
    /*
    for (int i=0; i < kNumChans; i++) {
      fast_peaks[i] = source.get_chan("FAST_PEAKS", Channel(i));
      live_time[i]  = source.get_chan("LIVE_TIME", Channel(i));
      ftdt[i]       = source.get_chan("FTDT", Channel(i));
      sfdt[i]       = source.get_chan("SFDT", Channel(i));
    }
    event_rate = source.get_mod("EVENT_RATE");
    total_time = source.get_mod("TOTAL_TIME");*/
  }

  // difference across all variables
  // except rate wouldn't make sense
  // and timestamp would require duration output type
  StatsUpdate operator-(const StatsUpdate other) const {
      StatsUpdate answer;
      for (int i=0; i < kNumChans; i++) {
        answer.fast_peaks[i] = fast_peaks[i] - other.fast_peaks[i];
        answer.live_time[i]  = live_time[i] - other.live_time[i];
        answer.ftdt[i]       = ftdt[i] - other.ftdt[i];
        answer.sfdt[i]       = sfdt[i] - other.sfdt[i];
      }
      answer.total_time = total_time - other.total_time;
      return answer;
  }

  // stacks two, adding up all variables
  // except rate wouldn't make sense, neither does timestamp
  StatsUpdate operator+(const StatsUpdate other) const {
      StatsUpdate answer;
      for (int i=0; i < kNumChans; i++) {
        answer.fast_peaks[i] = fast_peaks[i] + other.fast_peaks[i];
        answer.live_time[i]  = live_time[i] + other.live_time[i];
        answer.ftdt[i]       = ftdt[i] + other.ftdt[i];
        answer.sfdt[i]       = sfdt[i] + other.sfdt[i];
      }
      answer.total_time = total_time + other.total_time;
      return answer;
  }

  friend std::ostream &operator<<(std::ostream &out, StatsUpdate d) {
      out << "{StatsUpdate} "
          << " lab_time="  << to_simple_string(d.lab_time)
          << " spill_count=" << d.spill_count
          << " total_time=" << d.total_time
          << " event_rate=" << d.event_rate
          << "\n";
      for (int i=0; i < kNumChans; i++) {
        out << "{StatsUpdate chan" << i << "} "
            << " fast_peaks=" << d.fast_peaks[i]
            << " live_time=" << d.live_time[i]
            << " ftdt=" << d.ftdt[i]
            << " sfdt=" << d.sfdt[i]
            << "\n";
      }
  }

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
  double time_scale_factor() const {
    if (time_stop.is_not_a_date_time() ||
        time_start.is_not_a_date_time()) /* ||
        (p4_state.get_mod("TOTAL_TIME") == 0.0) ||
        (p4_state.get_mod("TOTAL_TIME") == -1))*/
      return 1.0;
    else 
      return (time_stop - time_start).total_microseconds() /
          (1000000 /* * p4_state.get_mod("TOTAL_TIME")*/);
  }
};


struct Spill {
  inline Spill(): stats(nullptr), run(nullptr) {}

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
