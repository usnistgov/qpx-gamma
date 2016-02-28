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
 *      MTDC32
 *
 ******************************************************************************/

#include "MTDC32_module.h"
#include "custom_logger.h"

#define MTDC32_Firmware                   0x0105

namespace Qpx {

static DeviceRegistrar<MTDC32> registrar("VME/MTDC32");

MTDC32::MTDC32() {
  m_controller = nullptr;
  m_baseAddress = 0;
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  module_firmware_code_ = MTDC32_Firmware;
}


MTDC32::~MTDC32() {
  this->die();
}


void MTDC32::rebuild_structure(Qpx::Setting &set) {

}

}
