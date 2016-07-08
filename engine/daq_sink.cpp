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

#include <boost/algorithm/string.hpp>
#include "daq_sink.h"
#include "custom_logger.h"
//#include "custom_timer.h"
#include "qpx_util.h"

namespace Qpx {

Metadata::Metadata()
  : type_("invalid")
  , dimensions_(0)
  , attributes_("Options")
{
  attributes_.metadata.setting_type = SettingType::stem;
}

Metadata::Metadata(std::string tp, std::string descr, uint16_t dim,
                   std::list<std::string> itypes, std::list<std::string> otypes)
  : type_(tp)
  , type_description_(descr)
  , dimensions_(dim)
  , input_types_(itypes)
  , output_types_(otypes)
  , attributes_("Options")
{
  attributes_.metadata.setting_type = SettingType::stem;
}

bool Metadata::shallow_equals(const Metadata& other) const
{
  return operator ==(other);
}

bool Metadata::operator!= (const Metadata& other) const
{
  return !operator==(other);
}

bool Metadata::operator== (const Metadata& other) const
{
  if (type_ != other.type_) return false; //assume other type info same
  if (attributes_ != other.attributes_) return false;
  return true;
}

Setting Metadata::get_attribute(Setting setting) const
{
  return attributes_.get_setting(setting, Match::id | Match::indices);
}

Setting Metadata::get_attribute(std::string setting) const
{
  return attributes_.get_setting(Setting(setting), Match::id | Match::indices);
}

Setting Metadata::get_attribute(std::string setting, int32_t idx) const
{
  Setting find(setting);
  find.indices.insert(idx);
  return attributes_.get_setting(find, Match::id | Match::indices);
}

void Metadata::set_attribute(const Setting &setting)
{
  attributes_.set_setting_r(setting, Match::id | Match::indices);
}

Setting Metadata::attributes() const
{
  return attributes_;
}

void Metadata::set_attributes(const Setting &settings)
{
  if (settings.branches.size())
    for (auto &s : settings.branches.my_data_)
      set_attributes(s);
  else if (attributes_.has(settings, Match::id | Match::indices))
    set_attribute(settings);
}

void Metadata::overwrite_all_attributes(const Setting &settings)
{
  attributes_ = settings;
}

void Metadata::disable_presets()
{
  attributes_.enable_if_flag(false, "preset");
}


void Metadata::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_attribute("Type").set_value(type_.c_str());

  if (attributes_.branches.size())
    attributes_.to_xml(node);

  if (!detectors.empty())
  {
    pugi::xml_node detnode = root.append_child("Detectors");
    for (auto &d : detectors)
      d.to_xml(detnode);
  }
}

void Metadata::from_xml(const pugi::xml_node &node) {
  if (std::string(node.name()) != xml_element_name())
    return;

  type_ = std::string(node.attribute("Type").value());

  if (node.child(attributes_.xml_element_name().c_str()))
    attributes_.from_xml(node.child(attributes_.xml_element_name().c_str()));

  if (node.child("Detectors")) {
    detectors.clear();
    for (auto &q : node.child("Detectors").children()) {
      Qpx::Detector det;
      det.from_xml(q);
      detectors.push_back(det);
    }
  }
}

