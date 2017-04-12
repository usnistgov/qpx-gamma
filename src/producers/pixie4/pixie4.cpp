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

#include "pixie4.h"
#include "producer_factory.h"
#include <boost/filesystem.hpp>
#include "custom_logger.h"
#include "custom_timer.h"

#define SLOT_WAVE_OFFSET      7
#define NUMBER_OF_CHANNELS    4

namespace Qpx {

void Pixie4::RunSetup::set_num_modules(uint16_t nmod)
{
  indices.resize(nmod);
  for (auto& i : indices)
    if (i.size() != NUMBER_OF_CHANNELS)
      i.resize(NUMBER_OF_CHANNELS, -1);
}

static ProducerRegistrar<Pixie4> registrar("Pixie4");

Pixie4::Pixie4()
{
  status_ = ProducerStatus::loaded | ProducerStatus::can_boot;
  boot_files_.resize(7);
  running_.store(false);
  terminating_.store(false);
}

Pixie4::~Pixie4()
{
  daq_stop();
  if (runner_ != nullptr)
  {
    runner_->detach();
    delete runner_;
  }
  if (parser_ != nullptr)
  {
    parser_->detach();
    delete parser_;
  }
  if (raw_queue_ != nullptr)
  {
    raw_queue_->stop();
    delete raw_queue_;
  }
}

bool Pixie4::daq_start(SpillQueue out_queue)
{
  if (running_.load())
    return false;

  terminating_.store(false);

  for (size_t i=0; i < run_setup.indices.size(); i++)
  {
    PixieAPI.clear_EM(i);
    // double-buffered:
    PixieAPI.set_mod(i, "DBLBUFCSR",   1);
    PixieAPI.set_mod(i, "MODULE_CSRA", 0);
  }

  for (size_t i=0; i < run_setup.indices.size(); ++i)
  {
    PixieAPI.set_mod(i, "RUN_TYPE", run_setup.type);
    PixieAPI.set_mod(i, "MAX_EVENTS", 0);
  }

  PixieAPI.reset_counters_next_run(); //assume new run

  raw_queue_ = new SynchronizedQueue<Spill*>();

  if (parser_ != nullptr)
    delete parser_;
  parser_ = new boost::thread(&worker_parse, run_setup, raw_queue_, out_queue);

  if (runner_ != nullptr)
    delete runner_;
  runner_ = new boost::thread(&worker_run_dbl, this, raw_queue_);

  return true;
}

bool Pixie4::daq_stop()
{
  if (!running_.load())
    return false;

  terminating_.store(true);

  if ((runner_ != nullptr) && runner_->joinable())
  {
    runner_->join();
    delete runner_;
    runner_ = nullptr;
  }

  wait_ms(500);
  while (raw_queue_->size() > 0)
    wait_ms(1000);
  wait_ms(500);
  raw_queue_->stop();
  wait_ms(500);

  if ((parser_ != nullptr) && parser_->joinable()) {
    parser_->join();
    delete parser_;
    parser_ = nullptr;
  }
  delete raw_queue_;
  raw_queue_ = nullptr;

  terminating_.store(false);
  return true;
}

bool Pixie4::daq_running()
{
  return (running_.load());
}

HitModel Pixie4::model_hit(uint16_t runtype)
{
  HitModel h;
  h.timebase = TimeStamp(1000, 75);
  h.add_value("energy", 16);
  h.add_value("front", 1);

  if (runtype < 259)
  {
    h.add_value("XIA_PSA", 16);
    h.add_value("user_PSA", 16);
  }

  if (runtype == 256)
    h.tracelength = 1024;
  else
    h.tracelength = 0;

  return h;
}

void Pixie4::fill_stats(std::map<int16_t, StatsUpdate> &all_stats, uint8_t module)
{
  StatsUpdate stats;
  stats.items["native_time"] = PixieAPI.get_mod(module, "TOTAL_TIME");
  stats.model_hit = model_hit(run_setup.type);
  //tracelength?!?!?!
  for (uint16_t i=0; i < run_setup.indices[module].size(); ++i)
  {
    stats.source_channel         = run_setup.indices[module][i];
    stats.items["trigger_count"] = PixieAPI.get_chan(module, i, "FAST_PEAKS");
    double live_time  = PixieAPI.get_chan(module, i, "LIVE_TIME");
    stats.items["live_time"]    = live_time -
        PixieAPI.get_chan(module, i, "SFDT");
    stats.items["live_trigger"] = live_time -
        PixieAPI.get_chan(module, i, "FTDT");
    all_stats[stats.source_channel] = stats;
  }
}


bool Pixie4::read_settings_bulk(Setting &set) const
{
  if (set.id_ != device_name())
    return false;

  for (auto &q : set.branches.my_data_) {
    if (set.metadata.setting_type == SettingType::command)
      set.metadata.writable =  ((status_ & ProducerStatus::booted) != 0);

    if (q.id_ == "Pixie4/Run settings")
    {
      for (auto &k : q.branches.my_data_)
      {
        if (k.id_ == "Pixie4/Run type")
          k.value_int = run_setup.type;
        if (k.id_ == "Pixie4/Poll interval")
          k.value_int = run_setup.run_poll_interval_ms;
      }
    }
    else if (q.id_ == "Pixie4/Files")
    {
      for (auto &k : q.branches.my_data_)
      {
        k.metadata.writable = !(status_ & ProducerStatus::booted);
        if (k.id_ == "Pixie4/Files/XIA_path")
          k.value_text = XIA_file_directory_;
        else if ((k.metadata.setting_type == SettingType::file_path) &&
                 (k.metadata.address > 0) && (k.metadata.address < 8))
          k.value_text = boot_files_[k.metadata.address - 1];
      }
    }
    else if (q.id_ == "Pixie4/System")
    {
      for (auto &k : q.branches.my_data_)
      {
        k.metadata.writable = (!(status_ & ProducerStatus::booted) &&
                               setting_definitions_.count(k.id_) &&
                               setting_definitions_.at(k.id_).writable);
        if (k.metadata.setting_type == SettingType::stem)
        {
          int16_t modnum = k.metadata.address;
          if (!PixieAPI.module_valid(modnum))
          {
            WARN << "<Pixie4> module address out of bounds, ignoring branch " << modnum;
            continue;
          }
          int filterrange = PixieAPI.get_mod(modnum, "FILTER_RANGE");
          for (auto &p : k.branches.my_data_)
          {
            if (p.metadata.setting_type == SettingType::stem)
            {
              int16_t channum = p.metadata.address;
              if (!PixieAPI.channel_valid(channum))
              {
                WARN << "<Pixie4> channel address out of bounds, ignoring branch " << channum;
                continue;
              }
              for (auto &o : p.branches.my_data_)
              {
                set_value(o, PixieAPI.get_chan(modnum, channum, o.metadata.address));
                if (o.metadata.name == "ENERGY_RISETIME")
                {
                  o.metadata.step = static_cast<double>(pow(2, filterrange)) / 75.0 ;
                  o.metadata.minimum = 2 * o.metadata.step;
                  o.metadata.maximum = 124 * o.metadata.step;
                }
                else if (o.metadata.name == "ENERGY_FLATTOP")
                {
                  o.metadata.step = static_cast<double>(pow(2, filterrange)) / 75.0;
                  o.metadata.minimum = 3 * o.metadata.step;
                  o.metadata.maximum = 125 * o.metadata.step;
                }
              }
            }
            else
              set_value(p, PixieAPI.get_mod(modnum, p.metadata.address));
          }
        }
        else
          set_value(k, PixieAPI.get_sys(k.metadata.address));
      }
    }
  }
  return true;
}

void Pixie4::rebuild_structure(Setting &set)
{
  auto maxmod = set.get_setting(Setting("Pixie4/System/MAX_NUMBER_MODULES"), Match::id);
  maxmod.value_int = std::min(std::max(int(maxmod.value_int), 1),
                              int(Pixie4Wrapper::hard_max_modules()));
  set.set_setting_r(maxmod, Match::id);

  auto oslots = set.find_all(Setting("Pixie4/System/SLOT_WAVE"), Match::id);
  std::vector<Setting> all_slots(oslots.begin(), oslots.end());

  all_slots.resize(maxmod.value_int, get_rich_setting("Pixie4/System/SLOT_WAVE"));

  for (size_t i=0;i < all_slots.size(); ++i)
    all_slots[i].metadata.address = SLOT_WAVE_OFFSET + i;

  set.del_setting(Setting("Pixie4/System/SLOT_WAVE"), Match::id);
  for (auto &q : all_slots)
    set.branches.add_a(q);

  auto totmod = set.get_setting(Setting("Pixie4/System/NUMBER_MODULES"), Match::id);
  totmod.value_int = 0;
  for (auto s : all_slots)
    if (s.value_int > 0)
      totmod.value_int++;
  set.set_setting_r(totmod, Match::id);

  auto omods = set.find_all(Setting("Pixie4/System/module"), Match::id);
  std::vector<Setting> old_modules(omods.begin(), omods.end());

  old_modules.resize(totmod.value_int, default_module());

  set.del_setting(Setting("Pixie4/System/module"), Match::id);
  for (auto &q : old_modules)
    set.branches.add_a(q);

  PixieAPI.set_num_modules(totmod.value_int);
  run_setup.set_num_modules(totmod.value_int);
}

Setting Pixie4::default_module() const
{
  auto chan = get_rich_setting("Pixie4/System/module/channel");
  auto mod = get_rich_setting("Pixie4/System/module");
  for (int j=0; j < NUMBER_OF_CHANNELS; ++j)
  {
    chan.metadata.address = j;
    mod.branches.add_a(chan);
  }
  return mod;
}

void Pixie4::reindex_modules(Setting &set)
{
  int module_address = 0;
  for (auto &m : set.branches.my_data_)
  {
    if (m.id_ != "Pixie4/System/module")
      continue;

    std::set<int32_t> new_set;
    int channel_address = 0;
    for (auto &c : m.branches.my_data_)
    {
      if (c.id_ != "Pixie4/System/module/channel")
        continue;

      if (c.indices.size() > 1)
      {
        int32_t i = *c.indices.begin();
        c.indices.clear();
        c.indices.insert(i);
      }
      if (c.indices.size() > 0)
        new_set.insert(*c.indices.begin());
      c.metadata.address = channel_address++;
    }

    m.indices = new_set;
    m.metadata.address = module_address++;
  }
}


bool Pixie4::write_settings_bulk(Setting &set)
{
  if (set.id_ != device_name())
    return false;

  bool ret = true;

  set.enrich(setting_definitions_);

  for (auto &q : set.branches.my_data_) {
    if ((q.metadata.setting_type == SettingType::command) && (q.value_int == 1))
    {
      q.value_int = 0;
      if (q.id_ == "Pixie4/Measure baselines")
        ret = PixieAPI.control_measure_baselines();
      else if (q.id_ == "Pixie4/Adjust offsets")
        ret = PixieAPI.control_adjust_offsets();
      else if (q.id_ == "Pixie4/Compute Tau")
        ret = PixieAPI.control_find_tau();
      else if (q.id_ == "Pixie4/Compute BLCUT")
        ret = PixieAPI.control_compute_BLcut();
    }
    else if ((q.id_ == "Pixie4/Files") && !(status_ & ProducerStatus::booted))
    {
      for (auto &k : q.branches.my_data_)
      {
        if (k.id_ == "Pixie4/Files/XIA_path")
        {
          if (XIA_file_directory_ != k.value_text)
          {
            if (!XIA_file_directory_.empty())
            {
              boost::filesystem::path path(k.value_text);
              for (auto &f : boot_files_)
              {
                boost::filesystem::path file(f);
                boost::filesystem::path full_path = path / file.filename();
                f = full_path.string();
              }
              break;
            }
            XIA_file_directory_ = k.value_text;
          }
        }
        else if ((k.metadata.setting_type == SettingType::file_path) &&
                 (k.metadata.address > 0) && (k.metadata.address < 8))
          boot_files_[k.metadata.address - 1] = k.value_text;
      }
    }
    else if (q.id_ == "Pixie4/Run settings")
    {
      for (auto &k : q.branches.my_data_)
      {
        if (k.id_ == "Pixie4/Run settings/Run type")
          run_setup.type = k.value_int;
        else if (k.id_ == "Pixie4/Run settings/Poll interval")
          run_setup.run_poll_interval_ms = k.value_int;
      }
    }
    else if (q.id_ == "Pixie4/System")
    {
      if (!(status_ & ProducerStatus::booted))
        rebuild_structure(q);

      reindex_modules(q);

      for (auto &k : q.branches.my_data_)
      {
        if (k.metadata.setting_type == SettingType::stem)
        {
          int16_t modnum = k.metadata.address;
          if (!PixieAPI.module_valid(modnum))
            continue;

          for (auto &p : k.branches.my_data_)
          {
            if (p.metadata.setting_type != SettingType::stem)
              p.indices = k.indices;

            if (p.metadata.setting_type == SettingType::stem)
            {
              int16_t channum = p.metadata.address;
              if (!Pixie4Wrapper::channel_valid(channum))
                continue;

              int det = -1;
              if (!p.indices.empty())
                det = *p.indices.begin();
              run_setup.indices[modnum][channum] = det;

              for (auto &o : p.branches.my_data_)
              {
                o.indices.clear();
                o.indices.insert(det);

                if (o.metadata.writable &&
                    (PixieAPI.get_chan(modnum, channum, o.metadata.address) != get_value(o)))
                  PixieAPI.set_chan(modnum, channum, o.metadata.name, get_value(o));
              }
            }
            else if (p.metadata.writable &&
                (PixieAPI.get_mod(modnum, p.metadata.address) != get_value(p)))
              PixieAPI.set_mod(modnum, p.metadata.name, get_value(p));
          }
        }
        else if (k.metadata.writable &&
            (PixieAPI.get_sys(k.metadata.address) != get_value(k)))
          PixieAPI.set_sys(k.metadata.name, get_value(k));
      }
    }
  }
  return ret;
}

bool Pixie4::boot()
{
  if (!(status_ & ProducerStatus::can_boot))
  {
    WARN << "<Pixie4> Cannot boot Pixie-4. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = ProducerStatus::loaded | ProducerStatus::can_boot;

  bool valid_files = true;
  for (int i=0; i < 7; i++)
    if (!boost::filesystem::exists(boot_files_[i]))
    {
      ERR << "<Pixie4> Boot file " << boot_files_[i] << " not found";
      valid_files = false;
    }

  if (!valid_files)
  {
    ERR << "<Pixie4> Problem with boot files. Boot aborting.";
    return false;
  }

  if (!PixieAPI.boot(boot_files_))
    return false;

  status_ = ProducerStatus::loaded | ProducerStatus::booted
      | ProducerStatus::can_run | ProducerStatus::can_oscil;
  return true;
}

bool Pixie4::die()
{
  daq_stop();
  status_ = ProducerStatus::loaded | ProducerStatus::can_boot;
  return true;
}

std::list<Hit> Pixie4::oscilloscope()
{
  std::list<Hit> result;

  uint32_t* oscil_data;

  for (size_t m=0; m < run_setup.indices.size(); ++m)
  {
    if ((oscil_data = PixieAPI.control_collect_ADC(m)) != nullptr)
    {
      for (size_t i = 0; i < run_setup.indices[m].size(); i++)
      {
        std::vector<uint16_t> trace = std::vector<uint16_t>(oscil_data + (i*Pixie4Wrapper::max_buf_len),
                                                            oscil_data + ((i+1)*Pixie4Wrapper::max_buf_len));
        if ((i < run_setup.indices[m].size()) &&
            (run_setup.indices[m][i] >= 0))
        {
          HitModel hm;
          hm.timebase = TimeStamp(PixieAPI.get_chan(m, i, "XDT") * 1000, 1); //us to ns
          hm.tracelength = Pixie4Wrapper::max_buf_len;
          Hit tr(run_setup.indices[m][i], hm);
          tr.set_trace(trace);
          result.push_back(tr);
        }
      }
    }

  }

  delete[] oscil_data;
  return result;
}

void Pixie4::get_all_settings()
{
  if (status_ & ProducerStatus::booted)
    PixieAPI.get_all_settings();
}

double Pixie4::get_value(const Setting& s)
{
  if (s.metadata.setting_type == SettingType::floating)
    return s.value_dbl;
  else if ((s.metadata.setting_type == SettingType::integer)
           || (s.metadata.setting_type == SettingType::boolean)
           || (s.metadata.setting_type == SettingType::int_menu)
           || (s.metadata.setting_type == SettingType::binary))
    return s.value_int;
}

void Pixie4::set_value(Setting& s, double val)
{
  if (s.metadata.setting_type == SettingType::floating)
    s.value_dbl = val;
  else if ((s.metadata.setting_type == SettingType::integer)
           || (s.metadata.setting_type == SettingType::boolean)
           || (s.metadata.setting_type == SettingType::int_menu)
           || (s.metadata.setting_type == SettingType::binary))
    s.value_int = val;
}

//////////////////////////////
//////////////////////////////
/////  T H R E A D S   ///////
//////////////////////////////
//////////////////////////////

void Pixie4::worker_run_dbl(Pixie4* callback, SpillQueue spill_queue)
{
  auto pixie = callback->PixieAPI;
  auto setup = callback->run_setup;
  Spill fetched_spill;

  //Start run;
  callback->running_.store(true);
  if(!pixie.start_run(setup.type))
  {
    callback->running_.store(false);
    return;
  }

  //Push spill with initial channel stats
  pixie.get_all_stats();
  for (size_t i=0; i < setup.indices.size(); i++)
    callback->fill_stats(fetched_spill.stats, i);
  for (auto &q : fetched_spill.stats)
  {
    q.second.lab_time = fetched_spill.time;
    q.second.stats_type = StatsUpdate::Type::start;
  }
  spill_queue->enqueue(new Spill(fetched_spill));

  //Main data acquisition loop
  bool timeout = false;
  std::set<uint16_t> triggered_modules;
  while (!timeout || !triggered_modules.empty())
  {
    //keep polling, pause if no trigger
    triggered_modules.clear();
    while (!timeout && triggered_modules.empty())
    {
      triggered_modules = pixie.get_triggered_modules();
      if (triggered_modules.empty())
        wait_ms(setup.run_poll_interval_ms);
      timeout = callback->terminating_.load();
    };

    //get time ASAP after trigger
    fetched_spill.time = boost::posix_time::microsec_clock::universal_time();

    //if timeout, stop run, wait, poll again
    if (timeout)
    {
      pixie.stop_run(setup.type);
      wait_ms(setup.run_poll_interval_ms);
      triggered_modules = pixie.get_triggered_modules();
    }

    //get stats ASAP
    pixie.get_all_settings(triggered_modules);

    //Read events, push spills
    bool success = false;
    for (auto &q : triggered_modules)
    {
      fetched_spill = Spill();
      fetched_spill.data.resize(Pixie4Wrapper::list_mem_len32, 0);
      if (pixie.read_EM_double_buffered(fetched_spill.data.data(), q))
        success = true;

      callback->fill_stats(fetched_spill.stats, q);
      for (auto &p : fetched_spill.stats)
      {
        p.second.lab_time = fetched_spill.time;
        if (timeout)
          p.second.stats_type = StatsUpdate::Type::stop;
      }
      spill_queue->enqueue(new Spill(fetched_spill));
    }

    if (!success)
      break;
  }

  //Push spill with end of acquisition stats, indicate stop
  fetched_spill = Spill();
  pixie.get_all_stats();
  for (size_t i=0; i < setup.indices.size(); i++)
    callback->fill_stats(fetched_spill.stats, i);
  for (auto &q : fetched_spill.stats)
  {
    q.second.lab_time = fetched_spill.time;
    q.second.stats_type = StatsUpdate::Type::stop;
  }
  spill_queue->enqueue(new Spill(fetched_spill));
  callback->running_.store(false);
}



void Pixie4::worker_parse (RunSetup setup,
                           SpillQueue in_queue,
                           SpillQueue out_queue)
{
  Spill* spill;
  auto run_setup = setup;

  uint64_t all_hits = 0, cycles = 0;
  CustomTimer parse_timer;

  while ((spill = in_queue->dequeue()) != NULL)
  {
    parse_timer.resume();

    if (spill->data.size() > 0)
    {
      cycles++;
      uint16_t* buff16 = (uint16_t*) spill->data.data();
      uint32_t idx = 0, spill_hits = 0;

      while (true)
      {
        uint16_t buf_ndata  = buff16[idx++];
        uint32_t buf_end = idx + buf_ndata - 1;

        if (   (buf_ndata == 0)
               || (buf_ndata > Pixie4Wrapper::max_buf_len)
               || (buf_end   > Pixie4Wrapper::list_mem_len16))
          break;

        uint16_t buf_module = buff16[idx++];
        uint16_t buf_format = buff16[idx++];
        uint16_t buf_timehi = buff16[idx++];
        uint16_t buf_timemi = buff16[idx++];
        idx++; //uint16_t buf_timelo = buff16[idx++]; unused
        uint16_t task_a = (buf_format & 0x0F00);
        uint16_t task_b = (buf_format & 0x000F);

        while ((task_a == 0x0100) && (idx < buf_end))
        {
          std::bitset<16> pattern (buff16[idx++]);

          uint16_t evt_time_hi = buff16[idx++];
          uint16_t evt_time_lo = buff16[idx++];

          std::multiset<Hit> ordered;

          for (size_t i=0; i < NUMBER_OF_CHANNELS; i++)
          {
            if (!pattern[i])
              continue;

            int16_t sourcechan = -1;
            if ((buf_module < run_setup.indices.size()) &&
                (i < run_setup.indices[buf_module].size()) &&
                (run_setup.indices[buf_module][i] >= 0))
              sourcechan = run_setup.indices[buf_module][i];

            Hit one_hit(sourcechan, model_hit(run_setup.type));

            uint64_t hi = buf_timehi;
            uint64_t mi = evt_time_hi;
            uint64_t lo = evt_time_lo;
            uint16_t chan_trig_time = lo;
            uint16_t chan_time_hi   = hi;

            one_hit.set_value(1, pattern[4]); //Front panel input value

            if (task_b == 0x0000)
            {
              uint16_t trace_len  = buff16[idx++] - 9;
              one_hit.set_value(0, buff16[idx++]); //energy
              one_hit.set_value(2, buff16[idx++]); //XIA_PSA
              one_hit.set_value(3, buff16[idx++]); //user_PSA
              idx += 3;
              hi                  = buff16[idx++]; //not always?
              one_hit.set_trace(std::vector<uint16_t>(buff16 + idx, buff16 + idx + trace_len));
              idx += trace_len;
            }
            else if (task_b == 0x0001)
            {
              idx++;
              chan_trig_time      = buff16[idx++];
              one_hit.set_value(0, buff16[idx++]); //energy
              one_hit.set_value(2, buff16[idx++]); //XIA_PSA
              one_hit.set_value(3, buff16[idx++]); //user_PSA
              idx += 3;
              hi                  = buff16[idx++];
            }
            else if (task_b == 0x0002)
            {
              chan_trig_time      = buff16[idx++];
              one_hit.set_value(0, buff16[idx++]); //energy
              one_hit.set_value(2, buff16[idx++]); //XIA_PSA
              one_hit.set_value(3, buff16[idx++]); //user_PSA
            }
            else if (task_b == 0x0003)
            {
              chan_trig_time      = buff16[idx++];
              one_hit.set_value(0, buff16[idx++]); //energy
            }
            else
              ERR << "<Pixie4::parser> Parsed event type invalid";

            if (!pattern[i+8])
              one_hit.set_value(0, 0); //energy invalid or approximate

            //Corrections for overflow, page 30 in Pixie-4 user manual
            if (chan_trig_time > evt_time_lo)
              mi--;
            if (evt_time_hi < buf_timemi)
              hi++;
            if ((task_b == 0x0000) || (task_b == 0x0001))
              hi = chan_time_hi;
            lo = chan_trig_time;
            uint64_t time = (hi << 32) + (mi << 16) + lo;

            one_hit.set_timestamp_native(time);

            if (sourcechan >= 0)
              ordered.insert(one_hit);
          }

          for (auto &q : ordered)
          {
            spill->hits.push_back(q);
            spill_hits++;
          }

        };
      }
      all_hits += spill_hits;
    }
    spill->data.clear();
    out_queue->enqueue(spill);
    parse_timer.stop();
  }

  if (cycles == 0)
    DBG << "<Pixie4::parser> Buffer queue closed without events";
  else
    DBG << "<Pixie4::parser> Parsed " << all_hits << " hits, with avg time/spill: " << parse_timer.us()/cycles << "us";
}

}
