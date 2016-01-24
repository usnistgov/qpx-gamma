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
 *      MesytecVME
 *
 ******************************************************************************/

#include "Mesytec_base_module.h"
#include "mesytec_external_module.h"
#include "vmecontroller.h"
#include "custom_logger.h"
#include "custom_timer.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "qpx_util.h"

namespace Qpx {

MesytecVME::MesytecVME() {
  m_controller = nullptr;
  m_baseAddress = 0;
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  module_firmware_code_ = -1;
  rc_bus = false;
}


MesytecVME::~MesytecVME() {
  die();
}

bool MesytecVME::boot() {
  rc_bus = false;
  if (VmeModule::boot()) {
    if (setting_definitions_.count(this->device_name() + "/rc_busno"))
      rc_busno = setting_definitions_.at(this->device_name() + "/rc_busno").address;
    else
      return true;

    if (setting_definitions_.count(this->device_name() + "/rc_modnum"))
      rc_modnum = setting_definitions_.at(this->device_name() + "/rc_modnum").address;
    else
      return true;

    if (setting_definitions_.count(this->device_name() + "/rc_opcode")) {
      Gamma::SettingMeta opcode = setting_definitions_.at(this->device_name() + "/rc_opcode");
      rc_opcode = opcode.address;
      for (auto &q : opcode.int_menu_items) {
        if (q.second == "RC_on")
          rc_opcode_on = q.first;
        if (q.second == "RC_off")
          rc_opcode_off = q.first;
        if (q.second == "read_id")
          rc_opcode_read_id = q.first;
        if (q.second == "read_data")
          rc_opcode_read_data = q.first;
        if (q.second == "write_data")
          rc_opcode_write_data = q.first;
      }
    } else
      return true;

    if (setting_definitions_.count(this->device_name() + "/rc_adr"))
      rc_adr = setting_definitions_.at(this->device_name() + "/rc_adr").address;
    else
      return true;

    if (setting_definitions_.count(this->device_name() + "/rc_dat"))
      rc_dat = setting_definitions_.at(this->device_name() + "/rc_dat").address;
    else
      return true;

    if (setting_definitions_.count(this->device_name() + "/rc_return_status")) {
      Gamma::SettingMeta ret = setting_definitions_.at(this->device_name() + "/rc_return_status");
      rc_return_status = ret.address;
      for (auto &q : ret.int_menu_items) {
        if (q.second == "active")
          rc_return_status_active_mask = 1 << q.first;
        if (q.second == "collision")
          rc_return_status_collision_mask = 1 << q.first;
        if (q.second == "no_response")
          rc_return_status_no_response_mask = 1 << q.first;
      }
    } else
      return true;

    rc_bus = true;

    bool success = false;

    for (auto &q : ext_modules_) {
      if ((q.second) && (!q.second->connected())) {
        PL_DBG << "<" << this->device_name() << "> Searching for module " << q.first;
  //      long base_address = 0;
  //      if (q.first == "VME/MADC32")
  //        base_address = 0x20000000;
        for (int addr = 0; addr < 16; addr++) {
          q.second->connect(this, addr);

          if (q.second->connected()) {
            q.second->boot(); //disregard return value
            PL_DBG << "<" << this->device_name() << "> Adding module " << q.first
                   << "[" << q.second->modnum()
                   << "] booted=" << (0 != (q.second->status() & DeviceStatus::booted));
            success = true;
            break;
          }
        }
      }
    }

    return true;

  } else
    return false;
}

bool MesytecVME::die() {

  for (auto &q : ext_modules_)
    delete q.second;
  ext_modules_.clear();

  VmeModule::die();
  rc_bus = false;
}


bool MesytecVME::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ != device_name())
    return false;

  for (auto &k : set.branches.my_data_) {
    if (k.metadata.setting_type != Gamma::SettingType::stem)
    {
      if (!read_setting(k)) {/*PL_DBG << "Could not read " << k.id_;*/}
    }
    else  {
      if (ext_modules_.count(k.id_) && ext_modules_.at(k.id_)) {
        //          PL_DBG << "read settings bulk " << q.id_;
        ext_modules_.at(k.id_)->read_settings_bulk(k);
      } else {

        for (auto &p : k.branches.my_data_) {
          if (k.metadata.setting_type != Gamma::SettingType::stem) {
            if (!read_setting(p)) {/*PL_DBG << "Could not read " << p.id_;*/}
          }
        }
      }
    }
  }

  return true;
}

