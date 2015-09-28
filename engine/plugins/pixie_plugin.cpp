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

#include "pixie_plugin.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "common_xia.h"
#include "custom_logger.h"
#include "custom_timer.h"


namespace Qpx {

Plugin::Plugin() {
  boot_files_.resize(7);
  system_parameter_values_.resize(N_SYSTEM_PAR, 0.0);
  module_parameter_values_.resize(PRESET_MAX_MODULES*N_MODULE_PAR, 0.0);
  channel_parameter_values_.resize(PRESET_MAX_MODULES*N_CHANNEL_PAR*NUMBER_OF_CHANNELS, 0.0);

  run_double_buffer_ = true;
  run_poll_interval_ms_ = 100;
  run_type_ = 0x103;

  live_ = LiveStatus::dead;

  interruptor_ = nullptr;
  runner_ = nullptr;
  parser_ = nullptr;
  raw_queue_ = nullptr;
}

Plugin::Plugin(const Plugin& other)
  : DaqDevice(other)
  , runner_(other.runner_)
  , parser_(other.parser_)
  , interruptor_(other.interruptor_)
  , run_type_(other.run_type_)
  , run_double_buffer_(other.run_double_buffer_)
  , run_poll_interval_ms_(other.run_poll_interval_ms_)
  , boot_files_(other.boot_files_)
  , system_parameter_values_(other.system_parameter_values_)
  , module_parameter_values_(other.module_parameter_values_)
  , channel_parameter_values_(other.channel_parameter_values_)
{}

Plugin::~Plugin() {
  stop_daq();
  if (runner_ != nullptr) {
    runner_->detach();
    delete runner_;
  }
  if (parser_ != nullptr) {
    parser_->detach();
    delete parser_;
  }
  if (raw_queue_ != nullptr) {
    raw_queue_->stop();
    delete raw_queue_;
  }
}

//DEPRECATE//
Plugin::Plugin(tinyxml2::XMLElement* root, std::vector<Gamma::Detector>& detectors) :
  Plugin()
{
  detectors = from_xml(root);
}

bool Plugin::start_daq(uint64_t timeout, SynchronizedQueue<Spill*>* out_queue,
                       Gamma::Setting root) {
  if (interruptor_ != nullptr)
    return false;

  if (interruptor_ == nullptr)
    interruptor_ = new boost::atomic<bool>();
  interruptor_->store(false);

  if (runner_ != nullptr) {
    delete runner_;
  }

  for (int i=0; i < channel_indices_.size(); ++i) {
    set_mod("RUN_TYPE",    static_cast<double>(run_type_), Module(i));
    set_mod("MAX_EVENTS",  static_cast<double>(0), Module(i));
  }

  raw_queue_ = new SynchronizedQueue<Spill*>();

  if (run_double_buffer_)
    runner_ = new boost::thread(&worker_run_dbl, this, timeout, raw_queue_, interruptor_, root);
  else
    runner_ = new boost::thread(&worker_run, this, timeout, raw_queue_, interruptor_, root);

  parser_ = new boost::thread(&worker_parse, this, raw_queue_, out_queue);
}

bool Plugin::stop_daq() {
  if ((interruptor_ == nullptr) || interruptor_->load())
    return false;

  interruptor_->store(true);

  if ((runner_ != nullptr) && runner_->joinable()) {
    runner_->join();
    delete runner_;
    runner_ = nullptr;
  }

  wait_ms(500);
  while (raw_queue_->size() > 0)
    wait_ms(1000);
  wait_ms(500);
  raw_queue_->stop();
  wait_ms(500);

  if ((parser_ != nullptr) && parser_->joinable()) {
    parser_->join();
    delete parser_;
    parser_ = nullptr;
  }
  delete interruptor_;
  interruptor_ = nullptr;

  delete raw_queue_;
  raw_queue_ = nullptr;

  return true;
}

void Plugin::fill_stats(std::list<StatsUpdate> &all_stats, uint8_t module) {
  StatsUpdate stats;

  stats.event_rate = get_mod("EVENT_RATE", Module(module));
  stats.total_time = get_mod("TOTAL_TIME", Module(module));
  for (int i=0; i<NUMBER_OF_CHANNELS; ++i) {
    stats.channel    = channel_indices_[module][i];
    stats.fast_peaks = get_chan("FAST_PEAKS", Channel(i), Module(module));
    stats.live_time  = get_chan("LIVE_TIME", Channel(i), Module(module));
    stats.ftdt       = get_chan("FTDT", Channel(i), Module(module));
    stats.sfdt       = get_chan("SFDT", Channel(i), Module(module));
    all_stats.push_back(stats);
  }
}


/*void Plugin::get_stats() {
  get_mod_stats(Module::all);
  get_chan_stats(Channel::all, Module::all);
  }*/

bool Plugin::read_settings_bulk(Gamma::Setting &set) const {
  if (set.name == "Pixie-4") {
    for (auto &q : set.branches.my_data_) {
      if ((q.setting_type == Gamma::SettingType::stem) && (q.name == "Run settings")) {
        for (auto &k : q.branches.my_data_) {
          if ((k.setting_type == Gamma::SettingType::boolean) && (k.name == "Double buffer"))
            k.value_int = run_double_buffer_;
          else if ((k.setting_type == Gamma::SettingType::int_menu) && (k.name == "Run type"))
            k.value_int = run_type_;
          if ((k.setting_type == Gamma::SettingType::integer) && (k.name == "Poll interval"))
            k.value_int = run_poll_interval_ms_;
        }
      } else if ((q.setting_type == Gamma::SettingType::stem) && (q.name == "Files")) {
        for (auto &k : q.branches.my_data_) {
          if (k.setting_type == Gamma::SettingType::file_path)
            k.value_text = boot_files_[k.address];
        }
      } else if ((q.setting_type == Gamma::SettingType::stem) && (q.name == "System")) {
        for (auto &k : q.branches.my_data_) {
          if (k.setting_type == Gamma::SettingType::stem) {
            uint16_t modnum = k.address;
            if (modnum < 0)
              continue;
            for (auto &p : k.branches.my_data_) {
              if (p.setting_type == Gamma::SettingType::stem) {
                uint16_t channum = p.address;
                if (channum < 0)
                  continue;
                for (auto &o : p.branches.my_data_) {
                  if (o.setting_type == Gamma::SettingType::floating)
                    o.value = channel_parameter_values_[o.address + modnum * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + channum * N_CHANNEL_PAR];
                  else if ((o.setting_type == Gamma::SettingType::integer)
                           || (o.setting_type == Gamma::SettingType::boolean)
                           || (o.setting_type == Gamma::SettingType::int_menu)
                           || (o.setting_type == Gamma::SettingType::binary))
                    o.value_int = channel_parameter_values_[o.address + modnum * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + channum * N_CHANNEL_PAR];
                }
              }
              else if (p.setting_type == Gamma::SettingType::floating)
                p.value = module_parameter_values_[modnum * N_MODULE_PAR +  p.address];
              else if ((p.setting_type == Gamma::SettingType::integer)
                       || (p.setting_type == Gamma::SettingType::boolean)
                       || (p.setting_type == Gamma::SettingType::int_menu)
                       || (p.setting_type == Gamma::SettingType::binary))
                p.value_int = module_parameter_values_[modnum * N_MODULE_PAR +  p.address];
            }
          }
          else if (k.setting_type == Gamma::SettingType::floating)
            k.value = system_parameter_values_[k.address];
          else if ((k.setting_type == Gamma::SettingType::integer)
                   || (k.setting_type == Gamma::SettingType::boolean)
                   || (k.setting_type == Gamma::SettingType::int_menu)
                   || (k.setting_type == Gamma::SettingType::binary))
            k.value_int = system_parameter_values_[k.address];
        }
      }
    }
  }
  return true;
}

bool Plugin::write_settings_bulk(Gamma::Setting &set) {
  if (live_ == LiveStatus::history)
    return false;

  if (set.name == "Pixie-4") {

    read_sys("NUMBER_MODULES");
    int expected_modules = system_parameter_values_[0];
    
    for (auto &q : set.branches.my_data_) {
      if ((q.setting_type == Gamma::SettingType::stem) && (q.name == "Files")) {
        for (auto &k : q.branches.my_data_) {
          if (k.setting_type == Gamma::SettingType::file_path)
            boot_files_[k.address] = k.value_text;
        }
      } else if ((q.setting_type == Gamma::SettingType::stem) && (q.name == "Run settings")) {
        for (auto &k : q.branches.my_data_) {
          if (k.name == "Double buffer")
            run_double_buffer_ = k.value_int;
          else if (k.name == "Run type")
            run_type_ = k.value_int;
          else if (k.name == "Poll interval")
            run_poll_interval_ms_ = k.value_int;
        }
      } else if ((q.setting_type == Gamma::SettingType::stem) && (q.name == "System")) {
        int prev_total_modules = 0;
        int new_total_modules = 0;
        Gamma::Setting model_module;
        std::set<int> used_chan_indices;
        bool hardware_changed = false;
        Gamma::Setting maxmods  = q.branches.get(Gamma::Setting("MAX_NUMBER_MODULES", 3, Gamma::SettingType::integer, -1));
        if (maxmods.value_int > N_SYSTEM_PAR - 7)
          maxmods.value_int = N_SYSTEM_PAR - 7;
        uint16_t oldmax = system_parameter_values_[3];
        if (oldmax != maxmods.value_int) {
          hardware_changed = true;
          std::vector<Gamma::Setting> old_modules(N_SYSTEM_PAR - 7);
          for (int i=0; i< N_SYSTEM_PAR - 7; ++i)
            old_modules[i] = q.branches.get(Gamma::Setting("SLOT_WAVE", 7+i, Gamma::SettingType::integer, -1));
          for (int i=0; i< N_SYSTEM_PAR - 7; ++i)
            q.branches.remove_a(Gamma::Setting("SLOT_WAVE", 7+i, Gamma::SettingType::integer, -1));
          system_parameter_values_[3] = maxmods.value_int;
          write_sys("MAX_NUMBER_MODULES");
          for (int i=0; i<maxmods.value_int; ++i) {
            Gamma::Setting slot = old_modules[i];
            if (slot == Gamma::Setting())
              slot = Gamma::Setting("SLOT_WAVE", 7+i, Gamma::SettingType::integer, -1);
            slot.maximum = maxmods.value_int;
            slot.writable = true;
            q.branches.add(slot);
          }
        }
        uint16_t total = 0;
        for (int i=0; i<maxmods.value_int; ++i) {
          Gamma::Setting modslot = q.branches.get(Gamma::Setting("SLOT_WAVE", 7+i, Gamma::SettingType::integer, -1));
          if ((modslot != Gamma::Setting()) && (system_parameter_values_[7+i] != modslot.value_int)) {
            hardware_changed = true;
          }
          if (modslot.value_int != 0)
            total++;
        }
        new_total_modules = total;
        if (system_parameter_values_[0] != total) {
          system_parameter_values_[0] = total;
          write_sys("NUMBER_MODULES");
          hardware_changed = true;
        }
        if (hardware_changed) {
          for (int i=0; i<maxmods.value_int; ++i) {
            Gamma::Setting modslot = q.branches.get(Gamma::Setting("SLOT_WAVE", 7+i, Gamma::SettingType::integer, -1));
            if ((modslot != Gamma::Setting()) && (system_parameter_values_[7+i] != modslot.value_int)) {
              system_parameter_values_[7+i] = modslot.value_int;
            }
          }
          write_sys("SLOT_WAVE");
        }

        if (new_total_modules < 1) {
          PL_DBG << "num modules below 1";
          Gamma::Setting modslot = q.branches.get(Gamma::Setting("SLOT_WAVE", 7, Gamma::SettingType::integer, -1));
          q.branches.remove_a(modslot);
          modslot.value_int = 1;
          q.branches.add(modslot);
          new_total_modules = 1;
          system_parameter_values_[7] = 2;
          write_sys("SLOT_WAVE");          
        }

        if (expected_modules != new_total_modules) {
          PL_DBG << "total modules changed from " << prev_total_modules << " to " << new_total_modules;
          module_indices_.resize(new_total_modules);
          channel_indices_.resize(new_total_modules);
          for (auto &q : channel_indices_)
            q.resize(NUMBER_OF_CHANNELS, -1);
        }

        for (auto &k : q.branches.my_data_) {

          if (k.setting_type == Gamma::SettingType::stem) {
            uint16_t modnum = k.address;
            if (modnum >= channel_indices_.size()) {
              PL_DBG << "module address out of bounds, ignoring branch";
              continue;
            }
            prev_total_modules++;
            if (model_module == Gamma::Setting())
              model_module = k;
            
            for (auto &p : k.branches.my_data_) {
              if (p.setting_type == Gamma::SettingType::stem) {
                uint16_t channum = p.address;
                if (channum >= NUMBER_OF_CHANNELS) {
                  PL_DBG << "channel address out of bounds, ignoring branch";
                  continue;
                }
                
                for (auto &o : p.branches.my_data_) {
                  if (o.index >= 0) {
                    used_chan_indices.insert(o.index);
                    channel_indices_[modnum][channum] = o.index;
                  }
                  
                  if (o.writable && (o.setting_type == Gamma::SettingType::floating) && (channel_parameter_values_[o.address + modnum * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + channum * N_CHANNEL_PAR] != o.value)) {
                    channel_parameter_values_[o.address + modnum * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + channum * N_CHANNEL_PAR] = o.value;
                    write_chan(o.name.c_str(), modnum, channum);
                  } else if (o.writable && ((o.setting_type == Gamma::SettingType::integer)
                                            || (o.setting_type == Gamma::SettingType::boolean)
                                            || (o.setting_type == Gamma::SettingType::int_menu)
                                            || (o.setting_type == Gamma::SettingType::binary))
                             && (channel_parameter_values_[o.address + modnum * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + channum * N_CHANNEL_PAR] != o.value_int)) {
                    channel_parameter_values_[o.address + modnum * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + channum * N_CHANNEL_PAR] = o.value_int;
                    write_chan(o.name.c_str(), modnum, channum);
                  }
                }
              } else if (p.writable && (p.setting_type == Gamma::SettingType::floating) && (module_parameter_values_[modnum * N_MODULE_PAR +  p.address] != p.value)) {
                module_parameter_values_[modnum * N_MODULE_PAR +  p.address] = p.value;
                write_mod(p.name.c_str(), modnum);
              } else if (p.writable && ((p.setting_type == Gamma::SettingType::integer)
                                        || (p.setting_type == Gamma::SettingType::boolean)
                                        || (p.setting_type == Gamma::SettingType::int_menu)
                                        || (p.setting_type == Gamma::SettingType::binary))
                         && (module_parameter_values_[modnum * N_MODULE_PAR +  p.address] != p.value_int)) {
                module_parameter_values_[modnum * N_MODULE_PAR +  p.address] = p.value_int;
                write_mod(p.name.c_str(), modnum);
              }
            }
          } else if (k.writable &&
                     (k.setting_type == Gamma::SettingType::boolean) &&
                     (system_parameter_values_[k.address] != k.value_int)) {
            system_parameter_values_[k.address] = k.value_int;
            write_sys(k.name.c_str());
          }
        }
              int next_chan = 0;
      if (prev_total_modules < new_total_modules) {
        PL_DBG << "copying module address=" << prev_total_modules;
        std::set<int32_t> new_set;
        while (new_set.size() < NUMBER_OF_CHANNELS) {
          while (used_chan_indices.count(next_chan))
            next_chan++;
          PL_DBG << "new channel index " << next_chan;
          new_set.insert(next_chan);
          used_chan_indices.insert(next_chan);
        }

        std::list<int32_t> copy_set(new_set.begin(), new_set.end());
        for (auto &k : model_module.branches.my_data_) {
          if (k.setting_type == Gamma::SettingType::stem) {
            int this_index = copy_set.front();
            copy_set.pop_front();
            for (auto &c : k.branches.my_data_) {
              c.index = this_index;
              c.indices.clear();
              c.indices.insert(this_index);
            }
          } else
            k.indices = new_set;
        }

        model_module.address = prev_total_modules;
        channel_indices_[prev_total_modules] = std::vector<int32_t>(new_set.begin(), new_set.end());
        q.branches.add(model_module);
        prev_total_modules++;
      }

      }
    }
  }
  return true;
}

bool Plugin::execute_command(Gamma::Setting &set) {
  if (live_ == LiveStatus::history)
    return false;

  if (set.name == "Pixie-4") {
    for (auto &q : set.branches.my_data_) {
      if ((q.setting_type == Gamma::SettingType::command) && (q.value == 1)) {
        q.value = 0;
        if (q.name == "Measure baselines")
          return control_measure_baselines(Module::all);
        else if (q.name == "Adjust offsets")
          return control_adjust_offsets(Module::all);
        else if (q.name == "Compute Tau")
          return control_find_tau(Module::all);
        else if (q.name == "Compute BLCUT")
          return control_compute_BLcut();
      }
    }
  }

  return false;
}


bool Plugin::boot() {
  if (live_ == LiveStatus::history)
    return false;

  live_ = LiveStatus::dead;

  S32 retval;
  set_sys("OFFLINE_ANALYSIS", 0);  //attempt live boot first
  set_sys("AUTO_PROCESSLMDATA", 0);  //do not write XIA datafile

  bool valid_files = true;
  for (int i=0; i < 7; i++) {
    memcpy(Boot_File_Name_List[i], (S8*)boot_files_[i].c_str(), boot_files_[i].size());
    Boot_File_Name_List[i][boot_files_[i].size()] = '\0';
    bool ex = boost::filesystem::exists(boot_files_[i]);
    if (!ex) {
      PL_ERR << "Boot file " << boot_files_[i] << " not found";
      valid_files = false;
    }
  }

  if (!valid_files) {
    PL_ERR << "Problem with boot files. Boot aborting.";
    return false;
  }

  int max = get_sys("NUMBER_MODULES");
  if (!max) {
    PL_ERR << "No valid module slots.";
  } else {
    PL_DBG << "Number of modules to boot: " << max;
    read_sys("SLOT_WAVE");
    for (int i=0; i < max; i++)
      PL_DBG << " module " << i << " in slot "
             << system_parameter_values_[i_sys("SLOT_WAVE") + i];
  }

  retval = Pixie_Boot_System(0x1F);
  //bad files do not result in boot error!!
  if (retval >= 0) {
    live_ = LiveStatus::online;

    //sleep 1 s and do offsets

    return true;
  }

  boot_err(retval);
  set_sys("OFFLINE_ANALYSIS", 1);  //else attempt offline boot
  retval = Pixie_Boot_System(0x1F);
  if (retval >= 0) {
//    live_ = LiveStatus::offline;
    live_ = LiveStatus::online;
    return true;
  } else {
    boot_err(retval);
    return false;
  }
}

std::map<int, std::vector<uint16_t>> Plugin::oscilloscope() {
  std::map<int, std::vector<uint16_t>> result;

  if (module_indices_.size() != channel_indices_.size()) {
    return result;
  }

  uint32_t* oscil_data;

  for (int m=0; m < channel_indices_.size(); ++m) {
    if ((oscil_data = control_collect_ADC(m)) != nullptr) {
      for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
        std::vector<uint16_t> trace = std::vector<uint16_t>(oscil_data + (i*max_buf_len),
                                                            oscil_data + ((i+1)*max_buf_len));
        if ((i < channel_indices_[m].size()) && (channel_indices_[m][i] >= 0))
          result[channel_indices_[m][i]] = trace;
      }
    }

  }

