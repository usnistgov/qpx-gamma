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
#include "producer_factory.h"


namespace Qpx {

static ProducerRegistrar<ParserRaw> registrar("ParserRaw");

ParserRaw::ParserRaw() {
  status_ = ProducerStatus::loaded | ProducerStatus::can_boot;

  runner_ = nullptr;

  run_status_.store(0);

  loop_data_ = false;
  override_timestamps_= false;
}

bool ParserRaw::die() {
  if ((status_ & ProducerStatus::booted) != 0)
    file_bin_.close();

  source_file_bin_.clear();

  spills_.clear();
  bin_begin_ = 0;
  bin_end_ = 0;

  status_ = ProducerStatus::loaded | ProducerStatus::can_boot;
//  for (auto &q : set.branches.my_data_) {
//    if ((q.metadata.setting_type == Qpx::SettingType::file_path) && (q.id_ == "ParserRaw/Producer file"))
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
      else if ((q.metadata.setting_type == Qpx::SettingType::file_path) && (q.id_ == "ParserRaw/Producer file")) {
        q.value_text = source_file_;
        q.metadata.writable = !(status_ & ProducerStatus::booted);
      }
      else if ((q.metadata.setting_type == Qpx::SettingType::file_path) && (q.id_ == "ParserRaw/Binary file"))
        q.value_text = source_file_bin_;
      else if ((q.metadata.setting_type == Qpx::SettingType::integer) && (q.id_ == "ParserRaw/Spills"))
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
    else if (q.id_ == "ParserRaw/Producer file")
      source_file_ = q.value_text;
  }
  return true;
}

bool ParserRaw::boot() {
  if (!(status_ & ProducerStatus::can_boot)) {
    WARN << "<ParserRaw> Cannot boot Sorter. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = ProducerStatus::loaded | ProducerStatus::can_boot;

  pugi::xml_document doc;

  if (!doc.load_file(source_file_.c_str())) {
    WARN << "<ParserRaw> Could not parse XML in " << source_file_;
    return false;
  }

  pugi::xml_node root = doc.first_child();
  if (!root || (std::string(root.name()) != "QpxListData")) {
    WARN << "<ParserRaw> Bad root ID in " << source_file_;
    return false;
  }

  boost::filesystem::path meta(source_file_);
  meta.make_preferred();
  boost::filesystem::path path = meta.remove_filename();

  if (!boost::filesystem::is_directory(meta)) {
    DBG << "<ParserRaw> Bad path for list mode data";
    return false;
  }

  boost::filesystem::path bin_path = path / "qpx_out.bin";

  file_bin_.open(bin_path.string(), std::ofstream::in | std::ofstream::binary);

  if (!file_bin_.is_open()) {
    DBG << "<ParserRaw> Could not open binary " << bin_path.string();
    return false;
  }

  if (!file_bin_.good()) {
    file_bin_.close();
    DBG << "<ParserRaw> Could not open binary " << bin_path.string();
    return false;
  }

  DBG << "<ParserRaw> Success opening binary " << bin_path.string();

  file_bin_.seekg (0, std::ios::beg);
  bin_begin_ = file_bin_.tellg();
  file_bin_.seekg (0, std::ios::end);
  bin_end_ = file_bin_.tellg();
  file_bin_.seekg (0, std::ios::beg);

  for (pugi::xml_node child : root.children()) {
    std::string name = std::string(child.name());
    if (name == Qpx::Spill().xml_element_name()) {
      Qpx::Spill spill;
      spill.from_xml(child);
      if (spill != Qpx::Spill()) {
        spills_.push_back(spill);
        hit_counts_.push_back(child.attribute("raw_hit_count").as_ullong(0));
        bin_offsets_.push_back(child.attribute("file_offset").as_ullong(0));

        std::string text = spill.to_string();
        if (hit_counts_.back() > 0)
          text += "  [" + std::to_string(hit_counts_.back()) + "]";
      }
    }
  }

  if (spills_.size() == 0) {
    file_bin_.close();
    return false;
  }

  current_spill_ = 0;
  source_file_bin_ = bin_path.string();
  status_ = ProducerStatus::loaded | ProducerStatus::booted | ProducerStatus::can_run;
  return true;
}


void ParserRaw::get_all_settings() {
  if (status_ & ProducerStatus::booted) {
  }
}


void ParserRaw::worker_run(ParserRaw* callback, SynchronizedQueue<Spill*>* spill_queue) {
  DBG << "<ParserRaw> Start run worker";

  Spill one_spill, prevspill;

  bool timeout = false;

  while ((callback->current_spill_ < callback->spills_.size()) && (!timeout)) {

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
          //        DBG << "<ParserRaw> Pause for " << dif.total_seconds();
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
    DBG << "<ParserRaw> Out of spills. Premature termination";
  }

  callback->run_status_.store(3);

  DBG << "<ParserRaw> Stop run worker";

}



Spill ParserRaw::get_spill() {
  Spill one_spill;

  if (spills_.empty() || (current_spill_ >= spills_.size()))
    return one_spill;

  one_spill = spills_.at(current_spill_);

  //      DBG << "<Sorter> will produce no of events " << spills_.front().events_in_spill;
  if (hit_counts_.at(current_spill_) > 0)
  {
    file_bin_.seekg(bin_offsets_.at(current_spill_), std::ios::beg);
    for (size_t i = 0; i < hit_counts_.at(current_spill_); ++i)
    {
      Qpx::Hit one_hit;
      one_hit.read_bin(file_bin_, hitmodels_);
      one_spill.hits.push_back(one_hit);
    }
  }

  DBG << "<ParserRaw> made events " << one_spill.hits.size()
         << " and " << one_spill.stats.size() << " stats updates";

  if (loop_data_) {

  }

  for (auto &s : one_spill.stats)
    hitmodels_[s.second.source_channel] = s.second.model_hit;

  current_spill_++;
  return one_spill;
}


}
