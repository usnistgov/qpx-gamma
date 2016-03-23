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
 *      Qpx::Spectrum::Spectrum generic spectrum type.
 *                       All public methods are thread-safe.
 *                       When deriving override protected methods.
 *
 ******************************************************************************/

#include <fstream>
#include <boost/algorithm/string.hpp>
#include "spectrum.h"
#include "custom_logger.h"
#include "xmlable.h"
#include "custom_timer.h"
#include "qpx_util.h"

namespace Qpx {
namespace Spectrum {

void Metadata::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_attribute("Type").set_value(type_.c_str());
  node.append_child("Name").append_child(pugi::node_pcdata).set_value(name.c_str());
  node.append_child("Resolution").append_child(pugi::node_pcdata).set_value(std::to_string(bits).c_str());

  if (attributes.branches.size())
    attributes.to_xml(node);
}

void Metadata::from_xml(const pugi::xml_node &node) {
  if (std::string(node.name()) != xml_element_name())
    return;

  type_ = std::string(node.attribute("Type").value());
  name = std::string(node.child_value("Name"));
  bits = boost::lexical_cast<short>(node.child_value("Resolution"));

  if (node.child(attributes.xml_element_name().c_str()))
    attributes.from_xml(node.child(attributes.xml_element_name().c_str()));
}

void Metadata::set_det_limit(uint16_t limit)
{
  if (limit < 1)
    limit = 1;

  for (auto &a : attributes.branches.my_data_)
    if (a.metadata.setting_type == Qpx::SettingType::pattern) {
      if (a.value_pattern.gates().size() != limit)
        changed = true;
      a.value_pattern.resize(limit);
    } else if (a.metadata.setting_type == Qpx::SettingType::stem) {
      Setting prototype;
      for (auto &p : a.branches.my_data_)
        if (p.indices.count(-1))
          prototype = p;
      if (prototype.metadata.setting_type == Qpx::SettingType::stem) {
        a.indices.clear();
        a.branches.clear();
        prototype.metadata.visible = false;
        a.branches.add(prototype);
        prototype.metadata.visible = true;
        for (int i=0; i < limit; ++i) {
          prototype.indices.clear();
          prototype.indices.insert(i);
          for (auto &p : prototype.branches.my_data_)
            p.indices = prototype.indices;
          a.branches.add_a(prototype);
//          a.indices.insert(i);
        }
      }
    }
}


Spectrum::Spectrum()
  : recent_count_(0)
  , coinc_window_(0)
  , max_delay_(0)
{
  Setting vis;
  vis.id_ = "visible";
  vis.metadata.setting_type = SettingType::boolean;
  vis.metadata.description = "Plot visible";
  vis.metadata.writable = true;
  metadata_.attributes.branches.add(vis);

  Setting app;
  app.id_ = "appearance";
  app.metadata.setting_type = SettingType::color;
  app.metadata.description = "Plot appearance";
  app.metadata.writable = true;
  metadata_.attributes.branches.add(app);

  Setting rescale;
  rescale.id_ = "rescale";
  rescale.metadata.setting_type = SettingType::floating_precise;
  rescale.metadata.description = "Rescale factor";
  rescale.metadata.writable = true;
  rescale.metadata.minimum = 0;
  rescale.metadata.maximum = 1e10;
  rescale.metadata.step = 1;
  rescale.value_precise = 1;
  metadata_.attributes.branches.add(rescale);

  Setting descr;
  descr.id_ = "description";
  descr.metadata.setting_type = SettingType::text;
  descr.metadata.description = "Description";
  descr.metadata.writable = true;
  metadata_.attributes.branches.add(descr);

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

  Setting start_time;
  start_time.id_ = "start_time";
  start_time.metadata.setting_type = SettingType::time;
  start_time.metadata.description = "Start time";
  start_time.metadata.writable = false;
  metadata_.attributes.branches.add(start_time);

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

bool Spectrum::initialize() {
  pattern_coinc_ = get_attr("pattern_coinc").value_pattern;
  pattern_anti_ = get_attr("pattern_anti").value_pattern;
  pattern_add_ = get_attr("pattern_add").value_pattern;
  coinc_window_ = get_attr("coinc_window").value_dbl;

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
      delay_ns_[idx]     = d.get_setting(Setting("cutoff_logic"), Match::id).value_dbl;
      if (delay_ns_[idx] > max_delay_)
        max_delay_ = delay_ns_[idx];
    }
  }