  delete[] oscil_data;
  return result;
}


void Plugin::get_all_settings() {
  get_sys_all();
  get_mod_all(Module::all);
  get_chan_all(Channel::all, Module::all);
}


void Plugin::reset_counters_next_run() { //for current module only
  if (live_ == LiveStatus::history)
    return;

  for (int i=0; i < channel_indices_.size(); ++i) {
    set_mod("SYNCH_WAIT", 1, Module(i));
    set_mod("IN_SYNCH", 0, Module(i));
  }
}


//////////////////////////////
//////////////////////////////
//////////////////////////////
//////////////////////////////
//////////////////////////////

bool Plugin::start_run(Module mod) {
  bool success = false;
  if (mod == Module::all) {
    for (int i=0; i< channel_indices_.size(); ++i) {
      PL_INFO << "Start run for module " << i;
      if (start_run(i))
        success = true;
    }
  } else {
    int module = static_cast<int>(mod);
    if ((module > -1) && (module < channel_indices_.size())) {
      PL_INFO << "Start run for module " << module;
      success = start_run(module);
    }
  }
  return success;
}

bool Plugin::resume_run(Module mod) {
  bool success = false;
  if (mod == Module::all) {
    for (int i=0; i< channel_indices_.size(); ++i) {
      PL_INFO << "Resume run for module " << i;
      if (resume_run(i))
        success = true;
    }
  } else {
    int module = static_cast<int>(mod);
    if ((module > -1) && (module < channel_indices_.size())) {
      PL_INFO << "Resume run for module " << module;
      success = resume_run(module);
    }
  }
  return success;  
}

