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
 *      Qpx::Simulator2D
 *
 ******************************************************************************/

#ifndef SIMULATOR2D_PLUGIN
#define SIMULATOR2D_PLUGIN

#include "daq_device.h"
#include "detector.h"
#include <boost/thread.hpp>
#include <boost/atomic.hpp>
#include <unordered_map>
#include <boost/random/discrete_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include "spectrum.h"


namespace Qpx {

class Simulator2D : public DaqDevice {
  
public:

  Simulator2D();
//  Simulator2D(std::string file);
  ~Simulator2D();

  static std::string plugin_name() {return "Simulator2D";}
  std::string device_name() const override {return plugin_name();}

  bool write_settings_bulk(Qpx::Setting &set) override;
  bool read_settings_bulk(Qpx::Setting &set) const override;
  void get_all_settings() override;
  bool boot() override;
  bool die() override;

  bool daq_start(SynchronizedQueue<Spill*>* out_queue) override;
  bool daq_stop() override;
  bool daq_running() override;

private:
  //no copying
  void operator=(Simulator2D const&);
  Simulator2D(const Simulator2D&);

  //Acquisition threads, use as static functors
  static void worker_run(Simulator2D* callback, SynchronizedQueue<Spill*>* spill_queue);

protected:

  std::string setting_definitions_file_;

  boost::atomic<int> run_status_;
  boost::thread *runner_;

  std::string source_file_;
  std::string source_spectrum_;
  int     bits_;
  int     spill_interval_;
  double  scale_rate_;
  int     chan0_;
  int     chan1_;
  int     coinc_thresh_;

  std::vector<std::string> spectra_;

  Spill get_spill();

  double gain0_, gain1_;


  boost::random::discrete_distribution<> dist_;
  boost::random::discrete_distribution<> refined_dist_;
  boost::random::mt19937 gen;

  std::array<int,2> channels_;
  uint16_t shift_by_;
  uint64_t resolution_;
  PreciseFloat count_;
  bool valid_;

  uint64_t OCR;
  Qpx::Setting settings;
  std::vector<Qpx::Detector> detectors;
  double lab_time, time_factor;

  StatsUpdate getBlock(double duration);
  bool valid() {return valid_;}

  Hit model_hit;




};

}

#endif
