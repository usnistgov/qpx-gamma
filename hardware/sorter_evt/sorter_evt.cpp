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

#include "URL.h"
#include "CRingItem.h"
#include "DataFormat.h"
#include "CRingItemFactory.h"

#include "CPhysicsEventItem.h"
#include "CDataFormatItem.h"
#include "CRingStateChangeItem.h"
#include "CRingPhysicsEventCountItem.h"

#include "qpx_util.h"

#include "MADC32_module.h"

namespace Qpx {

static DeviceRegistrar<SorterEVT> registrar("SorterEVT");

SorterEVT::SorterEVT() {
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  runner_ = nullptr;

  evt_file = nullptr;

  run_status_.store(0);

  loop_data_ = false;
  override_timestamps_= false;
  bad_buffers_rep_ = false;
  bad_buffers_dbg_ = false;
}

bool SorterEVT::die() {
  if (evt_file != nullptr)
    delete evt_file;

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
      else if ((q.metadata.setting_type == Gamma::SettingType::boolean) && (q.id_ == "SorterEVT/Bad_buffers_report"))
        q.value_int = bad_buffers_rep_;
      else if ((q.metadata.setting_type == Gamma::SettingType::boolean) && (q.id_ == "SorterEVT/Bad_buffers_output"))
        q.value_int = bad_buffers_dbg_;
      else if ((q.metadata.setting_type == Gamma::SettingType::integer) && (q.id_ == "SorterEVT/Pause"))
        q.value_int = pause_ms_;
      else if ((q.metadata.setting_type == Gamma::SettingType::file_path) && (q.id_ == "SorterEVT/Source file")) {
        q.value_text = source_file_;
        q.metadata.writable = !(status_ & DeviceStatus::booted);
      }
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
    else if (q.id_ == "SorterEVT/Bad_buffers_report")
      bad_buffers_rep_ = q.value_int;
    else if (q.id_ == "SorterEVT/Bad_buffers_output")
      bad_buffers_dbg_ = q.value_int;
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

  //does file exist?

  std::string filename("file://");
  filename += source_file_;

  try { URL url(filename); }
  catch (...) { PL_ERR << "<SorterEVT> could not parse URL"; return false; }

  URL url(filename);

  //catch exception
  evt_file = new CFileDataSource(url, std::vector<uint16_t>());

  PL_DBG << "<SorterEVT> opened file data source successfully";

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

  uint64_t count = 0;
  uint64_t events = 0;
  uint64_t lost_events = 0;

  CRingItem* item;
  CRingItemFactory  fact;

  boost::posix_time::ptime time_start;

  while (!timeout) {

    one_spill = Spill();

    bool done = false;
    std::list<uint32_t> prev_MADC_data;
    std::string prev_pattern;

    while ( /*(count < 8000) &&*/ (!done) && (item = callback->evt_file->getItem()) != NULL)  {
      count++;

      switch (item->type()) {

      case RING_FORMAT: {
        CDataFormatItem* pEvent = reinterpret_cast<CDataFormatItem*>(fact.createRingItem(*item));
        if (pEvent) {
          //PL_DBG << "Ring format: " << pEvent->toString();
          delete pEvent;
        }
        break;
      }
      case END_RUN:
      case BEGIN_RUN:
      {
        CRingStateChangeItem* pEvent = reinterpret_cast<CRingStateChangeItem*>(fact.createRingItem(*item));
        if (pEvent) {

          boost::posix_time::ptime ts = boost::posix_time::from_time_t(pEvent->getTimestamp());
  //        PL_DBG << "State :" << pEvent->toString();
          PL_DBG << "State  ts=" << boost::posix_time::to_iso_extended_string(ts)
                 << "  elapsed=" << pEvent->getElapsedTime()
                 << "  run#=" << pEvent->getRunNumber()
                 << "  barrier=" << pEvent->getBarrierType();

          if (pEvent->getBarrierType() == 1) {
            time_start = ts;
            starts_signalled.clear();
          } else if (pEvent->getBarrierType() == 2) {
            done = true;
          }

          delete pEvent;
        }
        break;
      }

      case PHYSICS_EVENT_COUNT:
      {
        CRingPhysicsEventCountItem* pEvent = reinterpret_cast<CRingPhysicsEventCountItem*>(fact.createRingItem(*item));
        if (pEvent) {
  //        PL_DBG << "Physics counts: " << pEvent->toString();
          boost::posix_time::ptime ts = boost::posix_time::from_time_t(pEvent->getTimestamp());
//          PL_DBG << "State  ts=" << boost::posix_time::to_iso_extended_string(ts)
//                 << "  elapsed=" << pEvent->getTimeOffset()
//                 << "  total_events=" << pEvent->getEventCount();

          for (auto &q : starts_signalled) {
            StatsUpdate udt;
            udt.channel = q;
            udt.lab_time = ts;
            udt.fast_peaks = pEvent->getEventCount();
            udt.live_time = pEvent->getTimeOffset();
            udt.total_time = pEvent->getTimeOffset();
            one_spill.stats.push_back(udt);
            done = true;
          }

          delete pEvent;
        }
        break;
      }


      case PHYSICS_EVENT: {
        CPhysicsEventItem* pEvent = reinterpret_cast<CPhysicsEventItem*>(fact.createRingItem(*item));
        if (pEvent) {

            uint32_t  bytes = pEvent->getBodySize();
            uint32_t  words = bytes/sizeof(uint16_t);

            const uint16_t* body  = reinterpret_cast<const uint16_t*>((const_cast<CPhysicsEventItem*>(pEvent))->getBodyPointer());
            uint16_t expected_words = *body;
            if (expected_words == (words - 1)) {
              //PL_DBG << "Header indicates " << expected_words << " expected 16-bit words. Correct";
              body++;

              std::list<uint32_t> MADC_data;
              for (int i=0; i < expected_words; i+=2) {
                uint32_t lower = *body++;
                uint32_t upper = *body++;
                MADC_data.push_back(lower | (upper << 16));
              }

              std::string madc_pattern;
              std::list<Hit> hits = Qpx::MADC32::parse(MADC_data, events, madc_pattern);

              for (auto &h : hits) {
                if (!starts_signalled.count(h.channel)) {
                  StatsUpdate udt;
                  udt.channel = h.channel;
                  udt.lab_time = time_start;
                  udt.stats_type = StatsType::start;
                  //udt.fast_peaks = pEvent->getEventCount();
                  //udt.live_time = pEvent->getTimeOffset();
                  //udt.real_time = pEvent->getTimeOffset();
                  Spill extra_spill;
                  extra_spill.stats.push_back(udt);
                  spill_queue->enqueue(new Spill(extra_spill));
                  starts_signalled.insert(h.channel);
                }
              }


              bool buffer_problem = false;

              size_t n_h = std::count(madc_pattern.begin(), madc_pattern.end(), 'H');
              size_t n_e = std::count(madc_pattern.begin(), madc_pattern.end(), 'E');
              size_t n_f = std::count(madc_pattern.begin(), madc_pattern.end(), 'F');
              size_t n_j = std::count(madc_pattern.begin(), madc_pattern.end(), 'J');

              if (n_j > 1) {
                if (callback->bad_buffers_rep_)
                  PL_DBG << "MADC32 parse has multiple junk words, pattern: " << madc_pattern << " after previous " << prev_pattern;
                buffer_problem = true;
              }
              if (n_e != hits.size()) {
                if (callback->bad_buffers_rep_)
                  PL_DBG << "MADC32 parse has mismatch in number of retrieved events, pattern: " << madc_pattern << " after previous " << prev_pattern;
                buffer_problem = true;
                lost_events += n_e;
              }
              if (n_h != n_f) {
                if (callback->bad_buffers_rep_)
                  PL_DBG << "MADC32 parse has mismatch in header and footer, pattern: " << madc_pattern << " after previous " << prev_pattern;
                buffer_problem = true;
              }

              if (callback->bad_buffers_dbg_ && buffer_problem) {
                PL_DBG << "  " << buffer_to_string(MADC_data);
              }



              one_spill.hits.splice(one_spill.hits.end(), hits);

              prev_MADC_data = MADC_data;
              prev_pattern = madc_pattern;


            } else
              PL_DBG << "Header indicates " << expected_words << " expected 16-bit words, but does not match body size = " << (words - 1);

          delete pEvent;
        }
        break;
      }

      default: {
        PL_DBG << "Unexpected ring buffer item type " << item->type();
      }

      }

      delete item;
    }

    PL_DBG << "<SorterEVT> Processed " << count << "  cumulative hits = " << events
           << "   hits lost in bad buffers = " << lost_events << " (" << 100.0*lost_events/(events + lost_events) << "%)";



    if (callback->override_timestamps_) {
      for (auto &q : one_spill.stats)
        q.lab_time = boost::posix_time::microsec_clock::local_time();
      // livetime and realtime are not changed accordingly
    }

    if (callback->override_pause_) {
      boost::this_thread::sleep(boost::posix_time::milliseconds(callback->pause_ms_));
    } else {

    }

//    for (auto &q : one_spill.stats) {
//      if (!starts_signalled.count(q.channel)) {
//        q.stats_type = StatsType::start;
//        starts_signalled.insert(q.channel);
//      }
//    }
    spill_queue->enqueue(new Spill(one_spill));

    timeout = (callback->run_status_.load() == 2) /*|| (count > 8000)*/;
    if (item == NULL)
      break;
  }

  for (auto &q : one_spill.stats)
    q.stats_type = StatsType::stop;

  spill_queue->enqueue(new Spill(one_spill));

  callback->run_status_.store(3);

  PL_DBG << "<SorterEVT> Stop run worker";

}

std::string SorterEVT::buffer_to_string(const std::list<uint32_t>& buffer) {
  std::ostringstream out2;
  int j=0;
  for (auto &q : buffer) {
    if (j && ( (j % 4) == 0))
      out2 << std::endl;
    out2 << itobin32(q) << "   ";
    j++;
  }
  return out2.str();
}

Spill SorterEVT::get_spill() {
  Spill one_spill;

  return one_spill;
}


}