bool Plugin::stop_run(Module mod) {
  bool success = false;
  if (mod == Module::all) {
    for (int i=0; i< channel_indices_.size(); ++i) {
      PL_INFO << "Stop run for module " << i;
      if (stop_run(i))
        success = true;
    }
  } else {
    int module = static_cast<int>(mod);
    if ((module > -1) && (module < channel_indices_.size())) {
      PL_INFO << "Stop run for module " << module;
      success = stop_run(module);
    }
  }
  return success;
}


bool Plugin::start_run(uint8_t mod) {
  uint16_t type = (run_type_ | 0x1000);
  int32_t ret = Pixie_Acquire_Data(type, NULL, 0, mod);
  switch (ret) {
    case 0x10:
      return true;
    case -0x11:
      PL_ERR << "Start run failed: Invalid Pixie module number";
      return false;
    case -0x12:
      PL_ERR << "Start run failed. Try rebooting";
      return false;
    default:
      PL_ERR << "Start run failed. Unknown error";
      return false;
  }
}

bool Plugin::resume_run(uint8_t mod) {
  uint16_t type = (run_type_ | 0x2000);
  int32_t ret = Pixie_Acquire_Data(type, NULL, 0, mod);
  switch (ret) {
    case 0x20:
      return true;
    case -0x21:
      PL_ERR << "Resume run failed: Invalid Pixie module number";
      return false;
    case -0x22:
      PL_ERR << "Resume run failed. Try rebooting";
      return false;
    default:
      PL_ERR << "Resume run failed. Unknown error";
      return false;
  }
}

bool Plugin::stop_run(uint8_t mod) {
  uint16_t type = (run_type_ | 0x3000);
  int32_t ret = Pixie_Acquire_Data(type, NULL, 0, mod);
  switch (ret) {
    case 0x30:
      return true;
    case -0x31:
      PL_ERR << "Stop run failed: Invalid Pixie module number";
      return false;
    case -0x32:
      PL_ERR << "Stop run failed. Try rebooting";
      return false;
    default:
      PL_ERR << "Stop run failed. Unknown error";
      return false;
  }
}


uint32_t Plugin::poll_run(uint8_t mod) {
  uint16_t type = (run_type_ | 0x4000);
  return Pixie_Acquire_Data(type, NULL, 0, mod);
}

