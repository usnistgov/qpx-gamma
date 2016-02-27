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

#include "spectra_set.h"


namespace Qpx {

static DeviceRegistrar<Simulator2D> registrar("Simulator2D");

Simulator2D::Simulator2D() {
  status_ = DeviceStatus::loaded;// | DeviceStatus::can_boot;
  runner_ = nullptr;
  run_status_.store(0);

  bits_  = 4;
  spill_interval_ = 5;
  scale_rate_ = 1.0;
  chan0_ = 0;
  chan1_ = 1;
  coinc_thresh_ = 3;
  gain0_ = 1;
  gain1_ = 1;
}

bool Simulator2D::die() {

  status_ = DeviceStatus::loaded; // | DeviceStatus::can_boot;
//  for (auto &q : set.branches.my_data_) {
//    if ((q.metadata.setting_type == Gamma::SettingType::file_path) && (q.id_ == "Simulator2D/Source file"))
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



bool Simulator2D::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ == device_name()) {
    for (auto &q : set.branches.my_data_) {
      if ((q.metadata.setting_type == Gamma::SettingType::integer) && (q.id_ == "Simulator2D/SpillInterval"))
        q.value_int = spill_interval_;
      else if ((q.metadata.setting_type == Gamma::SettingType::integer) && (q.id_ == "Simulator2D/Resolution"))
        q.value_int = bits_;
      else if ((q.metadata.setting_type == Gamma::SettingType::floating) && (q.id_ == "Simulator2D/ScaleRate"))
        q.value_dbl = scale_rate_;
      else if ((q.metadata.setting_type == Gamma::SettingType::integer) && (q.id_ == "Simulator2D/Gain0")) {
        q.value_dbl = gain0_;
        q.indices.clear();
        q.indices.insert(chan0_);
      }
      else if ((q.metadata.setting_type == Gamma::SettingType::integer) && (q.id_ == "Simulator2D/Gain1")) {
        q.value_dbl = gain1_;
        q.indices.clear();
        q.indices.insert(chan1_);
      }
      else if ((q.metadata.setting_type == Gamma::SettingType::integer) && (q.id_ == "Simulator2D/CoincThresh"))
        q.value_int = coinc_thresh_;
      else if ((q.metadata.setting_type == Gamma::SettingType::floating) && (q.id_ == "Simulator2D/TimebaseMult"))
        q.value_dbl = model_hit.timestamp.timebase_multiplier;
      else if ((q.metadata.setting_type == Gamma::SettingType::floating) && (q.id_ == "Simulator2D/TimebaseDiv"))
        q.value_dbl = model_hit.timestamp.timebase_divider;
      else if ((q.metadata.setting_type == Gamma::SettingType::file_path) && (q.id_ == "Simulator2D/Source file"))
        q.value_text = source_file_;
      else if ((q.metadata.setting_type == Gamma::SettingType::int_menu) && (q.id_ == "Simulator2D/Source spectrum")) {
        q.metadata.int_menu_items.clear();
        q.value_int = 0;

        for (int i=0; i < spectra_.size(); ++i) {
          q.metadata.int_menu_items[i] = spectra_[i];
          if (spectra_[i] == source_spectrum_)
            q.value_int = i;
        }

      }
    }
  }
  return true;
}


