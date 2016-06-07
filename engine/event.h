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

#include "hit.h"
#include <map>
#include "xmlable.h"

namespace Qpx {

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
    lower_time = newhit.timestamp();
    hits[newhit.source_channel()] = newhit;
    window_ns = win;
    max_delay_ns = std::max(win, max_delay);
  }
};

}

#endif