uint32_t Plugin::poll_run_dbl(uint8_t mod) {
  return Pixie_Acquire_Data(0x40FF, NULL, 0, mod);
}


bool Plugin::read_EM(uint32_t* data, uint8_t mod) {
  S32 retval = Pixie_Acquire_Data(0x9003, (U32*)data, NULL, mod);
  switch (retval) {
    case -0x93:
      PL_ERR << "Failure to read list mode section of external memory. Reboot recommended.";
      return false;
    case -0x95:
      PL_ERR << "Invalid external memory I/O request. Check run type.";
      return false;
    case 0x90:
      return true;
    case 0x0:
      return true; //undocumented by XIA, returns this rather than 0x90 upon success
    default:
      PL_ERR << "Unexpected error " << std::hex << retval;
      return false;
  }
}

bool Plugin::write_EM(uint32_t* data, uint8_t mod) {
  return (Pixie_Acquire_Data(0x9004, (U32*)data, NULL, mod) == 0x90);
}


uint16_t Plugin::i_dsp(const char* setting_name) {
  return Find_Xact_Match((S8*)setting_name, DSP_Parameter_Names, N_DSP_PAR);
}

//this function taken from XIA's code and modified, original comments remain
bool Plugin::read_EM_dbl(uint32_t* data, uint8_t mod) {
  U16 j, MCSRA;
  U16 DblBufCSR;
  U32 Aoffset[2], WordCountPP[2];
  U32 WordCount, NumWordsToRead, CSR;
  U32 dsp_word[2];

  char modcsra_str[] = "MODCSRA";
  char dblbufcsr_str[] = "DBLBUFCSR";
  Pixie_IODM(0, (U16)DATA_MEMORY_ADDRESS + i_dsp(modcsra_str), MOD_READ, 1, dsp_word);
  MCSRA = (U16)dsp_word[0];
  Pixie_IODM(0, (U16)DATA_MEMORY_ADDRESS + i_dsp(dblbufcsr_str), MOD_READ, 1, dsp_word);
  DblBufCSR = (U16)dsp_word[0];

  Aoffset[0] = 0;
  Aoffset[1] = LM_DBLBUF_BLOCK_LENGTH;

  Pixie_ReadCSR((U8)mod, &CSR);
  // A read of Pixie's word count register
  // This also indicates to the DSP that a readout has begun
  Pixie_RdWrdCnt((U8)mod, &WordCount);

  // The number of 16-bit words to read is in EMwords or EMwords2
  char emwords_str[] = "EMWORDS";
  char emwords2_str[] = "EMWORDS2";
  Pixie_IODM((U8)mod, (U16)DATA_MEMORY_ADDRESS + i_dsp(emwords_str), MOD_READ, 2, dsp_word);
  WordCountPP[0] = dsp_word[0] * 65536 + dsp_word[1];
  Pixie_IODM((U8)mod, (U16)DATA_MEMORY_ADDRESS + i_dsp(emwords2_str), MOD_READ, 2, dsp_word);
  WordCountPP[1] = dsp_word[0] * 65536 + dsp_word[1];

  if(TstBit(CSR_128K_FIRST, (U16)CSR) == 1)
    j=0;
  else		// block at 128K+64K was first
    j=1;

  if  (TstBit(CSR_DATAREADY, (U16)CSR) == 0 )
    // function called after a readout that cleared WCR => run stopped => read other block
  {
    j=1-j;
    PL_INFO << "(read_EM_dbl): Module " << mod <<
        ": Both memory blocks full (block " << 1-j << " older). Run paused (or finished).";
    Pixie_Print_MSG(ErrMSG);
  }

  if (WordCountPP[j] >0)
  {
      // Check if it is an odd or even number
    if(fmod(WordCountPP[j], 2.0) == 0.0)
      NumWordsToRead = WordCountPP[j] / 2;
    else
      NumWordsToRead = WordCountPP[j] / 2 + 1;

    if(NumWordsToRead > LIST_MEMORY_LENGTH)
    {
      PL_ERR << "(read_EM_dbl): invalid word count " << NumWordsToRead;
      Pixie_Print_MSG(ErrMSG);
      return false;
    }

      // Read out the list mode data
    Pixie_IOEM((U8)mod, LIST_MEMORY_ADDRESS+Aoffset[j], MOD_READ, NumWordsToRead, (U32*)data);
  }

  // A second read of Pixie's word count register to clear the DBUF_1FULL bit
  // indicating to the DSP that the read is complete
  Pixie_RdWrdCnt((U8)mod, &WordCount);
  return true;
}

bool Plugin::clear_EM(uint8_t mod) {
  std::vector<uint32_t> my_data(list_mem_len32, 0);
  return (write_EM(my_data.data(), mod) >= 0);
}

/////System Settings//////
//////////////////////////

void Plugin::set_sys(const std::string& setting, double val) {
  if (live_ == LiveStatus::history)
    return;

  PL_DBG << "Setting " << setting << " to " << val << " for system";

  //check bounds
  system_parameter_values_[i_sys(setting.c_str())] = val;
  write_sys(setting.c_str());
}


double Plugin::get_sys(const std::string& setting) {
  PL_TRC << "Getting " << setting << " for system";

  //check bounds
  read_sys(setting.c_str());
  return system_parameter_values_[i_sys(setting.c_str())];
}

void Plugin::get_sys_all() {
  if (live_ == LiveStatus::history)
    return;

  PL_TRC << "Getting all system";
  read_sys("ALL_SYSTEM_PARAMETERS");
}


//////Module Settings//////
///////////////////////////

void Plugin::set_mod(const std::string& setting, double val, Module mod) {
  if (live_ == LiveStatus::history)
    return;

  int module = static_cast<int>(mod);
  if (module > -1) {
    PL_INFO << "Setting " << setting << " to " << val << " for module " << module;
    module_parameter_values_[module * N_MODULE_PAR + i_mod(setting.c_str())] = val;
    write_mod(setting.c_str(), module);
  }
}

double Plugin::get_mod(const std::string& setting,
                       Module mod) const {
  int module = static_cast<int>(mod);
  if (module > -1) {
    PL_TRC << "Getting " << setting << " for module " << module;
    return module_parameter_values_[module * N_MODULE_PAR + i_mod(setting.c_str())];
  }
  else
    return -1;
}

double Plugin::get_mod(const std::string& setting,
                       Module mod,
                       LiveStatus force) {
  int module = static_cast<int>(mod);
  if (module > -1) {
    PL_TRC << "Getting " << setting << " for module " << module;
    if ((force == LiveStatus::online) && (live_ == force))
      read_mod(setting.c_str(), module);
    return module_parameter_values_[module * N_MODULE_PAR + i_mod(setting.c_str())];
  }
  else
    return -1;
}

void Plugin::get_mod_all(Module mod) {
  if (live_ == LiveStatus::history)
    return;

  if (mod == Module::all) {
    for (int i=0; i< channel_indices_.size(); ++i) {
      PL_TRC << "Getting all parameters for module " << i;
      read_mod("ALL_MODULE_PARAMETERS", i);
    }
  } else {
    int module = static_cast<int>(mod);
    if ((module > -1) && (module < channel_indices_.size())) {
      PL_TRC << "Getting all parameters for module " << module;
      read_mod("ALL_MODULE_PARAMETERS", module);
    }
  }
}

void Plugin::get_mod_stats(Module mod) {
  if (live_ == LiveStatus::history)
    return;

  if (mod == Module::all) {
    for (int i=0; i< channel_indices_.size(); ++i) {
      PL_TRC << "Getting run statistics for module " << i;
      read_mod("MODULE_RUN_STATISTICS", i);
    }
  } else {
    int module = static_cast<int>(mod);
    if ((module > -1) && (module < channel_indices_.size())) {
      PL_TRC << "Getting run statistics for module " << module;
      read_mod("MODULE_RUN_STATISTICS", module);
    }
  }
}


////////Channels////////////
////////////////////////////