  metadata_.attributes.enable_if_flag(false, "preset");

  if (coinc_window_ < 0)
    coinc_window_ = 0;

  return false;
}


PreciseFloat Spectrum::get_count(std::initializer_list<uint16_t> list ) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  if (list.size() != this->metadata_.dimensions())
    return 0;
  return this->_get_count(list);
}

std::unique_ptr<std::list<Entry>> Spectrum::get_spectrum(std::initializer_list<Pair> list) {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  if (list.size() != this->metadata_.dimensions())
    return 0; //wtf???
  else {
    //    std::vector<Pair> ranges(list.begin(), list.end());
    return this->_get_spectrum(list);
  }
}

void Spectrum::add_bulk(const Entry& e) {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  if (metadata_.dimensions() < 1)
    return;
  else
    this->_add_bulk(e);
}

bool Spectrum::from_prototype(const Metadata& newtemplate) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  if ((metadata_.type() != newtemplate.type()) ||
      (this->my_type() != newtemplate.type()))
    return false;

  metadata_.name = newtemplate.name;
  metadata_.bits = newtemplate.bits;
  metadata_.attributes = newtemplate.attributes;
  return (this->initialize());
}

void Spectrum::addSpill(const Spill& one_spill) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  CustomTimer addspill_timer(true);

  for (auto &q : one_spill.hits)
    this->pushHit(q);

  for (auto &q : one_spill.stats) {
    if (q.second.stats_type == StatsType::stop) {
//      PL_DBG << "<" << metadata_.name << "> final RunInfo received, dumping backlog of events " << backlog.size();
      while (!backlog.empty()) {
        recent_count_++;
        metadata_.total_count++;
        this->addEvent(backlog.front());
        backlog.pop_front();
      }
    }
    this->addStats(q.second);
  }

  if (one_spill.run != RunInfo())
    this->addRun(one_spill.run);

  addspill_timer.stop();
//  PL_DBG << "<" << metadata_.name << "> added " << hits << " hits in "
//         << addspill_timer.ms() << " ms at " << addspill_timer.us() / hits << " us/hit";

//  PL_DBG << "<" << metadata_.name << "> left in backlog " << backlog.size();
}

