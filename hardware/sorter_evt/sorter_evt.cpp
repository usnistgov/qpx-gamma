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
 *      Qpx::SorterEVT
 *
 ******************************************************************************/

#include "sorter_evt.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "custom_timer.h"


namespace Qpx {

static DeviceRegistrar<SorterEVT> registrar("SorterEVT");

SorterEVT::SorterEVT() {
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  runner_ = nullptr;

  run_status_.store(0);

  loop_data_ = false;
  override_timestamps_= false;
}

bool SorterEVT::die() {
  if ((status_ & DeviceStatus::booted) != 0)
    file_bin_.close();

  spills_.clear();
  spills2_.clear();

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
//  for (auto &q : set.branches.my_data_) {
//    if ((q.metadata.setting_type == Gamma::SettingType::file_path) && (q.id_ == "SorterEVT/Source file"))
//      q.metadata.writable = true;
//  }

  return true;
}


SorterEVT::~SorterEVT() {
  daq_stop();
  if (runner_ != nullptr) {
    runner_->detach();
    delete runner_;
  }
  die();
}

bool SorterEVT::daq_start(SynchronizedQueue<Spill*>* out_queue) {
  if (run_status_.load() > 0)
    return false;

  run_status_.store(1);

  if (runner_ != nullptr)
    delete runner_;

  runner_ = new boost::thread(&worker_run, this, out_queue);

  return true;
}

bool SorterEVT::daq_stop() {
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

bool SorterEVT::daq_running() {
  if (run_status_.load() == 3)
    daq_stop();
  return (run_status_.load() > 0);
}



bool SorterEVT::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ == device_name()) {
    for (auto &q : set.branches.my_data_) {
      if ((q.metadata.setting_type == Gamma::SettingType::boolean) && (q.id_ == "SorterEVT/Override timestamps"))
        q.value_int = override_timestamps_;
      else if ((q.metadata.setting_type == Gamma::SettingType::boolean) && (q.id_ == "SorterEVT/Loop data"))
        q.value_int = loop_data_;
      else if ((q.metadata.setting_type == Gamma::SettingType::boolean) && (q.id_ == "SorterEVT/Override pause"))
        q.value_int = override_pause_;
      else if ((q.metadata.setting_type == Gamma::SettingType::integer) && (q.id_ == "SorterEVT/Pause"))
        q.value_int = pause_ms_;
      else if ((q.metadata.setting_type == Gamma::SettingType::file_path) && (q.id_ == "SorterEVT/Source file"))
        q.value_text = source_file_;
      else if ((q.metadata.setting_type == Gamma::SettingType::integer) && (q.id_ == "SorterEVT/StatsUpdates"))
        q.value_int = spills_.size();
      else if ((q.metadata.setting_type == Gamma::SettingType::integer) && (q.id_ == "SorterEVT/Hits"))
        q.value_int = (bin_end_ - bin_begin_) / 12;
      else if ((q.metadata.setting_type == Gamma::SettingType::text) && (q.id_ == "SorterEVT/StartTime"))
        q.value_text = boost::posix_time::to_iso_extended_string(start_.time_start);
    }
  }
  return true;
}


bool SorterEVT::write_settings_bulk(Gamma::Setting &set) {
  set.enrich(setting_definitions_);

  if (set.id_ != device_name())
    return false;

  for (auto &q : set.branches.my_data_) {
    if (q.id_ == "SorterEVT/Override timestamps")
      override_timestamps_ = q.value_int;
    else if (q.id_ == "SorterEVT/Loop data")
      loop_data_ = q.value_int;
    else if (q.id_ == "SorterEVT/Override pause")
      override_pause_ = q.value_int;
    else if (q.id_ == "SorterEVT/Pause")
      pause_ms_ = q.value_int;
    else if (q.id_ == "SorterEVT/Source file")
      source_file_ = q.value_text;
  }
  return true;
}

