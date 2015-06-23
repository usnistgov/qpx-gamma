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
 *      Pixie::Spectrum::SpectrumRaw to output custom list mode to file
 *
 ******************************************************************************/


#include <fstream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/filesystem.hpp>
#include "spectrum_raw.h"

namespace Pixie {
namespace Spectrum {

static Registrar<SpectrumRaw> registrar("Raw");

SpectrumRaw::~SpectrumRaw() {
  if (open_)
    out_file_.close();
}

bool SpectrumRaw::initialize() {
  format_ = get_attr("format").value_int;
  file_name_ = get_attr("file_name").value_text;
  with_hit_pattern_ = get_attr("with_pattern").value_int;
  if (file_name_.empty())
    return false;

  std::ios_base::openmode mode = std::ofstream::out | std::ofstream::trunc;

  if (format_ == 0)
    mode = mode | std::ofstream::binary;

  out_file_.open(file_name_, mode);

  if (!out_file_.is_open())
    return false;

  if (!out_file_.good()) {
    out_file_.close();
    return false;
  }

  open_ = true;

  if (format_ == 0)
    init_bin();
  else if (format_ == 1)
    init_text();
  
  return true;
}

void SpectrumRaw::init_text() {
  out_file_ << "Qpx list mode" << std::endl;
  out_file_ << "Resolution " << std::to_string(bits_) << " bits" << std::endl;

  out_file_ << "Match pattern ";
  for (int i = 0; i < kNumChans; i++)
    out_file_ << " " << match_pattern_[i];
  out_file_ << std::endl;

  out_file_ << "Add pattern ";
  for (int i = 0; i < kNumChans; i++)
    out_file_ << " " << add_pattern_[i];
  out_file_ << std::endl;
}

void SpectrumRaw::init_bin() {
  std::string header("qpx_list/Pixie-4/");
  out_file_.write(header.data(), header.size());
  if (with_hit_pattern_) {
    std::string hp("with_pattern/");
    out_file_.write(hp.data(), hp.size());
  }
}

std::unique_ptr<std::list<Entry>> SpectrumRaw::_get_spectrum(std::initializer_list<Pair> list) {
  std::unique_ptr<std::list<Entry>> result(new std::list<Entry>);
  return result;
}

void SpectrumRaw::addHit(const Hit& newHit) {
  if ((!open_) || !out_file_.good())
    return;

  if (format_ == 0)
    hit_bin(newHit);
  else if (format_ == 1)
    hit_text(newHit);
}

void SpectrumRaw::addStats(const StatsUpdate& stats) {
  if ((!open_) || !out_file_.good())
    return;

  if (format_ == 0)
    stats_bin(stats);
  else if (format_ == 1)
    stats_text(stats);
}

void SpectrumRaw::addRun(const RunInfo& run) {
  if ((!open_) || !out_file_.good())
    return;

  if (format_ == 0)
    run_bin(run);
  else if (format_ == 1)
    run_text(run);
}

void SpectrumRaw::hit_text(const Hit &newHit) {

  if (with_hit_pattern_) {
    out_file_ << "p ";
    for (int i=0; i<newHit.pattern.size(); ++i) {
      out_file_ << newHit.pattern[i];
    }
    out_file_ << std::endl;
  }

  /*
  std::string pat = newHit.pattern.to_string();
  std::reverse(pat.begin(),pat.end());
  out_file_ << "p " << pat << std::endl;
  */

  for (int i = 0; i < kNumChans; i++)
    if ((add_pattern_[i]) && (newHit.pattern[i])) {
      out_file_ << "c " << i
                << " t " << newHit.buf_time_hi
                << " " << newHit.evt_time_hi
                << " " << newHit.chan_trig_time[i] //newHit.evt_time_lo
                << " e " << newHit.energy[i];
      out_file_ << std::endl;
    }
}

void SpectrumRaw::hit_bin(const Hit &newHit) {
  std::string h("h/");
  out_file_.write(h.c_str(), h.size());
//  uint16_t pt = newHit.pattern.to_ulong();
//  out_file_.write((char*)&pt, sizeof(pt));
}

void SpectrumRaw::stats_text(const StatsUpdate& stats) {
  out_file_ << "stats at " << to_iso_extended_string(stats.lab_time)
            << " spills " << stats.spill_count
            << " total_time " << stats.total_time
            << " event_rate " << stats.event_rate
            << std::endl;

  for (int i = 0; i < kNumChans; i++) {
    out_file_ << " c " << i
              << " fast_peaks " << stats.fast_peaks[i]
                 << " live_time " << stats.live_time[i]
                    << " ftdt " << stats.ftdt[i]
                       << " sfdt " << stats.sfdt[i]
                          << std::endl;
  }
}

void SpectrumRaw::stats_bin(const StatsUpdate& stats) {
  std::string s("s/");
  out_file_.write(s.c_str(), s.size());
  out_file_.write((char*)&stats.spill_count, sizeof(stats.spill_count));
  std::string x("\n");
  out_file_.write(x.c_str(), x.size());
}

void SpectrumRaw::run_text(const RunInfo& run) {
  out_file_ << "run start at " << to_iso_extended_string(run.time_start);
  if (run.time_start != run.time_stop)
    out_file_ << " end at " << to_iso_extended_string(run.time_stop)
              << " with " << run.total_events << " gross total events";
  out_file_ << std::endl;
}

void SpectrumRaw::run_bin(const RunInfo& run) {
  std::string r("r/");
  out_file_.write(r.c_str(), r.size());
//  out_file_.write((char*)&run.total_events, sizeof(run.total_events));
}


}}