void Spectrum::pushHit(const Hit& newhit)
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

  //  PL_DBG << "Processing " << newhit.to_string();

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
            PL_DBG << "<" << metadata_.name << "> hit " << hit.to_string() << " coincident with more than one other hit (counted >=2 times)";
          appended = true;
        } else {
          PL_DBG << "<" << metadata_.name << "> pileup hit " << hit.to_string() << " with " << q.to_string() << " already has " << q.hits[hit.source_channel].to_string();
          pileup = true;
        }
      } else if (q.past_due(hit))
        break;
      else if (q.antecedent(hit))
        PL_DBG << "<" << metadata_.name << "> antecedent hit " << hit.to_string() << ". Something wrong with presorter or daq_device?";
    }

    if (!appended && !pileup) {
      backlog.push_back(Event(hit, coinc_window_, max_delay_));
      PL_DBG << "append fresh";
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

void Spectrum::closeAcquisition() {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  this->_closeAcquisition();
}


bool Spectrum::validateEvent(const Event& newEvent) const {
  if (!pattern_coinc_.validate(newEvent))
    return false;
  if (!pattern_anti_.antivalidate(newEvent))
    return false;
  return true;
}

std::map<int, std::list<StatsUpdate>> Spectrum::get_stats() {
  return stats_list_;
}


void Spectrum::addStats(const StatsUpdate& newBlock) {
  //private; no lock required

  if (pattern_coinc_.relevant(newBlock.source_channel) ||
      pattern_anti_.relevant(newBlock.source_channel) ||
      pattern_add_.relevant(newBlock.source_channel)) {
    //PL_DBG << "Spectrum " << metadata_.name << " received update for chan " << newBlock.channel;
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
//          PL_DBG << "<Spectrum> \"" << metadata_.name << "\" RT + "
//                 << rt << " = " << real << "   LT + " << lt << " = " << live;
//          PL_DBG << "<Spectrum> \"" << metadata_.name << "\" FROM "
//                 << boost::posix_time::to_iso_extended_string(start.lab_time);
        } else {
          lt = rt = q.lab_time - start.lab_time;
          StatsUpdate diff = q - start;
//          PL_DBG << "<Spectrum> \"" << metadata_.name << "\" TOTAL_TIME " << q.total_time << "-" << start.total_time
//                 << " = " << diff.total_time;
          if (diff.total_time > 0) {
            double scale_factor = rt.total_microseconds() / diff.total_time;
            double this_time_unscaled = diff.live_time - diff.sfdt;
            lt = boost::posix_time::microseconds(static_cast<long>(this_time_unscaled * scale_factor));
//            PL_DBG << "<Spectrum> \"" << metadata_.name << "\" TO "
//                   << boost::posix_time::to_iso_extended_string(q.lab_time)
//                   << "  RT = " << rt << "   LT = " << lt;
          }
        }
      }

      if (stats_list_[newBlock.source_channel].back().stats_type != StatsType::start) {
        real += rt;
        live += lt;
//        PL_DBG << "<Spectrum> \"" << metadata_.name << "\" RT + "
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

//      PL_DBG << "<Spectrum> \"" << metadata_.name << "\"  ********* "
//             << "RT = " << to_simple_string(metadata_.real_time)
//             << "   LT = " << to_simple_string(metadata_.live_time) ;

    }
  }
}


void Spectrum::addRun(const RunInfo& run_info) {
  //private; no lock required

 if (run_info.detectors.size() > 0)
    this->_set_detectors(run_info.detectors);

 //backwards compat
 Setting start_time = get_attr("start_time");
 if (start_time.value_time.is_not_a_date_time() && (!run_info.time.is_not_a_date_time())) {
   start_time.value_time = run_info.time;
   metadata_.attributes.branches.replace(start_time);
 }
}

std::vector<double> Spectrum::energies(uint8_t chan) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  
  if (chan < energies_.size())
    return energies_[chan];
  else
    return std::vector<double>();
}
  
void Spectrum::set_detectors(const std::vector<Qpx::Detector>& dets) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  
  this->_set_detectors(dets);
  metadata_.changed = true;
}

void Spectrum::reset_changed() {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.changed = false;
}

void Spectrum::_set_detectors(const std::vector<Qpx::Detector>& dets) {
  //private; no lock required
//  PL_DBG << "<Spectrum> _set_detectors";

  metadata_.detectors.clear();

  //metadata_.detectors.resize(metadata_.dimensions(), Qpx::Detector());
  recalc_energies();
}

void Spectrum::recalc_energies() {
  //private; no lock required

  energies_.resize(metadata_.dimensions());
  if (energies_.size() != metadata_.detectors.size())
    return;

  for (int i=0; i < metadata_.detectors.size(); ++i) {
    uint32_t res = pow(2,metadata_.bits);

    Qpx::Calibration this_calib;
    if (metadata_.detectors[i].energy_calibrations_.has_a(Qpx::Calibration("Energy", metadata_.bits)))
      this_calib = metadata_.detectors[i].energy_calibrations_.get(Qpx::Calibration("Energy", metadata_.bits));
    else
      this_calib = metadata_.detectors[i].highest_res_calib();

    energies_[i].resize(res, 0.0);
    for (uint32_t j=0; j<res; j++)
      energies_[i][j] = this_calib.transform(j, metadata_.bits);
  }
}

