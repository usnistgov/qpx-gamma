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

#include "common_xia.h"
#include "custom_logger.h"
#include "wrapper.h"
#include <iomanip>

namespace Pixie {

bool Wrapper::boot() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  return my_settings_.boot();
}

void Wrapper::die() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  my_settings_ = Settings();
}


Settings& Wrapper::settings() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  return my_settings_;
}

Hit Wrapper::getOscil() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  
  Hit oscil_traces;
  uint32_t* oscil_data;
  int cur_mod = 0; //static_cast<int>(my_settings_.current_module());

  if (my_settings_.live() != LiveStatus::online)
    PL_WARN << "Pixie not online";
  else if ((oscil_data = control_collect_ADC(cur_mod)) != nullptr) {
    for (int i = 0; i < 4; i++) {     ///preset to 4 channels. Hardcoded
      oscil_traces.trace[i].resize(max_buf_len);
      oscil_traces.trace[i] = std::vector<U16>(oscil_data + (i*max_buf_len),
                                               oscil_data + ((i+1)*max_buf_len));
    }
    delete[] oscil_data;
  }
  return oscil_traces;
}

void Wrapper::getMca(RunType type, uint64_t timeout, SpectraSet& spectra,
                     boost::atomic<bool>& interruptor, bool double_buffer) {

  boost::unique_lock<boost::mutex> lock(mutex_);
  if (my_settings_.live() != LiveStatus::online) {
    PL_WARN << "Pixie not online";
    return;
  }
  
  PL_INFO << "Multithreaded MCA acquisition: type " << static_cast<int>(type)
          << " scheduled for " << timeout << " seconds";

  
  SynchronizedQueue<Spill*> rawQueue;
  SynchronizedQueue<Spill*> parsedQueue;

  boost::thread builder(boost::bind(&Pixie::Wrapper::worker_MCA, this, &parsedQueue, &spectra));
  boost::thread parser(boost::bind(&Pixie::Wrapper::worker_parse, this, &rawQueue, &parsedQueue));

  if (double_buffer)
    worker_run_dbl(type, timeout, &rawQueue, &interruptor);
  else
    worker_run(type, timeout, &rawQueue, &interruptor);

  while (rawQueue.size() > 0)
    wait_ms(1000);
  wait_ms(500);
  rawQueue.stop();
  wait_ms(500);

  parser.join();
  
  while (parsedQueue.size() > 0)
    wait_ms(1000);
  wait_ms(500);
  parsedQueue.stop();
  wait_ms(500);
  
  builder.join();
}

void Wrapper::getFakeMca(std::string source, SpectraSet& spectra,
                         uint64_t timeout, boost::atomic<bool>& interruptor) {

  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Multithreaded MCA faking scheduled for " << timeout << " seconds";

  SynchronizedQueue<Spill*> eventQueue;

  boost::thread builder(boost::bind(&Pixie::Wrapper::worker_MCA, this, &eventQueue, &spectra));
  worker_fake(source, &eventQueue, timeout, &interruptor);
  while (eventQueue.size() > 0)
    wait_ms(1000);
  wait_ms(500);
  eventQueue.stop();
  wait_ms(500);
  builder.join();
}