bool MesytecVME::write_settings_bulk(Gamma::Setting &set) {
  if (set.id_ != device_name())
    return false;

  set.enrich(setting_definitions_);

  this->rebuild_structure(set);

  boost::filesystem::path dir(profile_path_);
  dir.make_preferred();
  boost::filesystem::path path = dir.remove_filename();

  std::set<std::string> device_types;
  for (auto &q : Qpx::DeviceFactory::getInstance().types())
    device_types.insert(q);

  for (auto &k : set.branches.my_data_) {
    if (k.metadata.setting_type != Gamma::SettingType::stem) {
      Gamma::Setting s = k;
      if (k.metadata.writable && read_setting(s) && (s != k)) {
        if (!write_setting(k)) {/*PL_DBG << "Could not write " << k.id_;*/}
      }
    } else if ((ext_modules_.count(k.id_) && ext_modules_.at(k.id_))) {
      ext_modules_[k.id_]->write_settings_bulk(k);
    } else if (device_types.count(k.id_) && (k.id_.size() > 14) && (k.id_.substr(0,14) == "VME/MesytecRC/")) {
      boost::filesystem::path dev_settings = path / k.value_text;
      ext_modules_[k.id_] = dynamic_cast<MesytecExternal*>(DeviceFactory::getInstance().create_type(k.id_, dev_settings.string()));
      PL_DBG << "<" << this->device_name() << ">added module " << k.id_ << " with settings at " << dev_settings.string();
      ext_modules_[k.id_]->write_settings_bulk(k);
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

bool MesytecVME::read_setting(Gamma::Setting& set) const {
  if (set.metadata.setting_type == Gamma::SettingType::command)
    set.metadata.writable =  ((status_ & DeviceStatus::booted) != 0);

  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if (set.metadata.address < 0)
    return false;

  if ((set.metadata.setting_type == Gamma::SettingType::binary)
      || (set.metadata.setting_type == Gamma::SettingType::command)
      || (set.metadata.setting_type == Gamma::SettingType::integer)
      || (set.metadata.setting_type == Gamma::SettingType::boolean)
      || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    set.value_int = readShort(set.metadata.address);
  } else if (set.metadata.setting_type == Gamma::SettingType::floating) {
    set.value_dbl = readFloat(set.metadata.address);
  }
  return true;
}

bool MesytecVME::write_setting(Gamma::Setting& set) {
  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if (set.metadata.address < 0)
    return false;

  if ((set.metadata.setting_type == Gamma::SettingType::binary)
      || (set.metadata.setting_type == Gamma::SettingType::command)
      || (set.metadata.setting_type == Gamma::SettingType::integer)
      || (set.metadata.setting_type == Gamma::SettingType::boolean)
      || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    writeShort(set.metadata.address, set.value_int);
  } else if (set.metadata.setting_type == Gamma::SettingType::floating) {
    writeFloat(set.metadata.address, set.value_dbl);
  }
  return true;
}

uint16_t MesytecVME::readShort(int address) const
{
  if (m_controller) {
    return m_controller->read16(m_baseAddress + address, AddressModifier::A16_Priv);
  } else {
    return 0;
  }
}

void MesytecVME::writeShort(int address, uint16_t data) const
{
  if (m_controller) {
    m_controller->write16(m_baseAddress + address, AddressModifier::A16_Priv, data);
  }
}

float MesytecVME::readFloat(int address) const
{
  uint32_t data = readShort(address);
  return *reinterpret_cast<float*>( &data ) ;
}

void MesytecVME::writeFloat(int address, float data) const
{
  uint16_t data_int = *reinterpret_cast<uint16_t*>( &data );
  writeShort(address, data_int);
}

bool MesytecVME::connected() const
{
  if (!m_controller)
    return false;

  Gamma::Setting fwaddress(this->device_name() + "/firmware_version");
  fwaddress.enrich(setting_definitions_);
  read_setting(fwaddress);

  return (fwaddress.value_int == this->module_firmware_code_);
}

std::string MesytecVME::firmwareName() const
{
  Gamma::Setting fwaddress(this->device_name() + "/firmware_version");
  fwaddress.enrich(setting_definitions_);
  read_setting(fwaddress);

  return "0x" + itohex32(uint32_t(fwaddress.value_int));
}


// Mesytec RC bus

bool MesytecVME::RC_wait(double millisex) const {
  double total = 0.400;
  wait_us(400);

  uint16_t ret = readShort(rc_return_status);
  while ((ret & rc_return_status_active_mask) && (total < millisex)) {
    wait_us(100);
    total += 0.100;
    ret = readShort(rc_return_status);
  }

  if ((ret & rc_return_status_active_mask) ||
      (ret & rc_return_status_collision_mask) ||
      (ret & rc_return_status_no_response_mask))
    return false;

  return true;
}


bool MesytecVME::RC_get_ID(uint16_t module, uint16_t &data) const {
  if (!rc_bus)
    return false;

  writeShort(rc_modnum, module);
  writeShort(rc_opcode, rc_opcode_read_id);
  writeShort(rc_dat, 0);

  if (!RC_wait())
    return false;

  data =readShort(rc_dat);
  return true;
}

bool MesytecVME::RC_on(uint16_t module) {
  if (!rc_bus)
    return false;

  writeShort(rc_modnum, module);
  writeShort(rc_opcode, rc_opcode_on);
  writeShort(rc_dat, 0);
  return RC_wait();
}

bool MesytecVME::RC_off(uint16_t module) {
  if (!rc_bus)
    return false;

  writeShort(rc_modnum, module);
  writeShort(rc_opcode, rc_opcode_off);
  writeShort(rc_dat, 0);
  return RC_wait();
}

bool MesytecVME::RC_read(uint16_t module, uint16_t setting, uint16_t &data) const {
  if (!rc_bus)
    return false;

  writeShort(rc_modnum, module);
  writeShort(rc_opcode, rc_opcode_read_data);
  writeShort(rc_adr, setting);
  writeShort(rc_dat, 0);
  if (!RC_wait())
    return false;

  data =readShort(rc_dat);
  return true;
}

bool MesytecVME::RC_write(uint16_t module, uint16_t setting, uint16_t data) {
  if (!rc_bus)
    return false;

  writeShort(rc_modnum, module);
  writeShort(rc_opcode, rc_opcode_write_data);
  writeShort(rc_adr, setting);
  writeShort(rc_dat, 0);
  if (!RC_wait())
    return false;

  uint16_t data2 =readShort(rc_dat);
  return (data == data2);
}



}
