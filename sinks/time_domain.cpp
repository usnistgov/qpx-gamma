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
#include "time_domain.h"
#include "consumer_factory.h"
#include "custom_logger.h"

namespace Qpx {

static ConsumerRegistrar<TimeDomain> registrar("Time");

TimeDomain::TimeDomain()
  : codomain(0)
{
  Setting base_options = metadata_.attributes();
  metadata_ = ConsumerMetadata("Time", "Time-domain log of activity", 1,
                    {}, {});

  Qpx::Setting format_setting;
  format_setting.id_ = "co-domain";
  format_setting.metadata.setting_type = Qpx::SettingType::int_menu;
  format_setting.metadata.writable = true;
  format_setting.metadata.flags.insert("preset");
  format_setting.metadata.description = "Choice of dependent variable";
  format_setting.value_int = 0;
  format_setting.metadata.int_menu_items[0] = "event rate";
  format_setting.metadata.int_menu_items[1] = "% dead-time";
  base_options.branches.add(format_setting);

  metadata_.overwrite_all_attributes(base_options);
//  DBG << "<TimeDomain:" << metadata_.get_attribute("name").value_text << ">  made with dims=" << metadata_.dimensions();
}

bool TimeDomain::_initialize() {
  Spectrum::_initialize();

  int adds = 0;
  std::vector<bool> gts = pattern_add_.gates();
  for (size_t i=0; i < gts.size(); ++i)
    if (gts[i])
      adds++;

  if (adds != 1) {
    WARN << "<TimeDomain> Cannot initialize. Add pattern must have 1 selected channel.";
    return false;
  }

  codomain = metadata_.get_attribute("co-domain").value_int;
  return true;
}


void TimeDomain::_set_detectors(const std::vector<Qpx::Detector>& dets) {
  metadata_.detectors.resize(metadata_.dimensions(), Qpx::Detector());

  if (dets.size() == metadata_.dimensions())
    metadata_.detectors = dets;
  if (dets.size() >= metadata_.dimensions()) {

    for (size_t i=0; i < dets.size(); ++i) {
      if (pattern_add_.relevant(i)) {
        metadata_.detectors[0] = dets[i];
        break;
      }
    }
  }

  axes_.resize(1);
  axes_[0].clear();
  for (auto &q : seconds_)
    axes_[0].push_back(to_double(q));

//  DBG << "<TimeDomain> _set_detectors";
}

PreciseFloat TimeDomain::_data(std::initializer_list<size_t> list) const {
  if (list.size() != 1)
    return 0;
  
  uint32_t chan = *list.begin();
  if (chan >= spectrum_.size())
    return 0;
  else
    return spectrum_[chan];
}

std::unique_ptr<std::list<Entry>> TimeDomain::_data_range(std::initializer_list<Pair> list) {
  size_t min, max;
  if (list.size() != 1) {
    min = 0;
    max = spectrum_.size();
  } else {
    Pair range = *list.begin();
    min = range.first;
    max = range.second;
  }

  if (max >= spectrum_.size())
    max = spectrum_.size() - 1;

  //in range?
  
  std::unique_ptr<std::list<Entry>> result(new std::list<Entry>);
  for (size_t i=min; i <= max; i++) {
    Entry newentry;
    newentry.first.resize(1, 0);
    newentry.first[0] = i;
    newentry.second = spectrum_[i];
    result->push_back(newentry);
  }
  return result;
}

void TimeDomain::_append(const Entry& e) {
  for (size_t i = 0; i < e.first.size(); ++i)
    if (pattern_add_.relevant(i) && (e.first[i] < spectrum_.size())) {
//      spectrum_[e.first[i]] += e.second;
//      metadata_.total_count += e.second;
    }
}

void TimeDomain::_push_stats(const StatsUpdate& newStats)
{
  if (pattern_add_.relevant(newStats.source_channel))
  {
    PreciseFloat real = 0;
    PreciseFloat live = 0;
    PreciseFloat percent_dead = 0;
    PreciseFloat tot_time = 0;

    if (!updates_.empty())
    {
      if ((newStats.stats_type == StatsType::stop) &&
          (updates_.back().stats_type == StatsType::running))
      {
        updates_.pop_back();
        seconds_.pop_back();
        spectrum_.pop_back();
        counts_.push_back(recent_count_ + counts_.back());
      } else
        counts_.push_back(recent_count_);

      boost::posix_time::time_duration rt ,lt;
      lt = rt = newStats.lab_time - updates_.back().lab_time;

      StatsUpdate diff = newStats - updates_.back();
      PreciseFloat scale_factor = 1;
      if (diff.items.count("native_time") && (diff.items.at("native_time") > 0))
        scale_factor = rt.total_microseconds() / diff.items["native_time"];

      if (diff.items.count("live_time")) {
        PreciseFloat scaled_live = diff.items.at("live_time") * scale_factor;
        lt = boost::posix_time::microseconds(static_cast<long>(to_double(scaled_live)));
      }

      real     = rt.total_milliseconds()  * 0.001;
      tot_time = (newStats.lab_time - updates_.front().lab_time).total_milliseconds() * 0.001;

      live = lt.total_milliseconds() * 0.001;

      percent_dead = (real - live) / real * 100;

    } else
      counts_.push_back(recent_count_);


    if (seconds_.empty() || (tot_time != 0))
    {
      PreciseFloat count = 0;

      if (codomain == 0)
      {
        count = counts_.back();
        if (live > 0)
          count /= live;
      } else if (codomain == 1) {
        count = percent_dead;
      }

      seconds_.push_back(to_double(tot_time));
      updates_.push_back(newStats);

      spectrum_.push_back(count);

      axes_[0].clear();
      for (auto &q : seconds_)
        axes_[0].push_back(to_double(q));

//      DBG << "<TimeDomain> \"" << metadata_.name << "\" chan " << int(newStats.channel) << " nrgs.size="
//             << energies_[0].size() << " nrgs.last=" << energies_[0][energies_[0].size()-1]
//             << " spectrum.size=" << spectrum_.size() << " spectrum.last=" << spectrum_[spectrum_.size()-1].convert_to<double>();
    }
  }

  Spectrum::_push_stats(newStats);
}

std::string TimeDomain::_data_to_xml() const {
  std::stringstream channeldata;

  for (uint32_t i = 0; i < spectrum_.size(); i++)
    channeldata << std::setprecision(std::numeric_limits<PreciseFloat>::max_digits10) << spectrum_[i] << " ";

  channeldata << " | ";

  for (uint32_t i = 0; i < seconds_.size(); i++)
    channeldata << std::setprecision(std::numeric_limits<PreciseFloat>::max_digits10) << seconds_[i] << " ";

  return channeldata.str();
}

uint16_t TimeDomain::_data_from_xml(const std::string& thisData){
  std::stringstream channeldata;
  channeldata.str(thisData);

  spectrum_.clear();
  seconds_.clear();

  std::list<PreciseFloat> sp, secs;

  uint16_t i = 0;
  std::string numero;
  while (channeldata.rdbuf()->in_avail()) {
    channeldata >> numero;
    if (numero == "|")
      break;

    PreciseFloat nr { from_string(numero) };
    sp.push_back(nr);
    i++;
  }

  i = 0;
  while (channeldata.rdbuf()->in_avail()) {
    channeldata >> numero;

    PreciseFloat nr { from_string(numero) };
    secs.push_back(nr);
    i++;
  }

  if (sp.size() && (sp.size() == secs.size())) {
    spectrum_ = std::vector<PreciseFloat>(sp.begin(), sp.end());
    seconds_  = std::vector<PreciseFloat>(secs.begin(), secs.end());
  } else
    i = 0;

  return i;
}

#ifdef H5_ENABLED
void TimeDomain::_save_data(H5CC::Group& g) const
{
  auto dsdata = g.require_dataset<long double>("data", {spectrum_.size(), 2}, {128,2});
  std::vector<long double> seconds(spectrum_.size());
  std::vector<long double> counts(spectrum_.size());
  size_t i = 0;
  for (auto a : seconds_)
    seconds[i++] = static_cast<long double>(a);
  i = 0;
  for (auto a : spectrum_)
    counts[i++] = static_cast<long double>(a);

  dsdata.write(seconds, {seconds.size(), 1}, {0,0});
  dsdata.write(counts, {counts.size(), 1}, {0,1});

  //updates?
}

void TimeDomain::_load_data(H5CC::Group &g)
{
  if (!g.has_dataset("data"))
    return;

  auto dset = g.open_dataset("data");

  if (dset.shape().rank() != 2)
    return;

  std::vector<long double> dsecs(dset.shape().dim(0));
  std::vector<long double> dcounts(dset.shape().dim(0));

  dset.read(dsecs, {dsecs.size(), 1}, {0,0});
  dset.read(dcounts, {dcounts.size(), 1}, {0,1});

  seconds_.resize(dsecs.size());
  spectrum_.resize(dsecs.size());
  for (size_t i=0; i < dsecs.size(); ++i)
  {
    seconds_[i] = dsecs[i];
    spectrum_[i] = dcounts[i];
  }

  //updates?
}

#endif

}
