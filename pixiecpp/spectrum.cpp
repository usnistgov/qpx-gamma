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
 *      Pixie::Spectrum::Spectrum generic spectrum type.
 *                       All public methods are thread-safe.
 *                       When deriving override protected methods.
 *
 ******************************************************************************/

#include <fstream>
#include <boost/algorithm/string.hpp>
#include "spectrum.h"
#include "custom_logger.h"

namespace Pixie {
namespace Spectrum {

uint64_t Spectrum::get_count(std::initializer_list<uint16_t> list ) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  if (list.size() != this->dimensions_)
    return 0;
  return this->_get_count(list);
}

std::unique_ptr<std::list<Entry>> Spectrum::get_spectrum(std::initializer_list<Pair> list) {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  if (list.size() != this->dimensions_)
    return 0; //wtf???
  else {
    //    std::vector<Pair> ranges(list.begin(), list.end());
    return this->_get_spectrum(list);
  }
}

bool Spectrum::from_template(const Template& newtemplate) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  
  bits_ = newtemplate.bits;
  shift_by_ = 16 - bits_;
  resolution_ = pow(2,bits_);
  name_ = newtemplate.name_;
  appearance_ = newtemplate.appearance;
  visible_ = newtemplate.visible;
  match_pattern_ = newtemplate.match_pattern;
  add_pattern_ = newtemplate.add_pattern;
  generic_attributes_ = newtemplate.generic_attributes;

  return (this->initialize());
}

void Spectrum::addSpill(const Spill& one_spill) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  if (!dimensions_)
    return;
  
  for (auto &q : one_spill.hits) {
    if (this->validateHit(q))
      this->addHit(q);
  }

  if (one_spill.stats != nullptr)
    this->addStats(*one_spill.stats);

  if (one_spill.run != nullptr)
    this->addRun(*one_spill.run);
}

bool Spectrum::validateHit(const Hit& newHit) const {
  bool addit = true;
  for (int i = 0; i < kNumChans; i++)
    if (((match_pattern_[i] == 1) && (!newHit.pattern[i])) ||
        ((match_pattern_[i] == -1) && (newHit.pattern[i])))
      addit = false;
  return addit;
}

void Spectrum::addStats(const StatsUpdate& newBlock) {
  //private; no lock required
  
  if (!dimensions_)
    return;

  if (newBlock.spill_count == 0)
    start_stats = newBlock;
  else {
    real_time_   = newBlock.lab_time - start_stats.lab_time;
    StatsUpdate diff = newBlock - start_stats;
    if (!diff.total_time)
      return;
    double scale_factor = real_time_.total_microseconds() / 1000000 / diff.total_time;

    live_time_ = real_time_;
    for (int i = 0; i < kNumChans; i++) { //using shortest live time of all added channels
      double this_time_unscaled = diff.live_time[i] - diff.sfdt[i];
      if ((add_pattern_[i]) && (this_time_unscaled < live_time_.total_seconds()))
        live_time_ = boost::posix_time::microseconds(static_cast<long>(this_time_unscaled * scale_factor * 1000000));
    }
  }
}


void Spectrum::addRun(const RunInfo& run_info) {
  //private; no lock required
  
  if (!dimensions_)
    return;
  detectors_ = run_info.p4_state.get_detectors();
  recalc_energies();
  start_time_ = run_info.time_start;

  if (!run_info.total_events)
    return;
  real_time_ = run_info.time_stop - run_info.time_start;
  double scale_factor = run_info.time_scale_factor();
  live_time_ = real_time_;
  for (int i = 0; i < kNumChans; i++) { //using shortest live time of all added channels
    double this_time_unscaled = 1;
    /* = run_info.p4_state.get_chan("LIVE_TIME", Channel(i)) -
      run_info.p4_state.get_chan("SFDT", Channel(i));*/
    if ((add_pattern_[i]) && (this_time_unscaled < live_time_.total_seconds()))
      live_time_ = boost::posix_time::microseconds(static_cast<long>(this_time_unscaled * scale_factor * 1000000));
  }
}