ListData* Wrapper::getList(RunType type, uint64_t timeout,
                           boost::atomic<bool>& interruptor, bool double_buf) {

  boost::unique_lock<boost::mutex> lock(mutex_);
  if (my_settings_.live() != LiveStatus::online) {
    PL_WARN << "Pixie not online";
    return nullptr;
  }

  Spill* one_spill;
  ListData* result = new ListData;

  int         numChans_;
  PL_INFO << "Multithreaded list mode acquisition: type " << static_cast<int>(type)
          << " scheduled for " << timeout << " seconds";

  SynchronizedQueue<Spill*> rawQueue;
  SynchronizedQueue<Spill*> parsedQueue;

  boost::thread parser(boost::bind(&Pixie::Wrapper::worker_parse, this, &rawQueue, &parsedQueue));

  if (double_buf)
    worker_run_dbl(type, timeout, &rawQueue, &interruptor);
  else
    worker_run(type, timeout, &rawQueue, &interruptor);
  
  while (rawQueue.size() > 0)
    wait_ms(1000);
  wait_ms(500);
  rawQueue.stop();
  wait_ms(500);
  parser.join();

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

uint32_t* Wrapper::control_collect_ADC(uint8_t module) {
  PL_INFO << "Pixie Control: get ADC (oscilloscope) traces";
  ///why is NUMBER_OF_CHANNELS used? Same for multi-module?
  uint32_t* adc_data = new uint32_t[NUMBER_OF_CHANNELS * max_buf_len];
  int32_t retval = -1;
  //Pixie_Acquire_Data(0x0084, (U32*)adc_data, NULL, module);
  if (retval < 0) {
    control_err(retval);
    return NULL;
  }
  return adc_data;
}

void Wrapper::control_err(int32_t err_val) {
  switch (err_val) {
  case 0:
    break;
  case -1:
    PL_ERR << "Control command failed: Invalid Pixie modules number. Check ModNum";
    break;
  case -2:
    PL_ERR << "Control command failed: Failure to adjust offsets. Reboot recommended";
    break;
  case -3:
    PL_ERR << "Control command failed: Failure to acquire ADC traces. Reboot recommended";
    break;
  case -4:
    PL_ERR << "Control command failed: Failure to start the control task run. Reboot recommended";
    break;
  default:
    PL_ERR << "Control comman failed: Unknown error " << err_val;
  }
}



bool Wrapper::start_run(RunType type) {
  //getInstance().my_settings_.reset();
  xxusb_register_write(getInstance().my_settings_.udev, XXUSB_ACTION_REGISTER, XXUSB_ACTION_START);
  return true;
}

bool Wrapper::resume_run(RunType type) { //no different from start
  uint16_t runtype = (static_cast<uint16_t>(type) | 0x2000);
  int cur_mod = 0; //static_cast<int>(getInstance().my_settings_.current_module());
  xxusb_register_write(getInstance().my_settings_.udev, XXUSB_ACTION_REGISTER, XXUSB_ACTION_START);
  return true;
}

bool Wrapper::stop_run(RunType type) {
  xxusb_register_write(getInstance().my_settings_.udev, XXUSB_ACTION_REGISTER, 0);

  // Draining data / read from fifo until Time out error and discard all data
  long Data_Array[list_mem_len32];
  int ret = xxusb_bulk_read(getInstance().my_settings_.udev, Data_Array, 26624, 2000);
  while(ret > 0)
    ret = xxusb_bulk_read(getInstance().my_settings_.udev, Data_Array, 26624, 2000);
  return true;
}


uint32_t Wrapper::poll_run(RunType type) {
  uint16_t runtype = (static_cast<uint16_t>(type) | 0x4000);
  int cur_mod = 0; //static_cast<int>(getInstance().my_settings_.current_module());
  return 0;
}

uint32_t Wrapper::poll_run_dbl() {
  int cur_mod = 0; //static_cast<int>(getInstance().my_settings_.current_module());
  std::bitset<32> set;
  set[14] = 1;
  return set.to_ulong();
}


bool Wrapper::read_EM(uint32_t* data) {
  //int cur_mod = static_cast<int>(getInstance().my_settings_.current_module());
  long Data_Array[list_mem_len32];
  for (int i=0; i<list_mem_len32; i++)
    Data_Array[i]= 0;

  int ret = xxusb_bulk_read(getInstance().my_settings_.udev, Data_Array, list_mem_len16, 200);
  if(ret > 0) {
    int i;
    for (i=0; i< ret/4; i++)
      data[i] = Data_Array[i];
    //VME_write_16(getInstance().my_settings_.udev, AM_MADC, MADC_ADDR(MADC_READ_RESET), 1);
    return true;
  }
  PL_INFO << "no data retrieved (error?)";
  return true;
}

bool Wrapper::write_EM(uint32_t* data) {
  int cur_mod = 0; //static_cast<int>(getInstance().my_settings_.current_module());
  //  return (Pixie_Acquire_Data(0x9004, (U32*)data, NULL, cur_mod) == 0x90);
  return true;
};


uint16_t Wrapper::i_dsp(char* setting_name) {
  return 0;
  //Find_Xact_Match((S8*)setting_name, DSP_Parameter_Names, N_DSP_PAR);
};

//this function taken from XIA's code and modified, original comments remain
bool Wrapper::read_EM_dbl(uint32_t* data) {
  return read_EM(data);
}

bool Wrapper::clear_EM() {
  std::vector<uint32_t> my_data(list_mem_len32, 0);
  return (write_EM(my_data.data()) >= 0);
};

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
}

