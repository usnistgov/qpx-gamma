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
 *      Qpx::Sink   generic spectrum type.
 *                  All public methods are thread-safe.
 *                  When deriving override protected methods.
 *
 ******************************************************************************/

#include <fstream>
#include <boost/algorithm/string.hpp>
#include "daq_sink.h"
#include "custom_logger.h"
#include "xmlable.h"
#include "custom_timer.h"
#include "qpx_util.h"

namespace Qpx {

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
        a.branches.add_a(prototype);
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


Sink::Sink()
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

  Setting start_time;
  start_time.id_ = "start_time";
  start_time.metadata.setting_type = SettingType::time;
  start_time.metadata.description = "Start time";
  start_time.metadata.writable = false;
  metadata_.attributes.branches.add(start_time);
}

bool Sink::initialize() {
  metadata_.attributes.enable_if_flag(false, "preset");
  return false; //abstract sink indicates failure to init
}


PreciseFloat Sink::get_count(std::initializer_list<uint16_t> list ) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  if (list.size() != this->metadata_.dimensions())
    return 0;
  return this->_get_count(list);
}

std::unique_ptr<std::list<Entry>> Sink::get_spectrum(std::initializer_list<Pair> list) {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  if (list.size() != this->metadata_.dimensions())
    return 0; //wtf???
  else {
    //    std::vector<Pair> ranges(list.begin(), list.end());
    return this->_get_spectrum(list);
  }
}

void Sink::add_bulk(const Entry& e) {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  if (metadata_.dimensions() < 1)
    return;
  else
    this->_add_bulk(e);
}

bool Sink::from_prototype(const Metadata& newtemplate) {
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

void Sink::addSpill(const Spill& one_spill) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

//  CustomTimer addspill_timer(true);

  for (auto &q : one_spill.hits)
    this->pushHit(q);

  for (auto &q : one_spill.stats)
    this->addStats(q.second);

  if (!one_spill.detectors.empty())
    this->_set_detectors(one_spill.detectors);

  Setting start_time = get_attr("start_time");
  if (start_time.value_time.is_not_a_date_time() && (!one_spill.time.is_not_a_date_time())) {
    start_time.value_time = one_spill.time;
    metadata_.attributes.branches.replace(start_time);
  }

//  addspill_timer.stop();
//  PL_DBG << "<" << metadata_.name << "> added " << hits << " hits in "
//         << addspill_timer.ms() << " ms at " << addspill_timer.us() / hits << " us/hit";

//  PL_DBG << "<" << metadata_.name << "> left in backlog " << backlog.size();
}

void Sink::closeAcquisition() {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  this->_closeAcquisition();
}


//std::map<int, std::list<StatsUpdate>> Sink::get_stats() {
//  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
//  while (!uniqueLock.try_lock())
//    boost::this_thread::sleep_for(boost::chrono::seconds{1});
//  return stats_list_;
//}

std::vector<double> Sink::energies(uint8_t chan) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  
  if (chan < energies_.size())
    return energies_[chan];
  else
    return std::vector<double>();
}
  
void Sink::set_detectors(const std::vector<Qpx::Detector>& dets) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  
  this->_set_detectors(dets);
  metadata_.changed = true;
}

void Sink::reset_changed() {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.changed = false;
}

Setting Sink::get_attr(std::string setting) const {
  return metadata_.attributes.branches.get(Setting(setting));
}

bool Sink::write_file(std::string dir, std::string format) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return _write_file(dir, format);
}

bool Sink::read_file(std::string name, std::string format) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  return _read_file(name, format);
}

//accessors for various properties
Metadata Sink::metadata() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return metadata_;  
}

std::string Sink::type() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return my_type();
}

uint16_t Sink::dimensions() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return metadata_.dimensions();
}

std::string Sink::name() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return metadata_.name;
}

uint16_t Sink::bits() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return metadata_.bits;
}


//change stuff

void Sink::set_generic_attr(Setting setting, Match match) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.attributes.set_setting_r(setting, match);
  metadata_.changed = true;
}

void Sink::set_generic_attrs(Setting settings) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.attributes = settings;
  metadata_.changed = true;
}




/////////////////////
//Save and load//////
/////////////////////

void Sink::to_xml(pugi::xml_node &root) const {
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


bool Sink::from_xml(const pugi::xml_node &node) {
//  if (std::string(node.name()) != "Spectrum")
//    return false;

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

    Pattern pattern_coinc_;
    Pattern pattern_anti_;

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

    Pattern pattern_add_;

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


}
