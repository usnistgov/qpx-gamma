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
#include "xmlable2.h"

namespace Qpx {
namespace Spectrum {

uint64_t Spectrum::get_count(std::initializer_list<uint16_t> list ) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  if (list.size() != this->metadata_.dimensions)
    return 0;
  return this->_get_count(list);
}

std::unique_ptr<std::list<Entry>> Spectrum::get_spectrum(std::initializer_list<Pair> list) {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  if (list.size() != this->metadata_.dimensions)
    return 0; //wtf???
  else {
    //    std::vector<Pair> ranges(list.begin(), list.end());
    return this->_get_spectrum(list);
  }
}

void Spectrum::add_bulk(const Entry& e) {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  if (metadata_.dimensions < 1)
    return;
  else
    this->_add_bulk(e);
}

bool Spectrum::from_template(const Template& newtemplate) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  
  metadata_.bits = newtemplate.bits;
  shift_by_ = 16 - metadata_.bits;
  metadata_.resolution = pow(2,metadata_.bits);
  metadata_.name = newtemplate.name_;
  metadata_.appearance = newtemplate.appearance;
  metadata_.visible = newtemplate.visible;
  metadata_.match_pattern = newtemplate.match_pattern;
  metadata_.add_pattern = newtemplate.add_pattern;
  metadata_.attributes = newtemplate.generic_attributes;

  return (this->initialize());
}

void Spectrum::addSpill(const Spill& one_spill) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  for (auto &q : one_spill.hits)
    this->pushHit(q);

  if ((one_spill.run != nullptr) && (one_spill.run->total_events > 0)) {
    PL_DBG << "<" << metadata_.name << "> final RunInfo received, dumping backlog of events " << backlog.size();
    while (!backlog.empty()) {
      this->addEvent(backlog.front());
      backlog.pop_front();
    }
  }

  for (auto &q : one_spill.stats)
    this->addStats(q);

  if (one_spill.run != nullptr)
    this->addRun(*one_spill.run);

  //PL_DBG << "left in backlog " << backlog.size();
}

void Spectrum::pushHit(const Hit& newhit)
{
  double coinc_window = 3.0;
  bool appended = false;
  bool pileup = false;
  if (backlog.empty() || !backlog.back().in_window(newhit.timestamp)) {
    backlog.push_back(Event(newhit, coinc_window)); //simple add
  } else {
    for (auto &q : backlog) {
      if (q.in_window(newhit.timestamp)) {
        if ((q.hit.count(newhit.channel) == 0) && !appended) {
          //coincidence
          q.addHit(newhit);
          appended = true;
        } else if ((q.hit.count(newhit.channel) == 0) && appended) {
          //second coincidence
          q.addHit(newhit);
          PL_DBG << "<" << metadata_.name << "> processed hit " << newhit.to_string() << " coincident with more than one other hit (counted >=2 times)";
        } else {
          PL_DBG << "<" << metadata_.name << "> detected pileup hit " << newhit.to_string() << " with " << q.to_string() << " already has " << q.hit[newhit.channel].to_string();
          //PL_DBG << "Logical pileup detected. Event may be discarded";
          pileup = true;
        }
      } else
        break;
    }

    if (!appended && !pileup)
      backlog.push_back(Event(newhit, coinc_window));
  }

  if (!backlog.empty()) {
    Event evt = backlog.front();
    while ((newhit.timestamp - evt.upper_time) > (coinc_window*2)) {
      if (validateEvent(evt))
        this->addEvent(evt);
      backlog.pop_front();
      if (!backlog.empty())
        evt = backlog.front();
      else
        break;
    }
  }

}

void Spectrum::closeAcquisition() {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  _closeAcquisition();
}


bool Spectrum::validateEvent(const Event& newEvent) const {
  bool addit = true;
  for (int i = 0; i < metadata_.match_pattern.size(); i++)
    if (((metadata_.match_pattern[i] == 1) && (newEvent.hit.count(i) == 0)) ||
        ((metadata_.match_pattern[i] == -1) && (newEvent.hit.count(i) > 0)))
      addit = false;
  return addit;
}

