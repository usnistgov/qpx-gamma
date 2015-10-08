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
 *      Qpx::Engine online and offline setting describing the state of
 *      a device.
 *      Not thread safe, mostly for speed concerns (getting stats during run)
 *
 ******************************************************************************/

#ifndef QPX_SETTINGS
#define QPX_SETTINGS


#include <boost/thread.hpp>
#include <boost/atomic.hpp>

#include "detector.h"
#include "generic_setting.h"  //make full use of this!!!
#include "daq_device.h"
#include "synchronized_queue.h"

#include "simulator.h"
#include "sorter.h"
#include "custom_timer.h"

namespace Qpx {

struct Trace {
  int index;
  std::vector<uint16_t> data;
  double timescale;
  Gamma::Detector detector;
};


class Engine {
  
public:


  static Engine& getInstance()
  {
    static Engine singleton_instance;
    return singleton_instance;
  }

  ~Engine();

  void initialize(std::string settings_file);
  bool boot();
  bool die();
  DeviceStatus status() {return aggregate_status_;}

  ListData* getList(uint64_t timeout, boost::atomic<bool>& inturruptor);
  void getMca(uint64_t timeout, SpectraSet &spectra, boost::atomic<bool> &interruptor);
  void getFakeMca(Simulator &simulator, SpectraSet &spectra, uint64_t timeout, boost::atomic<bool> &interruptor);
  void simulateFromList(Sorter &sorter, SpectraSet &spectra, boost::atomic<bool> &interruptor);

  //detectors
  std::vector<Gamma::Detector> get_detectors() const {return detectors_;}
  void set_detector(int, Gamma::Detector);

  void save_optimization();
  void load_optimization();
  void load_optimization(int);

  void set_setting(Gamma::Setting address, Gamma::Match flags);

  /////SETTINGS/////
  Gamma::Setting pull_settings() const;
  void push_settings(const Gamma::Setting&);
  bool write_settings_bulk();
  bool read_settings_bulk();
  bool write_detector(const Gamma::Setting &set);
  
  void get_all_settings();
  
  bool execute_command();

  std::vector<Trace> oscilloscope();
  
  bool daq_start(uint64_t timeout, SynchronizedQueue<Spill*>* out_queue);
  bool daq_stop();
  bool daq_running();

protected:
  DeviceStatus aggregate_status_;

  std::map<std::string, DaqDevice*> devices_;  //use shared pointer instead

  Gamma::Setting settings_tree_;
  Gamma::SettingMeta total_det_num_;

  std::vector<Gamma::Detector> detectors_;

  //////////for internal use only///////////
  void save_det_settings(Gamma::Setting&, const Gamma::Setting&, Gamma::Match flags) const;
  void load_det_settings(Gamma::Setting, Gamma::Setting&, Gamma::Match flags);

private:
  mutable boost::mutex mutex_;
  std::string path_;

  //singleton assurance
  Engine();
  Engine(Engine const&);
  void operator=(Engine const&);

  //threads
  void worker_MCA(SynchronizedQueue<Spill*>* data_queue, SpectraSet* spectra);
  void worker_fake(Simulator* source, SynchronizedQueue<Spill*>* data_queue,
                    uint64_t timeout_limit, boost::atomic<bool>* interruptor);
  void worker_from_list(Sorter* sorter, SynchronizedQueue<Spill*>* data_queue, boost::atomic<bool>* interruptor);

};

}

#endif