void Wrapper::worker_fake(std::string source, SynchronizedQueue<Spill*>* data_queue,
                          uint64_t timeout_limit, boost::atomic<bool>* interruptor) {

  PL_INFO << "*Pixie Threads* Simulated event generator initiated";

  uint32_t secsperrun = 5;  ///calculate this based on ocr and buf size
  Spill* one_spill;

  uint64_t lines = 0;
  int ch;
  FILE *DataFile =  fopen(source.c_str(),"r");
  while(!feof(DataFile))
  {
    ch = fgetc(DataFile);
    if (ch == '\n')
    {
      lines++;
    }
  }
  uint32_t x;

  rewind(DataFile);



  uint64_t spill_count = 0, event_count = 0;
  bool timeout = false;
  CustomTimer total_timer(timeout_limit);
  boost::posix_time::ptime session_start_time, block_time;

  uint64_t   rate = 2000;
  StatsUpdate moving_stats,
      one_run;  ///bit more than secsperrun

  //start of run pixie status update
  //mainly for spectra to have calibration
  one_spill = new Spill;
  one_spill->run = new RunInfo;
  session_start_time =  boost::posix_time::microsec_clock::local_time();
  moving_stats.lab_time = session_start_time;
  one_spill->stats = new StatsUpdate(moving_stats);
  data_queue->enqueue(one_spill);
  
  total_timer.resume();
  while (!timeout) {
    spill_count++;
    
    PL_INFO << "  SIMULATION  Elapsed: " << total_timer.done()
            << "  ETA: " << total_timer.ETA();

    one_spill = new Spill;

    one_spill->data.resize(rate*secsperrun);
    for (uint32_t i=0; i<(rate*secsperrun); i++) {
      if (!lines) {
        interruptor->store(true);
        break;
      }
      fscanf(DataFile,"%x\n",&x);
      one_spill->data[i] = x;
      lines--;
    }

    block_time =  boost::posix_time::microsec_clock::local_time();

    event_count += (rate*secsperrun);

    moving_stats = moving_stats + one_run;
    moving_stats.spill_count = spill_count;
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
  one_spill->run->time_start = session_start_time;
  one_spill->run->time_stop = block_time;
  one_spill->run->total_events = event_count;
  data_queue->enqueue(one_spill);
}


void Wrapper::worker_run(RunType type, uint64_t timeout_limit,
                         SynchronizedQueue<Spill*>* spill_queue,
                         boost::atomic<bool>* interruptor) {
  
  PL_INFO << "*Pixie Threads* List runner initiated";
  Wrapper &pxi = getInstance();
  double poll_interval = pxi.poll_interval_ms_;
  int32_t retval = 0;
  uint64_t spill_count = 0;
  bool timeout = false;
  Spill* fetched_spill;
  CustomTimer readout_timer, start_timer, run_timer, stop_timer,
      total_timer(timeout_limit);
  boost::posix_time::ptime session_start_time, block_time;
  
  if (!clear_EM())
    return;

  //  pxi.my_settings_.set_mod("DBLBUFCSR",   static_cast<double>(0));
  //  pxi.my_settings_.set_mod("MODULE_CSRA", static_cast<double>(2));

  //start of run pixie status update
  //mainly for spectra to have calibration
  fetched_spill = new Spill;
  fetched_spill->run = new RunInfo;
  fetched_spill->stats = new StatsUpdate;
  pxi.my_settings_.get_all_settings();
  fetched_spill->run->p4_state = pxi.my_settings_;

  block_time = session_start_time = boost::posix_time::microsec_clock::local_time();

  if(!start_run(type)) {
    delete fetched_spill->run;
    delete fetched_spill->stats;
    delete fetched_spill;
    return;
  }
  
  total_timer.resume();
  
  //  pxi.my_settings_.get_chan_stats();
  //  pxi.my_settings_.get_mod_stats();
  fetched_spill->stats->eat_stats(pxi.my_settings_);
  fetched_spill->stats->lab_time = session_start_time;
  spill_queue->enqueue(fetched_spill);

  while (!timeout) {
    start_timer.resume();
    
    if (spill_count > 0)
      retval = resume_run(type);
    start_timer.stop();
    run_timer.resume();

    if(retval < 0)
      return;
    
    if (spill_count > 0) {
      //      pxi.my_settings_.get_mod_stats();
      //      pxi.my_settings_.get_chan_stats();
      fetched_spill->stats = new StatsUpdate;
      fetched_spill->stats->eat_stats(pxi.my_settings_);
      fetched_spill->stats->spill_count = spill_count;
      fetched_spill->stats->lab_time   = block_time;
      spill_queue->enqueue(fetched_spill);
    }

    PL_INFO << "  RUNNING(" << spill_count << ")"
            << "  Elapsed: " << total_timer.done()
            << "  ETA: " << total_timer.ETA();

    fetched_spill = new Spill;
    fetched_spill->data.resize(list_mem_len32, 0);

    spill_count++;

    retval = poll_run(type);
    while ((retval == 1) && (!timeout)) {
      wait_ms(poll_interval);
      retval = poll_run(type);
      timeout = (total_timer.timeout() || interruptor->load());
    };
    block_time = boost::posix_time::microsec_clock::local_time();
    run_timer.stop();

    stop_timer.resume();
    stop_run(type); //is this really necessary?
    stop_timer.stop();

    readout_timer.resume();
    if(!read_EM(fetched_spill->data.data())) {
      PL_DBG << "read_EM failed";
      delete fetched_spill;
      return;
    }
    readout_timer.stop();
  }

  //  pxi.my_settings_.get_mod_stats();
  //  pxi.my_settings_.get_chan_stats();
  fetched_spill->stats = new StatsUpdate;
  fetched_spill->stats->eat_stats(pxi.my_settings_);
  fetched_spill->stats->spill_count = spill_count;
  fetched_spill->stats->lab_time   = block_time;

  total_timer.stop();

  pxi.my_settings_.get_all_settings();
  fetched_spill->run = new RunInfo;
  fetched_spill->run->p4_state = pxi.my_settings_;
  fetched_spill->run->time_start = session_start_time;
  fetched_spill->run->time_stop  = block_time;

  spill_queue->enqueue(fetched_spill);

  timing_stats(total_timer, start_timer, run_timer, stop_timer, readout_timer, spill_count);
  PL_INFO << "**Acquisition runs stopped**";
}


void Wrapper::worker_run_dbl(RunType type, uint64_t timeout_limit,
                             SynchronizedQueue<Spill*>* spill_queue,
                             boost::atomic<bool>* interruptor) {
  
  PL_INFO << "*Pixie Threads* Buffered list runner initiated";
  Wrapper &pxi = getInstance();
  double poll_interval = pxi.poll_interval_ms_;
  int32_t retval = 0;
  std::bitset<32> csr;
  uint64_t spill_count = 0;
  bool timeout = false;
  Spill* fetched_spill;
  CustomTimer total_timer(timeout_limit);
  boost::posix_time::ptime session_start_time, block_time;

  CustomTimer *anouncement_timer = nullptr;
  double secs_between_anouncements = 5;
  
  if (!pxi.clear_EM())
    return;

  
  //start of run pixie status update
  //mainly for spectra to have calibration
  fetched_spill = new Spill;
  fetched_spill->run = new RunInfo;
  fetched_spill->stats = new StatsUpdate;
  pxi.my_settings_.get_all_settings();
  fetched_spill->run->p4_state = pxi.my_settings_;

  pxi.my_settings_.reset();
  session_start_time = boost::posix_time::microsec_clock::local_time();
  
  if(!start_run(type)) {
    delete fetched_spill->run;
    delete fetched_spill->stats;
    delete fetched_spill;
    return;
  }

  total_timer.resume();

  //pxi.my_settings_.get_chan_stats();
  //pxi.my_settings_.get_mod_stats();
  fetched_spill->stats->eat_stats(pxi.my_settings_);
  fetched_spill->stats->lab_time = session_start_time;
  spill_queue->enqueue(fetched_spill);

  anouncement_timer = new CustomTimer(true);

  while (!timeout) {
    spill_count++;
    if (anouncement_timer->s() > secs_between_anouncements) {
      PL_INFO << "  RUNNING Elapsed: " << total_timer.done()
              << "  ETA: " << total_timer.ETA();
      delete anouncement_timer;
      anouncement_timer = new CustomTimer(true);
    }

    do {
      wait_ms(poll_interval);
      csr = std::bitset<32>(poll_run_dbl());
      timeout = (total_timer.timeout() || interruptor->load());
    } while ((!csr[14]) && (!timeout));
    block_time = boost::posix_time::microsec_clock::local_time();
    //pxi.my_settings_.get_mod_stats();
    //pxi.my_settings_.get_chan_stats();

    fetched_spill = new Spill;
    fetched_spill->data.resize(list_mem_len32, 0);

    if (read_EM_dbl(fetched_spill->data.data())) {
      fetched_spill->stats = new StatsUpdate;
      fetched_spill->stats->eat_stats(pxi.my_settings_);
      fetched_spill->stats->spill_count = spill_count;
      fetched_spill->stats->lab_time    = block_time;
      spill_queue->enqueue(fetched_spill);
    } else {
      delete fetched_spill;
      return;
    }
  }

  stop_run(type);
  //pxi.my_settings_.get_all_settings();

  fetched_spill = new Spill;
  fetched_spill->run = new RunInfo;
  fetched_spill->run->p4_state = pxi.my_settings_;
  fetched_spill->run->time_start = session_start_time;
  fetched_spill->run->time_stop  = block_time;
  spill_queue->enqueue(fetched_spill);

  PL_INFO << "**Acquisition runs stopped**";
}



void Wrapper::worker_parse (SynchronizedQueue<Spill*>* in_queue, SynchronizedQueue<Spill*>* out_queue) {

  PL_INFO << "*Pixie Threads* Event parser initiated";

  uint32_t header_m   = 0xff008000; // Header Mask
  uint32_t header_c   = 0x40000000; // Header Compare

  uint32_t footer_m   = 0xc0000000; // Footer Mask
  uint32_t footer_c   = 0xc0000000; // Footer Compare

  uint32_t evt_mask = 0xffe04000; // event header mask
  uint32_t evt_c    = 0x04000000; // event compare
  uint32_t ovrfl_c  = 0x04004000; // Overflow compare

  uint32_t det_mask = 0x001f0000; // Detector Mask
  uint32_t nrg_mask = 0x00001fff;

  Spill* spill;

  uint64_t total_events = 0, good_events = 0, cycles = 0, total_overflow = 0, total_footers = 0, total_headers = 0,
      valid_spills = 0, bad_events = 0;
  CustomTimer parse_timer;

  //setup output file
  boost::filesystem::path current( boost::filesystem::current_path() );
  current /= "vme_raw.dat";

  FILE *DataFile;
  DataFile =  fopen(current.filename().string().c_str(),"w");
  
  while ((spill = in_queue->dequeue()) != NULL ) {
    parse_timer.resume();
    
    if (spill->data.size() > 0) {
      cycles++;

      uint32_t spill_events = 0, spill_overflow = 0, spill_headers = 0, spill_footers = 0,
          spill_bad_evts = 0, spill_good_events = 0;

      for (auto &entry : spill->data) {
        spill_events++;

//        fprintf(DataFile,"0x%08X\n",entry);

        
        if ((entry == 0xffffffff) || (entry == 0)) {
          //PL_INFO << "ignore";
          spill_bad_evts++;
        } else if ((entry & evt_mask) == ovrfl_c) {
          spill_overflow++;
          //PL_INFO << "overflow";
          //} else if ((entry & 0xffff) > 7679) {
          //PL_INFO << "supposedly too high?";
        } else if ((entry & header_m) == header_c) {
          uint32_t module = ((entry & 0x00ff0000) >> 16);
          uint32_t format = ((entry & 0x00007000) >> 12);
          uint32_t words = (entry & 0x00000fff);
          spill_headers++;
          /*PL_INFO << "header module=" << module
                  << " format=" << format
                  << " words=" << words;*/

          fprintf(DataFile, itobin(entry).c_str());
          fprintf(DataFile, "\n");

        } else if ((entry & footer_m) == footer_c) {
          uint32_t time = (entry & 0x7fffffff);
          spill_footers++;
          //PL_INFO << "footer time=" << time;

          fprintf(DataFile, itobin(entry).c_str());
          fprintf(DataFile, "\n");

        } else if ((entry & evt_mask) == evt_c) {
          uint8_t chan_nr = (entry & det_mask) >> 16;

          std::bitset<32> chan_pat;
          chan_pat[chan_nr] = 1;
          uint16_t nrg = (entry & nrg_mask) << 3;

          Hit one_hit;
          one_hit.pattern = chan_pat;
          one_hit.energy[chan_nr] = nrg;
          spill->hits.push_back(one_hit);
          spill_good_events++;
        }
      };
      total_overflow += spill_overflow;
      total_headers += spill_headers;
      total_footers += spill_footers;
      good_events += spill_good_events;
      bad_events += spill_bad_evts;
      total_events += (spill_events - spill_bad_evts);

      if (spill_events)
        valid_spills++;
    }
    if (spill->run != nullptr) {
      spill->run->total_events = good_events;
    }
    spill->data.clear();
    out_queue->enqueue(spill);
    parse_timer.stop();
  }
  if (cycles == 0)
    PL_DBG << "   Parse worker done. Buffer queue closed without events";
  else {
    if (!valid_spills)
      valid_spills++;
    if (!total_events)
      total_events++;
    
    PL_DBG << "   Parse worker done. Avg time/spill: " << parse_timer.us()/valid_spills << "us over "
           << valid_spills << " valid spills out of " << cycles << " total processed spills";
    PL_DBG << "          good events:    " << good_events << "    per spill: "     << good_events / valid_spills << " " << static_cast<double>(good_events)/static_cast<double>(total_events)*100 << "%";
    PL_DBG << "              headers:   " << total_headers << "    per spill: "  << total_headers / valid_spills << " " << static_cast<double>(total_headers)/static_cast<double>(total_events)*100 << "%";
    PL_DBG << "            overflows: " << total_overflow << "    per spill: " << total_overflow / valid_spills << " " << static_cast<double>(total_overflow)/static_cast<double>(total_events)*100 << "%";
    PL_DBG << "              footers:   " << total_footers << "    per spill: "  << total_footers / valid_spills << " " << static_cast<double>(total_footers)/static_cast<double>(total_events)*100 << "%";
    PL_DBG << "             rejected:  " << bad_events << "    per spill: "     << bad_events / valid_spills;
  }
  fclose(DataFile);
}

void Wrapper::timing_stats(CustomTimer &total, CustomTimer &start, CustomTimer &run, CustomTimer &stop, CustomTimer &read, uint32_t count) {
  uint16_t prc = 6;  //decimal precision
  double
      start_p  = 100 * start.ns() / total.ns(),
      run_p    = 100 * run.ns()   / total.ns(),
      stop_p   = 100 * stop.ns()  / total.ns(),
      read_p   = 100 * read.ns()  / total.ns();

  PL_INFO << "      Total time: " << total.s() << " s";
  PL_INFO << "        Run time: " << run.s()   << " s";
  PL_INFO << "-----------------------------------";
  PL_INFO << "    avg run: " << std::fixed << std::setprecision (prc)
          << total.s()/count << "s (100%)";
  PL_INFO << "  avg start: " << std::fixed << std::setprecision (prc)
          << start.s()/count << "s (" << start_p << "%)";
  PL_INFO << "    avg run: " << std::fixed << std::setprecision (prc)
          << run.s()/count << "s (" << run_p << "%)";
  PL_INFO << "   avg stop: " << std::fixed << std::setprecision (prc)
          << stop.s()/count << "s (" << stop_p << "%)";
  PL_INFO << "avg readout: " << std::fixed << std::setprecision (prc)
          << read.s()/count << "s (" << read_p << "%)";
}

void Wrapper::control_measure_baselines(uint8_t module) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: measure baselines";
  //control_err(Pixie_Acquire_Data(0x0006, NULL, NULL, module));
}

void Wrapper::control_compute_BLcut() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: compute BLcut";
  //control_err(Pixie_Acquire_Data(0x0080, NULL, NULL, 0));
}

void Wrapper::control_find_tau() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: find tau";
  //control_err(Pixie_Acquire_Data(0x0081, NULL, NULL, 0));
}

void Wrapper::control_adjust_offsets() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: adjust offsets";
  //control_err(Pixie_Acquire_Data(0x0083, NULL, NULL, 0));
}

std::string Wrapper::itobin (uint32_t bin)
{
  std::stringstream ss;
  int k=0;
  for (k = 31; k >= 0; --k) {
    if (bin & 0x80000000)
      ss << "1";
    else
      ss << "0";
    bin <<= 1;
    if (k == 16)
      ss << " ";
  }
  return ss.str();
}


}
