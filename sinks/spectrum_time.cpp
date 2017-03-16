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
 *      Qpx::SinkTime one-dimensional spectrum
 *
 ******************************************************************************/


#include <fstream>
#include "spectrum_time.h"
#include "daq_sink_factory.h"

namespace Qpx {

static SinkRegistrar<TimeSpectrum> registrar("TimeSpectrum");

TimeSpectrum::TimeSpectrum()
{
  Setting base_options = metadata_.attributes();
  metadata_ = Metadata("TimeSpectrum", "Set of spectra in time series", 2,
                    {}, {});

  metadata_.overwrite_all_attributes(base_options);
//  DBG << "<TimeSpectrum:" << metadata_.get_attribute("name").value_text << ">  made with dims=" << metadata_.dimensions();
}

bool TimeSpectrum::_initialize() {
  Spectrum::_initialize();

  int adds = 0;
  std::vector<bool> gts = pattern_add_.gates();
  for (size_t i=0; i < gts.size(); ++i)
    if (gts[i])
      adds++;

  if (adds != 1) {
    WARN << "<TimeSpectrum> Cannot initialize. Add pattern must have 1 selected channel.";
    return false;
  }

  return true;
}


void TimeSpectrum::_set_detectors(const std::vector<Qpx::Detector>& dets)
{
  metadata_.detectors.resize(metadata_.dimensions(), Qpx::Detector());

  if (dets.size() == metadata_.dimensions())
    metadata_.detectors = dets;
  else if (dets.size() > metadata_.dimensions()) {
    int j=0;
    for (size_t i=0; i < dets.size(); ++i) {
      if (pattern_add_.relevant(i)) {
        metadata_.detectors[j] = dets[i];
        j++;
        if (j >= metadata_.dimensions())
          break;
      }
    }
  }

  this->_recalc_axes();
}

PreciseFloat TimeSpectrum::_data(std::initializer_list<size_t> list) const {
  if (list.size() != 2)
    return 0;

  std::vector<uint16_t> coords(list.begin(), list.end());

  if (coords[0] >= spectra_.size())
    return 0;

  if (coords[1] >= spectra_.at(coords[0]).size())
    return 0;

  return spectra_.at(coords[0]).at(coords[1]);
}

std::unique_ptr<std::list<Entry>> TimeSpectrum::_data_range(std::initializer_list<Pair> list)
{
  size_t min0, min1, max0, max1;
  if (list.size() != 2)
  {
    min0 = min1 = 0;
    max0 = spectra_.size();
    max1 = pow(2, bits_);
  } else {
    Pair range0 = *list.begin(), range1 = *(list.begin()+1);
    min0 = range0.first; max0 = range0.second;
    min1 = range1.first; max1 = range1.second;
  }

  if (max0 >= spectra_.size())
    max0 = spectra_.size();
  if (max1 >= pow(2, bits_))
    max1 = pow(2, bits_);

  std::unique_ptr<std::list<Entry>> result(new std::list<Entry>);
//  CustomTimer makelist(true);

  for (size_t i = min0; i < max0; ++i)
  {
    for (size_t j = min1; j < max1; ++j)
    {
      PreciseFloat val = spectra_.at(i).at(j);
      if (to_double(val) == 0)
        continue;
      Entry newentry;
      newentry.first.resize(2, 0);
      newentry.first[0] = i;
      newentry.first[1] = j;
      newentry.second = val;
      result->push_back(newentry);
    }
  }

  return result;
}

void TimeSpectrum::_append(const Entry& e)
{
  //do this
}

void TimeSpectrum::addHit(const Hit& newHit)
{
  uint16_t en = newHit.value(energy_idx_.at(newHit.source_channel())).val(bits_);
//  if (en < cutoff_bin_)
//    return;

  spectra_[spectra_.size()-1][en]++;
  total_hits_++;

//  if (en > maxchan_)
//    maxchan_ = en;
}

void TimeSpectrum::addEvent(const Event& newEvent)
{
  for (auto &h : newEvent.hits)
    if (pattern_add_.relevant(h.first))
      this->addHit(h.second);
}

void TimeSpectrum::_push_stats(const StatsUpdate& newStats)
{
  if (pattern_add_.relevant(newStats.source_channel))
  {
    PreciseFloat real = 0;
    PreciseFloat live = 0;
    PreciseFloat percent_dead = 0;
    PreciseFloat tot_time = 0;

    spectra_.push_back(std::vector<PreciseFloat>(pow(2, bits_)));

    if (!updates_.empty())
    {
      if ((newStats.stats_type == StatsType::stop) &&
          (updates_.back().stats_type == StatsType::running))
      {
        updates_.pop_back();
        seconds_.pop_back();
      }

      boost::posix_time::time_duration rt ,lt;
      lt = rt = newStats.lab_time - updates_.back().lab_time;

      StatsUpdate diff = newStats - updates_.back();
      PreciseFloat scale_factor = 1;
      if (diff.items.count("native_time") && (diff.items.at("native_time") > 0))
        scale_factor = rt.total_microseconds() / diff.items["native_time"];

      if (diff.items.count("live_time"))
      {
        PreciseFloat scaled_live = diff.items.at("live_time") * scale_factor;
        lt = boost::posix_time::microseconds(static_cast<long>(to_double(scaled_live)));
      }

      real     = rt.total_milliseconds()  * 0.001;
      tot_time = (newStats.lab_time - updates_.front().lab_time).total_milliseconds() * 0.001;

      live = lt.total_milliseconds() * 0.001;

      percent_dead = (real - live) / real * 100;

    }


    if (seconds_.empty() || (tot_time != 0)) {

      seconds_.push_back(to_double(tot_time));
      updates_.push_back(newStats);

//      spectrum_.push_back(count);

      axes_[0].clear();
      for (auto &q : seconds_)
        axes_[0].push_back(to_double(q));
    }
  }

  Spectrum::_push_stats(newStats);
}


std::string TimeSpectrum::_data_to_xml() const
{
  // do this
  return "";
}

uint16_t TimeSpectrum::_data_from_xml(const std::string& thisData)
{
  // do this
  return false;
}

#ifdef H5_ENABLED
void TimeSpectrum::_save_data(H5CC::Group& g) const
{
  auto dgroup = g.require_group("data");

  auto dtime  = dgroup.require_dataset<long double>("seconds", {seconds_.size()});
  std::vector<long double> seconds(seconds_.size());
  for (size_t i = 0; i < seconds_.size(); ++i)
    seconds[i] = static_cast<long double>(seconds_[i]);
  dtime.write(seconds);

  hsize_t spsize = H5CC::kMax;
  if (spectra_.size())
    spsize = spectra_[0].size();

  auto dsdata = dgroup.require_dataset<long double>("spectra",
                                                    {spsize, spectra_.size()},
                                                    {128,1});
  for (size_t i = 0; i < spectra_.size(); ++i)
  {
    std::vector<long double> spectrum(spectra_[i].size());
    for (size_t j = 0; j < spectrum.size(); ++j)
      spectrum[j] = static_cast<long double>(spectra_[i][j]);
    dsdata.write(spectrum, {spectrum.size(), 1}, {0,i});
  }

  //updates?
}

void TimeSpectrum::_load_data(H5CC::Group &g)
{
  if (!g.has_group("data"))
    return;
  auto dgroup = g.open_group("data");

  if (!dgroup.has_dataset("seconds") || !dgroup.has_dataset("spectra"))
    return;

  auto dsecs = dgroup.open_dataset("seconds");
  auto dspec = dgroup.open_dataset("spectra");

  if ((dsecs.shape().rank() != 1) ||
      (dspec.shape().rank() != 2))
    return;

  std::vector<long double> secs(dsecs.shape().dim(0));
  dsecs.read(secs, {secs.size()}, {0});
  seconds_.resize(secs.size());
  for (size_t i = 0; i < secs.size(); ++i)
    seconds_[i] = secs[i];

  spectra_.resize(dspec.shape().dim(1));
  size_t size = dspec.shape().dim(0);
  for (size_t i = 0; i < spectra_.size(); ++i)
  {
    std::vector<long double> spectrum(size);
    dspec.read(spectrum, {size, 1}, {0,i});
    spectra_[i].resize(spectrum.size());
    for (size_t j = 0; j < spectrum.size(); ++j)
      spectra_[i][j] = spectrum[j];
  }

  //updates?
}

#endif


}