void Plugin::set_chan(const std::string& setting, double val,
                      Channel channel, Module module) {
  if (live_ == LiveStatus::history)
    return;

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_DBG << "Setting " << setting << " to " << val
         << " for module " << mod << " chan "<< chan;

  if ((mod > -1) && (chan > -1)) {
    channel_parameter_values_[i_chan(setting.c_str()) + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR] = val;
    write_chan(setting.c_str(), mod, chan);
  }
}

void Plugin::set_chan(uint8_t setting, double val,
                      Channel channel, Module module) {
  if (live_ == LiveStatus::history)
    return;

  S8* setting_name = Channel_Parameter_Names[setting];

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_DBG << "Setting " << setting_name << " to " << val
         << " for module " << mod << " chan "<< chan;

  if ((mod > -1) && (chan > -1)) {
    channel_parameter_values_[setting + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR] = val;
    write_chan((char*)setting_name, mod, chan);
  }
}

double Plugin::get_chan(uint8_t setting, Channel channel, Module module) const {
  S8* setting_name = Channel_Parameter_Names[setting];

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_DBG << "Getting " << setting_name
         << " for module " << mod << " channel " << chan;

  if ((mod > -1) && (chan > -1)) {
    return channel_parameter_values_[setting + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR];
  } else
    return -1;
}

double Plugin::get_chan(uint8_t setting, Channel channel,
                        Module module, LiveStatus force) {
  S8* setting_name = Channel_Parameter_Names[setting];

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_DBG << "Getting " << setting_name
         << " for module " << mod << " channel " << chan;

  if ((mod > -1) && (chan > -1)) {
    if ((force == LiveStatus::online) && (live_ == force))
      read_chan((char*)setting_name, mod, chan);
    return channel_parameter_values_[setting + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR];
  } else
    return -1;
}

double Plugin::get_chan(const std::string& setting,
                        Channel channel, Module module) const {
  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_TRC << "Getting " << setting << " for module " << mod << " channel " << chan;

  if ((mod > -1) && (chan > -1)) {
    return channel_parameter_values_[i_chan(setting.c_str()) + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR];
  } else
    return -1;
}

double Plugin::get_chan(const std::string& setting,
                        Channel channel, Module module,
                        LiveStatus force) {
  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_TRC << "Getting " << setting << " for module " << mod << " channel " << chan;

  if ((mod > -1) && (chan > -1)) {
    if ((force == LiveStatus::online) && (live_ == force))
      read_chan(setting.c_str(), mod, chan);
    return channel_parameter_values_[i_chan(setting.c_str()) + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR];
  } else
    return -1;
}

void Plugin::get_chan_all(Channel channel, Module module) {
  if (live_ == LiveStatus::history)
    return;

  int mod_start = -1;
  int mod_end = -1;

  if (module == Module::all) {
    mod_start = 0;
    mod_end = channel_indices_.size();
  } else {
    int mod = static_cast<int>(module);
    if ((mod > -1) && (mod < channel_indices_.size()))
      mod_start = mod_end = mod;
  }

  for (int i=mod_start; i < mod_end; ++i) {
    int chan_start = -1;
    int chan_end = -1;
    if (channel == Channel::all) {
      chan_start = 0;
      chan_end = channel_indices_[i].size();
    } else {
      int chan = static_cast<int>(channel);
      if ((chan > -1) && (chan < channel_indices_[i].size()))
          chan_start = chan_end = chan;
    }
    for (int j=chan_start; j < chan_end; ++j) {
      PL_TRC << "Getting all parameters for module " << i << " channel " << j;
      read_chan("ALL_CHANNEL_PARAMETERS", i, j);
    }
  }
}

void Plugin::get_chan_stats(Channel channel, Module module) {
  if (live_ == LiveStatus::history)
    return;

  int mod_start = -1;
  int mod_end = -1;

  if (module == Module::all) {
    mod_start = 0;
    mod_end = channel_indices_.size();
  } else {
    int mod = static_cast<int>(module);
    if ((mod > -1) && (mod < channel_indices_.size()))
      mod_start = mod_end = mod;
  }

  for (int i=mod_start; i < mod_end; ++i) {
    int chan_start = -1;
    int chan_end = -1;
    if (channel == Channel::all) {
      chan_start = 0;
      chan_end = channel_indices_[i].size();
    } else {
      int chan = static_cast<int>(channel);
      if ((chan > -1) && (chan < channel_indices_[i].size()))
          chan_start = chan_end = chan;
    }
    for (int j=chan_start; j < chan_end; ++j) {
      //      PL_DBG << "Getting channel run statistics for module " << i << " channel " << j;
      read_chan("CHANNEL_RUN_STATISTICS", i, j);
    }
  }
}

uint16_t Plugin::i_sys(const char* setting) const {
  return Find_Xact_Match((S8*)setting, System_Parameter_Names, N_SYSTEM_PAR);
}

uint16_t Plugin::i_mod(const char* setting) const {
  return Find_Xact_Match((S8*)setting, Module_Parameter_Names, N_MODULE_PAR);
}

uint16_t Plugin::i_chan(const char* setting) const {
  return Find_Xact_Match((S8*)setting, Channel_Parameter_Names, N_CHANNEL_PAR);
}

bool Plugin::write_sys(const char* setting) {
  S8 sysstr[8] = "SYSTEM\0";
  S32 ret = Pixie_User_Par_IO(system_parameter_values_.data(),
                              (S8*) setting, sysstr, MOD_WRITE, 0, 0);
  set_err(ret);
  if (ret == 0)
    return true;
  return false;
}

bool Plugin::read_sys(const char* setting) {
  S8 sysstr[8] = "SYSTEM\0";
  S32 ret = Pixie_User_Par_IO(system_parameter_values_.data(),
                              (S8*) setting, sysstr, MOD_READ, 0, 0);
  set_err(ret);
  if (ret == 0)
    return true;
  return false;
}

bool Plugin::write_mod(const char* setting, uint8_t mod) {
  S32 ret = -42;
  S8 modstr[8] = "MODULE\0";
  if (live_ == LiveStatus::online)
    ret = Pixie_User_Par_IO(module_parameter_values_.data(),
                            (S8*) setting, modstr, MOD_WRITE, mod, 0);
  set_err(ret);
  return (ret == 0);
}

bool Plugin::read_mod(const char* setting, uint8_t mod) {
  S32 ret = -42;
  S8 modstr[8] = "MODULE\0";
  if (live_ == LiveStatus::online)
    ret = Pixie_User_Par_IO(module_parameter_values_.data(),
                            (S8*) setting, modstr, MOD_READ, mod, 0);
  set_err(ret);
  return (ret == 0);
}

bool Plugin::write_chan(const char* setting, uint8_t mod, uint8_t chan) {
  S32 ret = -42;
  S8 chnstr[9] = "CHANNEL\0";
  if (live_ == LiveStatus::online)
    ret = Pixie_User_Par_IO(channel_parameter_values_.data(),
                            (S8*) setting, chnstr, MOD_WRITE, mod, chan);
  set_err(ret);
  return (ret == 0);
}

bool Plugin::read_chan(const char* setting, uint8_t mod, uint8_t chan) {
  S32 ret = -42;
  S8 chnstr[9] = "CHANNEL\0";
  if (live_ == LiveStatus::online)
    ret = Pixie_User_Par_IO(channel_parameter_values_.data(),
                            (S8*) setting, chnstr, MOD_READ, mod, chan);
  set_err(ret);
  return (ret == 0);
}

uint32_t* Plugin::control_collect_ADC(uint8_t module) {
  PL_TRC << "Pixie Control: get ADC (oscilloscope) traces";
  ///why is NUMBER_OF_CHANNELS used? Same for multi-module?
  uint32_t* adc_data = new uint32_t[NUMBER_OF_CHANNELS * max_buf_len];
  if (live_ == LiveStatus::online) {
    int32_t retval = Pixie_Acquire_Data(0x0084, (U32*)adc_data, NULL, module);
    if (retval < 0) {
      delete[] adc_data;
      control_err(retval);
      return nullptr;
    }
  }
  return adc_data;
}

