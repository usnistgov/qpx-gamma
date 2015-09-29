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
 *      Gamma::DaqDevice abstract class
 *
 ******************************************************************************/

#ifndef GAMMA_DAQ_DEVICE
#define GAMMA_DAQ_DEVICE

#include "generic_setting.h"
#include "synchronized_queue.h"
#include "daq_types.h"
#include "tinyxml2.h"

namespace Qpx {

enum class LiveStatus : int {dead = 0, offline = 1, online = 2, history = 3};

class DaqDevice {

public:

  DaqDevice() : live_(LiveStatus::dead) {}
  DaqDevice(const DaqDevice& other) : live_(LiveStatus::history) {}

  LiveStatus live() {return live_;}
  virtual bool boot() {return false;}
  virtual bool die() {return true;}

  virtual bool write_settings_bulk(Gamma::Setting &set) {return false;}
  virtual bool read_settings_bulk(Gamma::Setting &set) const {return false;}
  virtual void get_all_settings() {}

  virtual bool execute_command(Gamma::Setting &set) {return false;}
  virtual std::map<int, std::vector<uint16_t>> oscilloscope() {return std::map<int, std::vector<uint16_t>>();}
  
  virtual bool daq_start(uint64_t timeout, SynchronizedQueue<Spill*>* out_queue) {return false;}
  virtual bool daq_stop() {return true;}
  virtual bool daq_running() {return false;}

protected:
  LiveStatus  live_;

};

}

#endif
