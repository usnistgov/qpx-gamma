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

#include "simulator2d.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "custom_timer.h"

#include "project.h"
#include "daq_source_factory.h"


namespace Qpx {

static SourceRegistrar<Simulator2D> registrar("Simulator2D");

Simulator2D::Simulator2D() {
  status_ = SourceStatus::loaded;// | SourceStatus::can_boot;
  runner_ = nullptr;
  run_status_.store(0);

  bits_  = 4;
  spill_interval_ = 5;
  scale_rate_ = 1.0;
  chan0_ = 0;
  chan1_ = 1;
  coinc_thresh_ = 3;
  gain0_ = 100;
  gain1_ = 100;
}

bool Simulator2D::die() {

  status_ = SourceStatus::loaded; // | SourceStatus::can_boot;
  //  for (auto &q : set.branches.my_data_) {
  //    if ((q.metadata.setting_type == Qpx::SettingType::file_path) && (q.id_ == "Simulator2D/Source file"))
  //      q.metadata.writable = true;
  //  }

  return true;
}


Simulator2D::~Simulator2D() {
  daq_stop();
  if (runner_ != nullptr) {
    runner_->detach();
    delete runner_;
  }
  die();
}

bool Simulator2D::daq_start(SynchronizedQueue<Spill*>* out_queue) {
  if (run_status_.load() > 0)
    return false;

  run_status_.store(1);

  if (runner_ != nullptr)
    delete runner_;

  runner_ = new boost::thread(&worker_run, this, out_queue);

  return true;
}

bool Simulator2D::daq_stop() {
  if (run_status_.load() == 0)
    return false;

  run_status_.store(2);

  if ((runner_ != nullptr) && runner_->joinable()) {
    runner_->join();
    delete runner_;
    runner_ = nullptr;
  }

  wait_ms(500);

  run_status_.store(0);
  return true;
}

bool Simulator2D::daq_running() {
  if (run_status_.load() == 3)
    daq_stop();
  return (run_status_.load() > 0);
}



bool Simulator2D::read_settings_bulk(Qpx::Setting &set) const {
  if (set.id_ == device_name()) {
    for (auto &q : set.branches.my_data_) {
      if ((q.metadata.setting_type == Qpx::SettingType::integer) && (q.id_ == "Simulator2D/SpillInterval"))
        q.value_int = spill_interval_;
      else if ((q.metadata.setting_type == Qpx::SettingType::integer) && (q.id_ == "Simulator2D/Resolution"))
        q.value_int = bits_;
      else if ((q.metadata.setting_type == Qpx::SettingType::floating) && (q.id_ == "Simulator2D/ScaleRate"))
        q.value_dbl = scale_rate_;
      else if ((q.metadata.setting_type == Qpx::SettingType::floating) && (q.id_ == "Simulator2D/Gain0")) {
        q.value_dbl = gain0_;
        q.indices.clear();
        q.indices.insert(chan0_);
      }
      else if ((q.metadata.setting_type == Qpx::SettingType::floating) && (q.id_ == "Simulator2D/Gain1")) {
        q.value_dbl = gain1_;
        q.indices.clear();
        q.indices.insert(chan1_);
      }
      else if ((q.metadata.setting_type == Qpx::SettingType::integer) && (q.id_ == "Simulator2D/CoincThresh"))
        q.value_int = coinc_thresh_;
      else if ((q.metadata.setting_type == Qpx::SettingType::floating) && (q.id_ == "Simulator2D/TimebaseMult"))
        q.value_dbl = model_hit.timebase.timebase_multiplier();
      else if ((q.metadata.setting_type == Qpx::SettingType::floating) && (q.id_ == "Simulator2D/TimebaseDiv"))
        q.value_dbl = model_hit.timebase.timebase_divider();
      else if ((q.metadata.setting_type == Qpx::SettingType::file_path) && (q.id_ == "Simulator2D/Source file"))
        q.value_text = source_file_;
      else if ((q.metadata.setting_type == Qpx::SettingType::int_menu) && (q.id_ == "Simulator2D/Source spectrum")) {
        q.metadata.int_menu_items = spectra_names_;
        if (spectra_names_.count(source_spectrum_))
          q.value_int = source_spectrum_;
        else
          q.value_int = 0;
      }
    }
  }
  return true;
}


bool Simulator2D::write_settings_bulk(Qpx::Setting &set) {

  if (set.id_ != device_name())
    return false;

  double timebase_multiplier = model_hit.timebase.timebase_multiplier();
  double timebase_divider = model_hit.timebase.timebase_divider();

  for (auto &q : set.branches.my_data_) {
    if (q.id_ == "Simulator2D/SpillInterval")
      spill_interval_ = q.value_int;
    else if (q.id_ == "Simulator2D/Resolution")
    {
      bits_ = q.value_int;
      if (bits_ > 16)
        bits_ = 16;
    }
    else if (q.id_ == "Simulator2D/Gain0") {
      gain0_ = q.value_dbl;
      //      if (gain0_ > 100)
      //        gain0_ = 100;
      chan0_ = -1;
      for (auto &i : q.indices)
        chan0_ = i;
    }
    else if (q.id_ == "Simulator2D/Gain1") {
      gain1_ = q.value_dbl;
      chan1_ = -1;
      for (auto &i : q.indices)
        chan1_ = i;
    }
    else if (q.id_ == "Simulator2D/CoincThresh")
      coinc_thresh_ = q.value_int;
    else if (q.id_ == "Simulator2D/TimebaseMult")
      timebase_multiplier = q.value_dbl;
    else if (q.id_ == "Simulator2D/TimebaseDiv")
      timebase_divider = q.value_dbl;
    else if (q.id_ == "Simulator2D/ScaleRate")
      scale_rate_ = q.value_dbl;
    else if (q.id_ == "Simulator2D/Source file") {
      if (q.value_text != source_file_) {
        Qpx::Project temp_set;
        temp_set.read_xml(q.value_text, true, false);

        spectra_names_.clear();

        for (auto &q: temp_set.get_sinks(2)) {
          Qpx::Metadata md = q.second->metadata();
//          DBG << "<Simulator2D> Spectrum available: " << md.name << " t:" << md.type() << " r:" << md.bits;
          spectra_names_[q.first] = md.get_attribute("name").value_text
              + " ("
              + md.get_attribute("resolution").val_to_pretty_string()
              + ")";
        }

        if (!spectra_names_.empty())
          status_ = status_ | SourceStatus::can_boot;
      }
      source_file_ = q.value_text;
    }
    else if (q.id_ == "Simulator2D/Source spectrum") {
      if (!spectra_names_.count(q.value_int))
        q.value_int = 0;
      source_spectrum_ = q.value_int;
    }
  }

  model_hit = HitModel();
  model_hit.timebase = TimeStamp(timebase_multiplier, timebase_divider);
  model_hit.add_value("energy", 16);
  model_hit.add_value("junk", 16);
  model_hit.tracelength = 200;

  set.enrich(setting_definitions_);

  return true;
}

bool Simulator2D::boot() {
  if (!(status_ & SourceStatus::can_boot)) {
    WARN << "<Simulator2D> Cannot boot Simulator2D. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = SourceStatus::loaded | SourceStatus::can_boot;

  Qpx::Project temp_set;
  LINFO << "<Simulator2D> Reading data from " << source_file_;
  temp_set.read_xml(source_file_, true);

  SinkPtr spectrum = temp_set.get_sink(source_spectrum_);

  if (spectrum == nullptr) {
    DBG << "<Simulator2D> Could not find selected spectrum";
    return false;
  }

  Metadata md = spectrum->metadata();
  Setting sres = md.get_attribute("resolution");
  LINFO << "<Simulator2D> Will use " << md.get_attribute("name").value_text << " type:" << md.type() << " bits:" << sres.value_int;


  int source_res = sres.value_int;
  lab_time = md.get_attribute("real_time").value_duration.total_milliseconds() * 0.001;
  live_time = md.get_attribute("live_time").value_duration.total_milliseconds() * 0.001;

  if (lab_time == 0.0) {
    WARN << "<Simulator2D> Lab time = 0. Cannot create simulation.";
    return false;
  }

  double totevts = md.get_attribute("total_events").value_precise.convert_to<double>();
  OCR = totevts / lab_time;

  int adjust_bits = source_res - bits_;

  shift_by_ = 16 - bits_;
  resolution_ = pow(2, bits_);

  LINFO << "<Simulator2D> Building matrix for simulation from [" << md.get_attribute("name").value_text << "]"
       << " resolution=" << resolution_ << " shift="  << shift_by_
       << " rate=" << OCR << "cps";
  std::vector<double> distribution(resolution_*resolution_, 0.0);   //optimize somehow

  uint32_t res = pow(2, source_res);
  std::unique_ptr<std::list<Qpx::Entry>> spec_list(spectrum->data_range({{0,res},{0,res}}));

  if (adjust_bits >= 0)
  {
    for (auto it : *spec_list)
      distribution[(it.first[0] >> adjust_bits) * resolution_
          + (it.first[1] >> adjust_bits)]
          =  static_cast<double>(it.second) / totevts;
  }
  else
  {
    for (auto it : *spec_list)
      distribution[(it.first[0] << (-adjust_bits)) * resolution_
          + (it.first[1] << (-adjust_bits))]
          =  static_cast<double>(it.second) / totevts;
  }

  LINFO << "<Simulator2D> Creating discrete distribution for simulation";
  dist_ = boost::random::discrete_distribution<>(distribution);

  if (shift_by_) {
    int size_granular = pow(2, shift_by_);
    std::vector<double> distri_granular(size_granular, 1.0);
    refined_dist_ = boost::random::discrete_distribution<>(distri_granular);
    //Not physically accurate, of course
  }
  clock_ = 0;
  valid_ = true;

  status_ = SourceStatus::loaded | SourceStatus::booted | SourceStatus::can_run;
  return true;
}


void Simulator2D::get_all_settings() {
  if (status_ & SourceStatus::booted) {
  }
}


void Simulator2D::worker_run(Simulator2D* callback, SynchronizedQueue<Spill*>* spill_queue) {
  bool timeout = false;

  uint64_t   rate = callback->OCR * callback->scale_rate_;
  StatsUpdate moving_stats,
      one_run = callback->getBlock(callback->spill_interval_ * 0.999);

  Spill one_spill;

  DBG << "<Simulator2D> Start run   "
      << "  gains " << callback->gain0_ << " " << callback->gain1_
      << "  timebase " << callback->model_hit.timebase.to_string() << "ns"
      << "  rate " << rate << " cps";

  one_spill = Spill();
  moving_stats.model_hit = callback->model_hit;
  moving_stats.stats_type = StatsType::start;
  moving_stats.lab_time = boost::posix_time::microsec_clock::universal_time();

  moving_stats.source_channel = callback->chan0_;
  one_spill.stats[callback->chan0_] = moving_stats;
  moving_stats.source_channel = callback->chan1_;
  one_spill.stats[callback->chan1_] = moving_stats;

  spill_queue->enqueue(new Spill(one_spill));

  while (!timeout) {
    boost::this_thread::sleep(boost::posix_time::seconds(callback->spill_interval_));

    one_spill = Spill();

    for (uint32_t i=0; i< (rate * callback->spill_interval_); i++) {
      if (callback->resolution_ > 0) {
        uint64_t newpoint = callback->dist_(callback->gen);
        int32_t en1 = newpoint / callback->resolution_;
        int32_t en2 = newpoint % callback->resolution_;

        en1 = en1 << callback->shift_by_;
        en2 = en2 << callback->shift_by_;

        if (callback->shift_by_) {
          en1 += callback->refined_dist_(callback->gen);
          en2 += callback->refined_dist_(callback->gen);
        }

        callback->push_hit(one_spill, en1, en2);
      }
    }

    moving_stats = moving_stats + one_run;
    moving_stats.model_hit = callback->model_hit;
    moving_stats.stats_type = StatsType::running;
    moving_stats.lab_time = boost::posix_time::microsec_clock::universal_time();

//    DBG << "pushing with model " << moving_stats.model_hit.to_string();

    moving_stats.source_channel = callback->chan0_;
    one_spill.stats[callback->chan0_] = moving_stats;
    moving_stats.source_channel = callback->chan1_;
    one_spill.stats[callback->chan1_] = moving_stats;

    spill_queue->enqueue(new Spill(one_spill));

    timeout = (callback->run_status_.load() == 2);
  }

  one_spill.hits.clear();
  for (auto &q : one_spill.stats)
    q.second.stats_type = StatsType::stop;

  spill_queue->enqueue(new Spill(one_spill));

  callback->run_status_.store(3);

  //  DBG << "<Simulator2D> Stop run worker";
}

void Simulator2D::push_hit(Spill& one_spill, uint16_t en1, uint16_t en2)
{
  if (en1 > 0) {
    Hit h(chan0_, model_hit);
    h.set_timestamp_native(clock_);
    h.set_value(0, round(en1 * gain0_ * 0.01));
    h.set_value(1, rand() % 100);
    make_trace(h, 1000);
    one_spill.hits.push_back(h);
  }

  if (en2 > 0) {
    Hit h(chan1_, model_hit);
    h.set_timestamp_native(clock_);
    h.set_value(0, round(en2 * gain1_ * 0.01));
    h.set_value(1, rand() % 100);
    make_trace(h, 1000);
    one_spill.hits.push_back(h);
  }

  clock_ += coinc_thresh_ + 1;
}

void Simulator2D::make_trace(Hit& h, uint16_t baseline)
{
  uint16_t en = h.value(0).val(h.value(0).bits());
  std::vector<uint16_t> trc(h.trace().size(), baseline);
  size_t start = double(trc.size()) * 0.1;
  double slope1 = double(en) / double(start);
  double slope2 = - double(en) / double(trc.size() * 10);
  for (size_t i = 0; i < start; ++i)
    trc[start+i] += i*slope1;
  for (size_t i = start*2; i < trc.size(); ++i)
    trc[i] += en + (i - 2*start) * slope2;
  for (size_t i=0; i < trc.size(); ++i)
    trc[i] += (rand() % baseline) / 5 - baseline/10;
  h.set_trace(trc);
}



Spill Simulator2D::get_spill() {
  Spill one_spill;

  return one_spill;
}

StatsUpdate Simulator2D::getBlock(double duration) {
  StatsUpdate newBlock;

  double fraction;

  newBlock.items["native_time"] = duration;

  if (lab_time == 0.0)
    fraction = duration;
  else
    fraction = duration / lab_time;

  if (std::isfinite(live_time) && (live_time > 0))
  {
    newBlock.items["live_time"] = live_time;
    newBlock.items["live_trigger"] = live_time;
  }

  return newBlock;
}


}