bool Plugin::control_set_DAC(uint8_t module) {
  PL_INFO << "Pixie Control: set DAC";
  return control_err(Pixie_Acquire_Data(0x0000, NULL, NULL, module));
}

bool Plugin::control_connect(uint8_t module) {
  PL_INFO << "Pixie Control: connect";
  return control_err(Pixie_Acquire_Data(0x0001, NULL, NULL, module));
}

bool Plugin::control_disconnect(uint8_t module) {
  PL_INFO << "Pixie Control: disconnect";
  return control_err(Pixie_Acquire_Data(0x0002, NULL, NULL, module));
}

bool Plugin::control_program_Fippi(uint8_t module) {
  PL_INFO << "Pixie Control: program Fippi";
  return control_err(Pixie_Acquire_Data(0x0005, NULL, NULL, module));
}

bool Plugin::control_measure_baselines(Module mod) {
  bool success = false;
  if (mod == Module::all) {
    for (int i=0; i< channel_indices_.size(); ++i) {
      PL_INFO << "Pixie Control: measure baselines for module " << i;
      if (control_err(Pixie_Acquire_Data(0x0006, NULL, NULL, i)))
        success = true;
    }
  } else {
    int module = static_cast<int>(mod);
    if ((module > -1) && (module < channel_indices_.size())) {
      PL_INFO << "Pixie Control: measure baselines for module " << module;
      success = control_err(Pixie_Acquire_Data(0x0006, NULL, NULL, module));
    }
  }
  return success;
}

bool Plugin::control_test_EM_write(uint8_t module) {
  PL_INFO << "Pixie Control: test EM write";
  return control_err(Pixie_Acquire_Data(0x0016, NULL, NULL, module));
}

bool Plugin::control_test_HM_write(uint8_t module) {
  PL_INFO << "Pixie Control: test HM write";
  return control_err(Pixie_Acquire_Data(0x001A, NULL, NULL, module));
}

bool Plugin::control_compute_BLcut() {
  PL_INFO << "Pixie Control: compute BLcut";
  return control_err(Pixie_Acquire_Data(0x0080, NULL, NULL, 0));
}

bool Plugin::control_find_tau(Module mod) {
  bool success = false;
  if (mod == Module::all) {
    for (int i=0; i< channel_indices_.size(); ++i) {
      PL_INFO << "Pixie Control: find tau for module " << i;
      if (control_err(Pixie_Acquire_Data(0x0081, NULL, NULL, i)))
        success = true;
    }
  } else {
    int module = static_cast<int>(mod);
    if ((module > -1) && (module < channel_indices_.size())) {
      PL_INFO << "Pixie Control: find tau for module " << module;
      success = control_err(Pixie_Acquire_Data(0x0081, NULL, NULL, module));
    }
  }
  return success;
}

bool Plugin::control_adjust_offsets(Module mod) {
  bool success = false;
  if (mod == Module::all) {
    for (int i=0; i< channel_indices_.size(); ++i) {
      PL_INFO << "Pixie Control: adjust offsets for module " << i;
      if (control_err(Pixie_Acquire_Data(0x0083, NULL, NULL, i)))
        success = true;
    }
  } else {
    int module = static_cast<int>(mod);
    if ((module > -1) && (module < channel_indices_.size())) {
      PL_INFO << "Pixie Control: adjust offsets for module " << module;
      success = control_err(Pixie_Acquire_Data(0x0083, NULL, NULL, module));
    }
  }
  return success;
}

bool Plugin::control_err(int32_t err_val) {
  switch (err_val) {
    case 0:
      return true;
    case -1:
      PL_ERR << "Control command failed: Invalid Pixie modules number. Check ModNum";
      break;
    case -2:
      PL_ERR << "Control command failed: Failure to adjust offsets. Reboot recommended";
      break;
    case -3:
      PL_ERR << "Control command failed: Failure to acquire ADC traces. Reboot recommended";
      break;
    case -4:
      PL_ERR << "Control command failed: Failure to start the control task run. Reboot recommended";
      break;
    default:
      PL_ERR << "Control comman failed: Unknown error " << err_val;
  }
  return false;
}

void Plugin::set_err(int32_t err_val) {
  switch (err_val) {
  case 0:
    break;
  case -1:
    PL_ERR << "Set/get parameter failed: Null pointer for User_Par_Values";
    break;
  case -2:
    PL_ERR << "Set/get parameter failed: Invalid user parameter name";
    break;
  case -3:
    PL_ERR << "Set/get parameter failed: Invalid user parameter type";
    break;
  case -4:
    PL_ERR << "Set/get parameter failed: Invalid I/O direction";
    break;
  case -5:
    PL_ERR << "Set/get parameter failed: Invalid module number";
    break;
  case -6:
    PL_ERR << "Set/get parameter failed: Invalid channel number";
    break;
  case -42:
    //PL_ERR << "Set/get parameter failed: Pixie not online";
    break;
  default:
    PL_ERR << "Set/get parameter failed: Unknown error " << err_val;
  }
}

void Plugin::boot_err(int32_t err_val) {
  switch (err_val) {
  case 0:
    break;
  case -1:
    PL_ERR << "Boot failed: unable to scan crate slots. Check PXI slot map.";
    break;
  case -2:
    PL_ERR << "Boot failed: unable to read communication FPGA (rev. B). Check comFPGA file.";
    break;
  case -3:
    PL_ERR << "Boot failed: unable to read communication FPGA (rev. C). Check comFPGA file.";
    break;
  case -4:
    PL_ERR << "Boot failed: unable to read signal processing FPGA config. Check SPFPGA file.";
    break;
  case -5:
    PL_ERR << "Boot failed: unable to read DSP executable code. Check DSP code file.";
    break;
  case -6:
    PL_ERR << "Boot failed: unable to read DSP parameter values. Check DSP parameter file.";
    break;
  case -7:
    PL_ERR << "Boot failed: unable to initialize DSP parameter names. Check DSP .var file.";
    break;
  case -8:
    PL_ERR << "Boot failed: failed to boot all modules in the system. Check Pixie modules.";
    break;
  default:
    PL_ERR << "Boot failed, undefined error " << err_val;
  }
}

void Plugin::to_xml(tinyxml2::XMLPrinter& printer, std::vector<Gamma::Detector> detectors) const {
  printer.OpenElement("PixieSettings");
  printer.OpenElement("System");
  for (int i=0; i < N_SYSTEM_PAR; i++) {
    if (!std::string((char*)System_Parameter_Names[i]).empty()) {
      printer.OpenElement("Setting");
      printer.PushAttribute("key", std::to_string(i).c_str());
      printer.PushAttribute("name", std::string((char*)System_Parameter_Names[i]).c_str()); //not here?
      printer.PushAttribute("value", std::to_string(system_parameter_values_[i]).c_str());
      printer.CloseElement();
    }
  }
  printer.CloseElement(); //System
  for (int i=0; i < 1; i++) { //hardcoded. Make for multiple modules...
    printer.OpenElement("Module");
    printer.PushAttribute("number", std::to_string(i).c_str());
    for (int j=0; j < N_MODULE_PAR; j++) {
      if (!std::string((char*)Module_Parameter_Names[j]).empty()) {
        printer.OpenElement("Setting");
        printer.PushAttribute("key", std::to_string(j).c_str());
        printer.PushAttribute("name", std::string((char*)Module_Parameter_Names[j]).c_str());
        printer.PushAttribute("value", std::to_string(module_parameter_values_[j]).c_str());
        printer.CloseElement();
      }
    }
    for (int j=0; j<NUMBER_OF_CHANNELS; j++) {
      printer.OpenElement("Channel");
      printer.PushAttribute("number", std::to_string(j).c_str());
      detectors[j].to_xml(printer);
      for (int k=0; k < N_CHANNEL_PAR; k++) {
        if (!std::string((char*)Channel_Parameter_Names[k]).empty()) {
          printer.OpenElement("Setting");
          printer.PushAttribute("key", std::to_string(k).c_str());
          printer.PushAttribute("name", std::string((char*)Channel_Parameter_Names[k]).c_str());
          printer.PushAttribute("value",
                                std::to_string(get_chan(k,Channel(j), Module(0))).c_str());
          printer.CloseElement();
        }
      }
      printer.CloseElement(); //Channel
    }
    printer.CloseElement(); //Module
  }
  printer.CloseElement(); //Plugin
}

