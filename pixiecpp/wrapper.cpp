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
  int cur_mod = static_cast<int>(my_settings_.current_module());

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

  my_settings_.set_mod("RUN_TYPE",    static_cast<double>(type));
  my_settings_.set_mod("MAX_EVENTS",  static_cast<double>(0));
  
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

void Wrapper::getFakeMca(Simulator& source, SpectraSet& spectra,
                              uint64_t timeout, boost::atomic<bool>& interruptor) {

  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Multithreaded MCA faking scheduled for " << timeout << " seconds";

  SynchronizedQueue<Spill*> eventQueue;

  std::vector<Detector> detectors = my_settings_.get_detectors();
  for (std::size_t i = 0; i < detectors.size(); i++)
    source.settings.set_detector(Channel(i), detectors[i]);
  
  boost::thread builder(boost::bind(&Pixie::Wrapper::worker_MCA, this, &eventQueue, &spectra));
  worker_fake(&source, &eventQueue, timeout, &interruptor);
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

  my_settings_.set_mod("RUN_TYPE",    static_cast<double>(type));
  my_settings_.set_mod("MAX_EVENTS",  static_cast<double>(0));
  
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


void Wrapper::control_set_DAC(uint8_t module) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: set DAC";
  control_err(Pixie_Acquire_Data(0x0000, NULL, NULL, module));
}

void Wrapper::control_connect(uint8_t module) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: connect";
  control_err(Pixie_Acquire_Data(0x0001, NULL, NULL, module));
}
  
void Wrapper::control_disconnect(uint8_t module) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: disconnect";
  control_err(Pixie_Acquire_Data(0x0002, NULL, NULL, module));
}

void Wrapper::control_program_Fippi(uint8_t module) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: program Fippi";
  control_err(Pixie_Acquire_Data(0x0005, NULL, NULL, module));
}

void Wrapper::control_measure_baselines(uint8_t module) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: measure baselines";
  control_err(Pixie_Acquire_Data(0x0006, NULL, NULL, module));
}

void Wrapper::control_test_EM_write(uint8_t module) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: test EM write";
  control_err(Pixie_Acquire_Data(0x0016, NULL, NULL, module));
}

void Wrapper::control_test_HM_write(uint8_t module) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: test HM write";
  control_err(Pixie_Acquire_Data(0x001A, NULL, NULL, module));
}

void Wrapper::control_compute_BLcut() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: compute BLcut";
  control_err(Pixie_Acquire_Data(0x0080, NULL, NULL, 0));
}

void Wrapper::control_find_tau() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: find tau";
  control_err(Pixie_Acquire_Data(0x0081, NULL, NULL, 0));
}
                
void Wrapper::control_adjust_offsets() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "Pixie Control: adjust offsets";
  control_err(Pixie_Acquire_Data(0x0083, NULL, NULL, 0));
}


//////STUFF BELOW SHOULD NOT BE USED DIRECTLY////////////
//////ASSUME YOU KNOW WHAT YOU'RE DOING WITH THREADS/////

uint32_t* Wrapper::control_collect_ADC(uint8_t module) {
  PL_INFO << "Pixie Control: get ADC (oscilloscope) traces";
  ///why is NUMBER_OF_CHANNELS used? Same for multi-module?
  uint32_t* adc_data = new uint32_t[NUMBER_OF_CHANNELS * max_buf_len];
  int32_t retval = Pixie_Acquire_Data(0x0084, (U32*)adc_data, NULL, module);
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
  uint16_t runtype = (static_cast<uint16_t>(type) | 0x1000);
  int cur_mod = static_cast<int>(getInstance().my_settings_.current_module());
  int32_t ret = Pixie_Acquire_Data(runtype, NULL, 0, cur_mod);
  switch (ret) {
    case 0x10:
      return true;
    case -0x11:
      PL_ERR << "Start run failed: Invalid Pixie module number";
      return false;
    case -0x12:
      PL_ERR << "Start run failed. Try rebooting";
      return false;
    default:
      PL_ERR << "Start run failed. Unknown error";
      return false;
  }
};

bool Wrapper::resume_run(RunType type) {
  uint16_t runtype = (static_cast<uint16_t>(type) | 0x2000);
  int cur_mod = static_cast<int>(getInstance().my_settings_.current_module());
  int32_t ret = Pixie_Acquire_Data(runtype, NULL, 0, cur_mod);
  switch (ret) {
    case 0x20:
      return true;
    case -0x21:
      PL_ERR << "Resume run failed: Invalid Pixie module number";
      return false;
    case -0x22:
      PL_ERR << "Resume run failed. Try rebooting";
      return false;
    default:
      PL_ERR << "Start run failed. Unknown error";
      return false;
  }
};

