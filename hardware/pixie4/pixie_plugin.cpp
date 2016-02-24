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
#include "custom_logger.h"
#include "custom_timer.h"

//XIA stuff:
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "PlxTypes.h"
#include "Plx.h"
#include "globals.h"
#include "sharedfiles.h"
#include "utilities.h"

namespace Qpx {

static DeviceRegistrar<Plugin> registrar("Pixie4");

Plugin::Plugin() {
  boot_files_.resize(7);
  system_parameter_values_.resize(N_SYSTEM_PAR, 0.0);
  module_parameter_values_.resize(PRESET_MAX_MODULES*N_MODULE_PAR, 0.0);
  channel_parameter_values_.resize(PRESET_MAX_MODULES*N_CHANNEL_PAR*NUMBER_OF_CHANNELS, 0.0);

  run_poll_interval_ms_ = 100;
  run_type_ = 0x103;

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  runner_ = nullptr;
  parser_ = nullptr;
  raw_queue_ = nullptr;

  run_status_.store(0);
}


Plugin::~Plugin() {
  daq_stop();
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

bool Plugin::daq_start(SynchronizedQueue<Spill*>* out_queue) {
  if (run_status_.load() > 0)
    return false;

  run_status_.store(1);

  if (runner_ != nullptr)
    delete runner_;

  for (int i=0; i < channel_indices_.size(); ++i) {
    //PL_DBG << "start daq run mod " << i;
    for (auto &q : channel_indices_[i])
      //PL_DBG << " chan=" << q;
    set_mod("RUN_TYPE",    static_cast<double>(run_type_), Module(i));
    set_mod("MAX_EVENTS",  static_cast<double>(0), Module(i));
  }

  raw_queue_ = new SynchronizedQueue<Spill*>();
  runner_ = new boost::thread(&worker_run_dbl, this, raw_queue_);

  if (parser_ != nullptr)
    delete parser_;

  parser_ = new boost::thread(&worker_parse, this, raw_queue_, out_queue);

  return true;
}

bool Plugin::daq_stop() {
  if (run_status_.load() == 0)
    return false;

  run_status_.store(2);

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
  delete raw_queue_;
  raw_queue_ = nullptr;

  run_status_.store(0);
  return true;
}

bool Plugin::daq_running() {
  if (run_status_.load() == 3)
    daq_stop();
  return (run_status_.load() > 0);
}

void Plugin::fill_stats(std::list<StatsUpdate> &all_stats, uint8_t module) {
  StatsUpdate stats;

  stats.event_rate = get_mod("EVENT_RATE", Module(module));
  stats.total_time = get_mod("TOTAL_TIME", Module(module));
  stats.model_hit.timestamp.timebase_multiplier = 1000;
  stats.model_hit.timestamp.timebase_divider    = 75;
  for (int i=0; i<NUMBER_OF_CHANNELS; ++i) {
    stats.source_channel    = channel_indices_[module][i];
    stats.fast_peaks = get_chan("FAST_PEAKS", Channel(i), Module(module));
    stats.live_time  = get_chan("LIVE_TIME", Channel(i), Module(module));
    stats.ftdt       = get_chan("FTDT", Channel(i), Module(module));
    stats.sfdt       = get_chan("SFDT", Channel(i), Module(module));
    all_stats.push_back(stats);
    //PL_DBG << "stats chan " << stats.channel;
  }
}


/*void Plugin::get_stats() {
  get_mod_stats(Module::all);
  get_chan_stats(Channel::all, Module::all);
  }*/


bool Plugin::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ != device_name())
    return false;