std::vector<Gamma::Detector> Plugin::from_xml(tinyxml2::XMLElement* root) {
  std::vector<Gamma::Detector> detectors(NUMBER_OF_CHANNELS);

  live_ = LiveStatus::history;

  tinyxml2::XMLElement* TopElement = root->FirstChildElement();
  while (TopElement != nullptr) {
    std::string topElementName(TopElement->Name());
    if (topElementName == "System") {
      tinyxml2::XMLElement* SysSetting = TopElement->FirstChildElement("Setting");
      while (SysSetting != nullptr) {
        int thisKey = boost::lexical_cast<short>(SysSetting->Attribute("key"));
        double thisVal = boost::lexical_cast<double>(SysSetting->Attribute("value"));
        system_parameter_values_[thisKey] = thisVal;
        SysSetting = dynamic_cast<tinyxml2::XMLElement*>(SysSetting->NextSibling());
      }
    }
    if (topElementName == "Module") {
      int thisModule = boost::lexical_cast<short>(TopElement->Attribute("number"));
      tinyxml2::XMLElement* ModElement = TopElement->FirstChildElement();
      while (ModElement != nullptr) {
        std::string modElementName(ModElement->Name());
        if (modElementName == "Setting") {
          int thisKey =  boost::lexical_cast<short>(ModElement->Attribute("key"));
          double thisVal = boost::lexical_cast<double>(ModElement->Attribute("value"));
          module_parameter_values_[thisModule * N_MODULE_PAR + thisKey] = thisVal;
        } else if (modElementName == "Channel") {
          int thisChan = boost::lexical_cast<short>(ModElement->Attribute("number"));
          tinyxml2::XMLElement* ChanElement = ModElement->FirstChildElement();
          while (ChanElement != nullptr) {
            if (std::string(ChanElement->Name()) == "Detector") {
              detectors[thisChan].from_xml(ChanElement);
            } else if (std::string(ChanElement->Name()) == "Setting") {
              int thisKey =  boost::lexical_cast<short>(ChanElement->Attribute("key"));
              double thisVal = boost::lexical_cast<double>(ChanElement->Attribute("value"));
              channel_parameter_values_[thisModule * N_CHANNEL_PAR * NUMBER_OF_CHANNELS
                  + thisChan * N_CHANNEL_PAR + thisKey] = thisVal;
            }
            ChanElement = dynamic_cast<tinyxml2::XMLElement*>(ChanElement->NextSibling());
          }
        }
        ModElement = dynamic_cast<tinyxml2::XMLElement*>(ModElement->NextSibling());
      }
    }
    TopElement = dynamic_cast<tinyxml2::XMLElement*>(TopElement->NextSibling());
  }
  return detectors;
}



void Plugin::worker_run_test(Plugin* callback, uint64_t timeout_limit,
                             SynchronizedQueue<Spill*>* spill_queue,
                             boost::atomic<bool>* interruptor,
                             Gamma::Setting root) {

  PL_INFO << "<Qpx::Plugin> worker_run_test started";
  uint64_t spill_number = 0;
  bool timeout = false;
  CustomTimer total_timer(timeout_limit);

  while (!timeout) {
    PL_INFO << "<Qpx::Plugin>  RUNNING(" << spill_number << ")"
            << "  Elapsed: " << total_timer.done()
            << "  ETA: " << total_timer.ETA();

    spill_number++;

    while (!timeout) {
      wait_ms(callback->run_poll_interval_ms_);
      timeout = (total_timer.timeout() || interruptor->load());
    };
  }

  PL_INFO << "<Qpx::Plugin> worker_run_test stopped";
}



void Plugin::worker_run(Plugin* callback, uint64_t timeout_limit,
                   SynchronizedQueue<Spill*>* spill_queue,
                   boost::atomic<bool>* interruptor,
                   Gamma::Setting root) {

  PL_INFO << "<PixiePlugin> Double buffered daq starting";

  int32_t retval = 0;
  uint64_t spill_number = 0;
  bool timeout = false;
  Spill* fetched_spill;
  CustomTimer readout_timer, start_timer, run_timer, stop_timer,
      total_timer(timeout_limit);
  boost::posix_time::ptime session_start_time, block_time;

  if (!callback->clear_EM(0)) //one module
    return;

  callback->set_mod("DBLBUFCSR",   static_cast<double>(0), Module(0)); //one module
  callback->set_mod("MODULE_CSRA", static_cast<double>(2), Module(0)); //one module

  fetched_spill = new Spill;
  block_time = session_start_time = boost::posix_time::microsec_clock::local_time();

  if(!callback->start_run(Module::all)) {
    delete fetched_spill->run;
    delete fetched_spill;
    return;
  }

  total_timer.resume();

  callback->get_mod_stats(Module::all);
  callback->get_chan_stats(Channel::all, Module::all);
  for (int i=0; i < callback->channel_indices_.size(); i++)
    callback->fill_stats(fetched_spill->stats, i);
  for (auto &q : fetched_spill->stats)
    q.lab_time = session_start_time;
  spill_queue->enqueue(fetched_spill);

  while (!timeout) {
    start_timer.resume();

    if (spill_number > 0)
      retval = callback->resume_run(Module::all);
    start_timer.stop();
    run_timer.resume();

    if(retval < 0)
      return;

    if (spill_number > 0) {
      callback->get_mod_stats(Module::all);
      callback->get_chan_stats(Channel::all, Module::all);
      for (int i=0; i < callback->channel_indices_.size(); i++)
        callback->fill_stats(fetched_spill->stats, i);
      for (auto &q : fetched_spill->stats) {
        q.lab_time = block_time;
        q.spill_number = spill_number;
      }
      spill_queue->enqueue(fetched_spill);
    }

    PL_INFO << "  RUNNING(" << spill_number << ")"
            << "  Elapsed: " << total_timer.done()
            << "  ETA: " << total_timer.ETA();

    fetched_spill = new Spill;
    fetched_spill->data.resize(list_mem_len32, 0);

    spill_number++;

    retval = callback->poll_run(0);  //one module
    while ((retval == 1) && (!timeout)) {
      wait_ms(callback->run_poll_interval_ms_);
      retval = callback->poll_run(0);  //one module
      timeout = (total_timer.timeout() || interruptor->load());
    };
    block_time = boost::posix_time::microsec_clock::local_time();
    run_timer.stop();

    stop_timer.resume();
    callback->stop_run(Module::all); //is this really necessary?
    stop_timer.stop();

    readout_timer.resume();
    if(!callback->read_EM(fetched_spill->data.data(), 0)) {  //one module
      PL_DBG << "read_EM failed";
      delete fetched_spill;
      return;
    }
    readout_timer.stop();
  }

  callback->get_mod_stats(Module::all);
  callback->get_chan_stats(Channel::all, Module::all);
  for (int i=0; i < callback->channel_indices_.size(); i++)
    callback->fill_stats(fetched_spill->stats, i);
  for (auto &q : fetched_spill->stats) {
    q.lab_time = block_time;
    q.spill_number = spill_number;
  }
  spill_queue->enqueue(fetched_spill);

  total_timer.stop();

  PL_INFO << "<PixiePlugin> Single buffered daq stopping";
}


