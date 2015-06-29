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
#include "common_xia.h"
#include "custom_logger.h"

namespace Pixie {

Settings::Settings() {
  num_chans_ = NUMBER_OF_CHANNELS;  //This might need to be changed

  boot_files_.resize(7);
  system_parameter_values_.resize(N_SYSTEM_PAR, 0.0);
  module_parameter_values_.resize(PRESET_MAX_MODULES*N_MODULE_PAR, 0.0);
  channel_parameter_values_.resize(PRESET_MAX_MODULES*N_CHANNEL_PAR*NUMBER_OF_CHANNELS, 0.0);

  sys_set_meta_.resize(N_SYSTEM_PAR);
  for (int i=0; i < 4; i++)
    sys_set_meta_[i].writable = true;
  for (int i=6; i < N_SYSTEM_PAR; i++)
    sys_set_meta_[i].writable = true;

  mod_set_meta_.resize(N_MODULE_PAR);
  for (int i=1; i < 7; i++)
    mod_set_meta_[i].writable = true;
  for (int i=8; i < 16; i++)
    mod_set_meta_[i].writable = true;
  for (int i=30; i < 34; i++)
    mod_set_meta_[i].writable = true;

  chan_set_meta_.resize(N_CHANNEL_PAR);
  for (int i = 0; i < 25; i++)
    chan_set_meta_[i].writable = true;
  chan_set_meta_[41].writable = true;
  chan_set_meta_[42].writable = true; 
  
  current_module_ = Module::none;
  current_channel_ = Channel::none;
  live_ = LiveStatus::dead;
  detectors_.resize(num_chans_);

  current_module_ = Module(0);
  current_channel_ = Channel(0);
}

Settings::Settings(const Settings& other):
    live_(LiveStatus::history), //this is why we have this function, deactivate if copied
    num_chans_(other.num_chans_), detectors_(other.detectors_),
    current_module_(other.current_module_), current_channel_(other.current_channel_),
    boot_files_(other.boot_files_),
    system_parameter_values_(other.system_parameter_values_),
    module_parameter_values_(other.module_parameter_values_),
    channel_parameter_values_(other.channel_parameter_values_),
    sys_set_meta_(other.sys_set_meta_),
    mod_set_meta_(other.mod_set_meta_),
    chan_set_meta_(other.chan_set_meta_) {}

Settings::Settings(tinyxml2::XMLElement* root):
    Settings()
{
  from_xml(root);
}

bool Settings::boot() {
  if (live_ == LiveStatus::history)
    return false;

  S32 retval;
  int offline = get_sys("OFFLINE_ANALYSIS");

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

  if (!offline) {
    int max = get_sys("NUMBER_MODULES");
    if (!max) {
      PL_ERR << "No valid module slots. Boot aborting.";
      return false;
    } else {
      PL_DBG << "Number of modules to boot: " << max;
      read_sys("SLOT_WAVE");
      for (int i=0; i < max; i++)
        PL_DBG << " module " << i << " in slot "
               << system_parameter_values_[i_sys("SLOT_WAVE") + i];
    }
  }

  set_sys("AUTO_PROCESSLMDATA", 0);  //do not write XIA datafile

  retval = Pixie_Boot_System(0x1F);
  //bad files do not result in boot error!!
  if (retval >= 0) {
    if (offline)
      live_ = LiveStatus::offline;
    else 
      live_ = LiveStatus::online;
    initialize();
    return true;
  } else {
    boot_err(retval);
    return false;
  }
}

Module Settings::current_module() const {
  return current_module_;
}

void Settings::set_current_module(Module mod) {
  current_module_ = mod;
}

Channel Settings::current_channel() const {
  return current_channel_;
}

void Settings::set_current_channel(Channel chan) {
  current_channel_ = chan;
}

Gamma::Detector Settings::get_detector(Channel ch) const {
  if (ch == Channel::current)
    return detectors_[static_cast<int>(current_channel_)];
  else if ((static_cast<int>(ch) > -1) && (static_cast<int>(ch) < num_chans_))
    return detectors_[static_cast<int>(ch)];
  else
    return Gamma::Detector();
}

void Settings::set_detector(Channel ch, const Gamma::Detector& det) {
  if (ch == Channel::current)
    ch = current_channel_;
  if ((static_cast<int>(ch) > -1) && (static_cast<int>(ch) < num_chans_)) {
    PL_INFO << "Setting detector [" << det.name_ << "] for channel " << static_cast<int>(ch);
    detectors_[static_cast<int>(ch)] = det;
  }

  //set for all?
}

void Settings::save_optimization(Channel chan) {
  int start, stop;
  if (chan == Channel::all) {
    start = 0; stop = num_chans_;
  } else if (static_cast<int>(chan) < 0)
    return;
  else if (static_cast<int>(chan) < num_chans_){
    start = stop = static_cast<int>(chan);
  }

  //current module only

  for (int i = start; i < stop; i++) {
    PL_DBG << "Saving optimization channel " << i << " settings for " << detectors_[i].name_;
    detectors_[i].setting_names_.resize(43);
    detectors_[i].setting_values_.resize(43);
    for (int j = 0; j < 43; j++) {
      if (chan_set_meta_[j].writable) {
        detectors_[i].setting_names_[j] = chan_set_meta_[j].name;
        detectors_[i].setting_values_[j] = get_chan(j, Channel(i), Module::current);
      }
    }
  }
}

void Settings::load_optimization(Channel chan) {
  int start, stop;
  if (chan == Channel::all) {
    start = 0; stop = num_chans_;
  } else if (static_cast<int>(chan) < 0)
    return;
  else if (static_cast<int>(chan) < num_chans_){
    start = stop = static_cast<int>(chan);
  }

  //current module only
  
  for (int i = start; i < stop; i++) {
    if (detectors_[i].setting_values_.size()) {
      PL_DBG << "Optimizing channel " << i << " settings for " << detectors_[i].name_;
      for (std::size_t j = 0; j < detectors_[i].setting_values_.size(); j++) {
        if (chan_set_meta_[j].writable)
          set_chan(j, detectors_[i].setting_values_[j], Channel(i), Module::current);
      }
    }
  }
}

uint8_t Settings::chan_param_num() const
{
  if (live_ == LiveStatus::dead)
    return 0;
  else
    return 43; //hardcoded for pixie-4
}

////////////////////////////////////////
///////////////Settings/////////////////
////////////////////////////////////////

void Settings::get_all_settings() {
  get_sys_all();
  get_mod_all(); //might want this for more modules
  for (int i=0; i < num_chans_; i++) {
    get_chan_all(Channel(i));
  }
};


void Settings::reset_counters_next_run() { //for current module only
  if (live_ == LiveStatus::history)
    return;

  set_mod("SYNCH_WAIT", 1, Module::current);
  set_mod("IN_SYNCH", 0, Module::current);
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

void Settings::set_slots(const std::vector<uint8_t>& slots) {
  if (live_ == LiveStatus::history)
    return;

  int total = 0;
  std::size_t max = N_SYSTEM_PAR - i_sys("SLOT_WAVE");
  for (std::size_t i=0; i < max; i++) {
    if ((i < slots.size()) && (slots[i])) {
      system_parameter_values_[i_sys("SLOT_WAVE") + i] = static_cast<double>(slots[i]);
      total++;
    } else
      system_parameter_values_[i_sys("SLOT_WAVE") + i] = 0; 
  }

  set_sys("NUMBER_MODULES", total);
  write_sys("SLOT_WAVE");
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

void Settings::set_boot_files(std::vector<std::string>& files) {
  if (live_ == LiveStatus::history)
    return;
  
  if (files.size() != 7)
    PL_WARN << "Bad boot file array provided";
  else
    boot_files_ = files;
}


//////Module Settings//////
///////////////////////////

void Settings::set_mod(const std::string& setting, double val, Module mod) {
  if (live_ == LiveStatus::history)
    return;

  switch (mod) {
    case Module::current:
      mod = current_module_;
    default:
      int module = static_cast<int>(mod);
      if (module > -1) {
        PL_INFO << "Setting " << setting << " to " << val << " for module " << module;
        module_parameter_values_[module * N_MODULE_PAR + i_mod(setting.c_str())] = val;
        write_mod(setting.c_str(), module);
      }
  }
}

double Settings::get_mod(const std::string& setting,
                         Module mod) const {
  switch (mod) {
    case Module::current:
      mod = current_module_;
    default:
      int module = static_cast<int>(mod);
      if (module > -1) {
        PL_TRC << "Getting " << setting << " for module " << module;
        return module_parameter_values_[module * N_MODULE_PAR + i_mod(setting.c_str())];
      }
      else
        return -1;
  }
}

double Settings::get_mod(const std::string& setting,
                         Module mod,
                         LiveStatus force) {
  switch (mod) {
    case Module::current:
      mod = current_module_;
    default:
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
}

void Settings::get_mod_all(Module mod) {
  if (live_ == LiveStatus::history)
    return;
  
  switch (mod) {
    case Module::all: {
      /*loop through all*/
      break;
    }
    case Module::current:
      mod = current_module_;
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
    case Module::current:
      mod = current_module_;
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
  
  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

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
  
  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

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

  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

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

  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

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
  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

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
  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

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
  
  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_TRC << "Getting all parameters for module " << mod << " channel " << chan;
  
  if ((mod > -1) && (chan > -1))
    read_chan("ALL_CHANNEL_PARAMETERS", mod, chan);
}

void Settings::get_chan_stats(Module module) {
  if (live_ == LiveStatus::history)
    return;
  
  if (module == Module::current)
    module = current_module_;

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
      PL_ERR << "Set/get parameter failed: Pixie not online";
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

void Settings::initialize() {
  for (int i=0; i < N_SYSTEM_PAR; i++)
    if (System_Parameter_Names[i] != nullptr)
      sys_set_meta_[i].name = std::string((char*)System_Parameter_Names[i]);

  for (int i=0; i < N_MODULE_PAR; i++)
    if (Module_Parameter_Names[i] != nullptr)
      mod_set_meta_[i].name = std::string((char*)Module_Parameter_Names[i]);
  
  for (int i=0; i < N_CHANNEL_PAR; i++)
    if (Channel_Parameter_Names[i] != nullptr)
      chan_set_meta_[i].name = std::string((char*)Channel_Parameter_Names[i]);

  mod_set_meta_[i_mod("ACTUAL_COINCIDENCE_WAIT")].unit = "ns";
  mod_set_meta_[i_mod("MIN_COINCIDENCE_WAIT")].unit = "ticks";
  mod_set_meta_[i_mod("RUN_TIME")].unit = "s";
  mod_set_meta_[i_mod("EVENT_RATE")].unit = "cps";
  mod_set_meta_[i_mod("TOTAL_TIME")].unit = "s";
  
  chan_set_meta_[i_chan("CHANNEL_CSRA")].unit = "binary";
  chan_set_meta_[i_chan("CHANNEL_CSRB")].unit = "binary";
  chan_set_meta_[i_chan("CHANNEL_CSRC")].unit = "binary";

  chan_set_meta_[i_chan("INPUT_COUNT_RATE")].unit = "cps";
  chan_set_meta_[i_chan("OUTPUT_COUNT_RATE")].unit = "cps";
  chan_set_meta_[i_chan("GATE_RATE")].unit = "cps";
  chan_set_meta_[i_chan("CURRENT_ICR")].unit = "cps";

  chan_set_meta_[i_chan("LIVE_TIME")].unit = "s";
  chan_set_meta_[i_chan("FTDT")].unit = "s";
  chan_set_meta_[i_chan("SFDT")].unit = "s";
  chan_set_meta_[i_chan("GDT")].unit = "s";

  //microseconds
  chan_set_meta_[i_chan("ENERGY_RISETIME")].unit = "\u03BCs";
  chan_set_meta_[i_chan("ENERGY_FLATTOP")].unit = "\u03BCs";
  chan_set_meta_[i_chan("TRIGGER_RISETIME")].unit = "\u03BCs";
  chan_set_meta_[i_chan("TRIGGER_FLATTOP")].unit = "\u03BCs";
  chan_set_meta_[i_chan("TRACE_LENGTH")].unit = "\u03BCs";
  chan_set_meta_[i_chan("TRACE_DELAY")].unit = "\u03BCs";
  chan_set_meta_[i_chan("COINC_DELAY")].unit = "\u03BCs";
  chan_set_meta_[i_chan("PSA_START")].unit = "\u03BCs";
  chan_set_meta_[i_chan("PSA_END")].unit = "\u03BCs";
  chan_set_meta_[i_chan("TAU")].unit = "\u03BCs";
  chan_set_meta_[i_chan("XDT")].unit = "\u03BCs";
  chan_set_meta_[i_chan("GATE_WINDOW")].unit = "\u03BCs";
  chan_set_meta_[i_chan("GATE_DELAY")].unit = "\u03BCs";

  chan_set_meta_[i_chan("PSM_TEMP_AVG")].unit = "\u00B0C"; //degrees C
  chan_set_meta_[i_chan("VGAIN")].unit = "V/V";
  chan_set_meta_[i_chan("VOFFSET")].unit = "V";
  chan_set_meta_[i_chan("CURRENT_OORF")].unit = "%";

}

void Settings::to_xml(tinyxml2::XMLPrinter& printer) {
  printer.OpenElement("PixieSettings");
  printer.OpenElement("System");
  for (int i=0; i < N_SYSTEM_PAR; i++) {
    if (!std::string((char*)System_Parameter_Names[i]).empty()) {  //use metadata structure!!!!
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
                                std::to_string(get_chan(k,Channel(j))).c_str());
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
            if (std::string(ChanElement->Name()) == "Gamma::Detector") {
              detectors_[thisChan].from_xml(ChanElement);
            } else if (std::string(ChanElement->Name()) == "Setting") {
              int thisKey =  boost::lexical_cast<short>(ChanElement->Attribute("key"));
              double thisVal = boost::lexical_cast<double>(ChanElement->Attribute("value"));
              std::string thisName = std::string(ChanElement->Attribute("name"));
              channel_parameter_values_[thisModule * N_CHANNEL_PAR * NUMBER_OF_CHANNELS
                                     + thisChan * N_CHANNEL_PAR + thisKey] = thisVal;
              chan_set_meta_[thisKey].name = thisName;
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
