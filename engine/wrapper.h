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
 *      Qpx::Wrapper singleton for controlling a daq device.
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
#include "sorter.h"
#include "qpx_settings.h"

#ifndef QPX_WRAPPER
#define QPX_WRAPPER

namespace Qpx {

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

  ////THE MEAT: ACQUISITION////
  
  ListData* getList(uint64_t timeout, boost::atomic<bool>& inturruptor);
  void getMca(uint64_t timeout, SpectraSet &spectra, boost::atomic<bool> &interruptor);
  void getFakeMca(Simulator &simulator, SpectraSet &spectra, uint64_t timeout, boost::atomic<bool> &interruptor);
  void simulateFromList(Sorter &sorter, SpectraSet &spectra, boost::atomic<bool> &interruptor);

 private:
  Settings  my_settings_;
  mutable boost::mutex mutex_;

  //singleton assurance
  Wrapper() {}
  Wrapper(Wrapper const&);
  void operator=(Wrapper const&);

  //threads
  void worker_MCA(SynchronizedQueue<Spill*>* data_queue, SpectraSet* spectra);
  void worker_fake(Simulator* source, SynchronizedQueue<Spill*>* data_queue,
                    uint64_t timeout_limit, boost::atomic<bool>* interruptor);
  void worker_from_list(Sorter* sorter, SynchronizedQueue<Spill*>* data_queue, boost::atomic<bool>* interruptor);

};

}

#endif