bool Simulator2D::write_settings_bulk(Gamma::Setting &set) {

  if (set.id_ != device_name())
    return false;

  for (auto &q : set.branches.my_data_) {
    if (q.id_ == "Simulator2D/SpillInterval")
      spill_interval_ = q.value_int;
    else if (q.id_ == "Simulator2D/Resolution")
      bits_ = q.value_int;
    else if (q.id_ == "Simulator2D/Gain0") {
      gain0_ = q.value_dbl;
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
      model_hit.timestamp.timebase_multiplier = q.value_dbl;
    else if (q.id_ == "Simulator2D/TimebaseDiv")
      model_hit.timestamp.timebase_divider = q.value_dbl;
    else if (q.id_ == "Simulator2D/ScaleRate")
      scale_rate_ = q.value_dbl;
    else if (q.id_ == "Simulator2D/Source file") {
      if (q.value_text != source_file_) {
        Qpx::SpectraSet* temp_set = new Qpx::SpectraSet;
        temp_set->read_xml(q.value_text, true, false);

        spectra_.clear();

        for (auto &q: temp_set->spectra(2, -1)) {
          Qpx::Spectrum::Metadata md = q->metadata();
          PL_DBG << "<Simulator2D> Spectrum available: " << md.name << " t:" << md.type << " r:" << md.bits;
          spectra_.push_back(md.name);
        }

        if (!spectra_.empty())
          status_ = status_ | DeviceStatus::can_boot;

        delete temp_set;
      }
      source_file_ = q.value_text;
    }
    else if (q.id_ == "Simulator2D/Source spectrum")
      source_spectrum_ = q.metadata.int_menu_items[q.value_int];
  }

  set.enrich(setting_definitions_);

  return true;
}

bool Simulator2D::boot() {
  if (!(status_ & DeviceStatus::can_boot)) {
    PL_WARN << "<Simulator2D> Cannot boot Simulator2D. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  Qpx::SpectraSet* temp_set = new Qpx::SpectraSet;
  PL_INFO << "<Simulator2D> Reading data from " << source_file_;
  temp_set->read_xml(source_file_, true);

  Spectrum::Spectrum* spectrum = temp_set->by_name(source_spectrum_);

  if (spectrum == nullptr) {
    PL_DBG << "<Simulator2D> Could not find spectrum " << source_spectrum_;
    delete temp_set;
    return false;
  }

  Spectrum::Metadata md = spectrum->metadata();
  PL_INFO << "<Simulator2D> Will use " << md.name << " type:" << md.type << " bits:" << md.bits;


  int source_res = md.bits;

  PL_DBG << "<Simulator2D> source total events coinc " << md.total_count;

  settings = temp_set->runInfo().state;
  detectors = md.detectors;
  lab_time = md.real_time.total_milliseconds() * 0.001;

  if (lab_time == 0.0) {
    PL_WARN << "<Simulator2D> Lab time = 0. Cannot create simulation.";
    delete temp_set;
    return false;
  }

  count_ = md.total_count;

  time_factor = settings.get_setting(Gamma::Setting("Pixie4/System/module/TOTAL_TIME"), Gamma::Match::id).value_dbl / lab_time;
  OCR = static_cast<double>(count_) / lab_time;
  PL_DBG << "<Simulator2D> total count=" << count_ << " time_factor=" << time_factor << " OCR=" << OCR;

  int adjust_bits = source_res - bits_;

  shift_by_ = 16 - bits_;
  resolution_ = pow(2, bits_);

  PL_INFO << "<Simulator2D>  building matrix for simulation res=" << resolution_ << " shift="  << shift_by_;
  std::vector<double> distribution(resolution_*resolution_, 0.0);   //optimize somehow

  std::unique_ptr<std::list<Qpx::Spectrum::Entry>> spec_list(spectrum->get_spectrum({{0,spectrum->resolution()},{0,spectrum->resolution()}}));

  for (auto it : *spec_list)
    distribution[(it.first[0] >> adjust_bits) * resolution_
                 + (it.first[1] >> adjust_bits)]
        =  static_cast<double>(it.second) / static_cast<double> (count_);

  PL_INFO << "<Simulator2D>  creating discrete distribution for simulation";
  dist_ = boost::random::discrete_distribution<>(distribution);

  if (shift_by_) {
    int size_granular = pow(2, shift_by_);
    std::vector<double> distri_granular(size_granular, 1.0);
    refined_dist_ = boost::random::discrete_distribution<>(distri_granular);
    //Not physically accurate, of course
  }
  valid_ = true;







  delete temp_set;

  status_ = DeviceStatus::loaded | DeviceStatus::booted | DeviceStatus::can_run;
//  for (auto &q : set.branches.my_data_) {
//    if ((q.metadata.setting_type == Gamma::SettingType::file_path) && (q.id_ == "Simulator2D/Source file"))
//      q.metadata.writable = false;
//    if ((q.metadata.setting_type == Gamma::SettingType::file_path) && (q.id_ == "Simulator2D/Source spectrum"))
//      q.metadata.writable = false;
//  }

  return true;
}


void Simulator2D::get_all_settings() {
  if (status_ & DeviceStatus::booted) {
  }
}


void Simulator2D::worker_run(Simulator2D* callback, SynchronizedQueue<Spill*>* spill_queue) {
  PL_DBG << "<Simulator2D> Start run worker";


  bool timeout = false;

  uint64_t   rate = callback->OCR * callback->scale_rate_;
  uint64_t   t = 1;
  StatsUpdate moving_stats,
      one_run = callback->getBlock(callback->spill_interval_ * 0.999);

  Spill one_spill;


  PL_DBG << "<Simulator2D> gains " << callback->gain0_ << " " << callback->gain1_;

  std::set<int> starts_signalled;

  one_spill = Spill();
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

        Hit h = callback->model_hit;
        h.timestamp.time_native = t;

//        PL_DBG << "evt " << en1 << "x" << en2;

        if (en1 > 0) {
          h.source_channel = callback->chan0_;
          h.energy.set_val(round(en1 * callback->gain0_ * 0.01));
          one_spill.hits.push_back(h);
        }

        if (en2 > 0) {
          h.source_channel = callback->chan1_;
          h.energy.set_val(round(en2 * callback->gain1_ * 0.01));
          one_spill.hits.push_back(h);
        }

        t += callback->coinc_thresh_ + 1;
      }
    }

    moving_stats = moving_stats + one_run;
    moving_stats.stats_type = StatsType::running;
    moving_stats.lab_time = boost::posix_time::microsec_clock::universal_time();
    moving_stats.event_rate = one_run.event_rate;

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

  PL_DBG << "<Simulator2D> Stop run worker";
}



Spill Simulator2D::get_spill() {
  Spill one_spill;

  return one_spill;
}

StatsUpdate Simulator2D::getBlock(double duration) {
  StatsUpdate newBlock;
  newBlock.model_hit = model_hit;

  double fraction;

  newBlock.total_time = duration * time_factor;
  newBlock.event_rate = duration * OCR;

  if (lab_time == 0.0)
    fraction = duration;
  else
    fraction = duration / lab_time;


  //one channel only, find by indices?
  //  for (int i = 0; i<2; i++) {
    newBlock.fast_peaks = settings.get_setting(Gamma::Setting("FAST_PEAKS"), Gamma::Match::name).value_dbl * fraction;
    newBlock.live_time  = settings.get_setting(Gamma::Setting("LIVE_TIME"), Gamma::Match::name).value_dbl * fraction;
    newBlock.ftdt       = settings.get_setting(Gamma::Setting("FTDT"), Gamma::Match::name).value_dbl * fraction;
    newBlock.sfdt       = settings.get_setting(Gamma::Setting("SFDT"), Gamma::Match::name).value_dbl * fraction;
    //  }


  return newBlock;
}


}