bool Wrapper::stop_run(RunType type) {
  uint16_t runtype = (static_cast<uint16_t>(type) | 0x3000);
  int cur_mod = static_cast<int>(getInstance().my_settings_.current_module());
  int32_t ret = Pixie_Acquire_Data(runtype, NULL, 0, cur_mod);
  switch (ret) {
    case 0x30:
      return true;
    case -0x31:
      PL_ERR << "Resume run failed: Invalid Pixie module number";
      return false;
    case -0x32:
      PL_ERR << "Resume run failed. Try rebooting";
      return false;
    default:
      PL_ERR << "Start run failed. Unknown error";
      return false;
  }
};


uint32_t Wrapper::poll_run(RunType type) {
  uint16_t runtype = (static_cast<uint16_t>(type) | 0x4000);
  int cur_mod = static_cast<int>(getInstance().my_settings_.current_module());
  return Pixie_Acquire_Data(runtype, NULL, 0, cur_mod);
};

uint32_t Wrapper::poll_run_dbl() {
  int cur_mod = static_cast<int>(getInstance().my_settings_.current_module());
  return Pixie_Acquire_Data(0x40FF, NULL, 0, cur_mod);
};


bool Wrapper::read_EM(uint32_t* data) {
  int cur_mod = static_cast<int>(getInstance().my_settings_.current_module());
  PL_DBG << "Current module " << cur_mod;
  S32 retval = Pixie_Acquire_Data(0x9003, (U32*)data, NULL, cur_mod);
  switch (retval) {
    case -0x93:
      PL_ERR << "Failure to read list mode section of external memory. Reboot recommended.";
      return false;
    case -0x95:
      PL_ERR << "Invalid external memory I/O request. Check run type.";
      return false;
    case 0x90:
      return true;
    case 0x0:
      return true; //undocumented by XIA, returns this rather than 0x90 upon success
    default:
      PL_ERR << "Unexpected error " << std::hex << retval;
      return false;
  }
};

bool Wrapper::write_EM(uint32_t* data) {
  int cur_mod = static_cast<int>(getInstance().my_settings_.current_module());
  return (Pixie_Acquire_Data(0x9004, (U32*)data, NULL, cur_mod) == 0x90);
};


uint16_t Wrapper::i_dsp(char* setting_name) {
  return Find_Xact_Match((S8*)setting_name, DSP_Parameter_Names, N_DSP_PAR);
};

