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
 *      Types for organizing data aquired from Device
 *        Qpx::Hit        single energy event with coincidence flags
 *
 ******************************************************************************/

#include "event.h"
#include "custom_logger.h"
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <fstream>

namespace Qpx {

double TimeStamp::to_nanosec() const {
  if (timebase_divider == 0)
    return 0;
  else
    return time_native * timebase_multiplier / timebase_divider;
}

void TimeStamp::delay(double ns)
{
  time_native += std::ceil(ns * timebase_divider / timebase_multiplier);
}

std::string TimeStamp::to_string() const {
  std::stringstream ss;
  ss << time_native << "x(" << timebase_multiplier << "/" << timebase_divider << ")";
  return ss.str();
}


bool TimeStamp::operator<(const TimeStamp other) const {
  if (same_base((other)))
    return (time_native < other.time_native);
  else
    return (to_nanosec() < other.to_nanosec());
}

bool TimeStamp::operator>(const TimeStamp other) const {
  if (same_base((other)))
    return (time_native > other.time_native);
  else
    return (to_nanosec() > other.to_nanosec());
}

bool TimeStamp::operator<=(const TimeStamp other) const {
  if (same_base((other)))
    return (time_native <= other.time_native);
  else
    return (to_nanosec() <= other.to_nanosec());
}

bool TimeStamp::operator>=(const TimeStamp other) const {
  if (same_base((other)))
    return (time_native >= other.time_native);
  else
    return (to_nanosec() >= other.to_nanosec());
}

bool TimeStamp::operator==(const TimeStamp other) const {
  if (same_base((other)))
    return (time_native == other.time_native);
  else
    return (to_nanosec() == other.to_nanosec());
}

bool TimeStamp::operator!=(const TimeStamp other) const {
  if (same_base((other)))
    return (time_native != other.time_native);
  else
    return (to_nanosec() != other.to_nanosec());
}

std::string DigitizedVal::to_string() const {
  std::stringstream ss;
  ss << val_ << "(" << bits_ << "b)";
  return ss.str();
}


void Hit::write_bin(std::ofstream &outfile, bool extras) const {
  outfile.write((char*)&source_channel, sizeof(source_channel));
  uint16_t time_hy = (timestamp.time_native >> 48) & 0x000000000000FFFF;
  uint16_t time_hi = (timestamp.time_native >> 32) & 0x000000000000FFFF;
  uint16_t time_mi = (timestamp.time_native >> 16) & 0x000000000000FFFF;
  uint16_t time_lo =  timestamp.time_native        & 0x000000000000FFFF;
  outfile.write((char*)&time_hy, sizeof(time_hy));
  outfile.write((char*)&time_hi, sizeof(time_hi));
  outfile.write((char*)&time_mi, sizeof(time_mi));
  outfile.write((char*)&time_lo, sizeof(time_lo));
  uint16_t nrg = energy.val(energy.bits());
  outfile.write((char*)&nrg, sizeof(nrg));
}

void Hit::read_bin(std::ifstream &infile, std::map<int16_t, Hit> &model_hits, bool extras) {
  uint16_t entry[6];
  infile.read(reinterpret_cast<char*>(entry), 12);
  int16_t channel = reinterpret_cast<int16_t&>(entry[0]);
  *this = model_hits[channel];

  source_channel = channel;
  uint64_t time_hy = entry[1];
  uint64_t time_hi = entry[2];
  uint64_t time_mi = entry[3];
  uint64_t time_lo = entry[4];
  uint64_t timestamp = (time_hy << 48) + (time_hi << 32) + (time_mi << 16) + time_lo;
  this->timestamp.time_native = timestamp;
  energy.set_val(entry[5]);
}

std::string Hit::to_string() const {
  std::stringstream ss;
  ss << "[ch" << source_channel << "|t" << timestamp.to_string() << "|e" << energy.to_string() << "]";
  return ss.str();
}

bool Event::in_window(const Hit& h) const {
  return (h.timestamp >= lower_time) && ((h.timestamp - lower_time) <= window_ns);
}

bool Event::past_due(const Hit& h) const {
  return (h.timestamp >= lower_time) && ((h.timestamp - lower_time) > max_delay_ns);
}

bool Event::antecedent(const Hit& h) const {
  return (h.timestamp < lower_time);
}

bool Event::addHit(const Hit &newhit) {
  if (hits.count(newhit.source_channel))
    return false;
  if (lower_time > newhit.timestamp)
    lower_time = newhit.timestamp;
  hits[newhit.source_channel] = newhit;
  return true;
}

std::string Event::to_string() const {
  std::stringstream ss;
  ss << "EVT[t" << lower_time.to_string() << "w" << window_ns << "]";
  for (auto &q : hits)
    ss << " " << q.first << "=" << q.second.to_string();
  return ss.str();
}

}
