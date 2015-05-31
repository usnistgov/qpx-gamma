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

#ifndef PIXIE_SETTINGS
#define PIXIE_SETTINGS

#include "detector.h"
#include "generic_setting.h"  //make full use of this!!!
#include "tinyxml2.h"
#include "madc32.h"

namespace Pixie {

enum class LiveStatus : int {dead = 0, online = 1, offline = 2, history = 3};
enum class Module     : int {all = -4, current = -3, none = -2, invalid = -1};
enum class Channel    : int {all = -4, current = -3, none = -2, invalid = -1};

class Wrapper; //forward declared for friendship

class Settings {
  //Only wrapper can boot, inducing live_==online state, making it possible
  //to affect the device itself. Otherwise Settings is in dead or history state
  friend class Pixie::Wrapper;
  
public:

  Settings();
  Settings(const Settings& other);      //sets state to history if copied
  Settings(tinyxml2::XMLElement* root); //create from xml node

  LiveStatus live() {return live_;}

  //current module and channel
  Module current_module() const;
  void set_current_module(Module);   //!!!!rework in case of multiple modules
  Channel current_channel() const;
  void set_current_channel(Channel);

  //save
  void to_xml(tinyxml2::XMLPrinter&);
  
  //detectors
  std::vector<Detector> get_detectors() const {return detectors_;}
  Detector get_detector(Channel ch = Channel::current) const;
  void set_detector(Channel ch, const Detector& det);
  void save_optimization(Channel chan = Channel::all);  //specify module as well?
  void load_optimization(Channel chan = Channel::all);

  //metadata for display and editing
  std::vector<Setting> channel_meta() const {return chan_set_meta_;}

  uint8_t chan_param_num () const;  //non-empty parameters

  /////SETTINGS/////
  void get_all_settings();
  
  //system
  void set_sys(const std::string&, double);
  void set_slots(const std::vector<uint8_t>&);
  double get_sys(const std::string&);
  void get_sys_all();
  void set_boot_files(std::vector<std::string>&);

  //module
  void set_mod(const std::string&, double, Module mod = Module::current);
  double get_mod(const std::string&, Module mod = Module::current) const;
  double get_mod(const std::string&, Module mod = Module::current,
                 LiveStatus force = LiveStatus::offline);
  void get_mod_all(Module mod = Module::current);
  void get_mod_stats(Module mod = Module::current);

  //channel
  void set_chan(const std::string&, double val,
                Channel channel = Channel::current,
                Module  module  = Module::current);
  void set_chan(uint8_t setting, double val,
                Channel channel = Channel::current,
                Module  module  = Module::current);                
  double get_chan(const std::string&,
                  Channel channel = Channel::current,
                  Module  module = Module::current) const;
  double get_chan(const std::string&,
                  Channel channel = Channel::current,
                  Module  module = Module::current,
                  LiveStatus force = LiveStatus::offline);
  double get_chan(uint8_t setting,
                  Channel channel = Channel::current,
                  Module  module = Module::current) const;
  double get_chan(uint8_t setting,
                  Channel channel = Channel::current,
                  Module  module = Module::current,
                  LiveStatus force = LiveStatus::offline);
  void get_chan_all(Channel channel = Channel::current,
                    Module  module  = Module::current);
  void get_chan_stats(Module  module  = Module::current);

  void set_threshold(uint32_t nt);

  struct usb_device *dev;
  xxusb_device_type xxusbDev[32];
  struct usb_dev_handle *udev;       // Device Handle
  int DevFound;

  
protected:
  void initialize(); //populate metadata
  bool boot();       //only wrapper can use this
  void from_xml(tinyxml2::XMLElement*);

  void reset();

  int         num_chans_;
  LiveStatus  live_;
  Module      current_module_;
  Channel     current_channel_;

  uint32_t threshold_;

  std::vector<double> system_parameter_values_;
  std::vector<double> module_parameter_values_;
  std::vector<double> channel_parameter_values_;

  std::vector<Setting> sys_set_meta_;
  std::vector<Setting> mod_set_meta_;
  std::vector<Setting> chan_set_meta_;

  std::vector<Detector> detectors_;

  //////////for internal use only///////////
  //carry out task
  bool write_sys(int);
  bool write_mod(int, uint8_t);
  bool write_chan(int, uint8_t, uint8_t);
  bool read_sys(int);
  bool read_mod(int, uint8_t);
  bool read_chan(int, uint8_t, uint8_t);

  //find index
  int16_t i_sys(std::string) const;
  int16_t i_mod(std::string) const;
  int16_t i_chan(std::string) const;
};

}

#endif
