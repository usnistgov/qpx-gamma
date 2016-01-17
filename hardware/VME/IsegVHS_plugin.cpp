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
 *      QpxIsegVHSPlugin
 *
 ******************************************************************************/

#include "IsegVHS_plugin.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "custom_timer.h"

#define ISEG_VENDOR_ID									0x69736567
#define VME_ADDRESS_MODIFIER						0x29

// --- VHS12 ------------------------------------------------------------------
#define VhsFirmwareReleaseOFFSET          56
#define VhsDeviceClassOFFSET              62
#define VhsVendorIdOFFSET                 92
#define VhsNewBaseAddressOFFSET           0x03A0
#define VhsNewBaseAddressXorOFFSET        0x03A2
#define VhsNewBaseAddressAcceptedOFFSET   0x03A6

typedef union {
  long    l;
  float   f;
  uint16_t  w[2];
  uint8_t   b[4];
} TFloatWord;


namespace Qpx {

static DeviceRegistrar<QpxIsegVHSPlugin> registrar("VME/IsegVHS");

QpxIsegVHSPlugin::QpxIsegVHSPlugin() {
  m_controller = nullptr;
  m_baseAddress = 0;
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
}


QpxIsegVHSPlugin::~QpxIsegVHSPlugin() {
  die();
}


bool QpxIsegVHSPlugin::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ == "VME/IsegVHS") {
    double voltage_max = 0;
    double current_max = 0;
    for (auto &k : set.branches.my_data_) {
      if ((k.metadata.setting_type == Gamma::SettingType::floating)
          || (k.metadata.setting_type == Gamma::SettingType::binary)
          || (k.metadata.setting_type == Gamma::SettingType::integer)
          || (k.metadata.setting_type == Gamma::SettingType::boolean)
          || (k.metadata.setting_type == Gamma::SettingType::int_menu))
      {
        if (!read_setting(k, 0)) {}
//          PL_DBG << "Could not read " << k.id_;
        if (k.id_ == "VME/IsegVHS/VoltageMax") {
          voltage_max = k.value_dbl;
          //                PL_DBG << "voltage max = " << voltage_max;
        }
        if (k.id_ == "VME/IsegVHS/CurrentMax") {
          current_max = k.value_dbl;
          //                PL_DBG << "current max = " << current_max;
        }
      }
      else if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/IsegVHS/Channel")) {
        uint16_t channum = k.index;
        for (auto &p : k.branches.my_data_) {
          if (p.metadata.setting_type == Gamma::SettingType::command) {
            p.metadata.writable =  ((status_ & DeviceStatus::can_exec) != 0);
            //PL_DBG << "command " << p.id_ << p.index << " now " << p.metadata.writable;
          }
          else if ((p.metadata.setting_type == Gamma::SettingType::floating)
                   || (p.metadata.setting_type == Gamma::SettingType::integer)
                   || (p.metadata.setting_type == Gamma::SettingType::boolean)
                   || (p.metadata.setting_type == Gamma::SettingType::int_menu)
                   || (p.metadata.setting_type == Gamma::SettingType::binary)) {
            if (!read_setting(p, ((48 * channum) + 96))) {}
//              PL_DBG << "Could not read " << p.id_;
          }
        }

        for (auto &p : k.branches.my_data_) {
          if (p.id_ == "VME/IsegVHS/Channel/Status") {
            Gamma::Setting status = k.get_setting(Gamma::Setting("VME/IsegVHS/Channel/ChannelStatus"), Gamma::Match::id);
            if (status.value_int & 0x0010)
              p.value_int = 1;
            else if (status.value_int & 0x0008)
              p.value_int = 2;
            else
              p.value_int = 0;
          } else if (p.id_ == "VME/IsegVHS/Channel/VoltageSet") {
            Gamma::Setting nominal = k.get_setting(Gamma::Setting("VME/IsegVHS/Channel/VoltageNominalMaxSet"), Gamma::Match::id);
            p.metadata.maximum = nominal.value_dbl;// * voltage_max;
            //                  PL_DBG << "Volatge bounds "  << nominal.value_dbl << " " << p.metadata.maximum;
          } else if (p.id_ == "VME/IsegVHS/Channel/CurrentSetOrTrip") {
            Gamma::Setting nominal = k.get_setting(Gamma::Setting("VME/IsegVHS/Channel/CurrentNominalMaxSet"), Gamma::Match::id);
            p.metadata.maximum = nominal.value_dbl;// * current_max;
            //                  PL_DBG << "Current bounds "  << nominal.value_dbl << " " << p.metadata.maximum;
          }
        }
      }
    }

  }
  return true;
}