void Spectrum::addStats(const StatsUpdate& newBlock) {
  //private; no lock required

  if ((newBlock.channel >= 0) && (newBlock.channel < metadata_.add_pattern.size()) && (metadata_.add_pattern[newBlock.channel])) {
    if (start_stats.count(newBlock.channel) == 0)
      start_stats[newBlock.channel] = newBlock;
    else {
      metadata_.real_time   = newBlock.lab_time - start_stats[newBlock.channel].lab_time;
      StatsUpdate diff = newBlock - start_stats[newBlock.channel];
      if (!diff.total_time)
        return;
      double scale_factor = metadata_.real_time.total_microseconds() / 1000000 / diff.total_time;

      metadata_.live_time = metadata_.real_time;
      double this_time_unscaled = diff.live_time - diff.sfdt;
      if (this_time_unscaled < metadata_.live_time.total_seconds())
        metadata_.live_time = boost::posix_time::microseconds(static_cast<long>(this_time_unscaled * scale_factor * 1000000));
    }
  }
}


void Spectrum::addRun(const RunInfo& run_info) {
  //private; no lock required

  metadata_.start_time = run_info.time_start;
  if (run_info.detectors.size() > 0)
    _set_detectors(run_info.detectors);

  if (run_info.time_start == run_info.time_stop)
    return;
  metadata_.real_time = run_info.time_stop - run_info.time_start;
  double scale_factor = run_info.time_scale_factor();
  metadata_.live_time = metadata_.real_time;
  for (int i = 0; i < metadata_.add_pattern.size(); i++) { //using shortest live time of all added channels
    double live = run_info.state.get_setting(Gamma::Setting("Pixie4/System/module/channel/LIVE_TIME", 26, Gamma::SettingType::floating, i)).value_dbl;
    double sfdt = run_info.state.get_setting(Gamma::Setting("Pixie4/System/module/channel/SFDT", 34, Gamma::SettingType::floating, i)).value_dbl;
    double this_time_unscaled =live - sfdt;
    if ((metadata_.add_pattern[i]) && (this_time_unscaled <= metadata_.live_time.total_seconds()))
      metadata_.live_time = boost::posix_time::microseconds(static_cast<long>(this_time_unscaled * scale_factor * 1000000));
  }
}

std::vector<double> Spectrum::energies(uint8_t chan) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  
  if (chan < metadata_.dimensions)
    return energies_[chan];
  else
    return std::vector<double>();
}
  
void Spectrum::set_detectors(const std::vector<Gamma::Detector>& dets) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  
  _set_detectors(dets);
}

void Spectrum::_set_detectors(const std::vector<Gamma::Detector>& dets) {
  //private; no lock required

  metadata_.detectors.clear();

  //metadata_.detectors.resize(metadata_.dimensions, Gamma::Detector());
  recalc_energies();
}

void Spectrum::recalc_energies() {
  //private; no lock required

  energies_.resize(metadata_.dimensions);
  if (energies_.size() != metadata_.detectors.size())
    return;

  for (int i=0; i < metadata_.detectors.size(); ++i) {
    energies_[i].resize(metadata_.resolution, 0.0);
     Gamma::Calibration this_calib;
    if (metadata_.detectors[i].energy_calibrations_.has_a(Gamma::Calibration("Energy", metadata_.bits)))
      this_calib = metadata_.detectors[i].energy_calibrations_.get(Gamma::Calibration("Energy", metadata_.bits));
    else
      this_calib = metadata_.detectors[i].highest_res_calib();
    for (uint32_t j=0; j<metadata_.resolution; j++)
      energies_[i][j] = this_calib.transform(j, metadata_.bits);
  }
}

Gamma::Setting Spectrum::get_attr(std::string setting) const {
  return metadata_.attributes.get(Gamma::Setting(setting));
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
  return metadata_.dimensions;
}