//this function taken from XIA's code and modified, original comments remain
bool Wrapper::read_EM_dbl(uint32_t* data) {
  U16 i, j, MCSRA;
  U16 DblBufCSR;
  U32 Aoffset[2], WordCountPP[2];
  U32 WordCount, NumWordsToRead, CSR;
  U32 dsp_word[2];
  U32 WCRs[PRESET_MAX_MODULES], CSRs[PRESET_MAX_MODULES];

  char modcsra_str[] = "MODCSRA";
  char dblbufcsr_str[] = "DBLBUFCSR";
  Pixie_IODM(0, (U16)DATA_MEMORY_ADDRESS + i_dsp(modcsra_str), MOD_READ, 1, dsp_word);
  MCSRA = (U16)dsp_word[0];
  Pixie_IODM(0, (U16)DATA_MEMORY_ADDRESS + i_dsp(dblbufcsr_str), MOD_READ, 1, dsp_word);
  DblBufCSR = (U16)dsp_word[0];

  Aoffset[0] = 0;
  Aoffset[1] = LM_DBLBUF_BLOCK_LENGTH;
			
  for(i=0; i<Number_Modules; i++)
  {
    // read the CSR
    Pixie_ReadCSR((U8)i, &CSR);
    CSRs[i] = CSR;

    // A read of Pixie's word count register 
    // This also indicates to the DSP that a readout has begun 
    Pixie_RdWrdCnt((U8)i, &WordCount);
    WCRs[i] = WordCount;
  }	// CSR for loop

  // Read out list mode data module by module 
  for(i=0; i<Number_Modules; i++)
  {

    // The number of 16-bit words to read is in EMwords or EMwords2
    char emwords_str[] = "EMWORDS";
    char emwords2_str[] = "EMWORDS2";
    Pixie_IODM((U8)i, (U16)DATA_MEMORY_ADDRESS + i_dsp(emwords_str), MOD_READ, 2, dsp_word);
    WordCountPP[0] = dsp_word[0] * 65536 + dsp_word[1];
    Pixie_IODM((U8)i, (U16)DATA_MEMORY_ADDRESS + i_dsp(emwords2_str), MOD_READ, 2, dsp_word);
    WordCountPP[1] = dsp_word[0] * 65536 + dsp_word[1];

    if(TstBit(CSR_128K_FIRST, (U16)CSRs[i]) == 1) 
      j=0;			
    else		// block at 128K+64K was first
      j=1;
		
    if  (TstBit(CSR_DATAREADY, (U16)CSRs[i]) == 0 )		
      // function called after a readout that cleared WCR => run stopped => read other block
    {
      j=1-j;			
      PL_INFO << "(read_EM_dbl): Module " << i <<
          ": Both memory blocks full (block " << 1-j << " older). Run paused (or finished).";
      Pixie_Print_MSG(ErrMSG);
    }

    if (WordCountPP[j] >0)
    {
      // Check if it is an odd or even number 
      if(fmod(WordCountPP[j], 2.0) == 0.0)
        NumWordsToRead = WordCountPP[j] / 2;
      else
        NumWordsToRead = WordCountPP[j] / 2 + 1;

      if(NumWordsToRead > LIST_MEMORY_LENGTH)
      {
        PL_ERR << "(read_EM_dbl): invalid word count " << NumWordsToRead;
        Pixie_Print_MSG(ErrMSG);
        return false;
      }
		
      // Read out the list mode data 
      Pixie_IOEM((U8)i, LIST_MEMORY_ADDRESS+Aoffset[j], MOD_READ, NumWordsToRead, (U32*)data);
    }
  }	// readout for loop
  
  // A second read of Pixie's word count register to clear the DBUF_1FULL bit
  // indicating to the DSP that the read is complete
  for(i=0; i<Number_Modules; i++)
    Pixie_RdWrdCnt((U8)i, &WordCount);
  return true;
};

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

