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
 *      Qpx::Sorter generates spills from list files
 *
 ******************************************************************************/


#include "sorter.h"
#include <utility>
#include <set>
#include "custom_logger.h"

namespace Qpx {

Sorter::~Sorter() {
  if (open_bin_) {
    PL_DBG << "closing bin and eof " << (file_bin_.tellg() == bin_end_);
    file_bin_.close();
  }
}

Sorter::Sorter(std::string name_xml)
  : valid_(false)
  , open_bin_(false)
  , started(false)
  , ended(false)
  , file_name_xml_(name_xml)
{
  pugi::xml_document doc;

  if (!doc.load_file(file_name_xml_.c_str()))
    return;

  pugi::xml_node root = doc.first_child();
  if (!root || (std::string(root.name()) != "QpxListData"))
    return;


  if (!root.child("BinaryOut"))
    return;

  file_name_bin_ = std::string(root.child("BinaryOut").child_value("FileName"));

  PL_DBG << "Will read " << file_name_bin_;

  file_bin_.open(file_name_bin_, std::ofstream::in | std::ofstream::binary);

  if (!file_bin_.is_open())
    return;

  if (!file_bin_.good()) {
    file_bin_.close();
    return;
  }

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
      if (!info.time_start.is_not_a_date_time()) {
        PL_DBG << "info start";
        start_ = info;
      }
      if (!info.time_stop.is_not_a_date_time()) {
        PL_DBG << "info stop";
        end_ = info;
      }
    }
  }

  if (spills_.size() > 0)
    valid_ = true;
  else {
    file_bin_.close();
    return;
  }

  spills_.front().stats_type = StatsType::start;

  file_bin_.seekg (0, std::ios::beg);
  bin_begin_ = file_bin_.tellg();
  file_bin_.seekg (0, std::ios::end);
  bin_end_ = file_bin_.tellg();
  file_bin_.seekg (0, std::ios::beg);
  PL_DBG << "<Sorter> found " << spills_.size() << " spills with total_count = " << end_.total_events << " and binary file size = " << (bin_end_ - bin_begin_) << " bytes";
  open_bin_ = true;
}

void Sorter::order(std::string name_out) {
  std::ofstream  file_out;
  file_out.open(name_out, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);

  if (!file_out.is_open())
    return;

  if (!file_out.good() || !open_bin_ ) {
    file_out.close();
    return;
  }

  file_bin_.seekg (0, std::ios::beg);

  std::multiset<Hit> all_hits;

  uint64_t count = 0;
  uint16_t event_entry[5];
  PL_DBG << "reading all hits";
  while (file_bin_.tellg() < bin_end_) {
    Hit one_event;
    file_bin_.read(reinterpret_cast<char*>(event_entry), 10);
    one_event.channel = event_entry[0];
    uint64_t buf_time_hi = event_entry[1];
    uint64_t buf_time_mi = event_entry[2];
    uint64_t buf_time_lo = event_entry[3];
    one_event.timestamp.time = (buf_time_hi << 32) + (buf_time_mi << 16) + buf_time_lo;
    one_event.energy = event_entry[4];
    all_hits.insert(one_event);
    count++;
  }
  PL_DBG << "read out " << count << " hits in set as " << all_hits.size();

  file_bin_.seekg (0, std::ios::beg);

  for (auto &q : all_hits){
    file_out.write((char*)&q.channel, sizeof(q.channel));
    uint16_t time_hi = q.timestamp.time >> 32;
    uint16_t time_mi = (q.timestamp.time >> 16) & 0x000000000000FFFF;
    uint16_t time_lo = q.timestamp.time & 0x000000000000FFFF;
    file_out.write((char*)&time_hi, sizeof(time_hi));
    file_out.write((char*)&time_mi, sizeof(time_mi));
    file_out.write((char*)&time_lo, sizeof(time_lo));
    file_out.write((char*)&q.energy, sizeof(q.energy));
  }

  file_out.close();
  PL_DBG << "finished writing";
}


Spill Sorter::get_spill() {
  Spill one_spill;

  if (!started) {
    one_spill.run = start_;
    started = true;
//        PL_DBG << "<Sorter> start with " << start_.detectors.size();
  } else if (spills_.empty() && !ended) {
    one_spill.run = end_;
    ended = true;
//        PL_DBG << "<Sorter> end with " << end_.detectors.size();
  } else if (spills_.size() > 0) {
    //    PL_INFO << "<Sorter> Producing spill " /*<< spills_.front().spill_number <<*/ " (" << spills_.size() << " remaining) from list data";
    //    if (spills_.front().spill_number == 0)
    //      one_spill.run = start_;

    while (!spills_.empty() && (one_spill.stats.empty() || (one_spill.stats.back().lab_time == spills_.front().lab_time))) {
      one_spill.stats.push_back(spills_.front());

      uint64_t count = 0;
      uint16_t event_entry[6];
//      PL_DBG << "<Sorter> will produce no of events " << spills_.front().events_in_spill;
      while ((count < spills_.front().events_in_spill) && (file_bin_.tellg() < bin_end_)) {
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
        count++;
      }

      spills_.pop_front();
    }


  }
  PL_DBG << "<Sorter> made events " << one_spill.hits.size();
  //  PL_DBG << "eof " << (file_bin_.tellg() < bin_end_);
  return one_spill;
}


}
