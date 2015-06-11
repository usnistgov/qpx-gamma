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
 *      Pixie::Wrapper singleton for controlling a Pixie-4 device.
 *      Mutex locking on all public functions. Threads in private static fns.
 *      Use only double-buffer mode acquisition for reliable timing.
 *      Might also use lock file to ensure only one instance per system?
 *
 ******************************************************************************/

#include <boost/thread.hpp>
#include <boost/atomic.hpp>

#include "synchronized_queue.h"
#include "custom_timer.h"

#include "simulator.h"

#ifndef PIXIE_WRAPPER
#define PIXIE_WRAPPER

namespace Pixie {

  const double poll_interval_default = 100; //in millisecs

  enum class RunType : uint16_t {traces = 0x100, full = 0x101, psa_only = 0x102, compressed = 0x103};

class Wrapper {
 public:
  static Wrapper& getInstance()
  {
    static Wrapper singleton_instance;
    return singleton_instance;
  }

  //get a live reference to the pixie's status
  //mutex locks, but underlying class not thread-safe
  Settings& settings();

  //boot device(s)
  bool boot();
  void die();

  ////THE MEAT: ACQUISITION////
  Hit  getOscil ();
  
  ListData* getList(RunType type, uint64_t timeout,
                    boost::atomic<bool>& inturruptor,
                    bool double_buffer);
  
  void getMca(RunType type, uint64_t timeout,
              SpectraSet &spectra, boost::atomic<bool> &interruptor,
              bool double_buffer);
  
  void getFakeMca(std::string source, SpectraSet &spectra,
                  uint64_t timeout, boost::atomic<bool> &interruptor);

  /////CONTROL TASKS/////
  void control_measure_baselines(uint8_t module);
  void control_compute_BLcut();
  void control_find_tau();
  void control_adjust_offsets();

 private:
  Settings  my_settings_;
  int poll_interval_ms_;  //allow this to change
  mutable boost::mutex mutex_;

  //singleton assurance
  Wrapper(): poll_interval_ms_(poll_interval_default) {}
  Wrapper(Wrapper const&);
  void operator=(Wrapper const&);

  //threads
  void worker_MCA(SynchronizedQueue<Spill*>* data_queue, SpectraSet* spectra);
  void worker_parse(SynchronizedQueue<Spill*>* in_queue,
                              SynchronizedQueue<Spill*>* out_queue);
  void worker_run(RunType type, uint64_t timeout_limit,
                   SynchronizedQueue<Spill*>* spill_queue, boost::atomic<bool>* interruptor);
  void worker_run_dbl(RunType type, uint64_t timeout_limit,
                       SynchronizedQueue<Spill*>* spill_queue, boost::atomic<bool>* interruptor);
  void worker_fake(std::string source, SynchronizedQueue<Spill*>* data_queue,
                    uint64_t timeout_limit, boost::atomic<bool>* interruptor);

  //start and stop runs
  bool start_run(RunType type);
  bool resume_run(RunType type);
  bool stop_run(RunType type);

  //poll runs
  static uint32_t poll_run(RunType type);
  static uint32_t poll_run_dbl();

  //read and write external memory
  bool write_EM(uint32_t* data);
  bool read_EM(uint32_t* data);
  bool read_EM_dbl(uint32_t* data);
  bool clear_EM();

  //some helper functions
  uint32_t* control_collect_ADC(uint8_t module);
  void control_err(int32_t err_val);
  uint16_t i_dsp(char* setting_name);
  void timing_stats(CustomTimer&, CustomTimer&,
                           CustomTimer&, CustomTimer&,
                           CustomTimer&, uint32_t);

  std::string itobin (uint32_t bin);
  
  inline static void wait_ms(int millisecs) {
    boost::this_thread::sleep(boost::posix_time::milliseconds(millisecs));  
  }
};

}

#endif