  for (auto &q : set.branches.my_data_) {
    if (set.metadata.setting_type == Gamma::SettingType::command)
      set.metadata.writable =  ((status_ & DeviceStatus::booted) != 0);

    if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "Pixie4/Run settings")) {
      for (auto &k : q.branches.my_data_) {
        if ((k.metadata.setting_type == Gamma::SettingType::int_menu) && (k.id_ == "Pixie4/Run type"))
          k.value_int = run_type_;
        if ((k.metadata.setting_type == Gamma::SettingType::integer) && (k.id_ == "Pixie4/Poll interval"))
          k.value_int = run_poll_interval_ms_;
      }
    } else if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "Pixie4/Files")) {
      for (auto &k : q.branches.my_data_) {
        k.metadata.writable = !(status_ & DeviceStatus::booted);
        if ((k.metadata.setting_type == Gamma::SettingType::dir_path) && (k.id_ == "Pixie4/Files/XIA_path"))
          k.value_text = XIA_file_directory_;
        else if ((k.metadata.setting_type == Gamma::SettingType::file_path) && (k.metadata.address > 0) && (k.metadata.address < 8))
          k.value_text = boot_files_[k.metadata.address - 1];
      }
    } else if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "Pixie4/System")) {
      for (auto &k : q.branches.my_data_) {
        k.metadata.writable = (!(status_ & DeviceStatus::booted) && setting_definitions_.count(k.id_) && setting_definitions_.at(k.id_).writable);
        if (k.metadata.setting_type == Gamma::SettingType::stem) {
          int16_t modnum = k.metadata.address;
          if ((modnum < 0) || (modnum >= channel_indices_.size())) {
            PL_WARN << "<PixiePlugin> module address out of bounds, ignoring branch " << modnum;
            continue;
          }
          int filterrange = module_parameter_values_[modnum * N_MODULE_PAR + i_mod("FILTER_RANGE")];
          for (auto &p : k.branches.my_data_) {
            if (p.metadata.setting_type == Gamma::SettingType::stem) {
              int16_t channum = p.metadata.address;
              if ((channum < 0) || (channum >= NUMBER_OF_CHANNELS)) {
                PL_WARN << "<PixiePlugin> channel address out of bounds, ignoring branch " << channum;
                continue;
              }
              for (auto &o : p.branches.my_data_) {
                if (o.metadata.setting_type == Gamma::SettingType::floating) {
                  o.value_dbl = channel_parameter_values_[o.metadata.address + modnum * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + channum * N_CHANNEL_PAR];
                  if (o.metadata.name == "ENERGY_RISETIME") {
                    o.metadata.step = static_cast<double>(pow(2, filterrange)) / 75.0 ;
                    o.metadata.minimum = 2 * o.metadata.step;
                    o.metadata.maximum = 124 * o.metadata.step;
                    //                      PL_DBG << "risetime " << o.metadata.minimum << "-" << o.metadata.step << "-" << o.metadata.maximum;
                  } else if (o.metadata.name == "ENERGY_FLATTOP") {
                    o.metadata.step = static_cast<double>(pow(2, filterrange)) / 75.0;
                    o.metadata.minimum = 3 * o.metadata.step;
                    o.metadata.maximum = 125 * o.metadata.step;
                    //                      PL_DBG << "flattop " << o.metadata.minimum << "-" << o.metadata.step << "-" << o.metadata.maximum;
                  }
                }
                else if ((o.metadata.setting_type == Gamma::SettingType::integer)
                         || (o.metadata.setting_type == Gamma::SettingType::boolean)
                         || (o.metadata.setting_type == Gamma::SettingType::int_menu)
                         || (o.metadata.setting_type == Gamma::SettingType::binary))
                  o.value_int = channel_parameter_values_[o.metadata.address + modnum * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + channum * N_CHANNEL_PAR];
              }
            }
            else if (p.metadata.setting_type == Gamma::SettingType::floating)
              p.value_dbl = module_parameter_values_[modnum * N_MODULE_PAR +  p.metadata.address];
            else if ((p.metadata.setting_type == Gamma::SettingType::integer)
                     || (p.metadata.setting_type == Gamma::SettingType::boolean)
                     || (p.metadata.setting_type == Gamma::SettingType::int_menu)
                     || (p.metadata.setting_type == Gamma::SettingType::binary))
              p.value_int = module_parameter_values_[modnum * N_MODULE_PAR +  p.metadata.address];
          }
        }
        else if (k.metadata.setting_type == Gamma::SettingType::floating)
          k.value_dbl = system_parameter_values_[k.metadata.address];
        else if ((k.metadata.setting_type == Gamma::SettingType::integer)
                 || (k.metadata.setting_type == Gamma::SettingType::boolean)
                 || (k.metadata.setting_type == Gamma::SettingType::int_menu)
                 || (k.metadata.setting_type == Gamma::SettingType::binary))
          k.value_int = system_parameter_values_[k.metadata.address];
      }
    }
  }
  return true;
}

void Plugin::rebuild_structure(Gamma::Setting &set) {
  Gamma::Setting maxmod("Pixie4/System/MAX_NUMBER_MODULES");
  maxmod = set.get_setting(maxmod, Gamma::Match::id);

  Gamma::Setting totmod("Pixie4/System/NUMBER_MODULES");
  totmod = set.get_setting(totmod, Gamma::Match::id);

  Gamma::Setting slot("Pixie4/System/SLOT_WAVE");
  slot.enrich(setting_definitions_, true);

  Gamma::Setting chan("Pixie4/System/module/channel");
  chan.enrich(setting_definitions_, true);

  Gamma::Setting mod("Pixie4/System/module");
  mod.enrich(setting_definitions_, true);
  for (int j=0; j < NUMBER_OF_CHANNELS; ++j) {
    chan.metadata.address = j;
    mod.branches.add_a(chan);
  }


  int newmax = maxmod.value_int;
  int oldtot = totmod.value_int;

  if (newmax > N_SYSTEM_PAR - 7)
    newmax = N_SYSTEM_PAR - 7;
  else if (newmax < 1)
    newmax = 1;

  if (newmax != maxmod.value_int) {
    maxmod.value_int = newmax;
    set.branches.replace(maxmod);
  }

  int newtot = 0;
  std::vector<Gamma::Setting> old_slots;
  for (auto &q : set.branches.my_data_) {
    if (q.id_ == "Pixie4/System/SLOT_WAVE") {
      old_slots.push_back(q);
      if (q.value_int > 0)
        newtot++;
    }
    if (old_slots.size() == newmax)
      break;
  }

  while (old_slots.size() > newmax)
    old_slots.pop_back();

  while (old_slots.size() < newmax)
    old_slots.push_back(slot);

  if (newtot == 0) {
    newtot = 1;
    old_slots[0].value_int = 2;
  }

  bool hardware_changed = false;
  for (int i=0;i < old_slots.size(); ++i) {
    old_slots[i].metadata.address = 7+i;
    if (system_parameter_values_[7+i] != old_slots[i].value_int)
      hardware_changed = true;
  }

  if (newtot != oldtot)
    hardware_changed = true;

//  if (hardware_changed) {
    totmod.value_int = newtot;
    set.branches.replace(totmod);
    while (set.branches.has_a(slot))
      set.branches.remove_a(slot);
    for (auto &q : old_slots)
      set.branches.add_a(q);
//  }

  //if (hardware_changed) PL_DBG << "hardware changed";

//  if (newtot != oldtot) {
    std::vector<Gamma::Setting> old_modules;

    int actualmods = 0;
    for (auto &q : set.branches.my_data_) {
      if (q.id_ == "Pixie4/System/module")  {
        old_modules.push_back(q);
        actualmods++;
      }
      if (old_modules.size() == newtot)
        break;
    }

    while (old_modules.size() > newtot)
      old_modules.pop_back();

    while (old_modules.size() < newtot)
      old_modules.push_back(mod);

//    if (actualmods != newtot) {
      while (set.branches.has_a(mod))
        set.branches.remove_a(mod);

      for (auto &q : old_modules)
        set.branches.add_a(q);
//    }

    channel_indices_.resize(newtot);
    for (auto &q : channel_indices_)
      q.resize(NUMBER_OF_CHANNELS, -1);

//  }
}

