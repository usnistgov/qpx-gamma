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
  , bits_(0)
{
  Setting attributes = metadata_.attributes();

  Setting totalct;
  totalct.id_ = "total_hits";
  totalct.metadata.setting_type = SettingType::floating_precise;
  totalct.metadata.description = "Total hit count";
  totalct.metadata.writable = false;
  totalct.value_precise = 0;
  attributes.branches.add(totalct);

  Setting totalev;
  totalev.id_ = "total_events";
  totalev.metadata.setting_type = SettingType::floating_precise;
  totalev.metadata.description = "Total time-correlated event count";
  totalev.metadata.writable = false;
  totalev.value_precise = 0;
  attributes.branches.add(totalev);


  Qpx::Setting res;
  res.id_ = "resolution";
  res.metadata.setting_type = Qpx::SettingType::int_menu;
  res.metadata.writable = true;
  res.metadata.flags.insert("preset");
  res.metadata.description = "";
  res.value_int = 14;
  res.metadata.int_menu_items[4] = "4 bit (16)";
  res.metadata.int_menu_items[5] = "5 bit (32)";
  res.metadata.int_menu_items[6] = "6 bit (64)";
  res.metadata.int_menu_items[7] = "7 bit (128)";
  res.metadata.int_menu_items[8] = "8 bit (256)";
  res.metadata.int_menu_items[9] = "9 bit (512)";
  res.metadata.int_menu_items[10] = "10 bit (1024)";
  res.metadata.int_menu_items[11] = "11 bit (2048)";
  res.metadata.int_menu_items[12] = "12 bit (4096)";
  res.metadata.int_menu_items[13] = "13 bit (8192)";
  res.metadata.int_menu_items[14] = "14 bit (16384)";
  res.metadata.int_menu_items[15] = "15 bit (32768)";
  res.metadata.int_menu_items[16] = "16 bit (65536)";
  attributes.branches.add(res);

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
  attributes.branches.add(dets);

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
  attributes.branches.add(coinc_window);

  Setting pattern_coinc;
  pattern_coinc.id_ = "pattern_coinc";
  pattern_coinc.metadata.setting_type = SettingType::pattern;
  pattern_coinc.metadata.maximum = 1;
  pattern_coinc.metadata.description = "Coincidence pattern";
  pattern_coinc.metadata.writable = true;
  pattern_coinc.metadata.flags.insert("preset");
  attributes.branches.add(pattern_coinc);

  Setting pattern_anti;
  pattern_anti.id_ = "pattern_anti";
  pattern_anti.metadata.setting_type = SettingType::pattern;
  pattern_anti.metadata.maximum = 1;
  pattern_anti.metadata.description = "Anti-coindicence pattern";
  pattern_anti.metadata.writable = true;
  pattern_anti.metadata.flags.insert("preset");
  attributes.branches.add(pattern_anti);

  Setting pattern_add;
  pattern_add.id_ = "pattern_add";
  pattern_add.metadata.setting_type = SettingType::pattern;
  pattern_add.metadata.maximum = 1;
  pattern_add.metadata.description = "Add pattern";
  pattern_add.metadata.writable = true;
  pattern_add.metadata.flags.insert("preset");
  attributes.branches.add(pattern_add);

  Setting live_time;
  live_time.id_ = "live_time";
  live_time.metadata.setting_type = SettingType::time_duration;
  live_time.metadata.description = "Live time";
  live_time.metadata.writable = false;
  attributes.branches.add(live_time);

  Setting real_time;
  real_time.id_ = "real_time";
  real_time.metadata.setting_type = SettingType::time_duration;
  real_time.metadata.description = "Real time";
  real_time.metadata.writable = false;
  attributes.branches.add(real_time);

  Setting inst_rate;
  inst_rate.id_ = "instant_rate";
  inst_rate.metadata.setting_type = SettingType::floating;
  inst_rate.metadata.unit = "cps";
  inst_rate.metadata.description = "Instant count rate";
  inst_rate.metadata.writable = false;
  inst_rate.value_dbl = 0;
  attributes.branches.add(inst_rate);

  metadata_.overwrite_all_attributes(attributes);
}