void Wrapper::worker_fake(Simulator* source, SynchronizedQueue<Spill*>* data_queue,
                    uint64_t timeout_limit, boost::atomic<bool>* interruptor) {

  PL_INFO << "*Pixie Threads* Simulated event generator initiated";

  uint32_t secsperrun = 5;  ///calculate this based on ocr and buf size
  Spill* one_spill;

  uint64_t spill_count = 0, event_count = 0;
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
  one_spill->run->p4_state = source->settings;
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
    
    for (uint32_t i=0; i<(rate*secsperrun); i++)
      one_spill->hits.push_back(source->getHit());

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
  one_spill->run->p4_state = source->settings;
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

  pxi.my_settings_.set_mod("DBLBUFCSR",   static_cast<double>(0));
  pxi.my_settings_.set_mod("MODULE_CSRA", static_cast<double>(2));

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
  
  pxi.my_settings_.get_chan_stats();
  pxi.my_settings_.get_mod_stats();
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
      pxi.my_settings_.get_mod_stats();
      pxi.my_settings_.get_chan_stats();
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

  pxi.my_settings_.get_mod_stats();
  pxi.my_settings_.get_chan_stats();
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
  
  if (!pxi.clear_EM())
    return;

  pxi.my_settings_.set_mod("DBLBUFCSR",   static_cast<double>(1));
  pxi.my_settings_.set_mod("MODULE_CSRA", static_cast<double>(0));
  
  //start of run pixie status update
  //mainly for spectra to have calibration
  fetched_spill = new Spill;
  fetched_spill->run = new RunInfo;
  fetched_spill->stats = new StatsUpdate;

  pxi.my_settings_.get_all_settings();
  fetched_spill->run->p4_state = pxi.my_settings_;

  session_start_time = boost::posix_time::microsec_clock::local_time();
  
  if(!start_run(type)) {
    delete fetched_spill->run;
    delete fetched_spill->stats;
    delete fetched_spill;
    return;
  }

  total_timer.resume();

  pxi.my_settings_.get_chan_stats();
  pxi.my_settings_.get_mod_stats();
  fetched_spill->stats->eat_stats(pxi.my_settings_);
  fetched_spill->stats->lab_time = session_start_time;
  spill_queue->enqueue(fetched_spill);

  while (!timeout) {
    spill_count++;
    PL_INFO << "  RUNNING Elapsed: " << total_timer.done()
            << "  ETA: " << total_timer.ETA();

    csr = std::bitset<32>(poll_run_dbl());
    while ((!csr[14]) && (!timeout)) {
      wait_ms(poll_interval);
      csr = std::bitset<32>(poll_run_dbl());
      timeout = (total_timer.timeout() || interruptor->load());
    };
    block_time = boost::posix_time::microsec_clock::local_time();
    pxi.my_settings_.get_mod_stats();
    pxi.my_settings_.get_chan_stats();

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
  pxi.my_settings_.get_all_settings();

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

  Spill* spill;
 
  uint64_t all_events = 0, cycles = 0;
  CustomTimer parse_timer;

  while ((spill = in_queue->dequeue()) != NULL ) {
    parse_timer.resume();
    
    if (spill->data.size() > 0) {
      cycles++;
      uint16_t* buff16 = (uint16_t*) spill->data.data();
      uint32_t idx = 0, buf_end, spill_events = 0;

      while (true) {
        uint16_t buf_ndata  = buff16[idx++];
        uint32_t buf_end = idx + buf_ndata - 1;

        if (   (buf_ndata == 0)
               || (buf_ndata > max_buf_len)
               || (buf_end   > list_mem_len16))
          break;

        uint16_t buf_module = buff16[idx++];
        uint16_t buf_format = buff16[idx++];
        uint16_t buf_timehi = buff16[idx++];
        uint16_t buf_timemi = buff16[idx++];
        uint16_t buf_timelo = buff16[idx++];
        uint16_t task_a = (buf_format & 0x0F00);
        uint16_t task_b = (buf_format & 0x000F);

        while ((task_a == 0x0100) && (idx < buf_end)) {

          std::bitset<16> pattern (buff16[idx++]);

          Hit one_hit;
          one_hit.run_type  = buf_format;
          one_hit.module    = buf_module;
          one_hit.buf_time_hi = buf_timehi;
          one_hit.buf_time_mi = buf_timemi;
          one_hit.buf_time_lo = buf_timelo;
          one_hit.evt_time_hi = buff16[idx++];
          one_hit.evt_time_lo = buff16[idx++];
          one_hit.pattern = pattern;
          
          for (int i=0; i < 4; i++) {
            if (pattern[i]) {
              if (task_b == 0x0000) {
                uint16_t trace_len         = buff16[idx++] - 9;
                one_hit.chan_trig_time[i] = buff16[idx++];
                one_hit.energy[i]         = buff16[idx++];
                one_hit.XIA_PSA[i]        = buff16[idx++];
                one_hit.user_PSA[i]       = buff16[idx++];
                idx += 3;
                one_hit.chan_real_time[i] = buff16[idx++];
                one_hit.trace[i] = std::vector<uint16_t>
                    (buff16 + idx, buff16 + idx + trace_len);
                idx += trace_len;
              } else if (task_b == 0x0001) {
                idx++;
                one_hit.chan_trig_time[i] = buff16[idx++];
                one_hit.energy[i]         = buff16[idx++];
                one_hit.XIA_PSA[i]        = buff16[idx++];
                one_hit.user_PSA[i]       = buff16[idx++];
                idx += 3;
                one_hit.chan_real_time[i] = buff16[idx++];
              } else if (task_b == 0x0002) {
                one_hit.chan_trig_time[i] = buff16[idx++];
                one_hit.energy[i]         = buff16[idx++];
                one_hit.XIA_PSA[i]        = buff16[idx++];
                one_hit.user_PSA[i]       = buff16[idx++];
              } else if (task_b == 0x0003) {
                one_hit.chan_trig_time[i] = buff16[idx++];
                one_hit.energy[i]         = buff16[idx++];
              }
            }
          }
          spill->hits.push_back(one_hit);
          spill_events++;
        };
      }
      all_events += spill_events;
    }
    if (spill->run != nullptr) {
      spill->run->total_events = all_events;
    }
    spill->data.clear();
    out_queue->enqueue(spill);
    parse_timer.stop();
  }

  if (cycles == 0)
    PL_DBG << "   Parse worker done. Buffer queue closed without events";
  else
    PL_DBG << "   Parse worker done. " << all_events << " events, with avg time/spill: " << parse_timer.us()/cycles << "us";
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

}
