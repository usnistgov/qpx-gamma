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
 *      MADC32
 *
 ******************************************************************************/

#include "MADC32_module.h"
#include "custom_logger.h"

#define MADC32_Firmware                   0x0203

namespace Qpx {

static DeviceRegistrar<MADC32> registrar("VME/MADC32");

MADC32::MADC32() {
  m_controller = nullptr;
  m_baseAddress = 0;
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  module_firmware_code_ = MADC32_Firmware;
}


MADC32::~MADC32() {
  this->die();
}


void MADC32::rebuild_structure(Gamma::Setting &set) {
  for (auto &k : set.branches.my_data_) {
    if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/MADC32/ChannelThresholds")) {

      Gamma::Setting temp("VME/MADC32/Threshold");
      temp.enrich(setting_definitions_, true);

      while (k.branches.size() < 32)
        k.branches.my_data_.push_back(temp);
      while (k.branches.size() > 32)
        k.branches.my_data_.pop_back();

      uint32_t offset = k.metadata.address;

      std::set<int32_t> indices;
      int address = 0;
      for (auto &p : k.branches.my_data_) {
        if (p.id_ != temp.id_) {
          temp.indices = p.indices;
          p = temp;
        }

        p.metadata.address = offset + address * 2;
        p.metadata.name = "Threshold " + std::to_string(address);

        for (auto &i : p.indices)
          indices.insert(i);

        ++address;
      }
    }
  }
}

}
