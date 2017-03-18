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
 *      Qpx::Consumer   generic spectrum type.
 *                  All public methods are thread-safe.
 *                  When deriving override protected methods.
 *
 ******************************************************************************/

//#include <boost/algorithm/string.hpp>
#include "consumer.h"
#include "qpx_util.h"

#ifdef H5_ENABLED
#include "JsonH5.h"
#endif

namespace Qpx {

Consumer::Consumer()
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

bool Consumer::_initialize() {
  metadata_.disable_presets();
  return false; //abstract sink indicates failure to init
}


PreciseFloat Consumer::data(std::initializer_list<size_t> list ) const {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  if (list.size() != this->metadata_.dimensions())
    return 0;
  return this->_data(list);
}

std::unique_ptr<std::list<Entry>> Consumer::data_range(std::initializer_list<Pair> list) {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  if (list.size() != this->metadata_.dimensions())
    return 0; //wtf???
  else {
    //    std::vector<Pair> ranges(list.begin(), list.end());
    return this->_data_range(list);
  }
}

void Consumer::append(const Entry& e) {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  if (metadata_.dimensions() < 1)
    return;
  else
    this->_append(e);
}

bool Consumer::from_prototype(const ConsumerMetadata& newtemplate) {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  if (metadata_.type() != newtemplate.type())
    return false;

  metadata_.overwrite_all_attributes(newtemplate.attributes());
  metadata_.detectors.clear(); // really?

  return (this->_initialize());
//  DBG << "<Consumer::from_prototype>" << metadata_.get_attribute("name").value_text << " made with dims=" << metadata_.dimensions();
//  DBG << "from prototype " << metadata_.debug();
}

void Consumer::push_spill(const Spill& one_spill) {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  this->_push_spill(one_spill);
}

void Consumer::_push_spill(const Spill& one_spill) {
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

void Consumer::flush() {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  this->_flush();
}


std::vector<double> Consumer::axis_values(uint16_t dimension) const
{
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  
  if (dimension < axes_.size())
    return axes_[dimension];
  else
    return std::vector<double>();
}

bool Consumer::changed() const
{
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  return changed_;
}

void Consumer::set_detectors(const std::vector<Qpx::Detector>& dets) {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  
  this->_set_detectors(dets);
  changed_ = true;
}

void Consumer::reset_changed() {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  changed_ = false;
}

bool Consumer::write_file(std::string dir, std::string format) const {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  return _write_file(dir, format);
}

bool Consumer::read_file(std::string name, std::string format) {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  return _read_file(name, format);
}

//accessors for various properties
ConsumerMetadata Consumer::metadata() const {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  return metadata_;
}

std::string Consumer::type() const {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  return my_type();
}

uint16_t Consumer::dimensions() const {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  return metadata_.dimensions();
}

std::string Consumer::debug() const
{
  std::string prepend;

  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);
  std::stringstream ss;
  ss << my_type();
  if (changed_)
    ss << " changed";
  ss << "\n";
  if (axes_.empty())
    ss << prepend << k_branch_mid_B << "Axes empty";
  else
  {
    ss << prepend << k_branch_mid_B << "Axes:\n";
    for (size_t i=0; i < axes_.size();++i)
    {
      if ((i+1) == axes_.size())
        ss << prepend << k_branch_pre_B  << k_branch_end_B << i << ".size=" << axes_.at(i).size() << "\n";
      else
        ss << prepend << k_branch_pre_B  << k_branch_mid_B << i << ".size=" << axes_.at(i).size() << "\n";
    }
  }
  ss << prepend << k_branch_end_B << metadata_.debug(prepend + "  ");
  return ss.str();
}



//change stuff

void Consumer::set_attribute(const Setting &setting) {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.set_attribute(setting);
  changed_ = true;
}

void Consumer::set_attributes(const Setting &settings) {
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.set_attributes(settings);
  changed_ = true;
}




/////////////////////
//Save and load//////
/////////////////////

void Consumer::save(pugi::xml_node &root) const {
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);

  pugi::xml_node node = root.append_child("Consumer");
  node.append_attribute("type").set_value(this->my_type().c_str());

  metadata_.to_xml(node);
//  if (archive.operator bool())
//    _save_data(*archive);

  node.append_child("Data").append_child(pugi::node_pcdata).set_value(this->_data_to_xml().c_str());
}


bool Consumer::load(const pugi::xml_node &node) {

  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  if (node.child(metadata_.xml_element_name().c_str()))
    metadata_.from_xml(node.child(metadata_.xml_element_name().c_str()));

  std::string this_data;
  if (node.child("Data"))
    this_data = std::string(node.child_value("Data"));
  boost::algorithm::trim(this_data);
  this->_data_from_xml(this_data);


//  DBG << "Settings just prior to init \n" + metadata_.attributes().debug();

  bool ret = this->_initialize();

  if (ret)
    this->_recalc_axes();

  return ret;
}

#ifdef H5_ENABLED
bool Consumer::load(H5CC::Group& g, bool withdata)
{
  boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  if (!g.has_group("metadata"))
    return false;

  from_json(json(g.open_group("metadata")), metadata_);
//  metadata_.from_json(g.open_group("metadata"));

  bool ret = this->_initialize();

  if (ret && withdata)
    this->_load_data(g);

  if (ret)
    this->_recalc_axes();

  return ret;
}

bool Consumer::save(H5CC::Group& g) const
{
  boost::shared_lock<boost::shared_mutex> lock(shared_mutex_);

  g.write_attribute("type", this->my_type());

//  json j = metadata_.to_json();
  auto mdg = g.require_group("metadata");

  H5CC::from_json(json(metadata_), mdg);

  this->_save_data(g);
}

#endif


}