void QpxIsegVHSPlugin::rebuild_structure(Gamma::Setting &set) {

}


bool QpxIsegVHSPlugin::write_settings_bulk(Gamma::Setting &set) {
  set.enrich(setting_definitions_);

  if (set.id_ == "VME/IsegVHS") {
//    PL_DBG << "writing VME/IsegVHS";
    rebuild_structure(set);

    for (auto &k : set.branches.my_data_) {
      if ((k.metadata.setting_type == Gamma::SettingType::floating)
          || (k.metadata.setting_type == Gamma::SettingType::integer)
          || (k.metadata.setting_type == Gamma::SettingType::boolean)
          || (k.metadata.setting_type == Gamma::SettingType::int_menu)
          || (k.metadata.setting_type == Gamma::SettingType::binary)) {
        Gamma::Setting s = k;
        if (k.metadata.writable && read_setting(s, 0) && (s != k)) {
//          PL_DBG << "writing " << k.id_;
          if (!write_setting(k, 0)) {}
//            PL_DBG << "Could not write " << k.id_;
        }
      }
      else if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/IsegVHS/Channel")) {
//        PL_DBG << "writing VME/IsegVHS/Channel";
        uint16_t channum = k.index;
        for (auto &p : k.branches.my_data_) {
          if ((p.metadata.setting_type == Gamma::SettingType::floating)
              || (p.metadata.setting_type == Gamma::SettingType::integer)
              || (p.metadata.setting_type == Gamma::SettingType::boolean)
              || (p.metadata.setting_type == Gamma::SettingType::int_menu)
              || (p.metadata.setting_type == Gamma::SettingType::binary)) {
            Gamma::Setting s = p;
            if (p.metadata.writable && read_setting(s, ((48 * channum) + 96)) && (s != p)) {
//              PL_DBG << "writing " << p.id_;
              if (!write_setting(p, ((48 * channum) + 96))) {}
//                PL_DBG << "Could not write " << k.id_;
            }
          }
        }
      }
    }
  }
  return true;
}

bool QpxIsegVHSPlugin::execute_command(Gamma::Setting &set) {
  if (!(status_ & DeviceStatus::can_exec))
    return false;

  //  if (set.id_ != device_name())
  //    return false;

  if (set.id_ == "VME/IsegVHS") {

    for (auto &k : set.branches.my_data_) {
      if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/IsegVHS/Channel")) {

        uint16_t channum = k.index;
        for (auto &p : k.branches.my_data_) {

          if ((p.metadata.setting_type == Gamma::SettingType::command) && (p.value_int == 1)) {
            if (p.id_ == "VME/IsegVHS/Channel/OnOff") {
              //                  PL_DBG << "Triggered ON/OFF for " << p.index;
              p.value_int = 0;
              Gamma::Setting ctrl = k.get_setting(Gamma::Setting("VME/IsegVHS/Channel/ChannelControl"), Gamma::Match::id);
              if (p.metadata.writable && read_setting(ctrl, ((48 * channum) + 96))) {
                ctrl.value_int  ^= 0x0008;
                if (!write_setting(ctrl, ((48 * channum) + 96)))
                  PL_DBG << "Could not write " << k.id_;
              }
              return true;
            }
          }
        }
      }
    }
  }
  return false;
}


