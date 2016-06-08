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
 *      Qpx::Sink1D one-dimensional spectrum
 *
 ******************************************************************************/


#include <fstream>
#include <boost/algorithm/string.hpp>
#include "delayometer.h"
#include "daq_sink_factory.h"
#include "xylib.h"
#include "qpx_util.h"

namespace Qpx {

static SinkRegistrar<Delayometer> registrar("Delayometer");

Delayometer::Delayometer()
  : maxchan_(0)
{
  Setting base_options = metadata_.attributes;
  metadata_ = Metadata("Delayometer", "Helps find optimal delay for multi-detector setups",
                       1, {}, {});
  metadata_.attributes = base_options;

}

void Delayometer::_set_detectors(const std::vector<Qpx::Detector>& dets) {
  metadata_.detectors.resize(metadata_.dimensions(), Qpx::Detector());

  if (dets.size() == metadata_.dimensions())
    metadata_.detectors = dets;
  if (dets.size() >= metadata_.dimensions()) {

    for (int i=0; i < dets.size(); ++i) {
      if (pattern_add_.relevant(i)) {
        metadata_.detectors[0] = dets[i];
        break;
      }
    }
  }

  axes_.resize(1);
  axes_[0].clear();
  for (auto &q : ns_)
    axes_[0].push_back(q.second.convert_to<double>());
}

bool Delayometer::_initialize()
{
  Spectrum::_initialize();

//  int64_t max = std::ceil(max_delay_);
//  ns_.clear();
//  for (int64_t i= -max; i <= max; ++i)
//    ns_[i] = i;

  return true;
}

PreciseFloat Delayometer::_data(std::initializer_list<uint16_t> list) const {
  if (list.size() != 1)
    return 0;
  
  uint32_t chan = *list.begin();
  if (chan >= ns_.size())
    return 0;
  else if (spectrum_.count(chan))
    return spectrum_.at(chan);
  else
    return 0;
}

std::unique_ptr<std::list<Entry>> Delayometer::_data_range(std::initializer_list<Pair> list) {
//  int min, max;
//  if (list.size() != 1) {
//    min = 0;
//    max = spectrum_.size();
//  } else {
//    Pair range = *list.begin();
//    min = range.first;
//    max = range.second;
//  }

//  //in range?
  
  std::unique_ptr<std::list<Entry>> result(new std::list<Entry>);
  int i = 0;
  for (auto &q : spectrum_) {
    Entry newentry;
    newentry.first.resize(1, 0);
    newentry.first[0] = i;
    newentry.second = q.second;
    result->push_back(newentry);
    i++;
  }
  return result;
}

void Delayometer::_append(const Entry& e) {
//  for (int i = 0; i < e.first.size(); ++i)
//    if (pattern_add_.relevant(i) && (e.first[i] < spectrum_.size())) {
//      spectrum_[e.first[i]] += e.second;
//      metadata_.total_count += e.second;
//    }
}

void Delayometer::_push_hit(const Hit& newhit)
{
  if ((newhit.source_channel() < 0)
      || (newhit.source_channel() >= energy_idx_.size())
      || (energy_idx_.at(newhit.source_channel()) < 0))
    return;

  DigitizedVal energy = newhit.values.at(energy_idx_.at(newhit.source_channel()));

  if ((newhit.source_channel() < cutoff_logic_.size())
      && (energy.val(metadata_.bits) < cutoff_logic_[newhit.source_channel()]))
    return;

  if (newhit.source_channel() < 0)
    return;

  if (!(pattern_coinc_.relevant(newhit.source_channel()) ||
        pattern_anti_.relevant(newhit.source_channel()) ||
        pattern_add_.relevant(newhit.source_channel())))
    return;

  //  DBG << "Processing " << newhit.to_string();

  for (auto &q : backlog) {
    if (q.in_window(newhit)) {
      Event copy = q;
      if (copy.addHit(newhit)) {
        if (validateEvent(copy)) {
          recent_count_++;
          metadata_.total_count++;
          this->addEvent(copy);
        } else
          DBG << "<" << metadata_.name << "> not validated " << q.to_string();
      }
//      else
//        DBG << "<" << metadata_.name << "> pileup hit " << newhit.to_string() << " with " << q.to_string() << " already has " << q.hits[newhit.source_channel()].to_string();
    }
//    else if (q.past_due(newhit))
//      break;
    else if (q.antecedent(newhit))
      DBG << "<" << metadata_.name << "> antecedent hit " << newhit.to_string() << ". Something wrong with presorter or daq_device?";
  }

  backlog.push_back(Event(newhit, coinc_window_, max_delay_));

  Event evt;
  while (!backlog.empty() && (evt = backlog.front()).past_due(newhit))
    backlog.pop_front();
}

void Delayometer::addEvent(const Event& newEvent) {
  Hit a, b;
  for (auto &h : newEvent.hits) {
    if (a == Hit())
      a = h.second;
    else
      b = h.second;
  }

//  double df = std::abs(b.timestamp - a.timestamp);
//  if ((df > 0) && (df < 18000))
//    DBG << " d " << b.timestamp - a.timestamp;

  TimeStamp common = TimeStamp::common_timebase(a.timestamp(), b.timestamp());
//  TimeStamp res;
//  if (a.timestamp.timebase_divider > b.timestamp.timebase_divider)
//    res = a.timestamp;
//  else if ((a.timestamp.timebase_divider == b.timestamp.timebase_divider)
//           && (b.timestamp.timebase_multiplier < b.timestamp.timebase_multiplier))
//    res = a.timestamp;
//  else
//    res = b.timestamp;

  if (!timebase.same_base(common)) {
    timebase = TimeStamp::common_timebase(common, timebase);

    double max_ns = std::ceil(max_delay_);
    int64_t max_native = timebase.to_native(max_ns);

    for (int64_t i= -max_native; i <= max_native; ++i) {
      ns_[i] = timebase.to_nanosec(i);
      if (!spectrum_.count(i))
        spectrum_[i] = 0;
    }

    axes_.resize(1);
    axes_[0].clear();
    for (auto &q : ns_)
      axes_[0].push_back(q.second.convert_to<double>());
  }

  int64_t diff =  timebase.to_native(std::round((b.timestamp() - a.timestamp())));

  spectrum_[diff]++;

  if (!ns_.count(diff)) {
    ns_[diff] = timebase.to_nanosec(diff);

    axes_.resize(1);
    axes_[0].clear();
    for (auto &q : ns_)
      axes_[0].push_back(q.second.convert_to<double>());

  }

  if (diff > maxchan_)
    maxchan_ = diff;
}

std::string Delayometer::_data_to_xml() const {
  std::stringstream channeldata;

  return channeldata.str();
}

uint16_t Delayometer::_data_from_xml(const std::string& thisData){
  std::stringstream channeldata;
  channeldata.str(thisData);

  return maxchan_;
}



}
