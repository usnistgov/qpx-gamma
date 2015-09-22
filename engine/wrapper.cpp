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
 *      Qpx::Wrapper singleton for controlling a Pixie-4 device.
 *      Mutex locking on all public functions. Threads in private static fns.
 *      Use only double-buffer mode acquisition for reliable timing.
 *      Might also use lock file to ensure only one instance per system?
 *
 ******************************************************************************/

#include "custom_logger.h"
#include "wrapper.h"
#include <iomanip>

namespace Qpx {

Settings& Wrapper::settings() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  return my_settings_;
}

void Wrapper::getMca(uint64_t timeout, SpectraSet& spectra, boost::atomic<bool>& interruptor) {

  boost::unique_lock<boost::mutex> lock(mutex_);
  if (my_settings_.live() != LiveStatus::online) {
    PL_WARN << "Pixie not online";
    return;
  }

  PL_INFO << "Multithreaded MCA acquisition scheduled for " << timeout << " seconds";

  SynchronizedQueue<Spill*> parsedQueue;

  boost::thread builder(boost::bind(&Qpx::Wrapper::worker_MCA, this, &parsedQueue, &spectra));

  if (my_settings_.start_daq(timeout, &parsedQueue))
    PL_DBG << "Started generating threads";

  while (!interruptor.load()) {
    wait_ms(1000);
  }

  if (my_settings_.stop_daq())
    PL_DBG << "Stopped generating threads";

  wait_ms(500);
  while (parsedQueue.size() > 0)
    wait_ms(1000);
  wait_ms(500);
  parsedQueue.stop();
  wait_ms(500);
  
  builder.join();
}

void Wrapper::getFakeMca(Simulator& source, SpectraSet& spectra,
                              uint64_t timeout, boost::atomic<bool>& interruptor) {

  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Multithreaded MCA simulation scheduled for " << timeout << " seconds";

  SynchronizedQueue<Spill*> eventQueue;

  boost::thread builder(boost::bind(&Qpx::Wrapper::worker_MCA, this, &eventQueue, &spectra));
  worker_fake(&source, &eventQueue, timeout, &interruptor);
  while (eventQueue.size() > 0)
    wait_ms(1000);
  wait_ms(500);
  eventQueue.stop();
  wait_ms(500);
  builder.join();
}

void Wrapper::simulateFromList(Sorter &sorter, SpectraSet &spectra, boost::atomic<bool> &interruptor) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Acquisition simulation from saved list data";

  SynchronizedQueue<Spill*> eventQueue;

  boost::thread builder(boost::bind(&Qpx::Wrapper::worker_MCA, this, &eventQueue, &spectra));
  worker_from_list(&sorter, &eventQueue, &interruptor);
  while (eventQueue.size() > 0)
    wait_ms(1000);
  wait_ms(500);
  eventQueue.stop();
  wait_ms(500);
  builder.join();

}


ListData* Wrapper::getList(uint64_t timeout, boost::atomic<bool>& interruptor) {

  boost::unique_lock<boost::mutex> lock(mutex_);
  if (my_settings_.live() != LiveStatus::online) {
    PL_WARN << "Pixie not online";
    return nullptr;
  }

  Spill* one_spill;
  ListData* result = new ListData;

  PL_INFO << "Multithreaded list mode acquisition scheduled for " << timeout << " seconds";


  SynchronizedQueue<Spill*> parsedQueue;

  if (my_settings_.start_daq(timeout, &parsedQueue))
    PL_DBG << "Started generating threads";

  while (!interruptor.load()) {
    wait_ms(1000);
  }

  if (my_settings_.stop_daq())
    PL_DBG << "Stopped generating threads";

  wait_ms(500);

  while (parsedQueue.size() > 0) {
    one_spill = parsedQueue.dequeue();
    for (auto &q : one_spill->hits)
      result->hits.push_back(q);
    if (one_spill->run != nullptr) {
      result->run = *one_spill->run;
      delete one_spill->run;
    }
    if (one_spill->stats != nullptr)
      delete one_spill->stats;
    delete one_spill;
  }
        
  parsedQueue.stop();
  return result;
}

//////STUFF BELOW SHOULD NOT BE USED DIRECTLY////////////
//////ASSUME YOU KNOW WHAT YOU'RE DOING WITH THREADS/////

void Wrapper::worker_MCA(SynchronizedQueue<Spill*>* data_queue,
                   SpectraSet* spectra) {

  PL_INFO << "*Pixie Threads* Spectra builder initiated";
  Spill* one_spill;
  while ((one_spill = data_queue->dequeue()) != nullptr) {
    spectra->add_spill(one_spill);

    if (one_spill->stats != nullptr)
      delete one_spill->stats;
    if (one_spill->run != nullptr)
      delete one_spill->run;    
    delete one_spill;
  }
  spectra->closeAcquisition();
}

void Wrapper::worker_fake(Simulator* source, SynchronizedQueue<Spill*>* data_queue,
                    uint64_t timeout_limit, boost::atomic<bool>* interruptor) {

  PL_INFO << "*Pixie Threads* Simulated event generator initiated";

  uint32_t secsperrun = 5;  ///calculate this based on ocr and buf size
  Spill* one_spill;

  uint64_t spill_number = 0, event_count = 0;
  bool timeout = false;
  CustomTimer total_timer(timeout_limit);
  boost::posix_time::ptime session_start_time, block_time;

  uint64_t   rate = source->OCR;
  StatsUpdate moving_stats,
      one_run = source->getBlock(5.05);  ///bit more than secsperrun

  //start of run pixie status update
  //mainly for spectra to have calibration
  one_spill = new Spill;
  one_spill->run = new RunInfo;
  one_spill->run->state = source->settings;
  one_spill->run->detectors = source->detectors;
  session_start_time =  boost::posix_time::microsec_clock::local_time();
  moving_stats.lab_time = session_start_time;
  one_spill->stats = new StatsUpdate(moving_stats);
  data_queue->enqueue(one_spill);
  
  total_timer.resume();
  while (!timeout) {
    spill_number++;
    
    PL_INFO << "  SIMULATION  Elapsed: " << total_timer.done()
            << "  ETA: " << total_timer.ETA();

    one_spill = new Spill;
    
    for (uint32_t i=0; i<(rate*secsperrun); i++)
      one_spill->hits.push_back(source->getHit());

    block_time =  boost::posix_time::microsec_clock::local_time();

    event_count += (rate*secsperrun);

    moving_stats = moving_stats + one_run;
    moving_stats.spill_number = spill_number;
    moving_stats.lab_time = block_time;
    moving_stats.event_rate = one_run.event_rate;
    one_spill->stats = new StatsUpdate(moving_stats);
    data_queue->enqueue(one_spill);
    
    boost::this_thread::sleep(boost::posix_time::seconds(secsperrun));

    timeout = total_timer.timeout();
    if (*interruptor)
      timeout = true;
  }

  one_spill = new Spill;
  one_spill->run = new RunInfo;
  one_spill->run->state = source->settings;
  one_spill->run->time_start = session_start_time;
  one_spill->run->time_stop = block_time;
  one_spill->run->total_events = event_count;
  data_queue->enqueue(one_spill);
}


void Wrapper::worker_from_list(Sorter* sorter, SynchronizedQueue<Spill*>* data_queue, boost::atomic<bool>* interruptor) {
  Spill one_spill;

  while ((!((one_spill = sorter->get_spill()) == Spill())) && (!(*interruptor))) {
    data_queue->enqueue(new Spill(one_spill));
    boost::this_thread::sleep(boost::posix_time::seconds(2));
  }
  PL_DBG << "worker_from_list terminating";
}


}