void Plugin::reindex_modules(Gamma::Setting &set) {

  int ma = 0;
  for (auto &m : set.branches.my_data_) {
    if (m.id_ == "Pixie4/System/module") {
      std::set<int32_t> new_set;
      int ca = 0;
      for (auto &c : m.branches.my_data_) {
        if (c.id_ == "Pixie4/System/module/channel") {
          if (c.indices.size() > 1) {
            int32_t i = *c.indices.begin();
            c.indices.clear();
            c.indices.insert(i);
          }
          if (c.indices.size() > 0)
            new_set.insert(*c.indices.begin());
          c.metadata.address = ca;
//          PL_DBG << "<PixiePlugin> ca = " << c.metadata.address;
          ca++;
        }
      }
      m.indices = new_set;
      m.metadata.address = ma;
//      PL_DBG << "<PixiePlugin> ma = " << m.metadata.address;
      ma++;
    }
  }
}


bool Plugin::write_settings_bulk(Gamma::Setting &set) {
  if (set.id_ != device_name())
    return false;

  set.enrich(setting_definitions_);

  for (auto &q : set.branches.my_data_) {
    if ((q.metadata.setting_type == Gamma::SettingType::command) && (q.value_int == 1)) {
      q.value_int = 0;
      if (q.id_ == "Pixie4/Measure baselines")
        return control_measure_baselines(Module::all);
      else if (q.id_ == "Pixie4/Adjust offsets")
        return control_adjust_offsets(Module::all);
      else if (q.id_ == "Pixie4/Compute Tau")
        return control_find_tau(Module::all);
      else if (q.id_ == "Pixie4/Compute BLCUT")
        return control_compute_BLcut();
    } else if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "Pixie4/Files") && !(status_ & DeviceStatus::booted)) {
      for (auto &k : q.branches.my_data_) {
        if ((k.metadata.setting_type == Gamma::SettingType::dir_path) && (k.id_ == "Pixie4/Files/XIA_path")) {
          if (XIA_file_directory_ != k.value_text) {
            if (!XIA_file_directory_.empty()) {
              XIA_file_directory_ = k.value_text;
              boost::filesystem::path path(k.value_text);
              for (auto &f : boot_files_) {
                boost::filesystem::path file(f);
                boost::filesystem::path full_path = path / file.filename();
                f = full_path.string();
              }
              break;
            } else
              XIA_file_directory_ = k.value_text;
          }
        } else if ((k.metadata.setting_type == Gamma::SettingType::file_path) && (k.metadata.address > 0) && (k.metadata.address < 8))
          boot_files_[k.metadata.address - 1] = k.value_text;
      }
    } else if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "Pixie4/Run settings")) {
      for (auto &k : q.branches.my_data_) {
        if (k.id_ == "Pixie4/Run settings/Run type")
          run_type_ = k.value_int;
        else if (k.id_ == "Pixie4/Run settings/Poll interval")
          run_poll_interval_ms_ = k.value_int;
      }
    } else if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "Pixie4/System")) {
      if (!(status_ & DeviceStatus::booted))
        rebuild_structure(q);

      reindex_modules(q);

      for (auto &k : q.branches.my_data_) {

        if (k.metadata.setting_type == Gamma::SettingType::stem) {
          int16_t modnum = k.metadata.address;
          if ((modnum < 0) || (modnum >= channel_indices_.size())) {
            PL_WARN << "<PixiePlugin> module address out of bounds, ignoring branch " << modnum;
            continue;
          }
          for (auto &p : k.branches.my_data_) {
            if (p.metadata.setting_type != Gamma::SettingType::stem)
              p.indices = k.indices;

            if (p.metadata.setting_type == Gamma::SettingType::stem) {
              int16_t channum = p.metadata.address;
              if ((channum < 0) || (channum >= NUMBER_OF_CHANNELS)) {
                PL_WARN << "<PixiePlugin> channel address out of bounds, ignoring branch " << channum;
                continue;
              }

              int det = -1;
              if (!p.indices.empty())
                det = *p.indices.begin();
              channel_indices_[modnum][channum] = det;

              for (auto &o : p.branches.my_data_) {
                o.indices.clear();
                o.indices.insert(det);

                if (o.metadata.writable && (o.metadata.setting_type == Gamma::SettingType::floating) && (channel_parameter_values_[o.metadata.address + modnum * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + channum * N_CHANNEL_PAR] != o.value_dbl)) {
                  channel_parameter_values_[o.metadata.address + modnum * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + channum * N_CHANNEL_PAR] = o.value_dbl;
                  write_chan(o.metadata.name.c_str(), modnum, channum);
                } else if (o.metadata.writable && ((o.metadata.setting_type == Gamma::SettingType::integer)
                                                   || (o.metadata.setting_type == Gamma::SettingType::boolean)
                                                   || (o.metadata.setting_type == Gamma::SettingType::int_menu)
                                                   || (o.metadata.setting_type == Gamma::SettingType::binary))
                           && (channel_parameter_values_[o.metadata.address + modnum * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + channum * N_CHANNEL_PAR] != o.value_int)) {
                  channel_parameter_values_[o.metadata.address + modnum * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + channum * N_CHANNEL_PAR] = o.value_int;
                  write_chan(o.metadata.name.c_str(), modnum, channum);
                }
              }
            } else if (p.metadata.writable && (p.metadata.setting_type == Gamma::SettingType::floating) && (module_parameter_values_[modnum * N_MODULE_PAR +  p.metadata.address] != p.value_dbl)) {
              module_parameter_values_[modnum * N_MODULE_PAR +  p.metadata.address] = p.value_dbl;
              write_mod(p.metadata.name.c_str(), modnum);
            } else if (p.metadata.writable && ((p.metadata.setting_type == Gamma::SettingType::integer)
                                               || (p.metadata.setting_type == Gamma::SettingType::boolean)
                                               || (p.metadata.setting_type == Gamma::SettingType::int_menu)
                                               || (p.metadata.setting_type == Gamma::SettingType::binary))
                       && (module_parameter_values_[modnum * N_MODULE_PAR +  p.metadata.address] != p.value_int)) {
              module_parameter_values_[modnum * N_MODULE_PAR +  p.metadata.address] = p.value_int;
              write_mod(p.metadata.name.c_str(), modnum);
            }
          }
        } else if (((k.metadata.setting_type == Gamma::SettingType::integer)
                                           || (k.metadata.setting_type == Gamma::SettingType::boolean)
                                           || (k.metadata.setting_type == Gamma::SettingType::int_menu)
                                           || (k.metadata.setting_type == Gamma::SettingType::binary))
                   && (system_parameter_values_[k.metadata.address] != k.value_int)) {
          system_parameter_values_[k.metadata.address] = k.value_int;
          write_sys(k.metadata.name.c_str());
        }
      }
    }
  }
  return true;
}

