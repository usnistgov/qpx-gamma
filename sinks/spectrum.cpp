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
 *      Qpx::Sink generic spectrum type.
 *                       All public methods are thread-safe.
 *                       When deriving override protected methods.
 *
 ******************************************************************************/

#include "spectrum.h"
#include "custom_logger.h"

namespace Qpx {

Spectrum::Spectrum()
  : Sink()
  , recent_count_(0)
  , coinc_window_(0)
  , max_delay_(0)
{

  Setting dets;
  dets.id_ = "per_detector";
  dets.metadata.setting_type = SettingType::stem;

  Setting det;
  det.id_ = "detector";
  det.indices.insert(-1);
  det.metadata.setting_type = SettingType::stem;

  Setting ignore_zero;
  ignore_zero.id_ = "cutoff_logic";
  ignore_zero.metadata.setting_type = SettingType::integer;
  ignore_zero.metadata.description = "Hits rejected below minimum energy (affects coincidence logic and binning)";
  ignore_zero.metadata.writable = true;
  ignore_zero.metadata.flags.insert("preset");
  ignore_zero.metadata.minimum = 0;
  ignore_zero.metadata.step = 1;
  ignore_zero.metadata.maximum = 1000000;
  det.branches.add(ignore_zero);

  Setting delay;
  delay.id_ = "delay_ns";
  delay.metadata.setting_type = SettingType::floating;
  delay.metadata.description = "Digital delay (applied before coincidence logic)";
  delay.metadata.unit = "ns";
  delay.metadata.writable = true;
  delay.metadata.flags.insert("preset");
  delay.metadata.minimum = 0;
  delay.metadata.step = 1;
  delay.metadata.maximum = 1000000000;
  det.branches.add(delay);

  dets.branches.add(det);
  metadata_.attributes.branches.add(dets);

  Setting coinc_window;
  coinc_window.id_ = "coinc_window";
  coinc_window.metadata.setting_type = SettingType::floating;
  coinc_window.metadata.minimum = 0;
  coinc_window.metadata.step = 1;
  coinc_window.metadata.maximum = 1000000;
  coinc_window.metadata.unit = "ns";
  coinc_window.metadata.description = "Coincidence window";
  coinc_window.metadata.writable = true;
  coinc_window.metadata.flags.insert("preset");
  coinc_window.value_dbl = 50;
  metadata_.attributes.branches.add(coinc_window);

  Setting pattern_coinc;
  pattern_coinc.id_ = "pattern_coinc";
  pattern_coinc.metadata.setting_type = SettingType::pattern;
  pattern_coinc.metadata.maximum = 1;
  pattern_coinc.metadata.description = "Coincidence pattern";
  pattern_coinc.metadata.writable = true;
  pattern_coinc.metadata.flags.insert("preset");
  metadata_.attributes.branches.add(pattern_coinc);

  Setting pattern_anti;
  pattern_anti.id_ = "pattern_anti";
  pattern_anti.metadata.setting_type = SettingType::pattern;
  pattern_anti.metadata.maximum = 1;
  pattern_anti.metadata.description = "Anti-coindicence pattern";
  pattern_anti.metadata.writable = true;
  pattern_anti.metadata.flags.insert("preset");
  metadata_.attributes.branches.add(pattern_anti);

  Setting pattern_add;
  pattern_add.id_ = "pattern_add";
  pattern_add.metadata.setting_type = SettingType::pattern;
  pattern_add.metadata.maximum = 1;
  pattern_add.metadata.description = "Add pattern";
  pattern_add.metadata.writable = true;
  pattern_add.metadata.flags.insert("preset");
  metadata_.attributes.branches.add(pattern_add);

  Setting live_time;
  live_time.id_ = "live_time";
  live_time.metadata.setting_type = SettingType::time_duration;
  live_time.metadata.description = "Live time";
  live_time.metadata.writable = false;
  metadata_.attributes.branches.add(live_time);

  Setting real_time;
  real_time.id_ = "real_time";
  real_time.metadata.setting_type = SettingType::time_duration;
  real_time.metadata.description = "Real time";
  real_time.metadata.writable = false;
  metadata_.attributes.branches.add(real_time);

  Setting inst_rate;
  inst_rate.id_ = "instant_rate";
  inst_rate.metadata.setting_type = SettingType::floating;
  inst_rate.metadata.unit = "cps";
  inst_rate.metadata.description = "Instant count rate";
  inst_rate.metadata.writable = false;
  inst_rate.value_dbl = 0;
  metadata_.attributes.branches.add(inst_rate);
}

bool Spectrum::_initialize() {
  Sink::_initialize();

  pattern_coinc_ = get_attr("pattern_coinc").value_pattern;
  pattern_anti_ = get_attr("pattern_anti").value_pattern;
  pattern_add_ = get_attr("pattern_add").value_pattern;
  coinc_window_ = get_attr("coinc_window").value_dbl;
  if (coinc_window_ < 0)
    coinc_window_ = 0;

  max_delay_ = 0;
  Setting perdet = get_attr("per_detector");
  cutoff_logic_.resize(perdet.branches.size());
  delay_ns_.resize(perdet.branches.size());
  for (auto &d : perdet.branches.my_data_) {
    int idx = -1;
    if (d.indices.size())
      idx = *d.indices.begin();
    if (idx >= cutoff_logic_.size()) {
      cutoff_logic_.resize(idx + 1);
      delay_ns_.resize(idx + 1);
    }
    if (idx >= 0) {
      cutoff_logic_[idx] = d.get_setting(Setting("cutoff_logic"), Match::id).value_int;
      delay_ns_[idx]     = d.get_setting(Setting("delay_ns"), Match::id).value_dbl;
      if (delay_ns_[idx] > max_delay_)
        max_delay_ = delay_ns_[idx];
    }
  }
  max_delay_ += coinc_window_;
//   DBG << "<" << metadata_.name << "> coinc " << coinc_window_ << " max delay " << max_delay_;

  return false; //still too abstract
}


void Spectrum::_push_hit(const Hit& newhit)
{
  if ((newhit.source_channel < cutoff_logic_.size())
      && (newhit.energy.val(metadata_.bits) < cutoff_logic_[newhit.source_channel]))
    return;

  if (newhit.source_channel < 0)
    return;

  if (!(pattern_coinc_.relevant(newhit.source_channel) ||
      pattern_anti_.relevant(newhit.source_channel) ||
      pattern_add_.relevant(newhit.source_channel)))
    return;

  //  DBG << "Processing " << newhit.to_string();

  Hit hit = newhit;
  if (hit.source_channel < delay_ns_.size())
    hit.timestamp.delay(delay_ns_[hit.source_channel]);

  bool appended = false;
  bool pileup = false;
  if (backlog.empty() || backlog.back().past_due(hit))
    backlog.push_back(Event(hit, coinc_window_, max_delay_));
  else {
    for (auto &q : backlog) {
      if (q.in_window(hit)) {
        if (q.addHit(hit)) {
          if (appended)
            DBG << "<" << metadata_.name << "> hit " << hit.to_string() << " coincident with more than one other hit (counted >=2 times)";
          appended = true;
        } else {
          DBG << "<" << metadata_.name << "> pileup hit " << hit.to_string() << " with " << q.to_string() << " already has " << q.hits[hit.source_channel].to_string();
          pileup = true;
        }
      } else if (q.past_due(hit))
        break;
      else if (q.antecedent(hit))
        DBG << "<" << metadata_.name << "> antecedent hit " << hit.to_string() << ". Something wrong with presorter or daq_device?";
    }

    if (!appended && !pileup) {
      backlog.push_back(Event(hit, coinc_window_, max_delay_));
//      DBG << "append fresh";
    }
  }

  Event evt;
  while (!backlog.empty() && (evt = backlog.front()).past_due(hit)) {
    backlog.pop_front();
    if (validateEvent(evt)) {
      recent_count_++;
      metadata_.total_count++;
      this->addEvent(evt);
    }
  }

}


bool Spectrum::validateEvent(const Event& newEvent) const {
  if (!pattern_coinc_.validate(newEvent))
    return false;
  if (!pattern_anti_.antivalidate(newEvent))
    return false;
  return true;
}

void Spectrum::_push_stats(const StatsUpdate& newBlock) {
  //private; no lock required

  if (pattern_coinc_.relevant(newBlock.source_channel) ||
      pattern_anti_.relevant(newBlock.source_channel) ||
      pattern_add_.relevant(newBlock.source_channel)) {
    //DBG << "Spectrum " << metadata_.name << " received update for chan " << newBlock.channel;
    bool chan_new = (stats_list_.count(newBlock.source_channel) == 0);
    bool new_start = (newBlock.stats_type == StatsType::start);

    Setting start_time = get_attr("start_time");
    if (new_start && start_time.value_time.is_not_a_date_time()) {
      start_time.value_time = newBlock.lab_time;
      metadata_.attributes.branches.replace(start_time);
    }

    if (!chan_new && new_start && (stats_list_[newBlock.source_channel].back().stats_type == StatsType::running))
      stats_list_[newBlock.source_channel].back().stats_type = StatsType::stop;

    if (recent_end_.lab_time.is_not_a_date_time())
      recent_start_ = newBlock;
    else
      recent_start_ = recent_end_;

    recent_end_ = newBlock;

    Setting rate = get_attr("instant_rate");
    rate.value_dbl = 0;
    double recent_time = (recent_end_.lab_time - recent_start_.lab_time).total_milliseconds() * 0.001;
    if (recent_time > 0)
      rate.value_dbl = recent_count_ / recent_time;
    metadata_.attributes.branches.replace(rate);

    recent_count_ = 0;

    if (!chan_new && (stats_list_[newBlock.source_channel].back().stats_type == StatsType::running))
      stats_list_[newBlock.source_channel].pop_back();

    stats_list_[newBlock.source_channel].push_back(newBlock);

    if (!chan_new) {
      StatsUpdate start = stats_list_[newBlock.source_channel].front();

      boost::posix_time::time_duration rt ,lt;
      boost::posix_time::time_duration real ,live;

      for (auto &q : stats_list_[newBlock.source_channel]) {
        if (q.stats_type == StatsType::start) {
          real += rt;
          live += lt;
          start = q;
        } else {
          lt = rt = q.lab_time - start.lab_time;
          StatsUpdate diff = q - start;
          PreciseFloat scale_factor = 1;
          if (diff.items.count("native_time") && (diff.items.at("native_time") > 0))
            scale_factor = rt.total_microseconds() / diff.items["native_time"];
          if (diff.items.count("live_time")) {
            PreciseFloat scaled_live = diff.items.at("live_time") * scale_factor;
            lt = boost::posix_time::microseconds(scaled_live.convert_to<long>());
          }
        }
      }

      if (stats_list_[newBlock.source_channel].back().stats_type != StatsType::start) {
        real += rt;
        live += lt;
//        DBG << "<Spectrum> \"" << metadata_.name << "\" RT + "
//               << rt << " = " << real << "   LT + " << lt << " = " << live;
      }
      real_times_[newBlock.source_channel] = real;
      live_times_[newBlock.source_channel] = live;

      Setting live_time = get_attr("live_time");
      Setting real_time = get_attr("real_time");

      live_time.value_duration = real_time.value_duration = real;
      for (auto &q : real_times_)
        if (q.second.total_milliseconds() < real_time.value_duration.total_milliseconds())
          real_time.value_duration = q.second;

      for (auto &q : live_times_)
        if (q.second.total_milliseconds() < live_time.value_duration.total_milliseconds())
          live_time.value_duration = q.second;

      metadata_.attributes.branches.replace(live_time);
      metadata_.attributes.branches.replace(real_time);

//      DBG << "<Spectrum> \"" << metadata_.name << "\"  ********* "
//             << "RT = " << to_simple_string(metadata_.real_time)
//             << "   LT = " << to_simple_string(metadata_.live_time) ;

    }
  }
}
  
void Spectrum::_set_detectors(const std::vector<Qpx::Detector>& dets) {
  //private; no lock required
//  DBG << "<Spectrum> _set_detectors";

  metadata_.detectors.clear();

  //FIX THIS!!!!

  //metadata_.detectors.resize(metadata_.dimensions(), Qpx::Detector());
  this->_recalc_axes();
}

void Spectrum::_recalc_axes() {
  //private; no lock required

  axes_.resize(metadata_.dimensions());
  if (axes_.size() != metadata_.detectors.size())
    return;

  for (int i=0; i < metadata_.detectors.size(); ++i) {
    uint32_t res = pow(2,metadata_.bits);

    Qpx::Calibration this_calib;
    if (metadata_.detectors[i].energy_calibrations_.has_a(Qpx::Calibration("Energy", metadata_.bits)))
      this_calib = metadata_.detectors[i].energy_calibrations_.get(Qpx::Calibration("Energy", metadata_.bits));
    else
      this_calib = metadata_.detectors[i].highest_res_calib();

    axes_[i].resize(res, 0.0);
    for (uint32_t j=0; j<res; j++)
      axes_[i][j] = this_calib.transform(j, metadata_.bits);
  }
}

}
