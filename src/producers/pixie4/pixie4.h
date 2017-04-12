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
 ******************************************************************************/

#pragma once

#include "producer.h"
#include <boost/thread.hpp>
#include <boost/atomic.hpp>
#include "pixie4_api_wrapper.h"

namespace Qpx {


class Pixie4 : public Producer
{
  
public:
  Pixie4();
  ~Pixie4();

  static std::string plugin_name() {return "Pixie4";}
  std::string device_name() const override {return plugin_name();}

  bool write_settings_bulk(Setting &set) override;
  bool read_settings_bulk(Setting &set) const override;
  void get_all_settings() override;
  bool boot() override;
  bool die() override;

  std::list<Hit> oscilloscope() override;

  bool daq_start(SpillQueue out_queue) override;
  bool daq_stop() override;
  bool daq_running() override;

private:
  //no copying
  void operator=(Pixie4 const&);
  Pixie4(const Pixie4&);

protected:
  Pixie4Wrapper PixieAPI;
  std::string XIA_file_directory_;
  std::vector<std::string> boot_files_;

  struct RunSetup
  {
    std::vector<std::vector<int32_t>> indices;
    uint16_t type {0x103};
    int  run_poll_interval_ms {100};

    void set_num_modules(uint16_t nmod);
  }
  run_setup;

  // Threads
  boost::atomic<bool> running_;
  boost::atomic<bool> terminating_;
  boost::thread* runner_ {nullptr};
  boost::thread* parser_ {nullptr};
  SpillQueue raw_queue_ {nullptr};
  static void worker_parse(RunSetup setup, SpillQueue in_queue, SpillQueue out_queue);
  static void worker_run_dbl(Pixie4* callback, SpillQueue spill_queue);

  // Helper functions
  void rebuild_structure(Setting &set);
  void reindex_modules(Setting &set);
  void fill_stats(std::map<int16_t, StatsUpdate>&, uint8_t module);
  Setting default_module() const;

  static double get_value(const Setting& s);
  static void set_value(Setting& s, double val);
  static HitModel model_hit(uint16_t runtype);
};

}