bool Plugin::boot() {
  if (!(status_ & DeviceStatus::can_boot)) {
    PL_WARN << "<PixiePlugin> Cannot boot Pixie-4. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  S32 retval;
  set_sys("OFFLINE_ANALYSIS", 0);  //attempt live boot first
  set_sys("AUTO_PROCESSLMDATA", 0);  //do not write XIA datafile

  bool valid_files = true;
  for (int i=0; i < 7; i++) {
    memcpy(Boot_File_Name_List[i], (S8*)boot_files_[i].c_str(), boot_files_[i].size());
    Boot_File_Name_List[i][boot_files_[i].size()] = '\0';
    bool ex = boost::filesystem::exists(boot_files_[i]);
    if (!ex) {
      PL_ERR << "<PixiePlugin> Boot file " << boot_files_[i] << " not found";
      valid_files = false;
    }
  }

  if (!valid_files) {
    PL_ERR << "<PixiePlugin> Problem with boot files. Boot aborting.";
    return false;
  }

  int max = get_sys("NUMBER_MODULES");
  if (!max) {
    PL_ERR << "<PixiePlugin> No valid module slots.";
  } else {
    //PL_DBG << "Number of modules to boot: " << max;
    read_sys("SLOT_WAVE");
    for (int i=0; i < max; i++)
      PL_DBG << "<PixiePlugin> Booting module " << i << " in slot "
             << system_parameter_values_[i_sys("SLOT_WAVE") + i];
  }

  retval = Pixie_Boot_System(0x1F);
  //bad files do not result in boot error!!

  if (retval >= 0) {
    status_ = DeviceStatus::loaded | DeviceStatus::booted
        | DeviceStatus::can_run | DeviceStatus::can_oscil;

    //sleep 1 s and do offsets

    return true;
//  }

//  boot_err(retval);
//  set_sys("OFFLINE_ANALYSIS", 1);  //else attempt offline boot
//  retval = Pixie_Boot_System(0x1F);
//  if (retval >= 0) {
//    status_ = DeviceStatus::loaded | DeviceStatus::booted;

//    return true;
  } else {
    boot_err(retval);
    return false;
  }
}

std::map<int, std::vector<uint16_t>> Plugin::oscilloscope() {
  std::map<int, std::vector<uint16_t>> result;

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
  if (status_ & DeviceStatus::booted) {
    get_sys_all();
    get_mod_all(Module::all);
    get_chan_all(Channel::all, Module::all);
  }
}


void Plugin::reset_counters_next_run() {
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
      //PL_INFO << "Start run for module " << i;
      if (start_run(i))
        success = true;
    }
  } else {
    int module = static_cast<int>(mod);
    if ((module > -1) && (module < channel_indices_.size())) {
      //PL_INFO << "Start run for module " << module;
      success = start_run(module);
    }
  }
  return success;
}