Setting Spectrum::get_attr(std::string setting) const {
  return metadata_.attributes.branches.get(Setting(setting));
}

bool Spectrum::write_file(std::string dir, std::string format) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return _write_file(dir, format);
}

bool Spectrum::read_file(std::string name, std::string format) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  return _read_file(name, format);
}

std::string Spectrum::channels_to_xml() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return _channels_to_xml();
}

uint16_t Spectrum::channels_from_xml(const std::string& str) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  return _channels_from_xml(str);
}


//accessors for various properties
Metadata Spectrum::metadata() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return metadata_;  
}

std::string Spectrum::type() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return my_type();
}

uint16_t Spectrum::dimensions() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return metadata_.dimensions();
}

std::string Spectrum::name() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return metadata_.name;
}

uint16_t Spectrum::bits() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return metadata_.bits;
}


//change stuff

void Spectrum::set_generic_attr(Setting setting, Match match) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.attributes.set_setting_r(setting, match);
  metadata_.changed = true;
}

void Spectrum::set_generic_attrs(Setting settings) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.attributes = settings;
  metadata_.changed = true;
}




/////////////////////
//Save and load//////
/////////////////////

void Spectrum::to_xml(pugi::xml_node &root) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);

  pugi::xml_node node = root.append_child("Spectrum");

  node.append_attribute("type").set_value(this->my_type().c_str());

  node.append_child("Name").append_child(pugi::node_pcdata).set_value(metadata_.name.c_str());
  node.append_child("TotalEvents").append_child(pugi::node_pcdata).set_value(metadata_.total_count.str().c_str());
  node.append_child("Resolution").append_child(pugi::node_pcdata).set_value(std::to_string(metadata_.bits).c_str());

  if (metadata_.attributes.branches.size())
    metadata_.attributes.to_xml(node);

  if (metadata_.detectors.size()) {
    pugi::xml_node child = node.append_child("Detectors");
    for (auto q : metadata_.detectors) {
      q.settings_ = Setting();
      q.to_xml(child);
    }
  }

  if ((metadata_.bits > 0) && (metadata_.total_count > 0))
    node.append_child("ChannelData").append_child(pugi::node_pcdata).set_value(this->_channels_to_xml().c_str());
}


