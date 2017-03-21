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

#include <boost/thread.hpp>
#include <boost/atomic.hpp>

#include "detector.h"
#include "setting.h"
#include "producer.h"
#include "synchronized_queue.h"
#include "project.h"

#include "custom_timer.h"

namespace Qpx {

class Engine {
  
public:

  static Engine& getInstance()
  {
    static Engine singleton_instance;
    return singleton_instance;
  }

  void initialize(std::string profile_path, std::string settings_path);
  bool boot();
  bool die();
  ProducerStatus status() {return aggregate_status_;}

  ListData getList(uint64_t timeout, boost::atomic<bool>& inturruptor);
  void getMca(uint64_t timeout, ProjectPtr spectra, boost::atomic<bool> &interruptor);

  //detectors
  std::vector<Qpx::Detector> get_detectors() const {return detectors_;}
  void set_detector(size_t, Qpx::Detector);

  void load_optimization();

  void set_setting(Qpx::Setting address, Qpx::Match flags);

  /////SETTINGS/////
  Qpx::Setting pull_settings() const;
  void push_settings(const Qpx::Setting&);
  bool write_settings_bulk();
  bool read_settings_bulk(); 
  void get_all_settings();

  std::vector<Hit> oscilloscope();
  
  bool daq_start(SynchronizedQueue<Spill*>* out_queue);
  bool daq_stop();
  bool daq_running();

  static int print_version();
  static std::string version();

protected:
  std::string profile_path_;
  ProducerStatus aggregate_status_, intrinsic_status_;
  mutable boost::mutex mutex_;

  std::map<std::string, SourcePtr> devices_;

  Qpx::Setting settings_tree_;
  Qpx::SettingMeta total_det_num_, single_det_;

  std::vector<Qpx::Detector> detectors_;

  void save_det_settings(Qpx::Setting&, const Qpx::Setting&, Qpx::Match flags) const;
  void load_det_settings(Qpx::Setting, Qpx::Setting&, Qpx::Match flags);
  void rebuild_structure(Qpx::Setting &set);

  //threads
  void worker_MCA(SynchronizedQueue<Spill*>* data_queue, ProjectPtr spectra);

private:

  //singleton assurance
  Engine();
  Engine(Engine const&);
  void operator=(Engine const&);
  ~Engine();

  void save_optimization();
  void load_optimization(size_t);

};

}