bool Plugin::resume_run(Module mod) {
  bool success = false;
  if (mod == Module::all) {
    for (int i=0; i< channel_indices_.size(); ++i) {
      //PL_INFO << "Resume run for module " << i;
      if (resume_run(i))
        success = true;
    }
  } else {
    int module = static_cast<int>(mod);
    if ((module > -1) && (module < channel_indices_.size())) {
      //PL_INFO << "Resume run for module " << module;
      success = resume_run(module);
    }
  }
  return success;
}

bool Plugin::stop_run(Module mod) {
  bool success = false;
  if (mod == Module::all) {
    for (int i=0; i< channel_indices_.size(); ++i) {
      //PL_INFO << "Stop run for module " << i;
      if (stop_run(i))
        success = true;
    }
  } else {
    int module = static_cast<int>(mod);
    if ((module > -1) && (module < channel_indices_.size())) {
      //PL_INFO << "Stop run for module " << module;
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
  U16 j;
//  U16 DblBufCSR, MCSRA;
  U32 Aoffset[2], WordCountPP[2];
  U32 WordCount, NumWordsToRead, CSR;
  U32 dsp_word[2];

//  char modcsra_str[] = "MODCSRA";
//  char dblbufcsr_str[] = "DBLBUFCSR";
//  Pixie_IODM(0, (U16)DATA_MEMORY_ADDRESS + i_dsp("MODCSRA"), MOD_READ, 1, dsp_word);
//  MCSRA = (U16)dsp_word[0];
//  Pixie_IODM(0, (U16)DATA_MEMORY_ADDRESS + i_dsp("DBLBUFCSR"), MOD_READ, 1, dsp_word);
//  DblBufCSR = (U16)dsp_word[0];

  Aoffset[0] = 0;
  Aoffset[1] = LM_DBLBUF_BLOCK_LENGTH;

  Pixie_ReadCSR((U8)mod, &CSR);
  // A read of Pixie's word count register
  // This also indicates to the DSP that a readout has begun
  Pixie_RdWrdCnt((U8)mod, &WordCount);

  // The number of 16-bit words to read is in EMwords or EMwords2
//  char emwords_str[] = "EMWORDS";
//  char emwords2_str[] = "EMWORDS2";
  Pixie_IODM((U8)mod, (U16)DATA_MEMORY_ADDRESS + i_dsp("EMWORDS"), MOD_READ, 2, dsp_word);
  WordCountPP[0] = dsp_word[0] * 65536 + dsp_word[1];
  Pixie_IODM((U8)mod, (U16)DATA_MEMORY_ADDRESS + i_dsp("EMWORDS2"), MOD_READ, 2, dsp_word);
  WordCountPP[1] = dsp_word[0] * 65536 + dsp_word[1];

  if(TstBit(CSR_128K_FIRST, (U16)CSR) == 1)
    j=0;
  else		// block at 128K+64K was first
    j=1;

  if  (TstBit(CSR_DATAREADY, (U16)CSR) == 0 )
    // function called after a readout that cleared WCR => run stopped => read other block
  {
    j=1-j;
    PL_WARN << "<PixiePlugin> read_EM_dbl: module " << mod <<
               " csr reports both memory blocks full (block " << 1-j << " older). Run paused (or finished).";
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
      PL_ERR << "<PixiePlugin> read_EM_dbl: invalid word count " << NumWordsToRead;
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
  PL_TRC << "Setting " << setting << " to " << val << " for system";

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

  PL_TRC << "Getting all system";
  read_sys("ALL_SYSTEM_PARAMETERS");
}


//////Module Settings//////
///////////////////////////

void Plugin::set_mod(const std::string& setting, double val, Module mod) {
  int module = static_cast<int>(mod);
  if (module > -1) {
    //PL_INFO << "Setting " << setting << " to " << val << " for module " << module;
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

void Plugin::get_mod_all(Module mod) {
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

void Plugin::get_chan_all(Channel channel, Module module) {
  int mod_start = -1;
  int mod_end = -1;

  if (module == Module::all) {
    mod_start = 0;
    mod_end = channel_indices_.size();
  } else {
    int mod = static_cast<int>(module);
    if ((mod > -1) && (mod < channel_indices_.size())) {
      mod_start = mod;
      mod_end = mod + 1;
    }
  }

  for (int i=mod_start; i < mod_end; ++i) {
    int chan_start = -1;
    int chan_end = -1;
    if (channel == Channel::all) {
      chan_start = 0;
      chan_end = channel_indices_[i].size();
    } else {
      int chan = static_cast<int>(channel);
      if ((chan > -1) && (chan < channel_indices_[i].size())) {
        chan_start = chan;
        chan_end = chan + 1;
      }
    }
    for (int j=chan_start; j < chan_end; ++j) {
      PL_TRC << "Getting all parameters for module " << i << " channel " << j;
      read_chan("ALL_CHANNEL_PARAMETERS", i, j);
    }
  }
}

void Plugin::get_chan_stats(Channel channel, Module module) {
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
      //PL_DBG << "Getting channel run statistics for module " << i << " channel " << j;
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
  if (status_ & DeviceStatus::booted)
    ret = Pixie_User_Par_IO(module_parameter_values_.data(),
                            (S8*) setting, modstr, MOD_WRITE, mod, 0);
  set_err(ret);
  return (ret == 0);
}

bool Plugin::read_mod(const char* setting, uint8_t mod) {
  S32 ret = -42;
  S8 modstr[8] = "MODULE\0";
  if (status_ & DeviceStatus::booted)
    ret = Pixie_User_Par_IO(module_parameter_values_.data(),
                            (S8*) setting, modstr, MOD_READ, mod, 0);
  set_err(ret);
  return (ret == 0);
}

bool Plugin::write_chan(const char* setting, uint8_t mod, uint8_t chan) {
  S32 ret = -42;
  S8 chnstr[9] = "CHANNEL\0";
  if (status_ & DeviceStatus::booted)
    ret = Pixie_User_Par_IO(channel_parameter_values_.data(),
                            (S8*) setting, chnstr, MOD_WRITE, mod, chan);
  set_err(ret);
  return (ret == 0);
}

bool Plugin::read_chan(const char* setting, uint8_t mod, uint8_t chan) {
  S32 ret = -42;
  S8 chnstr[9] = "CHANNEL\0";
  if (status_ & DeviceStatus::booted)
    ret = Pixie_User_Par_IO(channel_parameter_values_.data(),
                            (S8*) setting, chnstr, MOD_READ, mod, chan);
  set_err(ret);
  return (ret == 0);
}

uint32_t* Plugin::control_collect_ADC(uint8_t module) {
  PL_TRC << "<PixiePlugin> get ADC (oscilloscope) traces";
  ///why is NUMBER_OF_CHANNELS used? Same for multi-module?
  uint32_t* adc_data = new uint32_t[NUMBER_OF_CHANNELS * max_buf_len];
  if (status_ & DeviceStatus::can_oscil) {
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
  //PL_INFO << "Pixie Control: set DAC";
  return control_err(Pixie_Acquire_Data(0x0000, NULL, NULL, module));
}

bool Plugin::control_connect(uint8_t module) {
  //PL_INFO << "Pixie Control: connect";
  return control_err(Pixie_Acquire_Data(0x0001, NULL, NULL, module));
}

bool Plugin::control_disconnect(uint8_t module) {
  //PL_INFO << "Pixie Control: disconnect";
  return control_err(Pixie_Acquire_Data(0x0002, NULL, NULL, module));
}

bool Plugin::control_program_Fippi(uint8_t module) {
  //PL_INFO << "Pixie Control: program Fippi";
  return control_err(Pixie_Acquire_Data(0x0005, NULL, NULL, module));
}

bool Plugin::control_measure_baselines(Module mod) {
  bool success = false;
  if (status_ & DeviceStatus::booted) {
    if (mod == Module::all) {
      for (int i=0; i< channel_indices_.size(); ++i) {
        PL_DBG << "<PixiePlugin> measure baselines for module " << i;
        if (control_err(Pixie_Acquire_Data(0x0006, NULL, NULL, i)))
          success = true;
      }
    } else {
      int module = static_cast<int>(mod);
      if ((module > -1) && (module < channel_indices_.size())) {
        PL_DBG << "<PixiePlugin> measure baselines for module " << module;
        success = control_err(Pixie_Acquire_Data(0x0006, NULL, NULL, module));
      }
    }
  }
  return success;
}

bool Plugin::control_test_EM_write(uint8_t module) {
  PL_TRC << "<PixiePlugin> test EM write";
  return control_err(Pixie_Acquire_Data(0x0016, NULL, NULL, module));
}

bool Plugin::control_test_HM_write(uint8_t module) {
  PL_TRC << "<PixiePlugin> test HM write";
  return control_err(Pixie_Acquire_Data(0x001A, NULL, NULL, module));
}

bool Plugin::control_compute_BLcut() {
  PL_TRC << "<PixiePlugin> compute BLcut";
  return control_err(Pixie_Acquire_Data(0x0080, NULL, NULL, 0));
}

bool Plugin::control_find_tau(Module mod) {
  bool success = false;
  if (status_ & DeviceStatus::booted) {
    if (mod == Module::all) {
      for (int i=0; i< channel_indices_.size(); ++i) {
        PL_DBG << "<PixiePlugin> find tau for module " << i;
        if (control_err(Pixie_Acquire_Data(0x0081, NULL, NULL, i)))
          success = true;
      }
    } else {
      int module = static_cast<int>(mod);
      if ((module > -1) && (module < channel_indices_.size())) {
        PL_DBG << "<PixiePlugin> find tau for module " << module;
        success = control_err(Pixie_Acquire_Data(0x0081, NULL, NULL, module));
      }
    }
  }
  return success;
}

bool Plugin::control_adjust_offsets(Module mod) {
  bool success = false;
  if (status_ & DeviceStatus::booted) {
    if (mod == Module::all) {
      for (int i=0; i< channel_indices_.size(); ++i) {
        PL_TRC << "<PixiePlugin> djust offsets for module " << i;
        if (control_err(Pixie_Acquire_Data(0x0083, NULL, NULL, i)))
          success = true;
      }
    } else {
      int module = static_cast<int>(mod);
      if ((module > -1) && (module < channel_indices_.size())) {
        PL_TRC << "<PixiePlugin> adjust offsets for module " << module;
        success = control_err(Pixie_Acquire_Data(0x0083, NULL, NULL, module));
      }
    }
  }
  return success;
}

bool Plugin::control_err(int32_t err_val) {
  switch (err_val) {
  case 0:
    return true;
  case -1:
    PL_ERR << "<PixiePlugin> Control command failed: Invalid Pixie modules number. Check ModNum";
    break;
  case -2:
    PL_ERR << "<PixiePlugin> Control command failed: Failure to adjust offsets. Reboot recommended";
    break;
  case -3:
    PL_ERR << "<PixiePlugin> Control command failed: Failure to acquire ADC traces. Reboot recommended";
    break;
  case -4:
    PL_ERR << "<PixiePlugin> Control command failed: Failure to start the control task run. Reboot recommended";
    break;
  default:
    PL_ERR << "<PixiePlugin> Control comman failed: Unknown error " << err_val;
  }
  return false;
}

void Plugin::set_err(int32_t err_val) {
  switch (err_val) {
  case 0:
    break;
  case -1:
    PL_ERR << "<PixiePlugin> Set/get parameter failed: Null pointer for User_Par_Values";
    break;
  case -2:
    PL_ERR << "<PixiePlugin> Set/get parameter failed: Invalid user parameter name";
    break;
  case -3:
    PL_ERR << "<PixiePlugin> Set/get parameter failed: Invalid user parameter type";
    break;
  case -4:
    PL_ERR << "<PixiePlugin> Set/get parameter failed: Invalid I/O direction";
    break;
  case -5:
    PL_ERR << "<PixiePlugin> Set/get parameter failed: Invalid module number";
    break;
  case -6:
    PL_ERR << "<PixiePlugin> Set/get parameter failed: Invalid channel number";
    break;
  case -42:
    //PL_ERR << "Set/get parameter failed: Pixie not online";
    break;
  default:
    PL_ERR << "<PixiePlugin> Set/get parameter failed: Unknown error " << err_val;
  }
}

void Plugin::boot_err(int32_t err_val) {
  switch (err_val) {
  case 0:
    break;
  case -1:
    PL_ERR << "<PixiePlugin> Boot failed: unable to scan crate slots. Check PXI slot map.";
    break;
  case -2:
    PL_ERR << "<PixiePlugin> Boot failed: unable to read communication FPGA (rev. B). Check comFPGA file.";
    break;
  case -3:
    PL_ERR << "<PixiePlugin> Boot failed: unable to read communication FPGA (rev. C). Check comFPGA file.";
    break;
  case -4:
    PL_ERR << "<PixiePlugin> Boot failed: unable to read signal processing FPGA config. Check SPFPGA file.";
    break;
  case -5:
    PL_ERR << "<PixiePlugin> Boot failed: unable to read DSP executable code. Check DSP code file.";
    break;
  case -6:
    PL_ERR << "<PixiePlugin> Boot failed: unable to read DSP parameter values. Check DSP parameter file.";
    break;
  case -7:
    PL_ERR << "<PixiePlugin> Boot failed: unable to initialize DSP parameter names. Check DSP .var file.";
    break;
  case -8:
    PL_ERR << "<PixiePlugin> Boot failed: failed to boot all modules in the system. Check Pixie modules.";
    break;
  default:
    PL_ERR << "<PixiePlugin> Boot failed, undefined error " << err_val;
  }
}


void Plugin::worker_run_dbl(Plugin* callback, SynchronizedQueue<Spill*>* spill_queue) {

  PL_DBG << "<PixiePlugin> Double buffered daq runner thread starting";

  callback->reset_counters_next_run(); //assume new run

  std::bitset<32> csr;
  bool timeout = false;
  Spill fetched_spill;
  boost::posix_time::ptime session_start_time, block_time;

  for (int i=0; i < callback->channel_indices_.size(); i++) {
    if (!callback->clear_EM(i))
      return;
    callback->set_mod("DBLBUFCSR",   static_cast<double>(1), Module(i));
    callback->set_mod("MODULE_CSRA", static_cast<double>(0), Module(i));
  }

  fetched_spill = Spill();
  session_start_time = boost::posix_time::microsec_clock::local_time();

  if(!callback->start_run(Module::all))
    return;

  callback->get_mod_stats(Module::all);
  callback->get_chan_stats(Channel::all, Module::all);
  for (int i=0; i < callback->channel_indices_.size(); i++)
    callback->fill_stats(fetched_spill.stats, i);
  for (auto &q : fetched_spill.stats) {
    q.lab_time = session_start_time;
    q.stats_type = StatsType::start;
  }
  spill_queue->enqueue(new Spill(fetched_spill));

  std::set<int> mods;
  while (!(timeout && mods.empty())) {

    mods.clear();
    while (!timeout && mods.empty()) {
      for (int i=0; i < callback->channel_indices_.size(); ++i) {
        std::bitset<32> csr = std::bitset<32>(poll_run_dbl(i));
        if (csr[14])
          mods.insert(i);
      }
      if (mods.empty())
        wait_ms(callback->run_poll_interval_ms_);
      timeout = (callback->run_status_.load() == 2);
    };

    block_time = boost::posix_time::microsec_clock::local_time();

    if (timeout) {
      callback->stop_run(Module::all);
      wait_ms(callback->run_poll_interval_ms_);
      for (int i=0; i < callback->channel_indices_.size(); ++i) {
        std::bitset<32> csr = std::bitset<32>(poll_run_dbl(i));
        if (csr[14])
          mods.insert(i);
      }
    }

    for (auto &q : mods) {
      //PL_DBG << "getting stats for mod " << q;
      callback->get_mod_stats(Module(q));
      for (int j=0; j < NUMBER_OF_CHANNELS; ++j)
        callback->read_chan("ALL_CHANNEL_PARAMETERS", q, j);
    }


    bool success = false;
    for (auto &q : mods) {
      fetched_spill = Spill();
      fetched_spill.data.resize(list_mem_len32, 0);
      if (read_EM_dbl(fetched_spill.data.data(), q))
        success = true;
      //      PL_DBG << "<PixiePlugin> fetched spill for mod " << q;
      callback->fill_stats(fetched_spill.stats, q);
      for (auto &p : fetched_spill.stats) {
        p.lab_time = block_time;
        if (timeout)
          p.stats_type = StatsType::stop;
      }
      spill_queue->enqueue(new Spill(fetched_spill));
    }

    if (!success) {
      break;
    }
  }


  fetched_spill = Spill();
  block_time = boost::posix_time::microsec_clock::local_time();

  callback->get_mod_stats(Module::all);
  callback->get_chan_stats(Channel::all, Module::all);
  for (int i=0; i < callback->channel_indices_.size(); i++)
    callback->fill_stats(fetched_spill.stats, i);
  for (auto &q : fetched_spill.stats) {
    q.lab_time = block_time;
    q.stats_type = StatsType::stop;
  }
  spill_queue->enqueue(new Spill(fetched_spill));


  callback->run_status_.store(3);
  PL_DBG << "<PixiePlugin> Double buffered daq runner thread completed";
}



void Plugin::worker_parse (Plugin* callback, SynchronizedQueue<Spill*>* in_queue, SynchronizedQueue<Spill*>* out_queue) {

  PL_DBG << "<PixiePlugin> parser thread starting";

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
              one_hit.source_channel = i;
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
                one_hit.extras["XIA_PSA"]  = buff16[idx++];
                one_hit.extras["user_PSA"] = buff16[idx++];
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
                one_hit.extras["XIA_PSA"]  = buff16[idx++];
                one_hit.extras["user_PSA"] = buff16[idx++];
                idx += 3;
                hi                     = buff16[idx++];
              } else if (task_b == 0x0002) {
                chan_trig_time         = buff16[idx++];
                if (pattern[i+8])
                  one_hit.energy       = buff16[idx++];
                else
                  idx++;
                one_hit.extras["XIA_PSA"]  = buff16[idx++];
                one_hit.extras["user_PSA"] = buff16[idx++];
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
              one_hit.timestamp.time_native = (hi << 32) + (mi << 16) + lo;
              one_hit.timestamp.timebase_divider = 75;
              one_hit.timestamp.timebase_multiplier = 1000;

              if ((buf_module < channel_indices.size()) &&
                  (i < channel_indices[buf_module].size()) &&
                  (channel_indices[buf_module][i] >= 0))
                one_hit.source_channel = channel_indices[buf_module][i];
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
    if (spill->run != RunInfo()) {
      spill->run.total_events = all_events;
    }
    spill->data.clear();
    out_queue->enqueue(spill);
    parse_timer.stop();
  }

  if (cycles == 0)
    PL_INFO << "<PixiePlugin> Parser thread stopping. Buffer queue closed without events";
  else
    PL_INFO << "<PixiePlugin> Parser thread stopping. " << all_events << " events, with avg time/spill: " << parse_timer.us()/cycles << "us";
}


}
