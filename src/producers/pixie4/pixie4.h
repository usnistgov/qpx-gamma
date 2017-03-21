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
 *      Qpx::Pixie4
 *
 ******************************************************************************/

#pragma once

#include "producer.h"
#include "detector.h"
#include <boost/thread.hpp>
#include <boost/atomic.hpp>

namespace Qpx {

enum class Module     : int {all = -3, none = -2, invalid = -1};
enum class Channel    : int {all = -3, none = -2, invalid = -1};
const uint32_t max_buf_len  = 8192; //get this from module?
const uint32_t list_mem_len32 = 16 * max_buf_len;
const uint32_t list_mem_len16 = 2 * list_mem_len32;


class Pixie4 : public Producer {
  
public:

  Pixie4();
//  Pixie4(std::string file);
  ~Pixie4();

  static std::string plugin_name() {return "Pixie4";}
  std::string device_name() const override {return plugin_name();}

  bool write_settings_bulk(Qpx::Setting &set) override;
  bool read_settings_bulk(Qpx::Setting &set) const override;
  void get_all_settings() override;
  bool boot() override;
  bool die() override {status_ = ProducerStatus::loaded | ProducerStatus::can_boot; return true;}

  std::list<Hit> oscilloscope() override;

  bool daq_start(SynchronizedQueue<Spill*>* out_queue) override;
  bool daq_stop() override;
  bool daq_running() override;

private:
  //no copying
  void operator=(Pixie4 const&);
  Pixie4(const Pixie4&);

protected:
  //setup
  int  run_type_;
  int  run_poll_interval_ms_;

  std::string XIA_file_directory_;
  std::vector<std::string> boot_files_;

  std::vector<double> system_parameter_values_;
  std::vector<double> module_parameter_values_;
  std::vector<double> channel_parameter_values_;

  std::vector<std::vector<int32_t>> channel_indices_;

  //Multithreading
  boost::atomic<int> run_status_;
  boost::thread *runner_;
  boost::thread *parser_;
  SynchronizedQueue<Spill*>* raw_queue_;

  static void worker_parse(Pixie4* callback, SynchronizedQueue<Spill*>* in_queue, SynchronizedQueue<Spill*>* out_queue);
  static void worker_run_dbl(Pixie4* callback, SynchronizedQueue<Spill*>* spill_queue);

  //CONVENIENCE FUNCTIONS//
  void rebuild_structure(Qpx::Setting &set);
  void reindex_modules(Qpx::Setting &set);
  void fill_stats(std::map<int16_t, StatsUpdate>&, uint8_t module);
  void reset_counters_next_run();

  //system
  void set_sys(const std::string&, double);
  double get_sys(const std::string&);
  void get_sys_all();

  //module
  void set_mod(const std::string&, double, Module mod);
  double get_mod(const std::string&, Module mod) const;
  void get_mod_all(Module mod);
  void get_mod_stats(Module mod);

  //channel
  double get_chan(const std::string&,
                  Channel channel,
                  Module  module) const;
  void get_chan_all(Channel channel,
                    Module  module);
  void get_chan_stats(Channel channel, Module  module);

  //////////LOW LEVEL///////////

  //CONTROL TASKS//
  uint32_t* control_collect_ADC(uint8_t module);
  bool control_set_DAC(uint8_t module);       //unused
  bool control_connect(uint8_t module);       //unused
  bool control_disconnect(uint8_t module);    //unused
  bool control_program_Fippi(uint8_t module); //unused
  bool control_measure_baselines(Module mod);
  bool control_test_EM_write(uint8_t module); //unused
  bool control_test_HM_write(uint8_t module); //unused
  bool control_compute_BLcut();
  bool control_find_tau(Module mod);
  bool control_adjust_offsets(Module mod);

  //DAQ//
  //start and stop runs
  bool start_run(Module mod);
  bool resume_run(Module mod);  //unused
  bool stop_run(Module mod);
  bool start_run(uint8_t mod);
  bool resume_run(uint8_t mod); //unused
  bool stop_run(uint8_t mod);
  //poll runs
  uint32_t poll_run(uint8_t mod);
  static uint32_t poll_run_dbl(uint8_t mod);
  //read and write external memory
  static bool write_EM(uint32_t* data, uint8_t mod);
  static bool read_EM(uint32_t* data,uint8_t mod);
  static bool read_EM_dbl(uint32_t* data, uint8_t mod);
  static bool clear_EM(uint8_t mod);

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

  HitModel model_hit() const;
};

}