std::vector<double> Spectrum::energies(uint8_t chan) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  
  if (chan < dimensions_)
    return energies_[chan];
  else
    return std::vector<double>();
}
  
void Spectrum::set_detectors(const std::vector<Detector>& dets) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});

  if (!dimensions_)
    return;
  
  detectors_ = dets;
  recalc_energies();
}

std::vector<Detector> Spectrum::get_detectors() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return detectors_;
}

Detector Spectrum::get_detector(uint16_t which) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  for (int i=0; i<detectors_.size(); ++i) {
    if (add_pattern_[i] == 1) {
      if (which == 0)
        return detectors_[i];
      else
        which--;
    }
  }
  return Detector();
}


void Spectrum::recalc_energies() {
  //private; no lock required

  //what if number of dets and dimensions mismatch?
  
  energies_.resize(dimensions_);
  int phys_chan=0, calib_chan=0;
  while ((calib_chan < dimensions_) && (phys_chan < kNumChans)){
    if (add_pattern_[phys_chan] == 1) {
      energies_[calib_chan].resize(resolution_, 0.0);
      //      PL_DBG << "spectrum " << name_ << " using calibration for " << detectors_[phys_chan].name_;
      Calibration this_calib;
      if (detectors_[phys_chan].energy_calibrations_.has_a(Calibration(bits_)))
        this_calib = detectors_[phys_chan].energy_calibrations_.get(Calibration(bits_));
      for (uint32_t j=0; j<resolution_; j++)
        energies_[calib_chan][j] = this_calib.transform(j);
      calib_chan++;
    }
    phys_chan++;
  }
}

double Spectrum::get_attr(std::string setting) const {
  //private; no lock required
  double ret = 0.0;
  
  for (auto &q : generic_attributes_)
    if (q.name == setting)
      ret = q.value;
  
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
std::string Spectrum::type() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return my_type();
}

uint16_t Spectrum::dimensions() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return dimensions_;
}

uint32_t Spectrum::resolution() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return resolution_;
}

std::string Spectrum::name() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return name_;
}

std::string Spectrum::description() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return description_;
}

double Spectrum::total_count() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return count_.convert_to<double>();
}

uint16_t Spectrum::bits() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return bits_;
}

std::vector<int16_t> const Spectrum::match_pattern() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return match_pattern_;
}

std::vector<int16_t> const Spectrum::add_pattern() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return add_pattern_;
}

uint64_t Spectrum::max_chan() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return max_chan_;
}

bool Spectrum::visible() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return visible_;
}

uint32_t Spectrum::appearance() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return appearance_;
}

boost::posix_time::time_duration Spectrum::real_time() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return real_time_;
}

boost::posix_time::time_duration Spectrum::live_time() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return live_time_;
}

boost::posix_time::ptime Spectrum::start_time() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return start_time_;
}

std::vector<Setting> Spectrum::generic_attributes() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return generic_attributes_;
}


//change stuff

void Spectrum::set_visible(bool vis) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  visible_ = vis;
}

void Spectrum::set_appearance(uint32_t newapp) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  appearance_ = newapp;
}

void Spectrum::set_start_time(boost::posix_time::ptime newtime) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  start_time_ = newtime; 
}

void Spectrum::set_description(std::string newdesc) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  description_ = newdesc; 
}

void Spectrum::set_generic_attr(std::string setting, double value) {
  boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
  while (!uniqueLock.try_lock())
    boost::this_thread::sleep_for(boost::chrono::seconds{1});
  for (auto &q : generic_attributes_) {
    if ((q.name == setting) && (q.writable))
      q.value = value;
  }
}



/////////////////////
//Save and load//////
/////////////////////

