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
 *      Qpx::ParserRaw
 *
 ******************************************************************************/

#include "parser_raw.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "custom_timer.h"
#include "daq_source_factory.h"


namespace Qpx {

static SourceRegistrar<ParserRaw> registrar("ParserRaw");

ParserRaw::ParserRaw() {
  status_ = SourceStatus::loaded | SourceStatus::can_boot;

  runner_ = nullptr;

  run_status_.store(0);

  loop_data_ = false;
  override_timestamps_= false;
}

bool ParserRaw::die() {
  if ((status_ & SourceStatus::booted) != 0)
    file_bin_.close();

  source_file_bin_.clear();

  spills_.clear();
  spills2_.clear();
  bin_begin_ = 0;
  bin_end_ = 0;

  status_ = SourceStatus::loaded | SourceStatus::can_boot;
//  for (auto &q : set.branches.my_data_) {
//    if ((q.metadata.setting_type == Qpx::SettingType::file_path) && (q.id_ == "ParserRaw/Source file"))
//      q.metadata.writable = true;
//  }

  return true;
}


ParserRaw::~ParserRaw() {
  daq_stop();
  if (runner_ != nullptr) {
    runner_->detach();
    delete runner_;
  }
  die();
}

bool ParserRaw::daq_start(SynchronizedQueue<Spill*>* out_queue) {
  if (run_status_.load() > 0)
    return false;

  run_status_.store(1);

  if (runner_ != nullptr)
    delete runner_;

  runner_ = new boost::thread(&worker_run, this, out_queue);

  return true;
}

bool ParserRaw::daq_stop() {
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

bool ParserRaw::daq_running() {
  if (run_status_.load() == 3)
    daq_stop();
  return (run_status_.load() > 0);
}



bool ParserRaw::read_settings_bulk(Qpx::Setting &set) const {
  if (set.id_ == device_name()) {
    for (auto &q : set.branches.my_data_) {
      if ((q.metadata.setting_type == Qpx::SettingType::boolean) && (q.id_ == "ParserRaw/Override timestamps"))
        q.value_int = override_timestamps_;
      else if ((q.metadata.setting_type == Qpx::SettingType::boolean) && (q.id_ == "ParserRaw/Loop data"))
        q.value_int = loop_data_;
      else if ((q.metadata.setting_type == Qpx::SettingType::boolean) && (q.id_ == "ParserRaw/Override pause"))
        q.value_int = override_pause_;
      else if ((q.metadata.setting_type == Qpx::SettingType::integer) && (q.id_ == "ParserRaw/Pause"))
        q.value_int = pause_ms_;
      else if ((q.metadata.setting_type == Qpx::SettingType::file_path) && (q.id_ == "ParserRaw/Source file")) {
        q.value_text = source_file_;
        q.metadata.writable = !(status_ & SourceStatus::booted);
      }
      else if ((q.metadata.setting_type == Qpx::SettingType::file_path) && (q.id_ == "ParserRaw/Binary file"))
        q.value_text = source_file_bin_;
      else if ((q.metadata.setting_type == Qpx::SettingType::integer) && (q.id_ == "ParserRaw/StatsUpdates"))
        q.value_int = spills_.size();
      else if ((q.metadata.setting_type == Qpx::SettingType::integer) && (q.id_ == "ParserRaw/Hits"))
        q.value_int = (bin_end_ - bin_begin_) / 12;
      else if ((q.metadata.setting_type == Qpx::SettingType::time) && (q.id_ == "ParserRaw/StartTime")) {
        if (!spills_.empty())
          q.value_time = spills_.front().time;
      }
      else if ((q.metadata.setting_type == Qpx::SettingType::time_duration) && (q.id_ == "ParserRaw/RunDuration")) {
        if (!spills_.empty())
          q.value_duration = spills_.back().time - spills_.front().time;
      }
    }
  }
  return true;
}


bool ParserRaw::write_settings_bulk(Qpx::Setting &set) {
  set.enrich(setting_definitions_);

  if (set.id_ != device_name())
    return false;

  for (auto &q : set.branches.my_data_) {
    if (q.id_ == "ParserRaw/Override timestamps")
      override_timestamps_ = q.value_int;
    else if (q.id_ == "ParserRaw/Loop data")
      loop_data_ = q.value_int;
    else if (q.id_ == "ParserRaw/Override pause")
      override_pause_ = q.value_int;
    else if (q.id_ == "ParserRaw/Pause")
      pause_ms_ = q.value_int;
    else if (q.id_ == "ParserRaw/Source file")
      source_file_ = q.value_text;
  }
  return true;
}

bool ParserRaw::boot() {
  if (!(status_ & SourceStatus::can_boot)) {
    PL_WARN << "<ParserRaw> Cannot boot Sorter. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = SourceStatus::loaded | SourceStatus::can_boot;

  pugi::xml_document doc;

  if (!doc.load_file(source_file_.c_str())) {
    PL_WARN << "<ParserRaw> Could not parse XML in " << source_file_;
    return false;
  }

  pugi::xml_node root = doc.first_child();
  if (!root || (std::string(root.name()) != "QpxListData")) {
    PL_WARN << "<ParserRaw> Bad root ID in " << source_file_;
    return false;
  }

  boost::filesystem::path meta(source_file_);
  meta.make_preferred();
  boost::filesystem::path path = meta.remove_filename();

  if (!boost::filesystem::is_directory(meta)) {
    PL_DBG << "<ParserRaw> Bad path for list mode data";
    return false;
  }

  boost::filesystem::path bin_path = path / "qpx_out.bin";

  file_bin_.open(bin_path.string(), std::ofstream::in | std::ofstream::binary);

  if (!file_bin_.is_open()) {
    PL_DBG << "<ParserRaw> Could not open binary " << bin_path.string();
    return false;
  }

  if (!file_bin_.good()) {
    file_bin_.close();
    PL_DBG << "<ParserRaw> Could not open binary " << bin_path.string();
    return false;
  }

  PL_DBG << "<ParserRaw> Success opening binary " << bin_path.string();

  file_bin_.seekg (0, std::ios::beg);
  bin_begin_ = file_bin_.tellg();
  file_bin_.seekg (0, std::ios::end);
  bin_end_ = file_bin_.tellg();
  file_bin_.seekg (0, std::ios::beg);

  for (pugi::xml_node child : root.children()) {
    std::string name = std::string(child.name());
    if (name == Spill().xml_element_name()) {
      Spill spill;
      spill.from_xml(child);
      if (spill != Spill())
        spills_.push_back(spill);
    }
  }

  if (spills_.size() == 0) {
    file_bin_.close();
    return false;
  }

  source_file_bin_ = bin_path.string();
  status_ = SourceStatus::loaded | SourceStatus::booted | SourceStatus::can_run;
  return true;
}


void ParserRaw::get_all_settings() {
  if (status_ & SourceStatus::booted) {
  }
}


void ParserRaw::worker_run(ParserRaw* callback, SynchronizedQueue<Spill*>* spill_queue) {
  PL_DBG << "<ParserRaw> Start run worker";

  Spill one_spill, prevspill;

  bool timeout = false;

  while ((!callback->spills_.empty()) && (!timeout)) {

    prevspill = one_spill;
    one_spill = callback->get_spill();

    if (callback->override_timestamps_) {
      one_spill.time = boost::posix_time::microsec_clock::universal_time();
      for (auto &q : one_spill.stats)
        q.second.lab_time = one_spill.time;
      // livetime and realtime are not changed accordingly
    }

    if (callback->override_pause_) {
      boost::this_thread::sleep(boost::posix_time::milliseconds(callback->pause_ms_));
    } else {
      if ((one_spill != Spill()) && (prevspill != Spill())) {
        if (one_spill.time > prevspill.time) {
          boost::posix_time::time_duration dif = one_spill.time - prevspill.time;
          //        PL_DBG << "<ParserRaw> Pause for " << dif.total_seconds();
          boost::this_thread::sleep(dif);
        }
      }
    }

//    for (auto &q : one_spill.stats) {
//      if (!starts_signalled.count(q.source_channel)) {
//        q.stats_type = StatsType::start;
//        starts_signalled.insert(q.source_channel);
//      }
//    }
    spill_queue->enqueue(new Spill(one_spill));

    timeout = (callback->run_status_.load() == 2);
  }

  for (auto &q : one_spill.stats)
    q.second.stats_type = StatsType::stop;
  one_spill.hits.clear();

  spill_queue->enqueue(new Spill(one_spill));

  if (callback->spills_.empty()) {
    PL_DBG << "<ParserRaw> Out of spills. Premature termination";
  }

  callback->run_status_.store(3);

  PL_DBG << "<ParserRaw> Stop run worker";

}



Spill ParserRaw::get_spill() {
  Spill one_spill;
  std::map<int16_t, Hit> model_hits;

  uint64_t hits_to_get = 0;

  if (!spills_.empty())
  {
    one_spill = spills_.front();
    hits_to_get += one_spill.hits.size();
    spills2_.push_back(one_spill);
    spills2_.back().time += boost::posix_time::seconds(10); //hack
    spills_.pop_front();
    one_spill.hits.clear();
  }

  for (auto &s : one_spill.stats)
    model_hits[s.second.source_channel] = s.second.model_hit;

  //      PL_DBG << "<Sorter> will produce no of events " << spills_.front().events_in_spill;
  while ((one_spill.hits.size() < hits_to_get) && (file_bin_.tellg() < bin_end_))
  {
    Hit one_hit;
    one_hit.read_bin(file_bin_, model_hits);
    one_spill.hits.push_back(one_hit);
  }


  PL_DBG << "<ParserRaw> made events " << one_spill.hits.size()
         << " and " << one_spill.stats.size() << " stats updates";

  if (loop_data_) {
    if (spills_.empty() && (!spills2_.empty())) {
      spills_ = spills2_;
      spills2_.clear();
      PL_DBG << "<ParserRaw> rewinding spills";
    }

    if (file_bin_.tellg() == bin_end_) {
      PL_DBG << "<ParserRaw> rewinding binary";
      file_bin_.seekg (0, std::ios::beg);
    }
  }

  return one_spill;
}


}