uint32_t Spectrum::resolution() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return metadata_.resolution;
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

void Spectrum::set_visible(bool vis) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.visible = vis;
}

void Spectrum::set_appearance(uint32_t newapp) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.appearance = newapp;
}

void Spectrum::set_start_time(boost::posix_time::ptime newtime) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.start_time = newtime;
}

void Spectrum::set_description(std::string newdesc) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  metadata_.description = newdesc;
}

void Spectrum::set_generic_attr(Gamma::Setting setting) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  for (auto &q : metadata_.attributes.my_data_) {
    if ((q.id_ == setting.id_) && (q.metadata.writable))
      q = setting;
  }
}



/////////////////////
//Save and load//////
/////////////////////

void Spectrum::to_xml(tinyxml2::XMLPrinter& printer) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);

  std::stringstream patterndata;
  
  printer.OpenElement("PixieSpectrum");
  printer.PushAttribute("type", this->my_type().c_str());

  printer.OpenElement("TotalEvents");
  printer.PushText(metadata_.total_count.str().c_str());
  printer.CloseElement();

  printer.OpenElement("Name");
  printer.PushText(metadata_.name.c_str());
  printer.CloseElement();

  printer.OpenElement("Appearance");
  printer.PushText(std::to_string(metadata_.appearance).c_str());
  printer.CloseElement();

  printer.OpenElement("Visible");
  printer.PushText(std::to_string(metadata_.visible).c_str());
  printer.CloseElement();

  printer.OpenElement("Resolution");
  printer.PushText(std::to_string(metadata_.bits).c_str());
  printer.CloseElement();

  printer.OpenElement("MatchPattern");
  for (auto &q: metadata_.match_pattern)
    patterndata << static_cast<short>(q) << " ";
  printer.PushText(boost::algorithm::trim_copy(patterndata.str()).c_str());
  printer.CloseElement();

  patterndata.str(std::string()); //clear it
  printer.OpenElement("AddPattern");
  for (auto &q: metadata_.add_pattern)
    patterndata << static_cast<short>(q) << " ";
  printer.PushText(boost::algorithm::trim_copy(patterndata.str()).c_str());
  printer.CloseElement();
  
  printer.OpenElement("RealTime");
  printer.PushText(to_simple_string(metadata_.real_time).c_str());
  printer.CloseElement();

  printer.OpenElement("LiveTime");
  printer.PushText(to_simple_string(metadata_.live_time).c_str());
  printer.CloseElement();

  if (metadata_.attributes.size()) {
    printer.OpenElement("GenericAttributes");
    for (auto &q: metadata_.attributes.my_data_)
      q.to_xml(printer);
    printer.CloseElement();
  }

  if (metadata_.detectors.size()) {
    printer.OpenElement("Detectors");
    for (auto q : metadata_.detectors) {
      q.settings_ = Gamma::Setting();
      q.to_xml(printer);
    }
    printer.CloseElement();
  }

  printer.OpenElement("ChannelData");
  if ((metadata_.resolution > 0) && (metadata_.total_count > 0))
    printer.PushText(this->_channels_to_xml().c_str());
  printer.CloseElement();

  printer.CloseElement(); //PixieSpectrum
}