void Plugin::worker_run_dbl(Plugin* callback, uint64_t timeout_limit,
                   SynchronizedQueue<Spill*>* spill_queue,
                   boost::atomic<bool>* interruptor,
                   Gamma::Setting root) {

  PL_INFO << "<PixiePlugin> Double buffered daq starting";

  std::bitset<32> csr;
  uint64_t spill_number = 0;
  bool timeout = false;
  Spill* fetched_spill;
  CustomTimer total_timer(timeout_limit);
  boost::posix_time::ptime session_start_time, block_time;

  for (int i=0; i < callback->channel_indices_.size(); i++) {
    if (!callback->clear_EM(i)) //one module
      return;
    callback->set_mod("DBLBUFCSR",   static_cast<double>(1), Module(i));
    callback->set_mod("MODULE_CSRA", static_cast<double>(0), Module(i));
  }

  fetched_spill = new Spill;
  session_start_time = boost::posix_time::microsec_clock::local_time();

  if(!callback->start_run(Module::all)) {
    delete fetched_spill->run;
    delete fetched_spill;
    return;
  }

  total_timer.resume();

  callback->get_mod_stats(Module::all);
  callback->get_chan_stats(Channel::all, Module::all);
  for (int i=0; i < callback->channel_indices_.size(); i++)
    callback->fill_stats(fetched_spill->stats, i);
  for (auto &q : fetched_spill->stats)
    q.lab_time = session_start_time;
  spill_queue->enqueue(fetched_spill);

  std::set<int> mods;
  while (!(timeout && mods.empty())) {
    spill_number++;
    PL_INFO << "<PixiePlugin>   RUNNING Elapsed: " << total_timer.done()
            << "  ETA: " << total_timer.ETA();

    mods.clear();
    while (!timeout && (mods.empty())) {
      for (int i=0; i < callback->channel_indices_.size(); ++i) {
        std::bitset<32> csr = std::bitset<32>(poll_run_dbl(i));
        if (csr[14])
          mods.insert(i);
      }
      if (mods.empty())
        wait_ms(callback->run_poll_interval_ms_);
      timeout = (total_timer.timeout() || interruptor->load());
    };
    block_time = boost::posix_time::microsec_clock::local_time();

    for (auto &q : mods) {
      callback->get_mod_stats(Module(q));
      callback->get_chan_stats(Channel::all, Module(q));
    }


    bool success = false;
    for (auto &q : mods) {
      fetched_spill = new Spill;
      fetched_spill->data.resize(list_mem_len32, 0);
      if (read_EM_dbl(fetched_spill->data.data(), q))
        success = true;
      PL_DBG << "<PixiePlugin> fetched spill for mod " << q;
      callback->fill_stats(fetched_spill->stats, q);
      for (auto &p : fetched_spill->stats) {
        p.lab_time = block_time;
        p.spill_number = spill_number;
      }
      spill_queue->enqueue(fetched_spill); 
    }

    if (!success) {
      break;
    }
  }

  callback->stop_run(Module::all);

  PL_INFO << "<PixiePlugin> Double buffered daq stopping";
}



void Plugin::worker_parse (Plugin* callback, SynchronizedQueue<Spill*>* in_queue, SynchronizedQueue<Spill*>* out_queue) {

  PL_INFO << "<PixiePlugin> parser starting";

  Spill* spill;
  std::vector<std::vector<int32_t>> channel_indices = callback->channel_indices_;

  uint64_t all_events = 0, cycles = 0;
  CustomTimer parse_timer;

  while ((spill = in_queue->dequeue()) != NULL ) {
    parse_timer.resume();

    if (spill->data.size() > 0) {
      cycles++;
      uint16_t* buff16 = (uint16_t*) spill->data.data();
      uint32_t idx = 0, buf_end, spill_events = 0;

      while (true) {
        uint16_t buf_ndata  = buff16[idx++];
        uint32_t buf_end = idx + buf_ndata - 1;

        if (   (buf_ndata == 0)
               || (buf_ndata > max_buf_len)
               || (buf_end   > list_mem_len16))
          break;

        uint16_t buf_module = buff16[idx++];
        uint16_t buf_format = buff16[idx++];
        uint16_t buf_timehi = buff16[idx++];
        uint16_t buf_timemi = buff16[idx++];
        uint16_t buf_timelo = buff16[idx++];
        uint16_t task_a = (buf_format & 0x0F00);
        uint16_t task_b = (buf_format & 0x000F);

        while ((task_a == 0x0100) && (idx < buf_end)) {

          std::bitset<16> pattern (buff16[idx++]);

          Hit one_hit;
          //          one_hit.run_type  = buf_format;
          //          one_hit.module    = buf_module;

          uint16_t evt_time_hi = buff16[idx++];
          uint16_t evt_time_lo = buff16[idx++];

          std::multiset<Hit> ordered;

          for (int i=0; i < NUMBER_OF_CHANNELS; i++) {
            if (pattern[i]) {
              one_hit.channel = i;
              uint64_t hi = buf_timehi;
              uint64_t mi = evt_time_hi;
              uint64_t lo = evt_time_lo;
              uint16_t chan_trig_time = lo;
              uint16_t chan_time_hi   = hi;

              if (task_b == 0x0000) {
                uint16_t trace_len     = buff16[idx++] - 9;
                chan_trig_time         = buff16[idx++];
                if (pattern[i+8])
                  one_hit.energy       = buff16[idx++];
                else
                  idx++;
                one_hit.XIA_PSA        = buff16[idx++];
                one_hit.user_PSA       = buff16[idx++];
                idx += 3;
                hi                     = buff16[idx++]; //not always!
                one_hit.trace = std::vector<uint16_t>
                    (buff16 + idx, buff16 + idx + trace_len);
                idx += trace_len;
              } else if (task_b == 0x0001) {
                idx++;
                chan_trig_time         = buff16[idx++];
                if (pattern[i+8])
                  one_hit.energy       = buff16[idx++];
                else
                  idx++;
                one_hit.XIA_PSA        = buff16[idx++];
                one_hit.user_PSA       = buff16[idx++];
                idx += 3;
                hi                     = buff16[idx++];
              } else if (task_b == 0x0002) {
                chan_trig_time         = buff16[idx++];
                if (pattern[i+8])
                  one_hit.energy       = buff16[idx++];
                else
                  idx++;
                one_hit.XIA_PSA        = buff16[idx++];
                one_hit.user_PSA       = buff16[idx++];
              } else if (task_b == 0x0003) {
                chan_trig_time         = buff16[idx++];
                if (pattern[i+8])
                  one_hit.energy       = buff16[idx++];
                else
                  idx++;
              }
              //Corrections for overflow, page 30 in Pixie-4 user manual
              if (chan_trig_time > evt_time_lo)
                mi--;
              if (evt_time_hi < buf_timemi)
                hi++;
              if ((task_b == 0x0000) || (task_b == 0x0001))
                hi = chan_time_hi;
              lo = chan_trig_time;
              one_hit.timestamp.time = (hi << 32) + (mi << 16) + lo;

              if ((buf_module < channel_indices.size()) &&
                  (i < channel_indices[buf_module].size()) &&
                  (channel_indices[buf_module][i] >= 0))
                one_hit.channel = channel_indices[buf_module][i];
              //else one_hit.channel will = -1, which is invalid

              ordered.insert(one_hit);
            }
          }

          for (auto &q : ordered) {
            spill->hits.push_back(q);
            spill_events++;
          }

        };
      }
      //FIX THIS!!!
      //spill->stats->events_in_spill = spill_events;
      all_events += spill_events;
    }
    if (spill->run != nullptr) {
      spill->run->total_events = all_events;
    }
    spill->data.clear();
    out_queue->enqueue(spill);
    parse_timer.stop();
  }

  if (cycles == 0)
    PL_INFO << "<PixiePlugin> Parser stopping. Buffer queue closed without events";
  else
    PL_INFO << "<PixiePlugin> Parser stopping. " << all_events << " events, with avg time/spill: " << parse_timer.us()/cycles << "us";
}


}
