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
 *      Pixie::Settings online and offline setting describing the state of
 *      a Pixie-4 device.
 *      Not thread safe, mostly for speed concerns (getting stats during run)
 *
 ******************************************************************************/

#include "px_settings.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "common_xia.h"
#include "custom_logger.h"

namespace Pixie {

Settings::Settings() {
  settings_tree_.setting_type = Gamma::SettingType::stem;
  settings_tree_.name = "QpxSettings";

  num_chans_ = NUMBER_OF_CHANNELS;  //This might need to be changed

  boot_files_.resize(7);
  system_parameter_values_.resize(N_SYSTEM_PAR, 0.0);
  module_parameter_values_.resize(PRESET_MAX_MODULES*N_MODULE_PAR, 0.0);
  channel_parameter_values_.resize(PRESET_MAX_MODULES*N_CHANNEL_PAR*NUMBER_OF_CHANNELS, 0.0);

  live_ = LiveStatus::dead;
}

Settings::Settings(const Settings& other) :
  live_(LiveStatus::history), //this is why we have this function, deactivate if copied
  settings_tree_(other.settings_tree_),

  num_chans_(other.num_chans_), detectors_(other.detectors_),
  boot_files_(other.boot_files_),
  system_parameter_values_(other.system_parameter_values_),
  module_parameter_values_(other.module_parameter_values_),
  channel_parameter_values_(other.channel_parameter_values_) {}

Settings::Settings(tinyxml2::XMLElement* root) :
  Settings()
{
  from_xml(root);
}

Gamma::Setting Settings::pull_settings() {
  return settings_tree_;
}

void Settings::push_settings(const Gamma::Setting& newsettings) {
  PL_INFO << "settings pushed";
  settings_tree_ = newsettings;
  write_settings_bulk();
}

bool Settings::read_settings_bulk(){
  if (live_ == LiveStatus::history)
    return false;

  for (auto &set : settings_tree_.branches.my_data_) {
    PL_INFO << "read bulk "  << set.name;
    if (set.name == "Pixie-4") {
      for (auto &q : set.branches.my_data_) {
        if ((q.setting_type == Gamma::SettingType::stem) && (q.name == "Files")) {
          for (auto &k : q.branches.my_data_) {
            if (k.setting_type == Gamma::SettingType::file_path)
              k.value_text = boot_files_[k.address];
          }
        } else if ((q.setting_type == Gamma::SettingType::stem) && (q.name == "System")) {
          for (auto &k : q.branches.my_data_) {
            if (k.setting_type == Gamma::SettingType::stem) {
              uint16_t modnum = k.address;
              for (auto &p : k.branches.my_data_) {
                if (p.setting_type == Gamma::SettingType::stem) {
                  uint16_t channum = p.address;
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

      settings_tree_.branches.replace(set);
    } else if (set.name == "Detectors") {
      if (detectors_.size() != set.branches.size()) {
        detectors_.resize(set.branches.size(), Gamma::Detector());
        for (auto &q : set.branches.my_data_)
          write_detector(q);
      }
      save_optimization();
    }
  }
  return true;
}

bool Settings::write_settings_bulk(){
  if (live_ == LiveStatus::history)
    return false;

  for (auto &set : settings_tree_.branches.my_data_) {
    PL_INFO << "write bulk "  << set.name;
    if (set.name == "Pixie-4") {
      for (auto &q : set.branches.my_data_) {
        if ((q.setting_type == Gamma::SettingType::stem) && (q.name == "Files")) {
          for (auto &k : q.branches.my_data_) {
            if (k.setting_type == Gamma::SettingType::file_path)
              boot_files_[k.address] = k.value_text;
          }
        } else if ((q.setting_type == Gamma::SettingType::stem) && (q.name == "System")) {
          bool hardware_changed = false;
          Gamma::Setting maxmods  = q.branches.get(Gamma::Setting("MAX_NUMBER_MODULES", 3, Gamma::SettingType::integer));
          if (maxmods.value_int > N_SYSTEM_PAR - 7)
            maxmods.value_int = N_SYSTEM_PAR - 7;
          uint16_t oldmax = system_parameter_values_[3];
          if (oldmax != maxmods.value_int) {
            PL_DBG << "max num of modules changed";
            hardware_changed = true;
            std::vector<Gamma::Setting> old_modules(N_SYSTEM_PAR - 7);
            for (int i=0; i< N_SYSTEM_PAR - 7; ++i)
              old_modules[i] = q.branches.get(Gamma::Setting("SLOT_WAVE", 7+i, Gamma::SettingType::integer));
            for (int i=0; i< N_SYSTEM_PAR - 7; ++i)
              q.branches.remove_a(Gamma::Setting("SLOT_WAVE", 7+i, Gamma::SettingType::integer));
            system_parameter_values_[3] = maxmods.value_int;
            write_sys("MAX_NUMBER_MODULES");
            for (int i=0; i<maxmods.value_int; ++i) {
              Gamma::Setting slot = old_modules[i];
              if (slot == Gamma::Setting())
                slot = Gamma::Setting("SLOT_WAVE", 7+i, Gamma::SettingType::integer);
              slot.maximum = maxmods.value_int;
              slot.writable = true;
              q.branches.add(slot);
            }
          }
          uint16_t total = 0;
          for (int i=0; i<maxmods.value_int; ++i) {
            Gamma::Setting modslot = q.branches.get(Gamma::Setting("SLOT_WAVE", 7+i, Gamma::SettingType::integer));
            PL_DBG << "iterating slot " << i << " as " << modslot.value_int;
            if ((modslot != Gamma::Setting()) && (system_parameter_values_[7+i] != modslot.value_int)) {
              hardware_changed = true;
            }
            if (modslot.value_int != 0)
              total++;
          }
          PL_DBG << "total slots = " << total;
          if (system_parameter_values_[0] != total) {
            PL_DBG << "total modules in slots changed";
            system_parameter_values_[0] = total;
            write_sys("NUMBER_MODULES");
            hardware_changed = true;
          }
          if (hardware_changed) {
            PL_DBG << "slot config changed";
            for (int i=0; i<maxmods.value_int; ++i) {
              Gamma::Setting modslot = q.branches.get(Gamma::Setting("SLOT_WAVE", 7+i, Gamma::SettingType::integer));
              if ((modslot != Gamma::Setting()) && (system_parameter_values_[7+i] != modslot.value_int)) {
                PL_DBG << "changing slot " << i << " to " << modslot.value_int;
                system_parameter_values_[7+i] = modslot.value_int;
              }
            }
            write_sys("SLOT_WAVE");
          }

          for (auto &k : q.branches.my_data_) {

            if (k.setting_type == Gamma::SettingType::stem) {
              uint16_t modnum = k.address;
              for (auto &p : k.branches.my_data_) {
                if (p.setting_type == Gamma::SettingType::stem) {
                  uint16_t channum = p.address;
                  for (auto &o : p.branches.my_data_) {
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
        }
      }
    }
  }
  return true;
}

bool Settings::write_detector(const Gamma::Setting &set) {
  if (set.setting_type != Gamma::SettingType::detector)
    return false;

  if ((set.index < 0) || (set.index >= detectors_.size()))
    return false;

  if (detectors_[set.index].name_ != set.value_text)
    detectors_[set.index] = Gamma::Detector(set.value_text);

  return true;
}

bool Settings::boot() {
  if (live_ == LiveStatus::history)
    return false;

  S32 retval;
  set_sys("OFFLINE_ANALYSIS", 0);  //attempt live boot first
  set_sys("AUTO_PROCESSLMDATA", 0);  //do not write XIA datafile

  bool valid_files = true;
  for (int i=0; i < 7; i++) {
    memcpy(Boot_File_Name_List[i], (S8*)boot_files_[i].c_str(), boot_files_[i].size()); //+0?
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
    read_settings_bulk(); //really?
    return true;
  }

  boot_err(retval);
  set_sys("OFFLINE_ANALYSIS", 1);  //else attempt offline boot
  retval = Pixie_Boot_System(0x1F);
  if (retval >= 0) {
    live_ = LiveStatus::offline;
    read_settings_bulk(); //really?
    return true;
  } else {
    boot_err(retval);
    return false;
  }
}

void Settings::set_detector(int ch, Gamma::Detector det) {
  if (ch < 0 || ch >= detectors_.size())
    return;
  PL_DBG << "px_settings changing detector " << ch << " to " << det.name_;
  detectors_[ch] = det;

  for (auto &set : settings_tree_.branches.my_data_) {
    if (set.name == "Detectors") {
      if (detectors_.size() == set.branches.size()) {
        PL_DBG << "inside loop";
        for (auto &q : set.branches.my_data_) {
          if (q.index == ch) {
            PL_DBG << "change and load optimization for " << q.index << " from " << q.value_text << " to " << detectors_[q.index].name_;
            q.value_text = detectors_[q.index].name_;
            load_optimization(Channel(q.index));
          }
        }
      }
    }
  }
}

void Settings::save_optimization(Channel chan) {
  int start, stop;
  if (chan == Channel::all) {
    start = 0; stop = detectors_.size() - 1;
  } else if (static_cast<int>(chan) < 0)
    return;
  else if (static_cast<int>(chan) < detectors_.size()){
    start = stop = static_cast<int>(chan);
  }

  //current module only

  for (int i = start; i <= stop; i++) {
    PL_DBG << "Saving optimization channel " << i << " settings for " << detectors_[i].name_;
    detectors_[i].settings_ = Gamma::Setting();
    save_det_settings(detectors_[i].settings_, settings_tree_, i);
    if (detectors_[i].settings_.branches.size() > 0) {
      detectors_[i].settings_.setting_type = Gamma::SettingType::stem;
      detectors_[i].settings_.name = "Optimization";
    } else {
      detectors_[i].settings_.setting_type = Gamma::SettingType::none;
      detectors_[i].settings_.name.clear();
    }


  }
}

void Settings::load_optimization(Channel chan) {
  PL_INFO << "load optimizations " << (int)chan;

  int start, stop;
  if (chan == Channel::all) {
    start = 0; stop = detectors_.size() - 1;
  } else if (static_cast<int>(chan) < 0)
    return;
  else if (static_cast<int>(chan) < detectors_.size()){
    start = stop = static_cast<int>(chan);
  }

  //current module only

  for (int i = start; i <= stop; i++) {
    PL_DBG << "loading optimization for " << detectors_[i].name_;

    if (detectors_[i].settings_.setting_type == Gamma::SettingType::stem) {
      PL_DBG << "really loading optimization for " << detectors_[i].name_;
      for (auto &q : detectors_[i].settings_.branches.my_data_)
        load_det_settings(q, settings_tree_, i);
    }
  }
}

void Settings::save_det_settings(Gamma::Setting& result, const Gamma::Setting& root, int index) const {
  std::string stem = result.name;
  if (root.setting_type == Gamma::SettingType::stem) {
    result.name = stem;
    if (!stem.empty())
      result.name += "/" ;
    result.name += root.name;
    for (auto &q : root.branches.my_data_)
      save_det_settings(result, q, index);
    result.name = stem;
    //PL_DBG << "maybe created stem " << stem << "/" << root.name;
  } else if ((root.index == index) && (root.setting_type != Gamma::SettingType::detector)) {
    Gamma::Setting set(root);
    set.name = stem + "/" + root.name;
    set.index = 0;
    result.branches.add(set);
    //PL_DBG << "saved setting " << stem << "/" << root.name;
  }
}

void Settings::load_det_settings(Gamma::Setting det, Gamma::Setting& root, int index) {
  if ((root.setting_type == Gamma::SettingType::none) || det.name.empty())
    return;
  if (root.setting_type == Gamma::SettingType::stem) {
    //PL_DBG << "comparing stem for " << det.name << " vs " << root.name;
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, det.name, boost::algorithm::is_any_of("/"));
    if ((tokens.size() > 1) && (root.name == tokens[0])) {
      Gamma::Setting truncated = det;
      truncated.name.clear();
      for (int i=1; i < tokens.size(); ++i) {
        truncated.name += tokens[i];
        if ((i+1) < tokens.size())
          truncated.name += "/";
      }
      //PL_DBG << "looking for " << truncated.name << " in " << root.name;
      for (auto &q : root.branches.my_data_)
        load_det_settings(truncated, q, index);
    }
  } else if ((det.name == root.name) && (root.index == index)) {
    //PL_DBG << "applying " << det.name;
    root.value = det.value;
    root.value_int = det.value_int;
    root.value_text = det.value_text;
  }
}

void Settings::set_setting(Gamma::Setting address, int index) {
  if ((index < 0) || (index >= detectors_.size()))
    return;
  load_det_settings(address, settings_tree_, index);
  write_settings_bulk();
  read_settings_bulk();
}

Gamma::Setting Settings::get_setting(Gamma::Setting address, int index) const {
  if ((index < 0) || (index >= detectors_.size()))
    return Gamma::Setting();
  Gamma::Setting addy = address;
  save_det_settings(addy, settings_tree_, index);
  if (addy != address)
    return addy;
  else
    return Gamma::Setting();
}

////////////////////////////////////////
///////////////Settings/////////////////
////////////////////////////////////////

void Settings::get_all_settings() {
  get_sys_all();
  get_mod_all(Module::all); //might want this for more modules
  for (int i=0; i < num_chans_; i++) {
    get_chan_all(Channel(i), Module::all);
  }
  read_settings_bulk();
}


void Settings::reset_counters_next_run() { //for current module only
  if (live_ == LiveStatus::history)
    return;

  set_mod("SYNCH_WAIT", 1, Module(0));
  set_mod("IN_SYNCH", 0, Module(0));
}


/////System Settings//////
//////////////////////////

void Settings::set_sys(const std::string& setting, double val) {
  if (live_ == LiveStatus::history)
    return;

  PL_DBG << "Setting " << setting << " to " << val << " for system";

  //check bounds
  system_parameter_values_[i_sys(setting.c_str())] = val;
  write_sys(setting.c_str());
}


double Settings::get_sys(const std::string& setting) {
  PL_TRC << "Getting " << setting << " for system";

  //check bounds
  read_sys(setting.c_str());
  return system_parameter_values_[i_sys(setting.c_str())];
}

void Settings::get_sys_all() {
  if (live_ == LiveStatus::history)
    return;

  PL_TRC << "Getting all system settings";
  read_sys("ALL_SYSTEM_PARAMETERS");
}


//////Module Settings//////
///////////////////////////

void Settings::set_mod(const std::string& setting, double val, Module mod) {
  if (live_ == LiveStatus::history)
    return;

    int module = static_cast<int>(mod);
    if (module > -1) {
      PL_INFO << "Setting " << setting << " to " << val << " for module " << module;
      module_parameter_values_[module * N_MODULE_PAR + i_mod(setting.c_str())] = val;
      write_mod(setting.c_str(), module);
    }
}

double Settings::get_mod(const std::string& setting,
                         Module mod) const {
    int module = static_cast<int>(mod);
    if (module > -1) {
      PL_TRC << "Getting " << setting << " for module " << module;
      return module_parameter_values_[module * N_MODULE_PAR + i_mod(setting.c_str())];
    }
    else
      return -1;
}

double Settings::get_mod(const std::string& setting,
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

void Settings::get_mod_all(Module mod) {
  if (live_ == LiveStatus::history)
    return;

  switch (mod) {
  case Module::all: {
    /*loop through all*/
    break;
  }
  default:
    int module = static_cast<int>(mod);
    if (module > -1) {
      PL_TRC << "Getting all parameters for module " << module;
      read_mod("ALL_MODULE_PARAMETERS", module);
    }
  }
}

void Settings::get_mod_stats(Module mod) {
  if (live_ == LiveStatus::history)
    return;

  switch (mod) {
  case Module::all: {
    /*loop through all*/
    break;
  }
  default:
    int module = static_cast<int>(mod);
    if (module > -1) {
      PL_DBG << "Getting run statistics for module " << module;
      read_mod("MODULE_RUN_STATISTICS", module);
    }
  }
}


////////Channels////////////
////////////////////////////

void Settings::set_chan(const std::string& setting, double val,
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

void Settings::set_chan(uint8_t setting, double val,
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

double Settings::get_chan(uint8_t setting, Channel channel, Module module) const {
  S8* setting_name = Channel_Parameter_Names[setting];

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_TRC << "Getting " << setting_name
         << " for module " << mod << " channel " << chan;

  if ((mod > -1) && (chan > -1)) {
    return channel_parameter_values_[setting + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR];
  } else
    return -1;
}

double Settings::get_chan(uint8_t setting, Channel channel,
                          Module module, LiveStatus force) {
  S8* setting_name = Channel_Parameter_Names[setting];

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_TRC << "Getting " << setting_name
         << " for module " << mod << " channel " << chan;

  if ((mod > -1) && (chan > -1)) {
    if ((force == LiveStatus::online) && (live_ == force))
      read_chan((char*)setting_name, mod, chan);
    return channel_parameter_values_[setting + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR];
  } else
    return -1;
}

double Settings::get_chan(const std::string& setting,
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

double Settings::get_chan(const std::string& setting,
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

void Settings::get_chan_all(Channel channel, Module module) {
  if (live_ == LiveStatus::history)
    return;

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_TRC << "Getting all parameters for module " << mod << " channel " << chan;

  if ((mod > -1) && (chan > -1))
    read_chan("ALL_CHANNEL_PARAMETERS", mod, chan);
}

void Settings::get_chan_stats(Module module) {
  if (live_ == LiveStatus::history)
    return;

  int mod = static_cast<int>(module);

  PL_TRC << "Getting channel run statistics for module " << mod;

  if (mod > -1)
    read_chan("CHANNEL_RUN_STATISTICS", mod, 0);
}

uint16_t Settings::i_sys(const char* setting) const {
  return Find_Xact_Match((S8*)setting, System_Parameter_Names, N_SYSTEM_PAR);
}

uint16_t Settings::i_mod(const char* setting) const {
  return Find_Xact_Match((S8*)setting, Module_Parameter_Names, N_MODULE_PAR);
}

uint16_t Settings::i_chan(const char* setting) const {
  return Find_Xact_Match((S8*)setting, Channel_Parameter_Names, N_CHANNEL_PAR);
}

bool Settings::write_sys(const char* setting) {
  S8 sysstr[8] = "SYSTEM\0";
  S32 ret = Pixie_User_Par_IO(system_parameter_values_.data(),
                              (S8*) setting, sysstr, MOD_WRITE, 0, 0);
  set_err(ret);
  if (ret == 0)
    return true;
  return false;
}

bool Settings::read_sys(const char* setting) {
  S8 sysstr[8] = "SYSTEM\0";
  S32 ret = Pixie_User_Par_IO(system_parameter_values_.data(),
                              (S8*) setting, sysstr, MOD_READ, 0, 0);
  set_err(ret);
  if (ret == 0)
    return true;
  return false;
}

bool Settings::write_mod(const char* setting, uint8_t mod) {
  S32 ret = -42;
  S8 modstr[8] = "MODULE\0";
  if (live_ == LiveStatus::online)
    ret = Pixie_User_Par_IO(module_parameter_values_.data(),
                            (S8*) setting, modstr, MOD_WRITE, mod, 0);
  set_err(ret);
  return (ret == 0);
}

bool Settings::read_mod(const char* setting, uint8_t mod) {
  S32 ret = -42;
  S8 modstr[8] = "MODULE\0";
  if (live_ == LiveStatus::online)
    ret = Pixie_User_Par_IO(module_parameter_values_.data(),
                            (S8*) setting, modstr, MOD_READ, mod, 0);
  set_err(ret);
  return (ret == 0);
}

bool Settings::write_chan(const char* setting, uint8_t mod, uint8_t chan) {
  S32 ret = -42;
  S8 chnstr[9] = "CHANNEL\0";
  if (live_ == LiveStatus::online)
    ret = Pixie_User_Par_IO(channel_parameter_values_.data(),
                            (S8*) setting, chnstr, MOD_WRITE, mod, chan);
  set_err(ret);
  return (ret == 0);
}

bool Settings::read_chan(const char* setting, uint8_t mod, uint8_t chan) {
  S32 ret = -42;
  S8 chnstr[9] = "CHANNEL\0";
  if (live_ == LiveStatus::online)
    ret = Pixie_User_Par_IO(channel_parameter_values_.data(),
                            (S8*) setting, chnstr, MOD_READ, mod, chan);
  set_err(ret);
  return (ret == 0);
}

void Settings::set_err(int32_t err_val) {
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

void Settings::boot_err(int32_t err_val) {
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

void Settings::to_xml(tinyxml2::XMLPrinter& printer) const {
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
    for (int j=0; j<num_chans_; j++) {
      printer.OpenElement("Channel");
      printer.PushAttribute("number", std::to_string(j).c_str());
      detectors_[j].to_xml(printer);
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
  printer.CloseElement(); //Settings
}

void Settings::from_xml(tinyxml2::XMLElement* root) {
  live_ = LiveStatus::history;
  detectors_.resize(num_chans_); //hardcoded

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
              detectors_[thisChan].from_xml(ChanElement);
            } else if (std::string(ChanElement->Name()) == "Setting") {
              int thisKey =  boost::lexical_cast<short>(ChanElement->Attribute("key"));
              double thisVal = boost::lexical_cast<double>(ChanElement->Attribute("value"));
              std::string thisName = std::string(ChanElement->Attribute("name"));
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
}


}
