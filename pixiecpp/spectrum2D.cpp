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
 *      Pixie::Spectrum::Spectrum2D spectrum type for true coincidence with
 *      two dimensions of energy.
 *
 ******************************************************************************/


#include <fstream>
#include "spectrum2D.h"
#include "custom_logger.h"
#include "custom_timer.h"

namespace Pixie {
namespace Spectrum {

static Registrar<Spectrum2D> registrar("2D");

bool Spectrum2D::initialize() {
  //make add pattern has exactly 2 channels
  //match pattern can be whatever
  
  int adds = 0;
  for (int i=0; i < kNumChans; i++) {
    if (add_pattern_[i] == 1)
      adds++;
  }

  if (adds != 2) {
    PL_WARN << "Invalid 2D spectrum created (bad add pattern)";
    return false;
  }
  
  resolution_ = pow(2, 16 - shift_by_);
  dimensions_ = 2;
  energies_.resize(2);
  pattern_.resize(2, 0);
  buffered_ = get_attr("buffered");
  
  adds = 0;
  for (int i=0; i < kNumChans; i++) {
    if (add_pattern_[i] == 1) {
      pattern_[adds] = i;
      adds++;
    }
  }
  return true;
}

uint64_t Spectrum2D::_get_count(std::initializer_list<uint16_t> list) const {
  if (list.size() != 2)
    return 0;

  std::vector<uint16_t> coords(list.begin(), list.end());

  if ((coords[0] >= resolution_) || (coords[1] >= resolution_))
    return 0;
  
  std::pair<uint16_t,uint16_t> point(coords[0], coords[1]);
  //if not empty
  return spectrum_.at(point);
}

std::unique_ptr<EntryList> Spectrum2D::_get_spectrum(std::initializer_list<Pair> list) {
  int min0, min1, max0, max1;
  if (list.size() != 2) {
    min0 = min1 = 0;
    max0 = max1 = resolution_;
  }

  Pair range0 = *list.begin(), range1 = *list.end();
  min0 = range0.first; max0 = range0.second;
  min1 = range1.first; max1 = range1.second;
      
  std::unique_ptr<std::list<Entry>> result(new std::list<Entry>);
  CustomTimer makelist(true);

  if (buffered_ && !temp_spectrum_.empty()) {
    for (auto it : temp_spectrum_) {
      int co0 = it.first.first, co1 = it.first.second;
      if ((min0 < co0) && (co0 < max0) && (min1 < co1) && (co1 < max1)) {
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
      if ((min0 < co0) && (co0 < max0) && (min1 < co1) && (co1 < max1)) {
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
    boost::unique_lock<boost::mutex> uniqueLock(u_mutex_, boost::defer_lock);
    while (!uniqueLock.try_lock())
      boost::this_thread::sleep_for(boost::chrono::seconds{1});
    temp_spectrum_.clear(); //assumption about client
  }
  PL_DBG << "Making list for " << name_ << " took " << makelist.ms() << "ms";
  return result;
}

void Spectrum2D::addHit(const Hit& newEvent, int chan1, int chan2) {
  uint64_t chan1_en = (newEvent.energy[chan1]) >> shift_by_;
  uint64_t chan2_en = (newEvent.energy[chan2]) >> shift_by_;
  spectrum_[std::pair<uint16_t, uint16_t>(chan1_en,chan2_en)] += 1;
  if (buffered_)
    temp_spectrum_[std::pair<uint16_t, uint16_t>(chan1_en,chan2_en)] =
        spectrum_[std::pair<uint16_t, uint16_t>(chan1_en,chan2_en)];
  if (chan1_en > max_chan_) max_chan_ = chan1_en;
  if (chan2_en > max_chan_) max_chan_ = chan2_en;
  count_++;
}

void Spectrum2D::addHit(const Hit& newHit) {
  addHit(newHit, pattern_[0], pattern_[1]);
}

bool Spectrum2D::_write_file(std::string dir, std::string format) const {
  if (format == "m") {
      write_m(dir + "/" + name_ + ".m");
      return true;
  } else
      return false;
}

void Spectrum2D::write_m(std::string name) const {
  //matlab script
  std::ofstream myfile(name, std::ios::out | std::ios::app);
  myfile << "%MCA 2d spectrum from Pixie" << std::endl
         << "%  Bit precision: " << (16-shift_by_) << std::endl
         << "%  Channels     : " << resolution_ << std::endl
         << "%  Total events : " << count_ << std::endl
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


std::string Spectrum2D::_channels_to_xml() const {
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
    channeldata << it->second  <<  " ";
    j = this_j + 1;
  }  
  return channeldata.str();
}

uint16_t Spectrum2D::_channels_from_xml(const std::string& thisData){
  std::stringstream channeldata;
  channeldata.str(thisData);

  spectrum_.clear();
  max_chan_ = 0;

  uint64_t i = 0, j = 0, max_i = 0;
  std::string numero, numero_z;
  while (channeldata.rdbuf()->in_avail()) {
    channeldata >> numero;
    if (numero == "+") {
      channeldata >> numero_z;
      if (j > max_chan_) max_chan_ = j;
      i += boost::lexical_cast<uint64_t>(numero_z);
      j=0;
      if (j) max_i = i;
    } else if (numero == "0") {
      channeldata >> numero_z;
      j += boost::lexical_cast<uint64_t>(numero_z);
    } else {
      spectrum_[std::pair<uint16_t, uint16_t>(i,j)] = boost::lexical_cast<uint64_t>(numero);
      j++;
    }
  }
  if (max_i > max_chan_)
    max_chan_ = max_i;
  return max_chan_;
}

}}
