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
 *      Qpx::Sink2D spectrum type for true coincidence with
 *      two dimensions of energy.
 *
 ******************************************************************************/

#include "spectrum2D.h"
#include "consumer_factory.h"

#include <fstream>
#include "custom_logger.h"
#include <boost/filesystem.hpp>

namespace Qpx {

static ConsumerRegistrar<Spectrum2D> registrar("2D");

Spectrum2D::Spectrum2D()
{
  Setting base_options = metadata_.attributes();
  metadata_ = ConsumerMetadata("2D", "2-dimensional coincidence matrix", 2,
                    {"m", "m4b", "mat"},
                    {"m4b", "mat"});

  Qpx::Setting buf;
  buf.id_ = "buffered";
  buf.metadata.setting_type = Qpx::SettingType::boolean;
  buf.metadata.unit = "T/F";
  buf.metadata.description = "Buffered output for efficient plotting (more memory)";
  buf.metadata.writable = true;
  buf.metadata.flags.insert("preset");
  base_options.branches.add(buf);

  Qpx::Setting sym;
  sym.id_ = "symmetrized";
  sym.metadata.setting_type = Qpx::SettingType::boolean;
  sym.metadata.unit = "T/F";
  sym.metadata.description = "Matrix is symmetrized";
  sym.metadata.writable = false;
  sym.value_int = 0;
  base_options.branches.add(sym);

  metadata_.overwrite_all_attributes(base_options);
}


bool Spectrum2D::_initialize() {
  Spectrum::_initialize();
  //make add pattern has exactly 2 channels
  //match pattern can be whatever
  
  int adds = 0;
  std::vector<bool> gts = pattern_add_.gates();
  for (size_t i=0; i < gts.size(); ++i)
    if (gts[i])
      adds++;

  if (adds != 2) {
    WARN << "<Spectrum2D> Cannot initialize. Add pattern must have 2 selected channels.";
    return false;
  }
  
//  energies_.resize(2);
  pattern_.resize(2, 0);
  buffered_ = (metadata_.get_attribute("buffered").value_int != 0);

  adds = 0;
  for (size_t i=0; i < gts.size(); ++i) {
    if (gts[i]) {
      pattern_[adds] = i;
      adds++;
    }
  }

  return true;
}

void Spectrum2D::init_from_file(std::string filename) {
  pattern_coinc_.resize(2);
  pattern_coinc_.set_gates(std::vector<bool>({true, true}));

  pattern_anti_.resize(2);
  pattern_anti_.set_gates(std::vector<bool>({false, false}));

  pattern_add_.resize(2);
  pattern_add_.set_gates(std::vector<bool>({true, true}));

  Qpx::Setting pattern;
  pattern = metadata_.get_attribute("pattern_coinc");
  pattern.value_pattern = pattern_coinc_;
  metadata_.set_attribute(pattern);

  pattern = metadata_.get_attribute("pattern_anti");
  pattern.value_pattern = pattern_anti_;
  metadata_.set_attribute(pattern);

  pattern = metadata_.get_attribute("pattern_add");
  pattern.value_pattern = pattern_add_;
  metadata_.set_attribute(pattern);

  Setting name = metadata_.get_attribute("name");
  name.value_text = boost::filesystem::path(filename).filename().string();
  std::replace( name.value_text.begin(), name.value_text.end(), '.', '_');
  metadata_.set_attribute(name);

  _initialize();
  _recalc_axes();
  _flush();

  Qpx::Setting cts;
  cts = metadata_.get_attribute("total_hits");
  cts.value_precise = total_hits_;
  metadata_.set_attribute(cts);

  cts = metadata_.get_attribute("total_events");
  cts.value_precise = total_events_;
  metadata_.set_attribute(cts);
}


bool Spectrum2D::check_symmetrization() {
  bool symmetrical = true;
  for (auto &q : spectrum_) {
    std::pair<uint16_t,uint16_t> point(q.first.second, q.first.first);
    if ((!spectrum_.count(point)) || (spectrum_.at(point) != q.second)) {
      symmetrical = false;
      break;
    }
  }
  Qpx::Setting symset = metadata_.get_attribute("symmetrized");
  symset.value_int = symmetrical;
  metadata_.set_attribute(symset);
  return symmetrical;
}


void Spectrum2D::_set_detectors(const std::vector<Qpx::Detector>& dets) {
  metadata_.detectors.resize(metadata_.dimensions(), Qpx::Detector());

  if (dets.size() == metadata_.dimensions())
    metadata_.detectors = dets;
  else if (dets.size() > metadata_.dimensions()) {
    int j=0;
    for (size_t i=0; i < dets.size(); ++i) {
      if (pattern_add_.relevant(i)) {
        metadata_.detectors[j] = dets[i];
        j++;
        if (j >= metadata_.dimensions())
          break;
      }
    }
  }

  this->_recalc_axes();
}

void Spectrum2D::_append(const Entry& e)
{
  if (e.first.size() == 2)
  {
    spectrum_[std::pair<uint16_t,uint16_t>(e.first[0], e.first[1])] += e.second;
    total_events_ += e.second;
    total_hits_ += (2 * e.second);
  }
}

PreciseFloat Spectrum2D::_data(std::initializer_list<size_t> list) const {
  if (list.size() != 2)
    return 0;

  std::vector<uint16_t> coords(list.begin(), list.end());

  std::pair<uint16_t,uint16_t> point(coords[0], coords[1]);

  if (spectrum_.count(point))
    return spectrum_.at(point);
  else
    return 0;
}

std::unique_ptr<EntryList> Spectrum2D::_data_range(std::initializer_list<Pair> list) {
  int min0, min1, max0, max1;
  if (list.size() != 2) {
    min0 = min1 = 0;
    max0 = max1 = pow(2, bits_);
  } else {
    Pair range0 = *list.begin(), range1 = *(list.begin()+1);
    min0 = range0.first; max0 = range0.second;
    min1 = range1.first; max1 = range1.second;
  }

  std::unique_ptr<std::list<Entry>> result(new std::list<Entry>);
//  CustomTimer makelist(true);

  if (buffered_ && !temp_spectrum_.empty()) {
    for (auto it : temp_spectrum_) {
      int co0 = it.first.first, co1 = it.first.second;
      if ((min0 <= co0) && (co0 <= max0) && (min1 <= co1) && (co1 <= max1)) {
        Entry newentry;
        newentry.first.resize(2, 0);
        newentry.first[0] = co0;
        newentry.first[1] = co1;
        newentry.second = it.second;
        result->push_back(newentry);
      }
    }
  } else {
    for (auto it : spectrum_) {
      int co0 = it.first.first, co1 = it.first.second;
      if ((min0 <= co0) && (co0 <= max0) && (min1 <= co1) && (co1 <= max1)) {
        Entry newentry;
        newentry.first.resize(2, 0);
        newentry.first[0] = co0;
        newentry.first[1] = co1;
        newentry.second = it.second;
        result->push_back(newentry);
      }
    }
  }
  if (!temp_spectrum_.empty()) {
    boost::unique_lock<boost::mutex> uniqueLock(unique_mutex_, boost::defer_lock);
    while (!uniqueLock.try_lock())
      boost::this_thread::sleep_for(boost::chrono::seconds{1});
    temp_spectrum_.clear(); //assumption about client
  }
//  DBG << "<Spectrum2D> Making list for " << metadata_.name << " took " << makelist.ms() << "ms filled with "
//         << result->size() << " elements";
  return result;
}

void Spectrum2D::addEvent(const Event& newEvent) {
  uint16_t chan1_en = 0;
  uint16_t chan2_en = 0;
  if (newEvent.hits.count(pattern_[0]))
    chan1_en = newEvent.hits.at(pattern_[0]).value(energy_idx_.at(pattern_[0])).val(bits_);
  if (newEvent.hits.count(pattern_[1]))
    chan2_en = newEvent.hits.at(pattern_[1]).value(energy_idx_.at(pattern_[1])).val(bits_);
  spectrum_[std::pair<uint16_t, uint16_t>(chan1_en,chan2_en)] += 1;
  if (buffered_)
    temp_spectrum_[std::pair<uint16_t, uint16_t>(chan1_en, chan2_en)] =
        spectrum_[std::pair<uint16_t, uint16_t>(chan1_en, chan2_en)];
  if (chan1_en)
    total_hits_++;
  if (chan2_en)
    total_hits_++;
}

bool Spectrum2D::_write_file(std::string dir, std::string format) const {
  std::string name = metadata_.get_attribute("name").value_text;
  //change illegal characters

  if (format == "m") {
    write_m(dir + "/" + name + ".m");
    return true;
  } else if (format == "mat") {
    write_mat(dir + "/" + name + ".mat");
    return true;
  } else if (format == "m4b") {
    write_m4b(dir + "/" + name + ".m4b");
    return true;
  } else
    return false;
}

bool Spectrum2D::_read_file(std::string name, std::string format) {
  if (format == "m4b")
    return read_m4b(name);
  else if (format == "mat")
    return read_mat(name);
  else
    return false;
}

void Spectrum2D::write_m(std::string name) const {
  //matlab script
  std::ofstream myfile(name, std::ios::out | std::ios::app);
  myfile << "%=========Qpx 2d spectrum=========" << std::endl
         << "%  Bit precision: " << bits_ << std::endl
         << "%  Total events : " << total_events_ << std::endl
         << "clear;" << std::endl;
  for (auto it = spectrum_.begin(); it != spectrum_.end(); ++it)
    myfile << "coinc(" << (it->first.first + 1)
           << ", " << (it->first.second + 1)
           << ") = " << it->second << ";" << std::endl;

  myfile << "figure;" << std::endl
         << "imagesc(log(coinc));" << std::endl
         << "colormap(hot);" << std::endl;
  myfile.close();
}

void Spectrum2D::write_m4b(std::string name) const {
  std::ofstream myfile(name, std::ios::out | std::ios::binary);

  uint32_t one;
  for (int i=0; i<4096; ++i) {
    for (int j=0; j<4096; ++j) {
      one = 0.0;
      std::pair<uint16_t,uint16_t> point(i, j);
      if (spectrum_.count(point))
        one = static_cast<uint32_t>(spectrum_.at(point));

      myfile.write((char*)&one, sizeof(uint32_t));

    }
  }
  myfile.close();
}

void Spectrum2D::write_mat(std::string name) const {
  std::ofstream myfile(name, std::ios::out | std::ios::binary);

  uint16_t one;
  for (int i=0; i<4096; ++i) {
    for (int j=0; j<4096; ++j) {
      one = 0;
      std::pair<uint16_t,uint16_t> point(i, j);
      if (spectrum_.count(point))
        one = static_cast<uint16_t>(spectrum_.at(point));

      myfile.write((char*)&one, sizeof(uint16_t));

    }
  }
  myfile.close();
}


bool Spectrum2D:: read_m4b(std::string name)
{
  //radware escl8r file
  std::ifstream myfile(name, std::ios::in | std::ios::binary);

  spectrum_.clear();
  total_events_ = total_hits_ = 0;
//  uint16_t max_i =0;

  uint32_t one;
  for (int i=0; i<4096; ++i) {
    for (int j=0; j<4096; ++j) {
      myfile.read ((char*)&one, sizeof(uint32_t));
      total_events_ += one;
      if (one > 0)
        spectrum_[std::pair<uint16_t, uint16_t>(i,j)] = one;
    }
  }
  total_hits_ += total_events_ * 2;
  bits_ = 12;
  Setting res = metadata_.get_attribute("resolution");
  res.value_int = bits_;
  metadata_.set_attribute(res);

  metadata_.detectors.resize(2);
  metadata_.detectors[0].name_ = "unknown1";
  metadata_.detectors[0].energy_calibrations_.add(Qpx::Calibration("Energy", bits_));
  if (check_symmetrization())
    metadata_.detectors[1] = metadata_.detectors[0];
  else {
    metadata_.detectors[1].name_ = "unknown2";
    metadata_.detectors[1].energy_calibrations_.add(Qpx::Calibration("Energy", bits_));
  }

  init_from_file(name);
  return true;
}

bool Spectrum2D:: read_mat(std::string name)
{
  //radware
  std::ifstream myfile(name, std::ios::in | std::ios::binary);

  spectrum_.clear();
  total_events_ = total_hits_ = 0;
//  uint16_t max_i =0;

  uint16_t one;
  for (int i=0; i<4096; ++i) {
    for (int j=0; j<4096; ++j) {
      myfile.read ((char*)&one, sizeof(uint16_t));
      total_events_ += one;
      if (one > 0)
        spectrum_[std::pair<uint16_t, uint16_t>(i,j)] = one;
    }
  }
  total_hits_ = 2 * total_events_;
  bits_ = 12;
  Setting res = metadata_.get_attribute("resolution");
  res.value_int = bits_;
  metadata_.set_attribute(res);

  metadata_.detectors.resize(2);
  metadata_.detectors[0].name_ = "unknown1";
  metadata_.detectors[0].energy_calibrations_.add(Qpx::Calibration("Energy", bits_));
  if (check_symmetrization())
    metadata_.detectors[1] = metadata_.detectors[0];
  else {
    metadata_.detectors[1].name_ = "unknown2";
    metadata_.detectors[1].energy_calibrations_.add(Qpx::Calibration("Energy", bits_));
  }

  init_from_file(name);
  return true;
}

std::string Spectrum2D::_data_to_xml() const {
  std::stringstream channeldata;

  int i=0, j=0;
  for (auto it = spectrum_.begin(); it != spectrum_.end(); ++it)
  {
    int this_i = it->first.first;
    int this_j = it->first.second;
    if (this_i > i) {
      channeldata << "+ " << (this_i - i) << " ";
      if (this_j > 0)
        channeldata << "0 " << this_j << " ";
      i = this_i;
      j = this_j + 1;
    }
    if (this_j > j)
      channeldata << "0 " << (this_j - j) << " ";
    channeldata << std::setprecision(std::numeric_limits<PreciseFloat>::max_digits10) << it->second  <<  " ";
    j = this_j + 1;
  }  
  return channeldata.str();
}

uint16_t Spectrum2D::_data_from_xml(const std::string& thisData){
  std::stringstream channeldata;
  channeldata.str(thisData);

  spectrum_.clear();

  uint16_t i = 0, j = 0, max_i = 0, max_j = 0;
  std::string numero, numero_z;
  while (channeldata.rdbuf()->in_avail()) {
    channeldata >> numero;
    if (numero == "+") {
      channeldata >> numero_z;
      if (j > max_j) max_j = j;
      i += boost::lexical_cast<uint16_t>(numero_z);
      j=0;
      if (j) max_i = i;
    } else if (numero == "0") {
      channeldata >> numero_z;
      j += boost::lexical_cast<uint16_t>(numero_z);
    } else {
      spectrum_[std::pair<uint16_t, uint16_t>(i,j)] = boost::lexical_cast<PreciseFloat>(numero);
      j++;
    }
  }
  return std::max(max_j, max_i);
}

#ifdef H5_ENABLED
void Spectrum2D::_save_data(H5CC::Group& g) const
{
  auto dgroup = g.require_group("data");
  auto didx = dgroup.require_dataset<uint16_t>("indices", {spectrum_.size(), 2}, {128,2});
  auto dcts = dgroup.require_dataset<long double>("counts", {spectrum_.size()}, {128});
  std::vector<uint16_t> dx(spectrum_.size());
  std::vector<uint16_t> dy(spectrum_.size());
  std::vector<long double> dc(spectrum_.size());
  size_t i = 0;
  for (auto it = spectrum_.begin(); it != spectrum_.end(); ++it)
  {
    dx[i] = it->first.first;
    dy[i] = it->first.second;
    dc[i] = static_cast<long double>(it->second);
    i++;
  }
  didx.write(dx, {spectrum_.size(), 1}, {0,0});
  didx.write(dy, {spectrum_.size(), 1}, {0,1});
  dcts.write(dc);
}

void Spectrum2D::_load_data(H5CC::Group &g)
{
  if (!g.has_group("data"))
    return;
  auto dgroup = g.open_group("data");

  if (!dgroup.has_dataset("indices") || !dgroup.has_dataset("counts"))
    return;

  auto didx = dgroup.open_dataset("indices");
  auto dcts = dgroup.open_dataset("counts");

  if ((didx.shape().rank() != 2) ||
      (dcts.shape().rank() != 1) ||
      (didx.shape().dim(0) != dcts.shape().dim(0)))
    return;

  std::vector<uint16_t> dx(didx.shape().dim(0));
  std::vector<uint16_t> dy(didx.shape().dim(0));
  std::vector<long double> dc(didx.shape().dim(0));

  didx.read(dx, {dx.size(), 1}, {0,0});
  didx.read(dy, {dy.size(), 1}, {0,1});
  dcts.read(dc, {dx.size()}, {0});

  for (size_t i=0; i < dx.size(); ++i)
    spectrum_[std::pair<uint16_t, uint16_t>(dx[i],dy[i])] = dc[i];
}
#endif

}
