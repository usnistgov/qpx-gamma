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
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      boost cpu_timer-derived class that does not start counting by default
 *   on construction. Timeout, ETA, output convenience functions.
 *
 ******************************************************************************/


#ifndef CUSTOM_TIMER_H_
#define CUSTOM_TIMER_H_

#include <string>
#include <cstdint>
#include <boost/timer/timer.hpp>

inline static void wait_ms(int millisecs) {
  boost::this_thread::sleep(boost::posix_time::milliseconds(millisecs));
}

class CustomTimer: public boost::timer::cpu_timer {
private:
  const double secs = pow(10, 9);
  const double msecs = pow(10, 6);
  const double usecs = pow(10, 3);
  uint64_t timeout_limit;
public:
  CustomTimer(bool start = false) {if (!start) stop(); timeout_limit = 0;};
  CustomTimer(uint64_t timeout, bool start = false) { if (!start) stop(); timeout_limit = timeout;};
  double s () {return elapsed().wall / secs;};
  double ms () {return elapsed().wall / msecs;};
  double us () {return elapsed().wall / usecs;};
  double ns() {return elapsed().wall;};
  bool timeout() {
    return (s() > static_cast<double>(timeout_limit));
  };

  std::string done() {
    uint64_t e_tot = static_cast<uint64_t>(s());
    uint64_t e_h = e_tot / 3600;
    uint64_t e_m = (e_tot % 3600) / 60;
    uint64_t e_s = (e_tot % 60);
    return (std::to_string(e_h) + ":" +
            std::to_string(e_m) + ":" +
            std::to_string(e_s));    
  };
  
  std::string ETA() {
    uint64_t ETA_tot = static_cast<uint64_t>(
        ceil(static_cast<double>(timeout_limit) - s())
                                   );
    uint64_t ETA_h = ETA_tot / 3600;
    uint64_t ETA_m = (ETA_tot % 3600) / 60;
    uint64_t ETA_s = (ETA_tot % 60);
    return (std::to_string(ETA_h) + ":" +
            std::to_string(ETA_m) + ":" +
            std::to_string(ETA_s));
  };
};

#endif
