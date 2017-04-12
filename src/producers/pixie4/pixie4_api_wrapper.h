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
#include <vector>
#include <string>
#include <set>

struct Pixie4Wrapper
{
  static constexpr uint32_t max_buf_len {8192}; //get from module?
  static constexpr uint32_t list_mem_len32 {16 * 8192};
  static constexpr uint32_t list_mem_len16 {32 * 8192};

  Pixie4Wrapper();

  void set_num_modules(uint16_t nmod);
  bool boot(const std::vector<std::string>& boot_files);

  ////////////////
  /// SETTINGS ///
  ////////////////
  void get_all_settings();
  void get_all_stats();
  void get_all_settings(std::set<uint16_t> mods);
  void get_all_stats(std::set<uint16_t> mods);

  //system
  void set_sys(const std::string&, double);
  double get_sys(const std::string&);
  double get_sys(uint16_t idx);
  void get_sys_all();

  //module
  static uint16_t hard_max_modules();
  bool module_valid(int16_t mod) const;
  void set_mod(uint16_t mod, const std::string& name, double val);
  double get_mod(uint16_t mod, const std::string& name) const;
  double get_mod(uint16_t mod, uint16_t idx) const;

  void get_mod_all(uint16_t mod);
  void get_mod_stats(uint16_t mod);

  void get_mod_all(std::set<uint16_t> mods);
  void get_mod_stats(std::set<uint16_t> mods);

  //channel
  static bool channel_valid(int16_t chan);
  double set_chan(uint16_t mod, uint16_t chan, const std::string& name, double val);
  double get_chan(uint16_t mod, uint16_t chan, const std::string& name) const;
  double get_chan(uint16_t mod, uint16_t chan, uint16_t idx) const;

  void get_chan_all(uint16_t mod, uint16_t chan);
  void get_chan_stats(uint16_t mod, uint16_t chan);

  void get_chan_all(uint16_t mod);
  void get_chan_stats(uint16_t mod);


  ///////////
  /// DAQ ///
  ///////////

  void reset_counters_next_run();
  bool start_run(uint16_t type);
  bool resume_run(uint16_t type);  //unused
  bool stop_run(uint16_t type);

  std::set<uint16_t> get_triggered_modules() const;

  // read and write external memory
  static bool write_EM(uint32_t* data, uint8_t mod);
  static bool read_EM(uint32_t* data,uint8_t mod);
  static bool read_EM_double_buffered(uint32_t* data, uint8_t mod);
  static bool clear_EM(uint8_t mod);

  // CONTROL TASKS
  uint32_t* control_collect_ADC(uint8_t module);
  bool control_set_DAC(uint8_t module);       //unused
  bool control_connect(uint8_t module);       //unused
  bool control_disconnect(uint8_t module);    //unused
  bool control_program_Fippi(uint8_t module); //unused
  bool control_measure_baselines();
  bool control_measure_baselines(uint8_t mod);
  bool control_test_EM_write(uint8_t module); //unused
  bool control_test_HM_write(uint8_t module); //unused
  bool control_compute_BLcut();
  bool control_find_tau(uint8_t mod);
  bool control_find_tau();
  bool control_adjust_offsets(uint8_t mod);
  bool control_adjust_offsets();

protected:

  /////////////////
  /// LOW LEVEL ///
  /////////////////

  //DAQ//
  bool start_run(uint8_t mod, uint16_t runtype);
  bool resume_run(uint8_t mod, uint16_t runtype); //unused
  bool stop_run(uint8_t mod, uint16_t runtype);

  //poll runs
  static uint32_t poll_run(uint8_t mod, uint16_t runtype);
  static uint32_t poll_run_dbl(uint8_t mod);

  //carry out task
  bool write_sys(const char*);
  bool write_mod(const char*, uint8_t);
  bool write_chan(const char*, uint8_t, uint8_t);
  bool read_sys(const char*);
  bool read_mod(const char*, uint8_t);
  bool read_chan(const char*, uint8_t, uint8_t);
  
  //find index
  static uint16_t i_dsp(const char*);
  static uint16_t i_sys(const char*);
  static uint16_t i_mod(const char*);
  static uint16_t i_chan(const char*);
  static uint16_t mod_val_idx(uint16_t mod, uint16_t idx);
  static uint16_t chan_val_idx(uint16_t mod, uint16_t chan, uint16_t idx);
  
  //interpret error
  bool set_err(int32_t);
  bool boot_err(int32_t);
  bool control_err(int32_t);

  std::vector<double> system_parameter_values_;
  std::vector<double> module_parameter_values_;
  std::vector<double> channel_parameter_values_;
  uint16_t num_modules_ {0};
};
