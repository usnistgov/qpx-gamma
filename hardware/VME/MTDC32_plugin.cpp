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

#include "MTDC32_plugin.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "custom_timer.h"

#define MTDC32_Firmware                   0x0104
#define MTDC32_Firmware_Address           0x600E

typedef union {
  long    l;
  float   f;
  uint16_t  w[2];
  uint8_t   b[4];
} TFloatWord;


namespace Qpx {

static DeviceRegistrar<MTDC32> registrar("VME/MTDC32");

MTDC32::MTDC32() {
  m_controller = nullptr;
  m_baseAddress = 0;
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
}


MTDC32::~MTDC32() {
  die();
}


bool MTDC32::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ == "VME/MTDC32") {
    for (auto &k : set.branches.my_data_) {
      if (k.metadata.setting_type != Gamma::SettingType::stem)
      {
        if (!read_setting(k)) {}
//          PL_DBG << "Could not read " << k.id_;
      }
      else  {
        for (auto &p : k.branches.my_data_) {
          if (p.metadata.setting_type == Gamma::SettingType::command) {
            p.metadata.writable =  ((status_ & DeviceStatus::can_exec) != 0);
            //PL_DBG << "command " << p.id_ << p.index << " now " << p.metadata.writable;
          } else if (k.metadata.setting_type != Gamma::SettingType::stem) {
            if (!read_setting(p)) {}
//              PL_DBG << "Could not read " << p.id_;
          }
        }
      }
    }

  }
  return true;
}

void MTDC32::rebuild_structure(Gamma::Setting &set) {
  for (auto &k : set.branches.my_data_) {
    if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/MTDC32/ChannelThresholds")) {
//        PL_DBG << "writing VME/MTDC32/Channel";
      while (k.branches.size() > 32) {
        k.branches.my_data_.pop_back();
      }

      Gamma::Setting temp = k.branches.get(0);
      temp.indices.clear();
      temp.index = -1;
      while (k.branches.size() < 32) {
        k.branches.my_data_.push_back(temp);
      }


      uint32_t offset = k.metadata.address;

      std::set<int32_t> indices;
      int address = 0;
      for (auto &p : k.branches.my_data_) {
        p.metadata.address = offset + address * 2;
        ++address;
      }
    }
  }
}


bool MTDC32::write_settings_bulk(Gamma::Setting &set) {
  set.enrich(setting_definitions_);

  if (set.id_ == "VME/MTDC32") {
//    PL_DBG << "writing VME/MTDC32";
    rebuild_structure(set);

    for (auto &k : set.branches.my_data_) {
      if (k.metadata.setting_type != Gamma::SettingType::stem) {
        Gamma::Setting s = k;
        if (k.metadata.writable && read_setting(s) && (s != k)) {
//          PL_DBG << "writing " << k.id_;
          if (!write_setting(k)) {}
//            PL_DBG << "Could not write " << k.id_;
        }
      } else {
        for (auto &p : k.branches.my_data_) {
          if (k.metadata.setting_type != Gamma::SettingType::stem) {
            Gamma::Setting s = p;
            if (p.metadata.writable && read_setting(s) && (s != p)) {
//              PL_DBG << "writing " << p.id_;
              if (!write_setting(p)) {}
//                PL_DBG << "Could not write " << k.id_;
            }
          }
        }
      }
    }
  }
  return true;
}

bool MTDC32::execute_command(Gamma::Setting &set) {
  if (!(status_ & DeviceStatus::can_exec))
    return false;

  //  if (set.id_ != device_name())
  //    return false;

  if (set.id_ == "VME/MTDC32") {

    for (auto &k : set.branches.my_data_) {
      if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/MTDC32/Channel")) {

        uint16_t channum = k.index;
        for (auto &p : k.branches.my_data_) {

          if ((p.metadata.setting_type == Gamma::SettingType::command) && (p.value_int == 1)) {

          }
        }
      }
    }
  }
  return false;
}


bool MTDC32::boot() {
  if (!(status_ & DeviceStatus::can_boot)) {
    PL_WARN << "<MTDC32> Cannot boot. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  if (!connected()) {
    PL_WARN << "<MTDC32> Not connected to controller";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::booted | DeviceStatus::can_exec;
  return true;
}

bool MTDC32::die() {
  PL_DBG << "<MTDC32> Disconnecting";

  disconnect();
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  return true;
}

void MTDC32::get_all_settings() {
}

bool MTDC32::connect(VmeController *controller, int baseAddress)
{
  m_controller = controller;
  setBaseAddress(baseAddress);
}

bool MTDC32::connected() const
{
  int firmware = 0;
  if (m_controller)
    firmware = readShort(m_baseAddress + MTDC32_Firmware_Address);

  return firmware == MTDC32_Firmware;
}

void MTDC32::disconnect()
{
  m_controller = nullptr;
  m_baseAddress = 0;
}

std::string MTDC32::address() const {
  std::stringstream stream;
  stream << "VME BA 0x"
         << std::setfill ('0') << std::setw(sizeof(uint16_t)*2)
         << std::hex << m_baseAddress;
  return stream.str();
}

bool MTDC32::read_setting(Gamma::Setting& set) const {
  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if ((set.metadata.setting_type == Gamma::SettingType::binary)
           || (set.metadata.setting_type == Gamma::SettingType::integer)
           || (set.metadata.setting_type == Gamma::SettingType::boolean)
           || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    set.value_int = readShort(m_baseAddress + set.metadata.address);
  } else if (set.metadata.setting_type == Gamma::SettingType::floating) {
    set.value_dbl = readFloat(m_baseAddress + set.metadata.address);
  }
  return true;
}

bool MTDC32::write_setting(Gamma::Setting& set) {
  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if ((set.metadata.setting_type == Gamma::SettingType::binary)
        || (set.metadata.setting_type == Gamma::SettingType::integer)
        || (set.metadata.setting_type == Gamma::SettingType::boolean)
        || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    writeShort(m_baseAddress + set.metadata.address, set.value_int);
  } else if (set.metadata.setting_type == Gamma::SettingType::floating) {
    writeFloat(m_baseAddress + set.metadata.address, set.value_dbl);
  }
  return true;
}

uint16_t MTDC32::readShort(int address) const
{
  if (m_controller) {
    return m_controller->readShort(address, AddressModifier::A16_Supervisory);
  } else {
    return 0;
  }
}

void MTDC32::writeShort(int address, uint16_t data)
{
  if (m_controller) {
    m_controller->writeShort(address, AddressModifier::A16_Supervisory, data);
  }
}

float MTDC32::readFloat(int address) const
{
  uint32_t data = readShort(address);
  return *reinterpret_cast<float*>( &data ) ;
}

void MTDC32::writeFloat(int address, float data)
{
  uint16_t data_int = *reinterpret_cast<uint16_t*>( &data );
  writeShort(address, data_int);
}



//=============================================================================
// Module Commands
//=============================================================================


std::string MTDC32::firmwareName() const
{
  int firmware = readShort(m_baseAddress + MTDC32_Firmware_Address);
  return std::to_string(firmware);
}


bool MTDC32::setBaseAddress(uint16_t baseAddress)
{
  m_baseAddress = baseAddress;

  int firmware = 0;
  if (m_controller)
    firmware = readShort(m_baseAddress + MTDC32_Firmware_Address);

  return (firmware == MTDC32_Firmware);
}


}
