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
 *      Types for organizing data aquired from device
 *        Qpx::Hit        single energy event with coincidence flags
 *
 ******************************************************************************/

#ifndef QPX_TIME_STAMP
#define QPX_TIME_STAMP

#include "xmlable.h"
#include <string>
#include <cmath>
#include <fstream>
#include "qpx_util.h"

namespace Qpx {

class TimeStamp
{

public:
  inline TimeStamp()
    : time_native_(0)
    , timebase_multiplier_(1)
    , timebase_divider_(1)
  {}

  inline TimeStamp(uint32_t multiplier, uint32_t divider)
    : time_native_(0)
    , timebase_multiplier_(multiplier)
    , timebase_divider_(divider)
  {
    if (!timebase_multiplier_)
      timebase_multiplier_ = 1;
    if (!timebase_divider_)
      timebase_divider_ = 1;
  }

  inline TimeStamp make(uint64_t native) const
  {
    TimeStamp ret = *this;
    ret.time_native_ = native;
    return ret;
  }

  inline double timebase_multiplier() const
  { return timebase_multiplier_; }

  inline double timebase_divider() const
  { return timebase_divider_; }

  static inline TimeStamp common_timebase(const TimeStamp& a, const TimeStamp& b)
  {
    if (a.timebase_divider_ == b.timebase_divider_)
    {
      if (a.timebase_multiplier_ < b.timebase_multiplier_)
        return a;
      else
        return b;
    }
    else
      return TimeStamp(1, lcm(a.timebase_divider_, b.timebase_divider_));
  }

  inline double operator-(const TimeStamp other) const
  {
    return (to_nanosec() - other.to_nanosec());
  }

  inline bool same_base(const TimeStamp other) const
  {
    return ((timebase_divider_ == other.timebase_divider_) && (timebase_multiplier_ == other.timebase_multiplier_));
  }

  inline double to_nanosec(uint64_t native) const
  {
    return native * double(timebase_multiplier_) / double(timebase_divider_);
  }

  inline int64_t to_native(double ns)
  {
    return std::ceil(ns * double(timebase_divider_) / double(timebase_multiplier_));
  }

  inline double to_nanosec() const
  {
    return time_native_ * double(timebase_multiplier_) / double(timebase_divider_);
  }

  inline void delay(double ns)
  {
    if (ns > 0)
      time_native_ += std::ceil(ns * double(timebase_divider_) / double(timebase_multiplier_));
  }

  inline bool operator<(const TimeStamp other) const
  {
    if (same_base((other)))
      return (time_native_ < other.time_native_);
    else
      return (to_nanosec() < other.to_nanosec());
  }

  inline bool operator>(const TimeStamp other) const
  {
    if (same_base((other)))
      return (time_native_ > other.time_native_);
    else
      return (to_nanosec() > other.to_nanosec());
  }

  inline bool operator<=(const TimeStamp other) const
  {
    if (same_base((other)))
      return (time_native_ <= other.time_native_);
    else
      return (to_nanosec() <= other.to_nanosec());
  }

  inline bool operator>=(const TimeStamp other) const
  {
    if (same_base((other)))
      return (time_native_ >= other.time_native_);
    else
      return (to_nanosec() >= other.to_nanosec());
  }

  inline bool operator==(const TimeStamp other) const
  {
    if (same_base((other)))
      return (time_native_ == other.time_native_);
    else
      return (to_nanosec() == other.to_nanosec());
  }

  inline bool operator!=(const TimeStamp other) const
  {
    if (same_base((other)))
      return (time_native_ != other.time_native_);
    else
      return (to_nanosec() != other.to_nanosec());
  }

  inline void write_bin(std::ofstream &outfile) const
  {
    outfile.write((char*)&time_native_, sizeof(time_native_));
  }

  inline void read_bin(std::ifstream &infile)
  {
    infile.read(reinterpret_cast<char*>(&time_native_), sizeof(time_native_));
  }

  void from_xml(const pugi::xml_node &);
  void to_xml(pugi::xml_node &) const;
  std::string xml_element_name() const {return "TimeStamp";}
  std::string to_string() const;

private:
  uint64_t time_native_;
  uint32_t timebase_multiplier_;
  uint32_t timebase_divider_;
};

}

#endif
