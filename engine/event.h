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

#ifndef QPX_EVENT
#define QPX_EVENT

#include <unordered_map>
#include <vector>
#include <map>

namespace Qpx {

struct TimeStamp {
  uint64_t time_native;
  double timebase_multiplier;
  double timebase_divider;

  TimeStamp() {
    time_native = 0;
    timebase_multiplier = 1;
    timebase_divider = 1;
  }

  double to_nanosec() const;
  std::string to_string() const;
  void delay(double ns);

  double operator-(const TimeStamp other) const {
    return (to_nanosec() - other.to_nanosec());
  }

  bool same_base(const TimeStamp other) const {
    return ((timebase_divider == other.timebase_divider) && (timebase_multiplier == other.timebase_multiplier));
  }

  bool operator<(const TimeStamp other) const;
  bool operator>(const TimeStamp other) const;
  bool operator<=(const TimeStamp other) const;
  bool operator>=(const TimeStamp other) const;
  bool operator==(const TimeStamp other) const;
  bool operator!=(const TimeStamp other) const;

};

class DigitizedVal {
public:

  DigitizedVal()   {
    val_ = 0;
    bits_ = 16;
  }

  DigitizedVal(uint16_t v, uint16_t b) {
    val_ = v;
    bits_ = b;
  }

  void set_val(uint16_t v) {
    val_ = v;
  }

  std::string to_string() const;

  uint16_t bits() const { return bits_; }

  uint16_t val(uint16_t bits) const {
    if (bits == bits_)
      return val_;
    else if (bits < bits_)
      return val_ >> (bits_ - bits);
    else
      return val_ << (bits - bits_);
  }

  bool operator==(const DigitizedVal other) const {
    if (val_ != other.val_) return false;
    if (bits_ != other.bits_) return false;
    return true;
  }
  bool operator!=(const DigitizedVal other) const { return !operator==(other); }

private:
  uint16_t  val_;
  uint16_t  bits_;
};

struct Hit {

  int16_t       source_channel;
  TimeStamp     timestamp;
  DigitizedVal  energy;
  std::unordered_map<std::string, uint16_t> extras;
  std::vector<uint16_t> trace;
  
  inline Hit() {
    source_channel = -1;
  }

  std::string to_string() const;

  void write_bin(std::ofstream &outfile, bool extras = false) const;
  void read_bin(std::ifstream &infile, std::map<int16_t, Hit> &model_hits, bool extras = false);

  bool operator<(const Hit other) const {return (timestamp < other.timestamp);}
  bool operator>(const Hit other) const {return (timestamp > other.timestamp);}
  bool operator==(const Hit other) const {
    if (source_channel != other.source_channel) return false;
    if (timestamp != other.timestamp) return false;
    if (energy != other.energy) return false;
    if (extras != other.extras) return false;
    if (trace != other.trace) return false;
    return true;
  }
  bool operator!=(const Hit other) const { return !operator==(other); }
};

struct Event {
  TimeStamp              lower_time;
  double                 window_ns;
  double                 max_delay_ns;
  std::map<int16_t, Hit> hits;

  bool in_window(const Hit& h) const;
  bool past_due(const Hit& h) const;
  bool antecedent(const Hit& h) const;
  bool addHit(const Hit &newhit);

  std::string to_string() const;

  inline Event() {
    window_ns = 0.0;
    max_delay_ns = 0.0;
  }

  inline Event(const Hit &newhit, double win, double max_delay) {
    lower_time = newhit.timestamp;
    hits[newhit.source_channel] = newhit;
    window_ns = win;
    max_delay_ns = std::max(win, max_delay + win);
  }
};

}

#endif