bool SorterEVT::boot() {
  if (!(status_ & DeviceStatus::can_boot)) {
    PL_WARN << "<SorterEVT> Cannot boot Sorter. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  pugi::xml_document doc;

  if (!doc.load_file(source_file_.c_str())) {
    PL_WARN << "<SorterEVT> Could not parse XML in " << source_file_;
    return false;
  }

  pugi::xml_node root = doc.first_child();
  if (!root || (std::string(root.name()) != "QpxListData")) {
    PL_WARN << "<SorterEVT> Bad root ID in " << source_file_;
    return false;
  }

  if (!root.child("BinaryOut")) {
    PL_WARN << "<SorterEVT> No reference to binary file in " << source_file_;
    return false;
  }

  std::string file_name_bin = std::string(root.child("BinaryOut").child_value("FileName"));

  boost::filesystem::path meta(source_file_);
  meta.make_preferred();
  boost::filesystem::path path = meta.remove_filename();

  if (!boost::filesystem::is_directory(meta)) {
    PL_DBG << "<SorterEVT> Bad path for list mode data";
    return false;
  }

  boost::filesystem::path bin_path = path / file_name_bin;

  file_bin_.open(bin_path.string(), std::ofstream::in | std::ofstream::binary);

  if (!file_bin_.is_open()) {
    PL_DBG << "<SorterEVT> Could not open binary " << bin_path.string();
    return false;
  }

  if (!file_bin_.good()) {
    file_bin_.close();
    PL_DBG << "<SorterEVT> Could not open binary " << bin_path.string();
    return false;
  }

  PL_DBG << "<SorterEVT> Success opening binary " << bin_path.string();

  file_bin_.seekg (0, std::ios::beg);
  bin_begin_ = file_bin_.tellg();
  file_bin_.seekg (0, std::ios::end);
  bin_end_ = file_bin_.tellg();
  file_bin_.seekg (0, std::ios::beg);

  for (pugi::xml_node child : root.children()) {
    std::string name = std::string(child.name());
    if (name == StatsUpdate().xml_element_name()) {
      StatsUpdate stats;
      stats.from_xml(child);
      if (stats != StatsUpdate())
        spills_.push_back(stats);
    } else if (name == RunInfo().xml_element_name()) {
      RunInfo info;
      info.from_xml(child);
      if (!info.time_start.is_not_a_date_time())
        start_ = info;
      if (!info.time_stop.is_not_a_date_time())
        end_ = info;
    }
  }

  if (spills_.size() == 0) {
    file_bin_.close();
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::booted | DeviceStatus::can_run;
//  for (auto &q : set.branches.my_data_) {
//    if ((q.metadata.setting_type == Gamma::SettingType::file_path) && (q.id_ == "SorterEVT/Source file"))
//      q.metadata.writable = false;
//  }

  return true;
}


void SorterEVT::get_all_settings() {
  if (status_ & DeviceStatus::booted) {
  }
}


void SorterEVT::worker_run(SorterEVT* callback, SynchronizedQueue<Spill*>* spill_queue) {
  PL_DBG << "<SorterEVT> Start run worker";

  Spill one_spill;

  bool timeout = false;

  std::set<int> starts_signalled;

  while ((!callback->spills_.empty()) && (!timeout)) {

    one_spill = callback->get_spill();

    if (callback->override_timestamps_) {
      for (auto &q : one_spill.stats)
        q.lab_time = boost::posix_time::microsec_clock::local_time();
      // livetime and realtime are not changed accordingly
    }

    if (callback->override_pause_) {
      boost::this_thread::sleep(boost::posix_time::milliseconds(callback->pause_ms_));
    } else {
      if (!one_spill.stats.empty() && (!callback->spills_.empty())) {
        StatsUpdate newstats = callback->spills_.front();
        StatsUpdate prevstats = one_spill.stats.front();
        if ((prevstats != StatsUpdate()) && (newstats != StatsUpdate()) && (newstats.lab_time > prevstats.lab_time)) {
          boost::posix_time::time_duration dif = newstats.lab_time - prevstats.lab_time;
          //        PL_DBG << "<SorterEVT> Pause for " << dif.total_seconds();
          boost::this_thread::sleep(dif);
        }
      }
    }

    for (auto &q : one_spill.stats) {
      if (!starts_signalled.count(q.channel)) {
        q.stats_type = StatsType::start;
        starts_signalled.insert(q.channel);
      }
    }
    spill_queue->enqueue(new Spill(one_spill));

    timeout = (callback->run_status_.load() == 2);
  }

  for (auto &q : one_spill.stats)
    q.stats_type = StatsType::stop;

  spill_queue->enqueue(new Spill(one_spill));

  if (callback->spills_.empty()) {
    PL_DBG << "<SorterEVT> Out of spills. Premature termination";
  }

  callback->run_status_.store(3);

  PL_DBG << "<SorterEVT> Stop run worker";

}



Spill SorterEVT::get_spill() {
  Spill one_spill;

  if (!spills_.empty()) {

    StatsUpdate this_spill = spills_.front();


    uint16_t event_entry[6];
    //      PL_DBG << "<Sorter> will produce no of events " << spills_.front().events_in_spill;
    while ((one_spill.hits.size() < this_spill.events_in_spill) && (file_bin_.tellg() < bin_end_)) {
      Hit one_event;
      file_bin_.read(reinterpret_cast<char*>(event_entry), 12);
      one_event.channel = event_entry[0];
      //      PL_DBG << "event chan " << chan;
      uint64_t time_hi = event_entry[2];
      uint64_t time_mi = event_entry[3];
      uint64_t time_lo = event_entry[4];
      one_event.timestamp.time = (time_hi << 32) + (time_mi << 16) + time_lo;
      one_event.energy = event_entry[5];
      //PL_DBG << "event created chan=" << one_event.channel << " time=" << one_event.timestamp.time << " energy=" << one_event.energy;
      //file_bin_.seekg(10, std::ios::cur);

      one_spill.hits.push_back(one_event);
    }
    one_spill.stats.push_back(this_spill);
    spills2_.push_back(this_spill);
    spills_.pop_front();

    while (!spills_.empty()
           && (spills_.front().events_in_spill == 0)
           && (spills_.front().lab_time == one_spill.stats.back().lab_time)) {
//      PL_DBG << "<SorterEVT> adding to update ch=" << one_spill.stats.back().channel << " another update ch=" << callback->spills_.front().channel;
      one_spill.stats.push_back(spills_.front());
      spills2_.push_back(spills_.front());
      spills_.pop_front();
    }

  }

//  PL_DBG << "<SorterEVT> made events " << one_spill.hits.size();

  if (loop_data_) {
    if (spills_.empty() && (!spills2_.empty())) {
      spills_ = spills2_;
      spills2_.clear();
      PL_DBG << "<SorterEVT> rewinding spills";
    }

    if (file_bin_.tellg() == bin_end_) {
      PL_DBG << "<SorterEVT> rewinding binary";
      file_bin_.seekg (0, std::ios::beg);
    }
  }

  return one_spill;
}


}
