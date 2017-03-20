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
#include <sstream>

namespace Qpx {

bool Event::in_window(const Hit& h) const {
  return (h.timestamp() >= lower_time) && ((h.timestamp() - lower_time) <= window_ns);
}

bool Event::past_due(const Hit& h) const {
  return (h.timestamp() >= lower_time) && ((h.timestamp() - lower_time) > max_delay_ns);
}

bool Event::antecedent(const Hit& h) const {
  return (h.timestamp() < lower_time);
}

bool Event::addHit(const Hit &newhit) {
  if (hits.count(newhit.source_channel()))
    return false;
  if (lower_time > newhit.timestamp())
    lower_time = newhit.timestamp();
  hits[newhit.source_channel()] = newhit;
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