void Spectrum::to_xml(pugi::xml_node &root) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);

  pugi::xml_node node = root.append_child("Spectrum");

  node.append_attribute("type").set_value(this->my_type().c_str());

  node.append_child("Name").append_child(pugi::node_pcdata).set_value(metadata_.name.c_str());
  node.append_child("TotalEvents").append_child(pugi::node_pcdata).set_value(metadata_.total_count.str().c_str());
  node.append_child("Appearance").append_child(pugi::node_pcdata).set_value(std::to_string(metadata_.appearance).c_str());
  node.append_child("Visible").append_child(pugi::node_pcdata).set_value(std::to_string(metadata_.visible).c_str());
  node.append_child("Resolution").append_child(pugi::node_pcdata).set_value(std::to_string(metadata_.bits).c_str());

  std::stringstream patterndata;
  for (auto &q: metadata_.match_pattern)
    patterndata << static_cast<short>(q) << " ";
  node.append_child("MatchPattern").append_child(pugi::node_pcdata).set_value(boost::algorithm::trim_copy(patterndata.str()).c_str());

  patterndata.str(std::string()); //clear it
  for (auto &q: metadata_.add_pattern)
    patterndata << static_cast<short>(q) << " ";
  node.append_child("AddPattern").append_child(pugi::node_pcdata).set_value(boost::algorithm::trim_copy(patterndata.str()).c_str());

  node.append_child("RealTime").append_child(pugi::node_pcdata).set_value(to_simple_string(metadata_.real_time).c_str());
  node.append_child("LiveTime").append_child(pugi::node_pcdata).set_value(to_simple_string(metadata_.live_time).c_str());

  if (metadata_.attributes.size())
    metadata_.attributes.to_xml(node);

  if (metadata_.detectors.size()) {
    pugi::xml_node child = node.append_child("Detectors");
    for (auto q : metadata_.detectors) {
      q.settings_ = Gamma::Setting();
      q.to_xml(child);
    }
  }

  if ((metadata_.resolution > 0) && (metadata_.total_count > 0))
    node.append_child("ChannelData").append_child(pugi::node_pcdata).set_value(this->_channels_to_xml().c_str());
}


bool Spectrum::from_xml(tinyxml2::XMLElement* root) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  std::string numero;

  tinyxml2::XMLElement* elem;

  if ((elem = root->FirstChildElement("Name")) != nullptr)
    metadata_.name = elem->GetText();
  else
    return false;
  
  if ((elem = root->FirstChildElement("TotalEvents")) != nullptr)
    metadata_.total_count = PreciseFloat(elem->GetText());
  if ((elem = root->FirstChildElement("Appearance")) != nullptr)
    metadata_.appearance = boost::lexical_cast<unsigned int>(std::string(elem->GetText()));
  if ((elem = root->FirstChildElement("Visible")) != nullptr)
    metadata_.visible = boost::lexical_cast<bool>(std::string(elem->GetText()));
  if ((elem = root->FirstChildElement("Resolution")) != nullptr) 
    metadata_.bits = boost::lexical_cast<short>(std::string(elem->GetText()));
  else
    return false;
  
  shift_by_ = 16 - metadata_.bits;
  metadata_.resolution = pow(2, metadata_.bits);
  metadata_.match_pattern.clear();
  metadata_.add_pattern.clear();
  
  if ((elem = root->FirstChildElement("MatchPattern")) != nullptr) {
    std::stringstream pattern_match(elem->GetText());
    while (pattern_match.rdbuf()->in_avail()) {
      pattern_match >> numero;
      metadata_.match_pattern.push_back(boost::lexical_cast<short>(boost::algorithm::trim_copy(numero)));
    }
  }
  if ((elem = root->FirstChildElement("AddPattern")) != nullptr) {
    std::stringstream pattern_add(elem->GetText());
    while (pattern_add.rdbuf()->in_avail()) {
      pattern_add >> numero;
      metadata_.add_pattern.push_back(boost::lexical_cast<short>(boost::algorithm::trim_copy(numero)));
    }
  }
  
  if ((elem = root->FirstChildElement("RealTime")) != nullptr)
    metadata_.real_time = boost::posix_time::duration_from_string(elem->GetText());
  if ((elem = root->FirstChildElement("LiveTime")) != nullptr)
    metadata_.live_time = boost::posix_time::duration_from_string(elem->GetText());
  if ((elem = root->FirstChildElement("GenericAttributes")) != nullptr) {
    metadata_.attributes.clear();
    tinyxml2::XMLElement* one_setting = elem->FirstChildElement("Setting");
    while (one_setting != nullptr) {
      metadata_.attributes.add(Gamma::Setting(one_setting));
      one_setting = dynamic_cast<tinyxml2::XMLElement*>(one_setting->NextSibling());
    }
  }

  if ((elem = root->FirstChildElement("Detectors")) != nullptr) {
    metadata_.detectors.clear();
    tinyxml2::XMLElement* one_det = elem->FirstChildElement(Gamma::Detector().xml_element_name().c_str());
    while (one_det != nullptr) {
      metadata_.detectors.push_back(Gamma::Detector(one_det));
      one_det = dynamic_cast<tinyxml2::XMLElement*>(one_det->NextSibling());
    }
  }

  if (((elem = root->FirstChildElement("ChannelData")) != nullptr) &&
      (elem->GetText() != nullptr)) {
    std::string this_data = elem->GetText();
    boost::algorithm::trim(this_data);
    this->_channels_from_xml(this_data);
  }

  bool ret = this->initialize();

  if (ret)
   this->recalc_energies();

  return ret;
}