bool QpxIsegVHSPlugin::boot() {
  if (!(status_ & DeviceStatus::can_boot)) {
    PL_WARN << "<IsegVHS> Cannot boot. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  if (!connected()) {
    PL_WARN << "<IsegVHS> Not connected to controller";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::booted | DeviceStatus::can_exec;
  return true;
}

bool QpxIsegVHSPlugin::die() {
  PL_DBG << "<IsegVHS> Disconnecting";

  disconnect();
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  return true;
}

void QpxIsegVHSPlugin::get_all_settings() {
}

bool QpxIsegVHSPlugin::connect(VmeController *controller, int baseAddress)
{
  m_controller = controller;
  setBaseAddress(baseAddress);
}

bool QpxIsegVHSPlugin::connected() const
{
  uint32_t vendorId = 0;
  if (m_controller)
    vendorId = readLong(m_baseAddress + VhsVendorIdOFFSET);

  return vendorId == ISEG_VENDOR_ID;
}

void QpxIsegVHSPlugin::disconnect()
{
  m_controller = nullptr;
  m_baseAddress = 0;
}

std::string QpxIsegVHSPlugin::address() const {
  std::stringstream stream;
  stream << "VME BA 0x"
         << std::setfill ('0') << std::setw(sizeof(uint16_t)*2)
         << std::hex << m_baseAddress;
  return stream.str();
}

bool QpxIsegVHSPlugin::read_setting(Gamma::Setting& set, int address_modifier) const {
  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if (set.metadata.setting_type == Gamma::SettingType::floating)
    set.value_dbl = readFloat(m_baseAddress + address_modifier + set.metadata.address);
  else if (set.metadata.setting_type == Gamma::SettingType::binary) {
    if (set.metadata.maximum == 32)
      set.value_int = readLongBitfield(m_baseAddress + address_modifier + set.metadata.address);
    else if (set.metadata.maximum == 16)
      set.value_int = readShortBitfield(m_baseAddress + address_modifier + set.metadata.address);
    else {
      PL_DBG << "Setting " << set.id_ << " does not have a well defined hardware type";
      return false;
    }
  }
  else if ((set.metadata.setting_type == Gamma::SettingType::integer)
           || (set.metadata.setting_type == Gamma::SettingType::boolean)
           || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    if (set.metadata.hardware_type == "u32")
      set.value_int = readLong(m_baseAddress + address_modifier + set.metadata.address);
    else if (set.metadata.hardware_type == "u16")
      set.value_int = readShort(m_baseAddress + address_modifier + set.metadata.address);
    else {
      PL_DBG << "Setting " << set.id_ << " does not have a well defined hardware type";
      return false;
    }
  }
  return true;
}

bool QpxIsegVHSPlugin::write_setting(Gamma::Setting& set, int address_modifier) {
  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if (set.metadata.setting_type == Gamma::SettingType::floating)
    writeFloat(m_baseAddress + address_modifier + set.metadata.address, set.value_dbl);
  else if (set.metadata.setting_type == Gamma::SettingType::binary) {
    if (set.metadata.maximum == 32)
      writeLongBitfield(m_baseAddress + address_modifier + set.metadata.address, set.value_int);
    else if (set.metadata.maximum == 16)
      writeShortBitfield(m_baseAddress + address_modifier + set.metadata.address, set.value_int);
    else {
      PL_DBG << "Setting " << set.id_ << " does not have a well defined hardware type";
      return false;
    }
  }
  else if ((set.metadata.setting_type == Gamma::SettingType::integer)
           || (set.metadata.setting_type == Gamma::SettingType::boolean)
           || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    if (set.metadata.hardware_type == "u32")
      writeLong(m_baseAddress + address_modifier + set.metadata.address, set.value_int);
    else if (set.metadata.hardware_type == "u16")
      writeShort(m_baseAddress + address_modifier + set.metadata.address, set.value_int);
    else {
      PL_DBG << "Setting " << set.id_ << " does not have a well defined hardware type";
      return false;
    }
  }
  return true;
}

uint16_t QpxIsegVHSPlugin::readShort(int address) const
{
  if (m_controller) {
    return m_controller->readShort(address, VME_ADDRESS_MODIFIER);
  } else {
    return 0;
  }
}

void QpxIsegVHSPlugin::writeShort(int address, uint16_t data)
{
  if (m_controller) {
    m_controller->writeShort(address, VME_ADDRESS_MODIFIER, data);
  }
}

uint16_t QpxIsegVHSPlugin::readShortBitfield(int address) const
{
  return readShort(address);
}

void QpxIsegVHSPlugin::writeShortBitfield(int address, uint16_t data)
{
  writeShort(address, data);
}

float QpxIsegVHSPlugin::readFloat(int address) const
{
  TFloatWord fw = { 0 };

#ifdef BOOST_LITTLE_ENDIAN
    fw.w[1] = readShort(address + 0); // swap words
    fw.w[0] = readShort(address + 2);
#elif BOOST_BIG_ENDIAN
    fw.w[0] = readShort(address + 0);
    fw.w[1] = readShort(address + 2);
#endif

  return fw.f;
}

void QpxIsegVHSPlugin::writeFloat(int address, float data)
{
  TFloatWord fw;

  fw.f = data;

#ifdef BOOST_LITTLE_ENDIAN
    writeShort(address + 0, fw.w[1]);
    writeShort(address + 2, fw.w[0]);
#elif BOOST_BIG_ENDIAN
    writeShort(address + 0, fw.w[0]);
    writeShort(address + 2, fw.w[1]);
#endif
}

uint32_t QpxIsegVHSPlugin::readLong(int address) const
{
  TFloatWord fw = { 0 };

#ifdef BOOST_LITTLE_ENDIAN
    fw.w[1] = readShort(address + 0); // swap words
    fw.w[0] = readShort(address + 2);
#elif BOOST_BIG_ENDIAN
    fw.w[0] = readShort(address + 0);
    fw.w[1] = readShort(address + 2);
#endif

  return fw.l;
}

void QpxIsegVHSPlugin::writeLong(int address, uint32_t data)
{
  TFloatWord fw;

  fw.l = data;

#ifdef BOOST_LITTLE_ENDIAN
    writeShort(address + 0, fw.w[1]);
    writeShort(address + 2, fw.w[0]);
#elif BOOST_BIG_ENDIAN
    writeShort(address + 0, fw.w[0]);
    writeShort(address + 2, fw.w[1]);
#endif
}

uint32_t QpxIsegVHSPlugin::readLongBitfield(int address) const
{
  return readLong(address);
}

void QpxIsegVHSPlugin::writeLongBitfield(int address, uint32_t data)
{
  writeLong(address, data);
}

/**
 * Mirrors the Bit positions in a 16 bit word.
 */
uint16_t QpxIsegVHSPlugin::mirrorShort(uint16_t data)
{
  uint16_t result = 0;

  for (int i = 16; i; --i) {
    result >>= 1;
    result |= data & 0x8000;
    data <<= 1;
  }

  return result;
}

/**
 * Mirrors the Bit positions in a 32 bit word.
 */
uint32_t QpxIsegVHSPlugin::mirrorLong(uint32_t data)
{
  uint32_t result = 0;

  for (int i = 32; i; --i) {
    result >>= 1;
    result |= data & 0x80000000;
    data <<= 1;
  }

  return result;
}

//=============================================================================
// Module Commands
//=============================================================================


std::string QpxIsegVHSPlugin::firmwareName() const
{
  if (!m_controller)
    return std::string();

  uint32_t version = readLong(m_baseAddress + VhsFirmwareReleaseOFFSET);

  return std::to_string(version >> 24) + std::to_string(version >> 16) + "." +
    std::to_string(version >>  8) + std::to_string(version >>  0);
}


bool QpxIsegVHSPlugin::setBaseAddress(uint16_t baseAddress)
{
  m_baseAddress = baseAddress;

  //uint32_t vendorId = readLong(m_baseAddress + VhsVendorIdOFFSET);
  uint32_t tmp = readLong(m_baseAddress + 60);

  int deviceClass = (uint16_t)tmp;

  //PL_DBG << "device class = " << deviceClass;

  int V12C0 = 20;		// QpxIsegVHSPlugin 12 channel common ground VME

  return (deviceClass == V12C0);
}

//=============================================================================
// Special Commands
//=============================================================================

void QpxIsegVHSPlugin::programBaseAddress(uint16_t address)
{
  writeShort(m_baseAddress + VhsNewBaseAddressOFFSET, address);
  writeShort(m_baseAddress + VhsNewBaseAddressXorOFFSET, address ^ 0xFFFF);
}

uint16_t QpxIsegVHSPlugin::verifyBaseAddress(void) const
{
  uint16_t newAddress = -1;
  uint16_t temp;

  newAddress = readShort(m_baseAddress + VhsNewBaseAddressAcceptedOFFSET);
  temp = readShort(m_baseAddress + VhsNewBaseAddressOFFSET);
  if (newAddress != temp) {
    newAddress = -1;
  }

  return newAddress;
}


}
