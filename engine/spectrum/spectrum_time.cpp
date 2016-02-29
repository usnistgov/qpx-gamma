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
 *      Qpx::Spectrum::SpectrumTime one-dimensional spectrum
 *
 ******************************************************************************/


#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/filesystem.hpp>
#include "spectrum_time.h"
#include "xylib.h"

namespace Qpx {
namespace Spectrum {

static Registrar<SpectrumTime> registrar("Time");

void SpectrumTime::_set_detectors(const std::vector<Qpx::Detector>& dets) {
  metadata_.detectors.resize(metadata_.dimensions, Qpx::Detector());

  if (dets.size() == metadata_.dimensions)
    metadata_.detectors = dets;
  if (dets.size() >= metadata_.dimensions) {

    for (int i=0; i < dets.size(); ++i) {
      if (pattern_add_.relevant(i)) {
        metadata_.detectors[0] = dets[i];
        break;
      }
    }
  }

  energies_.resize(1);
  energies_[0].clear();
  for (auto &q : seconds_)
    energies_[0].push_back(q.convert_to<double>());

//  PL_DBG << "<SpectrumTime> _set_detectors";
}

PreciseFloat SpectrumTime::_get_count(std::initializer_list<uint16_t> list) const {
  if (list.size() != 1)
    return 0;
  
  uint32_t chan = *list.begin();
  if (chan >= spectrum_.size())
    return 0;
  else
    return spectrum_[chan];
}

std::unique_ptr<std::list<Entry>> SpectrumTime::_get_spectrum(std::initializer_list<Pair> list) {
  int min, max;
  if (list.size() != 1) {
    min = 0;
    max = spectrum_.size();
  } else {
    Pair range = *list.begin();
    min = range.first;
    max = range.second;
  }

  if (max > spectrum_.size())
    max = spectrum_.size();

  //in range?
  
  std::unique_ptr<std::list<Entry>> result(new std::list<Entry>);
  for (int i=min; i < max; i++) {
    Entry newentry;
    newentry.first.resize(1, 0);
    newentry.first[0] = i;
    newentry.second = spectrum_[i];
    result->push_back(newentry);
  }
  return result;
}

void SpectrumTime::_add_bulk(const Entry& e) {
  for (int i = 0; i < e.first.size(); ++i)
    if (pattern_add_.relevant(i) && (e.first[i] < spectrum_.size())) {
//      PreciseFloat nr = (spectrum_[e.first[i]] += e.second);

//      if (nr > metadata_.max_count)
//        metadata_.max_count = nr;

//      metadata_.total_count += e.second;
    }
}

void SpectrumTime::addStats(const StatsUpdate& newStats)
{

  if (pattern_add_.relevant(newStats.source_channel)) {

    PreciseFloat rt = 0;
    PreciseFloat tot_time = 0;

    if (!updates_.empty()) {
      if ((newStats.stats_type == StatsType::stop) && (updates_.back().stats_type == StatsType::running)) {
        updates_.pop_back();
        seconds_.pop_back();
        spectrum_.pop_back();
        counts_.push_back(recent_count_ + counts_.back());
      } else
        counts_.push_back(recent_count_);

      rt       = (newStats.lab_time - updates_.back().lab_time).total_milliseconds()  * 0.001;
      tot_time = (newStats.lab_time - updates_.front().lab_time).total_milliseconds() * 0.001;
    } else
      counts_.push_back(recent_count_);


    if (seconds_.empty() || (tot_time != 0)) {

      PreciseFloat count = 0;

      if (codomain == 0) {
        count = counts_.back();
        if (rt > 0)
          count /= rt;
      } else if (codomain == 1) {
        //should be % dead-time, unimplemented

        count = counts_.back();
        if (rt > 0)
          count /= rt;
      }

      seconds_.push_back(tot_time.convert_to<double>());
      updates_.push_back(newStats);

      metadata_.resolution = seconds_.size();
      metadata_.max_chan = seconds_.size() - 1;

      spectrum_.push_back(count);

      if (count > metadata_.max_count)
        metadata_.max_count = count;

      energies_[0].clear();
      for (auto &q : seconds_)
        energies_[0].push_back(q.convert_to<double>());

//      PL_DBG << "<SpectrumTime> \"" << metadata_.name << "\" chan " << int(newStats.channel) << " nrgs.size="
//             << energies_[0].size() << " nrgs.last=" << energies_[0][energies_[0].size()-1]
//             << " spectrum.size=" << spectrum_.size() << " spectrum.last=" << spectrum_[spectrum_.size()-1].convert_to<double>();
    }
  }

  Spectrum::addStats(newStats);
}


std::string SpectrumTime::_channels_to_xml() const {
  std::stringstream channeldata;

  for (uint32_t i = 0; i < spectrum_.size(); i++)
    channeldata << std::setprecision(std::numeric_limits<PreciseFloat>::max_digits10) << spectrum_[i] << " ";

  channeldata << " | ";

  for (uint32_t i = 0; i < seconds_.size(); i++)
    channeldata << std::setprecision(std::numeric_limits<PreciseFloat>::max_digits10) << seconds_[i] << " ";

  return channeldata.str();
}

uint16_t SpectrumTime::_channels_from_xml(const std::string& thisData){
  std::stringstream channeldata;
  channeldata.str(thisData);

  spectrum_.clear();
  spectrum_.resize(metadata_.resolution, 0);
  seconds_.clear();
  seconds_.resize(metadata_.resolution, 0);

  metadata_.max_count = 0;

  uint16_t i = 0;
  std::string numero;
  while (channeldata.rdbuf()->in_avail()) {
    channeldata >> numero;
    if (numero == "|")
      break;

    PreciseFloat nr(numero);
    spectrum_[i] = nr;
    if (nr > metadata_.max_count)
      metadata_.max_count = nr;
    i++;
  }

  i = 0;
  while (channeldata.rdbuf()->in_avail()) {
    channeldata >> numero;

    PreciseFloat nr(numero);
    seconds_[i] = nr;
    i++;
  }

  metadata_.max_chan = i;
  return metadata_.max_chan;
}


}}