bool Spectrum::_initialize() {
  Sink::_initialize();

  pattern_coinc_ = metadata_.get_attribute("pattern_coinc").value_pattern;
  pattern_anti_ = metadata_.get_attribute("pattern_anti").value_pattern;
  pattern_add_ = metadata_.get_attribute("pattern_add").value_pattern;
  coinc_window_ = metadata_.get_attribute("coinc_window").value_dbl;
  bits_ = metadata_.get_attribute("resolution").value_int;
  if (coinc_window_ < 0)
    coinc_window_ = 0;

  max_delay_ = 0;
  Setting perdet = metadata_.get_attribute("per_detector");
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
  if ((newhit.source_channel() < 0)
      || (newhit.source_channel() >= energy_idx_.size()))
    return;

  if ((newhit.source_channel() < cutoff_logic_.size())
      && (newhit.value(energy_idx_.at(newhit.source_channel())).val(bits_) < cutoff_logic_[newhit.source_channel()]))
    return;

  if (newhit.source_channel() < 0)
    return;

  if (!(pattern_coinc_.relevant(newhit.source_channel()) ||
        pattern_anti_.relevant(newhit.source_channel()) ||
        pattern_add_.relevant(newhit.source_channel())))
    return;

  //  DBG << "Processing " << newhit.to_string();

  Hit hit = newhit;
  if (hit.source_channel() < delay_ns_.size())
    hit.delay_ns(delay_ns_[hit.source_channel()]);

  bool appended = false;
  bool pileup = false;
  if (backlog.empty() || backlog.back().past_due(hit))
    backlog.push_back(Event(hit, coinc_window_, max_delay_));
  else {
    for (auto &q : backlog) {
      if (q.in_window(hit)) {
        if (q.addHit(hit)) {
          if (appended)
            DBG << "<" << metadata_.get_attribute("name").value_text << "> "
                << "hit " << hit.to_string() << " coincident with more than one other hit (counted >=2 times)";
          appended = true;
        } else {
          DBG << "<" << metadata_.get_attribute("name").value_text << "> "
              << "pileup hit " << hit.to_string() << " with " << q.to_string() << " already has " << q.hits[hit.source_channel()].to_string();
          pileup = true;
        }
      } else if (q.past_due(hit))
        break;
      else if (q.antecedent(hit))
        DBG << "<" << metadata_.get_attribute("name").value_text << "> "
            << "antecedent hit " << hit.to_string() << ". Something wrong with presorter or daq_device?";
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
      total_events_++;
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

  if (!(
        pattern_coinc_.relevant(newBlock.source_channel) ||
        pattern_anti_.relevant(newBlock.source_channel) ||
        pattern_add_.relevant(newBlock.source_channel)
        ))
    return;

  //DBG << "Spectrum " << metadata_.name << " received update for chan " << newBlock.channel;
  bool chan_new = (stats_list_.count(newBlock.source_channel) == 0);
  bool new_start = (newBlock.stats_type == StatsType::start);

  if (newBlock.source_channel >= energy_idx_.size())
    energy_idx_.resize(newBlock.source_channel + 1, -1);
  if (newBlock.model_hit.name_to_idx.count("energy"))
    energy_idx_[newBlock.source_channel] = newBlock.model_hit.name_to_idx.at("energy");

  Setting start_time = metadata_.get_attribute("start_time");
  if (new_start && start_time.value_time.is_not_a_date_time()) {
    start_time.value_time = newBlock.lab_time;
    metadata_.set_attribute(start_time);
  }

  if (!chan_new
      && new_start
      && (stats_list_[newBlock.source_channel].back().stats_type == StatsType::running))
    stats_list_[newBlock.source_channel].back().stats_type = StatsType::stop;

  if (recent_end_.lab_time.is_not_a_date_time())
    recent_start_ = newBlock;
  else
    recent_start_ = recent_end_;

  recent_end_ = newBlock;

  Setting rate = metadata_.get_attribute("instant_rate");
  rate.value_dbl = 0;
  double recent_time = (recent_end_.lab_time - recent_start_.lab_time).total_milliseconds() * 0.001;
  if (recent_time > 0)
    rate.value_dbl = recent_count_ / recent_time;
  metadata_.set_attribute(rate);

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

    Setting live_time = metadata_.get_attribute("live_time");
    Setting real_time = metadata_.get_attribute("real_time");

    live_time.value_duration = real_time.value_duration = real;
    for (auto &q : real_times_)
      if (q.second.total_milliseconds() < real_time.value_duration.total_milliseconds())
        real_time.value_duration = q.second;

    for (auto &q : live_times_)
      if (q.second.total_milliseconds() < live_time.value_duration.total_milliseconds())
        live_time.value_duration = q.second;

    metadata_.set_attribute(live_time);
    metadata_.set_attribute(real_time);

    //      DBG << "<Spectrum> \"" << metadata_.name << "\"  ********* "
    //             << "RT = " << to_simple_string(metadata_.real_time)
    //             << "   LT = " << to_simple_string(metadata_.live_time) ;

  }

  Setting res = metadata_.get_attribute("total_hits");
  res.value_precise = total_hits_;
  metadata_.set_attribute(res);

  Setting res2 = metadata_.get_attribute("total_events");
  res2.value_precise = total_events_;
  metadata_.set_attribute(res2);
}


void Spectrum::_flush()
{
  Setting res = metadata_.get_attribute("total_hits");
  res.value_precise = total_hits_;
  metadata_.set_attribute(res);

  Setting res2 = metadata_.get_attribute("total_events");
  res2.value_precise = total_events_;
  metadata_.set_attribute(res2);
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
    uint32_t res = pow(2,bits_);

    Qpx::Calibration this_calib;
    if (metadata_.detectors[i].energy_calibrations_.has_a(Qpx::Calibration("Energy", bits_)))
      this_calib = metadata_.detectors[i].energy_calibrations_.get(Qpx::Calibration("Energy", bits_));
    else
      this_calib = metadata_.detectors[i].highest_res_calib();

    axes_[i].resize(res, 0.0);
    for (uint32_t j=0; j<res; j++)
      axes_[i][j] = this_calib.transform(j, bits_);
  }
}

template<class Archive>
void Spectrum::serialize(Archive & ar, const unsigned int version)
{
  ar & boost::serialization::base_object<Sink>(*this);
  //live and real times?
}

}
