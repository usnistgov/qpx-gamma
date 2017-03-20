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
 ******************************************************************************/

#pragma once

#include "hit_model.h"
#include <vector>
#include <fstream>

#include "xmlable.h"

#include "json.hpp"
using namespace nlohmann;

namespace Qpx {

class Hit
{
private:
  int16_t       source_channel_;
  TimeStamp     timestamp_;
  std::vector<DigitizedVal> values_;
  std::vector<uint16_t>     trace_;

public:
  inline Hit()
    : Hit(-1, HitModel())
  {}

  inline Hit(int16_t sourcechan, const HitModel &model)
    : source_channel_(sourcechan)
    , timestamp_(model.timebase)
    , values_ (model.values)
  {
    trace_.resize(model.tracelength);
  }

  //Accessors
  inline const int16_t& source_channel() const
  {
    return source_channel_;
  }

  inline const TimeStamp& timestamp() const
  {
    return timestamp_;
  }

  inline size_t value_count() const
  {
    return values_.size();
  }

  inline DigitizedVal value(size_t idx) const
  {
    if (idx < values_.size())
      return values_.at(idx);
    else
      return DigitizedVal();
  }

  inline const std::vector<uint16_t>& trace() const
  {
    return trace_;
  }

  //Setters
  inline void set_timestamp_native(uint64_t native)
  {
    timestamp_ = timestamp_.make(native);
  }

  inline void set_value(size_t idx, uint16_t val)
  {
    if (idx < values_.size())
      values_[idx].set_val(val);
  }

  inline void set_trace(const std::vector<uint16_t> &trc)
  {
    size_t len = std::min(trc.size(), trace_.size());
    for (size_t i=0; i < len; ++i)
      trace_[i] = trc.at(i);
  }

  //Comparators
  inline bool operator==(const Hit other) const
  {
    if (source_channel_ != other.source_channel_) return false;
    if (timestamp_ != other.timestamp_) return false;
    if (values_ != other.values_) return false;
    if (trace_ != other.trace_) return false;
    return true;
  }

  inline bool operator!=(const Hit other) const
  {
    return !operator==(other);
  }

  inline bool operator<(const Hit other) const
  {
    return (timestamp_ < other.timestamp_);
  }

  inline bool operator>(const Hit other) const
  {
    return (timestamp_ > other.timestamp_);
  }

  inline void delay_ns(double ns)
  {
    timestamp_.delay(ns);
  }

  //binary stream i/o
  inline void write_bin(std::ofstream &outfile) const
  {
    outfile.write((char*)&source_channel_, sizeof(source_channel_));
    timestamp_.write_bin(outfile);
    for (auto &v : values_)
      v.write_bin(outfile);
    if (trace_.size())
      outfile.write((char*)trace_.data(), sizeof(uint16_t) * trace_.size());
  }

  inline void read_bin(std::ifstream &infile, const std::map<int16_t, HitModel> &model_hits)
  {
    int16_t channel = -1;
    infile.read(reinterpret_cast<char*>(&channel), sizeof(channel));
    if (!model_hits.count(channel))
      return;
    *this = Hit(channel, model_hits.at(channel));
    timestamp_.read_bin(infile);

    for (auto &v : values_)
      v.read_bin(infile);

    if (trace_.size())
      infile.read(reinterpret_cast<char*>(trace_.data()), sizeof(uint16_t) * trace_.size());
  }

  std::string to_string() const;
};

}
