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
 *      MesytecExternal
 *
 ******************************************************************************/

#include "Mesytec_base_module.h"
#include "mesytec_external_module.h"

namespace Qpx {

MesytecExternal::MesytecExternal() {
  controller_ = nullptr;
  modnum_ = 0;
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  module_ID_code_ = -1;
}


MesytecExternal::~MesytecExternal() {
  die();
}

bool MesytecExternal::die() {
  disconnect();
  status_ = Qpx::DeviceStatus::loaded | Qpx::DeviceStatus::can_boot;
  return true;
}

bool MesytecExternal::boot() {
  if (!(status_ & Qpx::DeviceStatus::can_boot))
    return false;

  status_ = Qpx::DeviceStatus::loaded | Qpx::DeviceStatus::can_boot;

  if (!connected())
    return false;

  if (!controller_->RC_on(modnum_))
    return false;

  status_ = Qpx::DeviceStatus::loaded | Qpx::DeviceStatus::booted;
  return true;
}

bool MesytecExternal::connected() const {
  if (!controller_)
    return false;

  uint16_t status;
  if (!controller_->RC_get_ID(modnum_, status))
    return false;

  PL_DBG << "Mesytec external module at " << modnum_
         << " has status " << ((status & 1) != 0)
         << " with ID " << (status >> 1);

  return ((status >> 1) != module_ID_code_);
}

bool MesytecExternal::connect(MesytecVME *controller, int addr) {
  controller_ = controller;
  modnum_ = addr;
  return connected();
}

void MesytecExternal::disconnect() {
  controller_ = nullptr;
  modnum_ = 0;
}

bool MesytecExternal::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ != device_name())
    return false;

  for (auto &k : set.branches.my_data_) {
    if (k.metadata.setting_type != Gamma::SettingType::stem)
    {
      if (!read_setting(k)) {/*PL_DBG << "Could not read " << k.id_;*/}
    }
    else  {
      for (auto &p : k.branches.my_data_) {
        if (k.metadata.setting_type != Gamma::SettingType::stem) {
          if (!read_setting(p)) {/*PL_DBG << "Could not read " << p.id_;*/}
        }
      }
    }
  }

  return true;
}

bool MesytecExternal::write_settings_bulk(Gamma::Setting &set) {
  if (set.id_ != device_name())
    return false;

  set.enrich(setting_definitions_);

  this->rebuild_structure(set);

  for (auto &k : set.branches.my_data_) {
    if (k.metadata.setting_type != Gamma::SettingType::stem) {
      Gamma::Setting s = k;
      if (k.metadata.writable && read_setting(s) && (s != k)) {
        if (!write_setting(k)) {/*PL_DBG << "Could not write " << k.id_;*/}
      }
    } else {
      for (auto &p : k.branches.my_data_) {
        if (k.metadata.setting_type != Gamma::SettingType::stem) {
          Gamma::Setting s = p;
          if (p.metadata.writable && read_setting(s) && (s != p)) {
            if (!write_setting(p)) {/*PL_DBG << "Could not write " << p.id_;*/}
          }
        }
      }
    }
  }
  return true;
}

bool MesytecExternal::read_setting(Gamma::Setting& set) const {
  if (set.metadata.setting_type == Gamma::SettingType::command)
    set.metadata.writable =  ((status_ & DeviceStatus::booted) != 0);

  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if ((set.metadata.setting_type == Gamma::SettingType::binary)
      || (set.metadata.setting_type == Gamma::SettingType::command)
      || (set.metadata.setting_type == Gamma::SettingType::integer)
      || (set.metadata.setting_type == Gamma::SettingType::boolean)
      || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    uint16_t val;
    if (controller_->RC_read(modnum_, set.metadata.address, val)) {
      set.value_int = val;
      return true;
    }
  }
  return false;
}

bool MesytecExternal::write_setting(Gamma::Setting& set) {
  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if ((set.metadata.setting_type == Gamma::SettingType::binary)
      || (set.metadata.setting_type == Gamma::SettingType::command)
      || (set.metadata.setting_type == Gamma::SettingType::integer)
      || (set.metadata.setting_type == Gamma::SettingType::boolean)
      || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    return controller_->RC_write(modnum_, set.metadata.address, set.value_int);
  }
  return false;
}

}