void Spectrum::to_xml(tinyxml2::XMLPrinter& printer) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);

  if (!dimensions_)
    return;

  std::stringstream patterndata;
  
  printer.OpenElement("PixieSpectrum");
  printer.PushAttribute("type", this->my_type().c_str());

  printer.OpenElement("TotalEvents");
  printer.PushText(count_.str().c_str());
  printer.CloseElement();

  printer.OpenElement("Name");
  printer.PushText(name_.c_str());
  printer.CloseElement();

  printer.OpenElement("Appearance");
  printer.PushText(std::to_string(appearance_).c_str());
  printer.CloseElement();

  printer.OpenElement("Visible");
  printer.PushText(std::to_string(visible_).c_str());
  printer.CloseElement();

  printer.OpenElement("Resolution");
  printer.PushText(std::to_string(bits_).c_str());
  printer.CloseElement();

  printer.OpenElement("MatchPattern");
  for (int i = 0; i < kNumChans; i++)
    patterndata << match_pattern_[i] << " ";
  printer.PushText(boost::algorithm::trim_copy(patterndata.str()).c_str());
  printer.CloseElement();

  patterndata.str(std::string()); //clear it
  printer.OpenElement("AddPattern");
  for (int i = 0; i < kNumChans; i++)
    patterndata << add_pattern_[i] << " ";
  printer.PushText(boost::algorithm::trim_copy(patterndata.str()).c_str());
  printer.CloseElement();
  
  printer.OpenElement("RealTime");
  printer.PushText(to_simple_string(real_time_).c_str());
  printer.CloseElement();

  printer.OpenElement("LiveTime");
  printer.PushText(to_simple_string(live_time_).c_str());
  printer.CloseElement();

  if (generic_attributes_.size()) {
    printer.OpenElement("GenericAttributes");
    for (auto &q: generic_attributes_)
      q.to_xml(printer);
    printer.CloseElement();
  }

  printer.OpenElement("ChannelData");
  if ((resolution_ > 0) && (count_ > 0))
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
    name_ = elem->GetText();
  else
    return false;
  
  if ((elem = root->FirstChildElement("TotalEvents")) != nullptr)
    count_ = PreciseFloat(elem->GetText());
  if ((elem = root->FirstChildElement("Appearance")) != nullptr)
    appearance_ = boost::lexical_cast<unsigned int>(std::string(elem->GetText()));
  if ((elem = root->FirstChildElement("Visible")) != nullptr)
    visible_ = boost::lexical_cast<bool>(std::string(elem->GetText()));  
  if ((elem = root->FirstChildElement("Resolution")) != nullptr) 
    bits_ = boost::lexical_cast<short>(std::string(elem->GetText()));
  else
    return false;
  
  shift_by_ = 16 - bits_;
  resolution_ = pow(2, bits_);
  match_pattern_.resize(kNumChans);
  add_pattern_.resize(kNumChans);
  
  if ((elem = root->FirstChildElement("MatchPattern")) != nullptr) {
    std::stringstream pattern_match(elem->GetText());
    for (int i = 0; i < kNumChans; i++) {
      pattern_match >> numero;
      match_pattern_[i] = boost::lexical_cast<int>(boost::algorithm::trim_copy(numero));
    }
  }
  if ((elem = root->FirstChildElement("AddPattern")) != nullptr) {
    std::stringstream pattern_add(elem->GetText());
    for (int i = 0; i < kNumChans; i++) {
      pattern_add >> numero;
      add_pattern_[i] = boost::lexical_cast<int>(boost::algorithm::trim_copy(numero));
    }
  }
  
  if ((elem = root->FirstChildElement("RealTime")) != nullptr)
    real_time_ = boost::posix_time::duration_from_string(elem->GetText());
  if ((elem = root->FirstChildElement("LiveTime")) != nullptr)
    live_time_ = boost::posix_time::duration_from_string(elem->GetText());
  if ((elem = root->FirstChildElement("GenericAttributes")) != nullptr) {
    generic_attributes_.clear();
    tinyxml2::XMLElement* one_setting = elem->FirstChildElement("Setting");
    while (one_setting != nullptr) {
      generic_attributes_.push_back(Setting(one_setting));
      one_setting = dynamic_cast<tinyxml2::XMLElement*>(one_setting->NextSibling());
    }
  }
  if (((elem = root->FirstChildElement("ChannelData")) != nullptr) &&
      (elem->GetText() != nullptr)) {
    std::string this_data = elem->GetText();
    boost::algorithm::trim(this_data);
    this->_channels_from_xml(this_data);
  }
  return (this->initialize());
}



}}