void Metadata::set_det_limit(uint16_t limit)
{
  if (limit < 1)
    limit = 1;

  for (auto &a : attributes_.branches.my_data_)
    if (a.metadata.setting_type == Qpx::SettingType::pattern)
      a.value_pattern.resize(limit);
    else if (a.metadata.setting_type == Qpx::SettingType::stem) {
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

bool Metadata::chan_relevant(uint16_t chan) const
{
  for (const Setting &s : attributes_.branches.my_data_)
    if ((s.metadata.setting_type == SettingType::pattern) &&
        (s.value_pattern.relevant(chan)))
      return true;
  return false;
}




Sink::Sink()
{
  Setting attributes = metadata_.attributes();

  Setting name;
  name.id_ = "name";
  name.metadata.setting_type = SettingType::text;
  name.metadata.writable = true;
  attributes.branches.add(name);

  Setting vis;
  vis.id_ = "visible";
  vis.metadata.setting_type = SettingType::boolean;
  vis.metadata.description = "Plot visible";
  vis.metadata.writable = true;
  attributes.branches.add(vis);

  Setting app;
  app.id_ = "appearance";
  app.metadata.setting_type = SettingType::color;
  app.metadata.description = "Plot appearance";
  app.metadata.writable = true;
  attributes.branches.add(app);

  Setting rescale;
  rescale.id_ = "rescale";
  rescale.metadata.setting_type = SettingType::floating_precise;
  rescale.metadata.description = "Rescale factor";
  rescale.metadata.writable = true;
  rescale.metadata.minimum = 0;
  rescale.metadata.maximum = 1e10;
  rescale.metadata.step = 1;
  rescale.value_precise = 1;
  attributes.branches.add(rescale);

  Setting descr;
  descr.id_ = "description";
  descr.metadata.setting_type = SettingType::text;
  descr.metadata.description = "Description";
  descr.metadata.writable = true;
  attributes.branches.add(descr);

  Setting start_time;
  start_time.id_ = "start_time";
  start_time.metadata.setting_type = SettingType::time;
  start_time.metadata.description = "Start time";
  start_time.metadata.writable = false;
  attributes.branches.add(start_time);

  metadata_.overwrite_all_attributes(attributes);
}

bool Sink::_initialize() {
  metadata_.disable_presets();
  return false; //abstract sink indicates failure to init
}


PreciseFloat Sink::data(std::initializer_list<uint16_t> list ) const {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  if (list.size() != this->metadata_.dimensions())
    return 0;
  return this->_data(list);
}

std::unique_ptr<std::list<Entry>> Sink::data_range(std::initializer_list<Pair> list) {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  if (list.size() != this->metadata_.dimensions())
    return 0; //wtf???
  else {
    //    std::vector<Pair> ranges(list.begin(), list.end());
    return this->_data_range(list);
  }
}

void Sink::append(const Entry& e) {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  if (metadata_.dimensions() < 1)
    return;
  else
    this->_append(e);
}

bool Sink::from_prototype(const Metadata& newtemplate) {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  if ((metadata_.type() != newtemplate.type()) ||
      (this->my_type() != newtemplate.type()))
    return false;

  metadata_ = newtemplate;
  metadata_.detectors.clear(); // really?
  return (this->_initialize());
}

void Sink::push_spill(const Spill& one_spill) {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  this->_push_spill(one_spill);
}

void Sink::_push_spill(const Spill& one_spill) {
  //  CustomTimer addspill_timer(true);

  if (!one_spill.detectors.empty())
    this->_set_detectors(one_spill.detectors);

  for (auto &q : one_spill.hits)
    this->_push_hit(q);

  for (auto &q : one_spill.stats)
    this->_push_stats(q.second);

  //  addspill_timer.stop();
  //  DBG << "<" << metadata_.name << "> added " << hits << " hits in "
  //         << addspill_timer.ms() << " ms at " << addspill_timer.us() / hits << " us/hit";

  //  DBG << "<" << metadata_.name << "> left in backlog " << backlog.size();
}

void Sink::flush() {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  this->_flush();
}


std::vector<double> Sink::axis_values(uint16_t dimension) const
{
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  
  if (dimension < axes_.size())
    return axes_[dimension];
  else
    return std::vector<double>();
}

bool Sink::changed() const
{
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  return changed_;
}

void Sink::set_detectors(const std::vector<Qpx::Detector>& dets) {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  
  this->_set_detectors(dets);
  changed_ = true;
}

void Sink::reset_changed() {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  changed_ = false;
}

bool Sink::write_file(std::string dir, std::string format) const {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  return _write_file(dir, format);
}

bool Sink::read_file(std::string name, std::string format) {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  return _read_file(name, format);
}

//accessors for various properties
Metadata Sink::metadata() const {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  return metadata_;
}

std::string Sink::type() const {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  return my_type();
}

uint16_t Sink::dimensions() const {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  return metadata_.dimensions();
}



//change stuff

void Sink::set_attribute(const Setting &setting) {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.set_attribute(setting);
  changed_ = true;
}

void Sink::set_attributes(const Setting &settings) {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.set_attributes(settings);
  changed_ = true;
}




/////////////////////
//Save and load//////
/////////////////////

void Sink::to_xml(pugi::xml_node &root) const {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);

  pugi::xml_node node = root.append_child("Sink");
  node.append_attribute("type").set_value(this->my_type().c_str());

  metadata_.to_xml(node);

  node.append_child("Data").append_child(pugi::node_pcdata).set_value(this->_data_to_xml().c_str());
}


bool Sink::from_xml(const pugi::xml_node &node) {

  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  if (node.child(metadata_.xml_element_name().c_str()))
    metadata_.from_xml(node.child(metadata_.xml_element_name().c_str()));

  if (node.child(metadata_.attributes().xml_element_name().c_str()))
  {
    Setting attrs = metadata_.attributes();
    attrs.from_xml(node.child(attrs.xml_element_name().c_str()));
    metadata_.overwrite_all_attributes(attrs);
  }
  else if (node.child("attributes"))
  {
    Setting attrs = metadata_.attributes();
    attrs.from_xml(node.child("attributes"));
    metadata_.overwrite_all_attributes(attrs);
  }

  std::string numero;

  if (node.child("Name"))
  {
    Setting res = metadata_.get_attribute("name");
    if (res.metadata.setting_type == SettingType::text)
    {
      res.value_text = std::string(node.child_value("Name"));
      metadata_.set_attribute(res);
    }
  }

  if (node.child("TotalEvents")) {
    Setting res = metadata_.get_attribute("total_events");
    if (res.metadata.setting_type == SettingType::floating_precise)
    {
      res.value_precise = PreciseFloat(node.child_value("TotalEvents"));
      metadata_.set_attribute(res);
    }
    Setting res2 = metadata_.get_attribute("total_hits");
    if (res2.metadata.setting_type == SettingType::floating_precise)
    {
      res2.value_precise = res.value_precise * metadata_.dimensions();
      metadata_.set_attribute(res2);
    }
  }

  if (node.child("Resolution")) {
    Setting res = metadata_.get_attribute("resolution");
    if (res.metadata.setting_type == SettingType::int_menu)
    {
      res.value_int = boost::lexical_cast<short>(std::string(node.child_value("Resolution")));
      metadata_.set_attribute(res);
    }
  }

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
    pattern = metadata_.get_attribute("pattern_coinc");
    pattern.value_pattern = pattern_coinc_;
    metadata_.set_attribute(pattern);

    pattern = metadata_.get_attribute("pattern_anti");
    pattern.value_pattern = pattern_anti_;
    metadata_.set_attribute(pattern);
  }

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
    pattern = metadata_.get_attribute("pattern_add");
    pattern.value_pattern = pattern_add_;
    metadata_.set_attribute(pattern);
  }

  if (node.child("StartTime")) {
    Setting start_time = metadata_.get_attribute("start_time");
    start_time.value_time = from_iso_extended(node.child_value("StartTime"));
    metadata_.set_attribute(start_time);
  }

  if (node.child("RealTime")) {
    Setting real_time = metadata_.get_attribute("real_time");
    real_time.value_duration = boost::posix_time::duration_from_string(node.child_value("RealTime"));
    metadata_.set_attribute(real_time);
  }

  if (node.child("LiveTime")) {
    Setting live_time = metadata_.get_attribute("live_time");
    live_time.value_duration = boost::posix_time::duration_from_string(node.child_value("LiveTime"));
    metadata_.set_attribute(live_time);
  }

  if (node.child("RescaleFactor")) {
    Setting rescale = metadata_.get_attribute("rescale");
    rescale.value_precise = PreciseFloat(node.child_value("RescaleFactor"));
    metadata_.set_attribute(rescale);
  }

  if (node.child("Detectors")) {
    metadata_.detectors.clear();
    for (auto &q : node.child("Detectors").children()) {
      Qpx::Detector det;
      det.from_xml(q);
      metadata_.detectors.push_back(det);
    }
  }

  if (node.child("Appearance")) {
    uint32_t col = boost::lexical_cast<unsigned int>(std::string(node.child_value("Appearance")));
    Setting app = metadata_.get_attribute("appearance");
    app.value_text = "#" + itohex32(col);
    metadata_.set_attribute(app);
  }

  if (node.child("Visible")) {
    Setting vis = metadata_.get_attribute("visible");
    vis.value_int = boost::lexical_cast<bool>(std::string(node.child_value("Visible")));
    metadata_.set_attribute(vis);
  }

  std::string this_data;
  if (node.child("ChannelData"))  //back compat
    this_data = std::string(node.child_value("ChannelData"));
  else if (node.child("Data"))   //current
    this_data = std::string(node.child_value("Data"));
  boost::algorithm::trim(this_data);
  this->_data_from_xml(this_data);

  bool ret = this->_initialize();

  if (ret)
    this->_recalc_axes();

  //  for (auto &det : this->metadata_.detectors)
  //    DBG << "det2 " << this->metadata_.name << " " << det.name_;

  return ret;
}


}
