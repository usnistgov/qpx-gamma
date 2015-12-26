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

static Registrar<SpectrumTime> registrar("1D");

void SpectrumTime::_set_detectors(const std::vector<Gamma::Detector>& dets) {
  metadata_.detectors.clear();

  if (dets.size() == 1)
    metadata_.detectors = dets;
  if (dets.size() >= 1) {
    int total = metadata_.add_pattern.size();
    if (dets.size() < total)
      total = dets.size();
    metadata_.detectors.resize(1, Gamma::Detector());

    for (int i=0; i < total; ++i) {
      if (metadata_.add_pattern[i]) {
        metadata_.detectors[0] = dets[i];
      }
    }
  }

  this->recalc_energies();
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
  int sz = metadata_.add_pattern.size();
  if (e.first.size() < sz)
    sz = e.first.size();
  for (int i = 0; i < sz; ++i)
    if (metadata_.add_pattern[i] && (e.first[i] < spectrum_.size())) {
      /*PreciseFloat nr = (spectrum_[e.first[i]] += e.second);

      if (nr > metadata_.max_count)
        metadata_.max_count = nr;

      metadata_.total_count += e.second;*/
    }
}

void SpectrumTime::addHit(const Hit& newHit) {
  if (recent_count_ > metadata_.max_count)
    metadata_.max_count = recent_count_;

  metadata_.total_count++;
}

void SpectrumTime::addStats(const StatsUpdate& newStats)
{

  if ((newStats.channel >= 0) || (newStats.channel < metadata_.add_pattern.size())
      || (metadata_.add_pattern[newStats.channel] == 1)) {

    PreciseFloat rt = 0;
    if (!updates_.empty())
      rt = (newStats.lab_time - updates_.back().lab_time).total_seconds();

    seconds_.push_back(rt.convert_to<double>());
    updates_.push_back(newStats);

    if (rt > metadata_.max_chan)
      metadata_.max_chan = seconds_.size() - 1;

    PreciseFloat count = recent_count_;
    if (rt > 0)
      count /= rt;

    spectrum_.push_back(count);

    energies_[0] = seconds_;
  }

  Spectrum::addStats(newStats);
}


void SpectrumTime::addEvent(const Event& newEvent) {
  for (int16_t i = 0; i < metadata_.add_pattern.size(); i++)
    if ((metadata_.add_pattern[i]) && (newEvent.hit.count(i) > 0))
      this->addHit(newEvent.hit.at(i));
}


std::string SpectrumTime::_channels_to_xml() const {
  std::stringstream channeldata;

  PreciseFloat z_count = 0;
  for (uint32_t i = 0; i < metadata_.resolution; i++)
    if (spectrum_[i])
      if (z_count == 0)
        channeldata << std::setprecision(std::numeric_limits<PreciseFloat>::max_digits10) << spectrum_[i] << " ";
      else {
        channeldata << "0 " << z_count << " "
                    << std::setprecision(std::numeric_limits<PreciseFloat>::max_digits10) << spectrum_[i] << " ";
        z_count = 0;
      }
    else
      z_count++;

  return channeldata.str();
}

uint16_t SpectrumTime::_channels_from_xml(const std::string& thisData){
  std::stringstream channeldata;
  channeldata.str(thisData);

  spectrum_.clear();
  spectrum_.resize(metadata_.resolution, 0);

  metadata_.max_count = 0;

  uint16_t i = 0;
  std::string numero, numero_z;
  while (channeldata.rdbuf()->in_avail()) {
    channeldata >> numero;
    if (numero == "0") {
      channeldata >> numero_z;
      i += boost::lexical_cast<uint16_t>(numero_z);
    } else {
      PreciseFloat nr(numero);
      spectrum_[i] = nr;
      if (nr > metadata_.max_count)
        metadata_.max_count = nr;
      i++;
    }
  }
  metadata_.max_chan = i;
  return metadata_.max_chan;
}


bool SpectrumTime::channels_from_string(std::istream &data_stream, bool compression) {
  std::list<Entry> entry_list;

  int i = 0;

  std::string numero, numero_z;
  if (compression) {
    while (data_stream.rdbuf()->in_avail()) {
      data_stream >> numero;
      if (numero == "0") {
        data_stream >> numero_z;
        i += boost::lexical_cast<uint16_t>(numero_z);
      } else {
        Entry new_entry;
        new_entry.first.resize(1);
        new_entry.first[0] = i;
        PreciseFloat nr(numero);
        new_entry.second = nr;
        entry_list.push_back(new_entry);
        i++;
      }
    }    
  } else {
    while (data_stream.rdbuf()->in_avail()) {
      data_stream >> numero;
      Entry new_entry;
      new_entry.first.resize(1);
      new_entry.first[0] = i;
      PreciseFloat nr(numero);
      new_entry.second = nr;
      entry_list.push_back(new_entry);
      i++;
    }
  } 
  
  if (i == 0)
    return false;

  metadata_.bits = log2(i);
  if (pow(2, metadata_.bits) < i)
    metadata_.bits++;
  metadata_.resolution = pow(2, metadata_.bits);
  shift_by_ = 16 - metadata_.bits;
  metadata_.max_chan = i;

  spectrum_.clear();
  spectrum_.resize(metadata_.resolution, 0);
  metadata_.max_count = 0;
      
  for (auto &q : entry_list) {
    spectrum_[q.first[0]] = q.second;
    metadata_.total_count += q.second;
    if (q.second > metadata_.max_count)
      metadata_.max_count = q.second;

  }

  return true;
}


}}