bool Spectrum::from_xml(const pugi::xml_node &node) {
  if (std::string(node.name()) != "Spectrum")
    return false;

  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  std::string numero;

  if (!node.child("Name"))
    return false;
  metadata_.name = std::string(node.child_value("Name"));
  PL_DBG << "name: " << metadata_.name;

  metadata_.total_count = PreciseFloat(node.child_value("TotalEvents"));
  metadata_.appearance = boost::lexical_cast<unsigned int>(std::string(node.child_value("Appearance")));
  metadata_.visible = boost::lexical_cast<bool>(std::string(node.child_value("Visible")));
  metadata_.bits = boost::lexical_cast<short>(std::string(node.child_value("Resolution")));

  PL_DBG << "totalct: " << metadata_.total_count;
  PL_DBG << "bits: " << metadata_.bits;

  if ((!metadata_.bits) || (metadata_.total_count == 0))
    return false;

  shift_by_ = 16 - metadata_.bits;
  metadata_.resolution = pow(2, metadata_.bits);
  metadata_.match_pattern.clear();
  metadata_.add_pattern.clear();

  std::stringstream pattern_match(node.child_value("MatchPattern"));
  while (pattern_match.rdbuf()->in_avail()) {
    pattern_match >> numero;
    PL_DBG << "match " << numero;
    metadata_.match_pattern.push_back(boost::lexical_cast<short>(boost::algorithm::trim_copy(numero)));
  }

  std::stringstream pattern_add(node.child_value("AddPattern"));
  while (pattern_add.rdbuf()->in_avail()) {
    pattern_add >> numero;
    PL_DBG << "add " << numero;
    metadata_.add_pattern.push_back(boost::lexical_cast<short>(boost::algorithm::trim_copy(numero)));
  }

  if (node.child("RealTime"))
    metadata_.real_time = boost::posix_time::duration_from_string(node.child_value("RealTime"));
  if (node.child("LiveTime"))
    metadata_.live_time = boost::posix_time::duration_from_string(node.child_value("LiveTime"));

  PL_DBG << "rt " << to_simple_string(metadata_.real_time);
  PL_DBG << "lt " << to_simple_string(metadata_.live_time);

  metadata_.attributes.from_xml(node.child(metadata_.attributes.xml_element_name().c_str()));

  XMLable2DB<Gamma::Detector> dets("Detectors");
  dets.from_xml(node.child("Detectors"));
  PL_DBG << "dets " << dets.size();
  for (auto &q : dets.my_data_)
    PL_DBG << "det " << q.name_;

  metadata_.detectors = dets.to_vector();
  PL_DBG << "dets " << metadata_.detectors.size();

  std::string this_data(node.child_value("ChannelData"));
  boost::algorithm::trim(this_data);
  PL_DBG << "data length " << this_data.size();
  this->_channels_from_xml(this_data);

  bool ret = this->initialize();

  if (ret)
   this->recalc_energies();

  return ret;
}


}}
