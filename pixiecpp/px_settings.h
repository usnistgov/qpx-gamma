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

const uint32_t max_buf_len  = 8192; //get this from module
const uint32_t list_mem_len32 = 13312;
const uint32_t list_mem_len16 = 2 * list_mem_len32;

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

  //save
  void to_xml(tinyxml2::XMLPrinter&) const;
  
  //detectors
  std::vector<Gamma::Detector> get_detectors() const {return detectors_;}
  Gamma::Detector get_detector(Channel ch = Channel::current) const;
  void set_detector(Channel ch, const Gamma::Detector& det);
  void set_detector_DB(XMLableDB<Gamma::Detector>);

  void save_optimization(Channel chan = Channel::all);  //specify module as well?
  void load_optimization(Channel chan = Channel::all);

  /////SETTINGS/////
  Setting pull_settings();
  void push_settings(const Setting&);
  
  void get_all_settings();

  bool write_settings_bulk();
  bool read_settings_bulk();

  bool read_setting_MADC(Setting &set);
  bool write_setting_MADC(const Setting &set);

  bool read_detector(Setting &set);
  bool write_detector(const Setting &set);

  double get_chan(const std::string&,
                  Channel channel = Channel::current,
                  Module  module = Module::current) const
	             {return 0;}
  
protected:
  void initialize(); //populate metadata
  bool boot();       //only wrapper can use this
  void from_xml(tinyxml2::XMLElement*);

  void reset();

  int         num_chans_;
  LiveStatus  live_;

  Setting settings_tree_;

  std::vector<Gamma::Detector> detectors_;
  XMLableDB<Gamma::Detector> detector_db_;

  struct usb_dev_handle *udev;       // Device Handle

};

}

#endif
