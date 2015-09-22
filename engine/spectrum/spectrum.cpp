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

  if (one_spill.stats != nullptr)
    this->addStats(*one_spill.stats);

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
        if ((q.hit[newhit.channel].channel == -1) && !appended) {
          //coincidence
          q.addHit(newhit);
          appended = true;
        } else if ((q.hit[newhit.channel].channel == -1) && appended) {
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
  for (int i = 0; i < kNumChans; i++)
    if (((metadata_.match_pattern[i] == 1) && (newEvent.hit[i].channel < 0)) ||
        ((metadata_.match_pattern[i] == -1) && (newEvent.hit[i].channel > -1)))
      addit = false;
  return addit;
}

void Spectrum::addStats(const StatsUpdate& newBlock) {
  //private; no lock required

  if (newBlock.spill_number == 0)
    start_stats = newBlock;
  else {
    metadata_.real_time   = newBlock.lab_time - start_stats.lab_time;
    StatsUpdate diff = newBlock - start_stats;
    if (!diff.total_time)
      return;
    double scale_factor = metadata_.real_time.total_microseconds() / 1000000 / diff.total_time;

    metadata_.live_time = metadata_.real_time;
    for (int i = 0; i < kNumChans; i++) { //using shortest live time of all added channels
      double this_time_unscaled = diff.live_time[i] - diff.sfdt[i];
      if ((metadata_.add_pattern[i]) && (this_time_unscaled < metadata_.live_time.total_seconds()))
        metadata_.live_time = boost::posix_time::microseconds(static_cast<long>(this_time_unscaled * scale_factor * 1000000));
    }
  }
}


void Spectrum::addRun(const RunInfo& run_info) {
  //private; no lock required

  metadata_.start_time = run_info.time_start;
  if (run_info.detectors.size() > 0)
    _set_detectors(run_info.detectors);

  if (!run_info.total_events)
    return;
  metadata_.real_time = run_info.time_stop - run_info.time_start;
  double scale_factor = run_info.time_scale_factor();
  metadata_.live_time = metadata_.real_time;
  for (int i = 0; i < kNumChans; i++) { //using shortest live time of all added channels
    double live = run_info.state.get_setting(Gamma::Setting("QpxSettings/Pixie-4/System/module/channel/LIVE_TIME", 26, Gamma::SettingType::floating, i)).value;
    double sfdt = run_info.state.get_setting(Gamma::Setting("QpxSettings/Pixie-4/System/module/channel/SFDT", 34, Gamma::SettingType::floating, i)).value;
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
  //private; no lock required
  Gamma::Setting ret;
  
  for (auto &q : metadata_.attributes)
    if (q.name == setting)
      ret = q;
  
  return ret;
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
  for (auto &q : metadata_.attributes) {
    if ((q.name == setting.name) && (q.writable))
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
  for (int i = 0; i < kNumChans; i++)
    patterndata << metadata_.match_pattern[i] << " ";
  printer.PushText(boost::algorithm::trim_copy(patterndata.str()).c_str());
  printer.CloseElement();

  patterndata.str(std::string()); //clear it
  printer.OpenElement("AddPattern");
  for (int i = 0; i < kNumChans; i++)
    patterndata << metadata_.add_pattern[i] << " ";
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
    for (auto &q: metadata_.attributes)
      q.to_xml(printer);
    printer.CloseElement();
  }

  if (metadata_.detectors.size()) {
    printer.OpenElement("Detectors");
    for (auto &q : metadata_.detectors) {
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
  metadata_.match_pattern.resize(kNumChans);
  metadata_.add_pattern.resize(kNumChans);
  
  if ((elem = root->FirstChildElement("MatchPattern")) != nullptr) {
    std::stringstream pattern_match(elem->GetText());
    for (int i = 0; i < kNumChans; i++) {
      pattern_match >> numero;
      metadata_.match_pattern[i] = boost::lexical_cast<int>(boost::algorithm::trim_copy(numero));
    }
  }
  if ((elem = root->FirstChildElement("AddPattern")) != nullptr) {
    std::stringstream pattern_add(elem->GetText());
    for (int i = 0; i < kNumChans; i++) {
      pattern_add >> numero;
      metadata_.add_pattern[i] = boost::lexical_cast<int>(boost::algorithm::trim_copy(numero));
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
      metadata_.attributes.push_back(Gamma::Setting(one_setting));
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



}}
