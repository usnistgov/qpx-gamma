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

#include "MADC32_plugin.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "custom_timer.h"

#define MADC32_Firmware                   0x0203
#define MADC32_Firmware_Address           0x600E

typedef union {
  long    l;
  float   f;
  uint16_t  w[2];
  uint8_t   b[4];
} TFloatWord;


namespace Qpx {

static DeviceRegistrar<MADC32> registrar("VME/MADC32");

MADC32::MADC32() {
  m_controller = nullptr;
  m_baseAddress = 0;
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
}


MADC32::~MADC32() {
  die();
}


bool MADC32::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ != device_name())
    return false;

  for (auto &k : set.branches.my_data_) {
    if (k.metadata.setting_type != Gamma::SettingType::stem)
    {
      if (!read_setting(k)) {}
      //          PL_DBG << "Could not read " << k.id_;
    }
    else  {
      for (auto &p : k.branches.my_data_) {
        if (k.metadata.setting_type != Gamma::SettingType::stem) {
          if (!read_setting(p)) {}
          //              PL_DBG << "Could not read " << p.id_;
        }
      }
    }
  }

  return true;
}

void MADC32::rebuild_structure(Gamma::Setting &set) {
  for (auto &k : set.branches.my_data_) {
    if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/MADC32/ChannelThresholds")) {
//        PL_DBG << "writing VME/MADC32/Channel";
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
        p.metadata.name = "Threshold " + std::to_string(address);

        for (auto &i : p.indices)
          indices.insert(i);

        ++address;
      }
    }
  }
}


bool MADC32::write_settings_bulk(Gamma::Setting &set) {
  if (set.id_ != device_name())
    return false;

  set.enrich(setting_definitions_);

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
  return true;
}

bool MADC32::boot() {
  if (!(status_ & DeviceStatus::can_boot)) {
    PL_WARN << "<MADC32> Cannot boot. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  if (!connected()) {
    PL_WARN << "<MADC32> Not connected to controller";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::booted;
  return true;
}

bool MADC32::die() {
  PL_DBG << "<MADC32> Disconnecting";

  disconnect();
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  return true;
}

void MADC32::get_all_settings() {
}

bool MADC32::connect(VmeController *controller, int baseAddress)
{
  m_controller = controller;
  setBaseAddress(baseAddress);
}

bool MADC32::connected() const
{

  int firmware = 0;
  if (m_controller)
    firmware = readShort(m_baseAddress + MADC32_Firmware_Address);

  return firmware == MADC32_Firmware;
}

void MADC32::disconnect()
{
  m_controller = nullptr;
  m_baseAddress = 0;
}

std::string MADC32::address() const {
  std::stringstream stream;
  stream << "VME BA 0x"
         << std::setfill ('0') << std::setw(sizeof(uint16_t)*2)
         << std::hex << m_baseAddress;
  return stream.str();
}

bool MADC32::read_setting(Gamma::Setting& set) const {
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
    set.value_int = readShort(m_baseAddress + set.metadata.address);
  } else if (set.metadata.setting_type == Gamma::SettingType::floating) {
    set.value_dbl = readFloat(m_baseAddress + set.metadata.address);
  }
  return true;
}

bool MADC32::write_setting(Gamma::Setting& set) {
  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if ((set.metadata.setting_type == Gamma::SettingType::binary)
      || (set.metadata.setting_type == Gamma::SettingType::command)
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

uint16_t MADC32::readShort(int address) const
{
  if (m_controller) {
    return m_controller->readShort(address, AddressModifier::A16_Supervisory);
  } else {
    return 0;
  }
}

void MADC32::writeShort(int address, uint16_t data)
{
  if (m_controller) {
    m_controller->writeShort(address, AddressModifier::A16_Supervisory, data);
  }
}

float MADC32::readFloat(int address) const
{
  uint32_t data = readShort(address);
  return *reinterpret_cast<float*>( &data ) ;
}

void MADC32::writeFloat(int address, float data)
{
  uint16_t data_int = *reinterpret_cast<uint16_t*>( &data );
  writeShort(address, data_int);
}



//=============================================================================
// Module Commands
//=============================================================================


std::string MADC32::firmwareName() const
{
  uint16_t firmware = readShort(m_baseAddress + MADC32_Firmware_Address);

  std::stringstream stream;
  stream << "0x" << std::setfill ('0') << std::setw(sizeof(uint16_t)*2)
         << std::hex << firmware;

  return stream.str();
}


bool MADC32::setBaseAddress(uint32_t baseAddress)
{
  m_baseAddress = baseAddress;

  int firmware = 0;
  if (m_controller)
    firmware = readShort(m_baseAddress + MADC32_Firmware_Address);

  //PL_DBG << "MADC32 BA=" << m_baseAddress <<  " firmware " << firmware;

  return (firmware == MADC32_Firmware);
}


}
