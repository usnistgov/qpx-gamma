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
 *      Qpx::Settings online and offline setting describing the state of
 *      a Pixie-4 device.
 *      Not thread safe, mostly for speed concerns (getting stats during run)
 *
 ******************************************************************************/

#ifndef QPX_SETTINGS
#define QPX_SETTINGS

#include "detector.h"
#include "generic_setting.h"  //make full use of this!!!
#include "tinyxml2.h"
#include "pixie_plugin.h"

namespace Qpx {

struct Trace {
  int index;
  std::vector<uint16_t> data;
  double timescale;
  Gamma::Detector detector;
};

class Wrapper; //forward declared for friendship

class Settings {
  //Only wrapper can boot, inducing live_==online state, making it possible
  //to affect the device itself. Otherwise Settings is in dead or history state
  friend class Qpx::Wrapper;
  
public:

  Settings();
  Settings(const Settings& other);                                              //sets state to history if copied
  Settings(tinyxml2::XMLElement* root, Gamma::Setting tree = Gamma::Setting()); //create from xml node

  bool boot();
  bool die();
  LiveStatus live() {return pixie_plugin_.live();}

  //save
  void to_xml(tinyxml2::XMLPrinter&) const;
  
  //detectors
  std::vector<Gamma::Detector> get_detectors() const {return detectors_;}
  void set_detector(int, Gamma::Detector);

  void save_optimization();
  void load_optimization();
  void load_optimization(int);

  void set_setting(Gamma::Setting address, bool precise_index = false);

  /////SETTINGS/////
  Gamma::Setting pull_settings() const;
  void push_settings(const Gamma::Setting&);
  bool write_settings_bulk();
  bool read_settings_bulk();
  bool write_detector(const Gamma::Setting &set);
  
  void get_all_settings();
  void reset_counters_next_run();
  
  bool execute_command();

  std::vector<Trace> oscilloscope();
  
protected:
  bool start_daq(uint64_t timeout, SynchronizedQueue<Spill*>* out_queue);
  bool stop_daq();

  void from_xml(tinyxml2::XMLElement*);

  Qpx::Plugin pixie_plugin_;
  Gamma::Setting settings_tree_;
  
  std::vector<Gamma::Detector> detectors_;

  //////////for internal use only///////////
  void save_det_settings(Gamma::Setting&, const Gamma::Setting&, bool precise_index = false) const;
  void load_det_settings(Gamma::Setting, Gamma::Setting&, bool precise_index = false);

};

}

#endif