bool Spectrum::from_xml(const pugi::xml_node &node) {
  if (std::string(node.name()) != "Spectrum")
    return false;

  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  if (node.child(metadata_.attributes.xml_element_name().c_str()))
    metadata_.attributes.from_xml(node.child(metadata_.attributes.xml_element_name().c_str()));
  else if (node.child("Attributes"))
    metadata_.attributes.branches.from_xml(node.child("Attributes"));

  std::string numero;

  if (!node.child("Name"))
    return false;
  metadata_.name = std::string(node.child_value("Name"));

  metadata_.total_count = PreciseFloat(node.child_value("TotalEvents"));
  metadata_.bits = boost::lexical_cast<short>(std::string(node.child_value("Resolution")));

  if ((!metadata_.bits) || (metadata_.total_count == 0))
    return false;

  //backwards compat
  if (node.child("MatchPattern")) {
    std::vector<int16_t> match_pattern;
    std::stringstream pattern_match(node.child_value("MatchPattern"));
    while (pattern_match.rdbuf()->in_avail()) {
      pattern_match >> numero;
      match_pattern.push_back(boost::lexical_cast<short>(boost::algorithm::trim_copy(numero)));
    }

    pattern_coinc_.resize(match_pattern.size());
    pattern_anti_.resize(match_pattern.size());

    std::vector<bool> gts_co(match_pattern.size(), false);
    std::vector<bool> gts_a(match_pattern.size(), false);
    size_t thresh_co(0), thresh_a(0);

    for (int i = 0; i < match_pattern.size(); ++i) {
      if (match_pattern[i] > 0) {
        gts_co[i] = true;
        thresh_co++;
      }
      if (match_pattern[i] < 0) {
        gts_a[i] = true;
        thresh_a++;
      }
    }

    pattern_coinc_.set_gates(gts_co);
    pattern_coinc_.set_theshold(thresh_co);
    pattern_anti_.set_gates(gts_a);
    pattern_anti_.set_theshold(thresh_a);

    Setting pattern;
    pattern = metadata_.attributes.branches.get(Setting("pattern_coinc"));
    pattern.value_pattern = pattern_coinc_;
    metadata_.attributes.branches.replace(pattern);

    pattern = metadata_.attributes.branches.get(Setting("pattern_anti"));
    pattern.value_pattern = pattern_anti_;
    metadata_.attributes.branches.replace(pattern);
  }

  //backwards compat
  if (node.child("AddPattern")) {
    std::vector<int16_t> add_pattern;
    std::stringstream pattern_add(node.child_value("AddPattern"));
    while (pattern_add.rdbuf()->in_avail()) {
      pattern_add >> numero;
      add_pattern.push_back(boost::lexical_cast<short>(boost::algorithm::trim_copy(numero)));
    }

    pattern_add_.resize(add_pattern.size());
    std::vector<bool> gts(add_pattern.size(), false);
    size_t thresh(0);

    for (int i = 0; i < add_pattern.size(); ++i) {
      if (add_pattern[i] != 0) {
        gts[i] = true;
        thresh++;
      }
    }

    pattern_add_.set_gates(gts);
    pattern_add_.set_theshold(thresh);

    Setting pattern;
    pattern = metadata_.attributes.branches.get(Setting("pattern_add"));
    pattern.value_pattern = pattern_add_;
    metadata_.attributes.branches.replace(pattern);
  }

  if (node.child("StartTime")) {
    Setting start_time = get_attr("start_time");
    start_time.value_time = from_iso_extended(node.child_value("StartTime"));
    metadata_.attributes.branches.replace(start_time);
  }

  if (node.child("RealTime")) {
    Setting real_time = get_attr("real_time");
    real_time.value_duration = boost::posix_time::duration_from_string(node.child_value("RealTime"));
    metadata_.attributes.branches.replace(real_time);
  }

  if (node.child("LiveTime")) {
    Setting live_time = get_attr("live_time");
    live_time.value_duration = boost::posix_time::duration_from_string(node.child_value("LiveTime"));
    metadata_.attributes.branches.replace(live_time);
  }

  if (node.child("RescaleFactor")) {
    Setting rescale = get_attr("rescale");
    rescale.value_precise = PreciseFloat(node.child_value("RescaleFactor"));
    metadata_.attributes.branches.replace(rescale);
  }

  if (node.child("Detectors")) {
    metadata_.detectors.clear();
    for (auto &q : node.child("Detectors").children()) {
      Qpx::Detector det;
      det.from_xml(q);
      metadata_.detectors.push_back(det);
    }
  }

  std::string this_data(node.child_value("ChannelData"));
  boost::algorithm::trim(this_data);
  this->_channels_from_xml(this_data);


  if (node.child("Appearance")) {
    uint32_t col = boost::lexical_cast<unsigned int>(std::string(node.child_value("Appearance")));
    Setting app = metadata_.attributes.branches.get(Setting("appearance"));
    app.value_text = "#" + itohex32(col);
    metadata_.attributes.branches.replace(app);
  }

  if (node.child("Visible")) {
    Setting vis = metadata_.attributes.branches.get(Setting("visible"));
    vis.value_int = boost::lexical_cast<bool>(std::string(node.child_value("Visible")));
    metadata_.attributes.branches.replace(vis);
  }


  bool ret = this->initialize();

  if (ret)
   this->recalc_energies();

  return ret;
}


}}
