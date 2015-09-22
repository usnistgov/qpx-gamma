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
 *      Qpx::Plugin
 *
 ******************************************************************************/

#ifndef PIXIE_PLUGIN
#define PIXIE_PLUGIN

#include "daq_device.h"
#include "tinyxml2.h"
#include "detector.h"
#include <boost/thread.hpp>
#include <boost/atomic.hpp>

namespace Qpx {

enum class Module     : int {all = -3, none = -2, invalid = -1};
enum class Channel    : int {all = -3, none = -2, invalid = -1};
const uint32_t max_buf_len  = 8192; //get this from module
const uint32_t list_mem_len32 = 16 * max_buf_len;
const uint32_t list_mem_len16 = 2 * list_mem_len32;


class Plugin : public DaqDevice {
  
public:

  Plugin();
  Plugin(const Plugin& other);      //sets state to history if copied
  Plugin(tinyxml2::XMLElement* root, std::vector<Gamma::Detector>& dets); //create from xml node
  ~Plugin();

  
  //Overrides
  bool write_settings_bulk(Gamma::Setting &set) override;
  bool read_settings_bulk(Gamma::Setting &set) const override;
  void get_all_settings() override;
  bool boot() override;
  bool execute_command(Gamma::Setting &set) override;
  std::map<int, std::vector<uint16_t>> oscilloscope() override;
  bool start_daq(uint64_t timeout, SynchronizedQueue<Spill*>* out_queue) override;
  bool stop_daq() override;


  //Unique
  void get_stats(int module);
  void reset_counters_next_run();

  //DEPRECATE//
  void to_xml(tinyxml2::XMLPrinter&, std::vector<Gamma::Detector> dets) const;

private:
  //Acquisition threads, use as static functors
  static void worker_parse(SynchronizedQueue<Spill*>* in_queue, SynchronizedQueue<Spill*>* out_queue);
  static void worker_run(Plugin* callback, uint64_t timeout_limit, SynchronizedQueue<Spill*>* spill_queue, boost::atomic<bool>* interruptor);
  static void worker_run_dbl(Plugin* callback, uint64_t timeout_limit, SynchronizedQueue<Spill*>* spill_queue, boost::atomic<bool>* interruptor);
  static void worker_run_test(Plugin* callback, uint64_t timeout_limit, SynchronizedQueue<Spill*>* spill_queue, boost::atomic<bool>* interruptor);


protected:

  //DEPRECATE//
  std::vector<Gamma::Detector> from_xml(tinyxml2::XMLElement*);

  boost::atomic<bool>* interruptor_;
  boost::thread *runner_;
  boost::thread *parser_;
  SynchronizedQueue<Spill*>* raw_queue_;

  //member variables
  bool run_double_buffer_;
  int  run_type_;
  int  run_poll_interval_ms_;

  std::vector<std::string> boot_files_;
  std::vector<double> system_parameter_values_;
  std::vector<double> module_parameter_values_;
  std::vector<double> channel_parameter_values_;


  //////////for internal use only///////////

  //CONTROL TASKS//

  uint32_t* control_collect_ADC(uint8_t module);
  bool control_set_DAC(uint8_t module);
  bool control_connect(uint8_t module);
  bool control_disconnect(uint8_t module);
  bool control_program_Fippi(uint8_t module);
  bool control_measure_baselines(uint8_t module);
  bool control_test_EM_write(uint8_t module);
  bool control_test_HM_write(uint8_t module);
  bool control_compute_BLcut();
  bool control_find_tau();
  bool control_adjust_offsets();

  //start and stop runs
  static bool start_run(uint16_t type);
  static bool resume_run(uint16_t type);
  static bool stop_run(uint16_t type);

  //poll runs
  static uint32_t poll_run(uint16_t type);
  static uint32_t poll_run_dbl();

  //read and write external memory
  static bool write_EM(uint32_t* data);
  static bool read_EM(uint32_t* data);
  static bool read_EM_dbl(uint32_t* data);
  static bool clear_EM();

  //carry out task
  bool write_sys(const char*);
  bool write_mod(const char*, uint8_t);
  bool write_chan(const char*, uint8_t, uint8_t);
  bool read_sys(const char*);
  bool read_mod(const char*, uint8_t);
  bool read_chan(const char*, uint8_t, uint8_t);
  
  //find index
  static uint16_t i_dsp(const char*);
  uint16_t i_sys(const char*) const;
  uint16_t i_mod(const char*) const;
  uint16_t i_chan(const char*) const;
  
  //print error
  void set_err(int32_t);
  void boot_err(int32_t);
  bool control_err(int32_t);

  //system
  void set_sys(const std::string&, double);
  double get_sys(const std::string&);
  void get_sys_all();

  //module
  void set_mod(const std::string&, double, Module mod);
  double get_mod(const std::string&, Module mod) const;
  double get_mod(const std::string&, Module mod,
                 LiveStatus force = LiveStatus::offline);
  void get_mod_all(Module mod);
  void get_mod_stats(Module mod);

  //channel
  void set_chan(const std::string&, double val,
                Channel channel,
                Module  module);
  void set_chan(uint8_t setting, double val,
                Channel channel,
                Module  module);
  double get_chan(const std::string&,
                  Channel channel,
                  Module  module) const;
  double get_chan(const std::string&,
                  Channel channel,
                  Module  module,
                  LiveStatus force = LiveStatus::offline);
  double get_chan(uint8_t setting,
                  Channel channel,
                  Module  module) const;
  double get_chan(uint8_t setting,
                  Channel channel,
                  Module  module,
                  LiveStatus force = LiveStatus::offline);
  void get_chan_all(Channel channel,
                    Module  module);
  void get_chan_stats(Module  module);

};

}

#endif
