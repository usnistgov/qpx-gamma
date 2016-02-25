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

#include "IsegVHS_module.h"
#include "vmecontroller.h"
#include "custom_logger.h"

#define ISEG_VENDOR_ID        0x69736567
#define ISEG_DEVCLASS_V12C0   20          // QpxIsegVHSPlugin 12 channel common ground VME

// --- VHS12 ------------------------------------------------------------------
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
  if (set.id_ != device_name())
    return false;

  double voltage_max = 0;
  double current_max = 0;
  for (auto &k : set.branches.my_data_) {
    if (k.metadata.setting_type != Gamma::SettingType::stem) {
      if (!read_setting(k)) {}
      //          PL_DBG << "Could not read " << k.id_;
      if (k.id_ == "VME/IsegVHS/VoltageMax") {
        voltage_max = k.value_dbl;
        //                PL_DBG << "voltage max = " << voltage_max;
      }
      if (k.id_ == "VME/IsegVHS/CurrentMax") {
        current_max = k.value_dbl;
        //                PL_DBG << "current max = " << current_max;
      }
    } else {
      for (auto &p : k.branches.my_data_) {
        if (p.metadata.setting_type != Gamma::SettingType::stem) {
          if (!read_setting(p)) {}
          //              PL_DBG << "Could not read " << p.id_;
        } else {
          for (auto &q : p.branches.my_data_) {
            if (q.metadata.setting_type != Gamma::SettingType::stem) {
              if (!read_setting(q)) {}
              //              PL_DBG << "Could not read " << p.id_;
            }
          }

          for (auto &q : p.branches.my_data_) {
            if (q.id_ == "VME/IsegVHS/Channel/Status") {
              Gamma::Setting status = p.get_setting(Gamma::Setting("VME/IsegVHS/Channel/ChannelStatus"), Gamma::Match::id);
              if (status.value_int & 0x0010)
                q.value_int = 1;
              else if (status.value_int & 0x0008)
                q.value_int = 2;
              else
                q.value_int = 0;
            } else if (q.id_ == "VME/IsegVHS/Channel/VoltageSet") {
              Gamma::Setting nominal = p.get_setting(Gamma::Setting("VME/IsegVHS/Channel/VoltageNominalMaxSet"), Gamma::Match::id);
              q.metadata.maximum = nominal.value_dbl;// * voltage_max;
              //                  PL_DBG << "Volatge bounds "  << nominal.value_dbl << " " << p.metadata.maximum;
            } else if (q.id_ == "VME/IsegVHS/Channel/CurrentSetOrTrip") {
              Gamma::Setting nominal = p.get_setting(Gamma::Setting("VME/IsegVHS/Channel/CurrentNominalMaxSet"), Gamma::Match::id);
              q.metadata.maximum = nominal.value_dbl;// * current_max;
              //                  PL_DBG << "Current bounds "  << nominal.value_dbl << " " << p.metadata.maximum;
            }
          }
        }
      }

    }
  }

  return true;
}

void QpxIsegVHSPlugin::rebuild_structure(Gamma::Setting &set) {
  for (auto &k : set.branches.my_data_) {
    if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/IsegVHS/Channels")) {

      Gamma::Setting temp("VME/IsegVHS/Channel");
      temp.enrich(setting_definitions_, true);

      while (k.branches.size() < 12)
        k.branches.my_data_.push_back(temp);
      while (k.branches.size() > 12)
        k.branches.my_data_.pop_back();

      uint32_t offset = k.metadata.address;

      std::set<int32_t> indices;
      int address = 0;
      for (auto &p : k.branches.my_data_) {
        if (p.id_ != temp.id_) {
          temp.indices = p.indices;
          p = temp;
        }

        p.metadata.name = "Channel " + std::to_string(address);
        p.metadata.address = offset + address * 48;

        for (auto &i : p.indices)
          indices.insert(i);

        for (auto &q : p.branches.my_data_) {
          q.enrich(setting_definitions_);
          q.metadata.address = q.metadata.address + p.metadata.address;
        }

        ++address;
      }
    }
  }
}


bool QpxIsegVHSPlugin::write_settings_bulk(Gamma::Setting &set) {
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
        if (p.metadata.setting_type != Gamma::SettingType::stem) {
          Gamma::Setting s = p;
          if (p.metadata.writable && read_setting(s) && (s != p)) {
            //              PL_DBG << "writing " << p.id_;
            if (!write_setting(p)) {}
            //                PL_DBG << "Could not write " << k.id_;
          }
        } else {
          for (auto &q : p.branches.my_data_) {
              if ((q.metadata.setting_type == Gamma::SettingType::command) && (q.value_int != 0)) {
              if (q.id_ == "VME/IsegVHS/Channel/OnOff") {
                q.value_int = 0;
                Gamma::Setting ctrl = p.get_setting(Gamma::Setting("VME/IsegVHS/Channel/ChannelControl"), Gamma::Match::id);
                ctrl.value_int  ^= 0x0008;
                if (!write_setting(ctrl)) {}
              }
            }
          }


          for (auto &q : p.branches.my_data_) {
            if (q.metadata.setting_type != Gamma::SettingType::stem) {
              Gamma::Setting s = q;
              if (q.metadata.writable && read_setting(s) && (s != q)) {
                //              PL_DBG << "writing " << p.id_;
                if (!write_setting(q)) {}
                //                PL_DBG << "Could not write " << k.id_;
              }
            }
          }
        }
      }
    }
  }

  return true;
}


void QpxIsegVHSPlugin::get_all_settings() {
}

bool QpxIsegVHSPlugin::read_setting(Gamma::Setting& set) const {
  if (set.metadata.setting_type == Gamma::SettingType::command)
    set.metadata.writable =  ((status_ & DeviceStatus::booted) != 0);

  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if (set.metadata.address < 0)
    return false;

  if (set.metadata.setting_type == Gamma::SettingType::floating) {
    set.value_dbl = readFloat(m_baseAddress + set.metadata.address);
    return true;
  }
  else if ((set.metadata.setting_type == Gamma::SettingType::binary)
            || (set.metadata.setting_type == Gamma::SettingType::integer)
            || (set.metadata.setting_type == Gamma::SettingType::boolean)
             || (set.metadata.setting_type == Gamma::SettingType::int_menu)) {
    if (set.metadata.flags.count("u32"))
      set.value_int = readLong(m_baseAddress + set.metadata.address);
    else if (set.metadata.flags.count("u16"))
      set.value_int = readShort(m_baseAddress + set.metadata.address);
    else {
      PL_DBG << "<IsegVHS> Setting " << set.id_ << " does not have a well defined hardware type";
      return false;
    }
    return true;
  }
  return false;
}

bool QpxIsegVHSPlugin::write_setting(Gamma::Setting& set) {
  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if (set.metadata.address < 0)
    return false;

  if (set.metadata.setting_type == Gamma::SettingType::floating) {
    writeFloat(m_baseAddress + set.metadata.address, set.value_dbl);
    return true;
  } else if ((set.metadata.setting_type == Gamma::SettingType::binary)
           || (set.metadata.setting_type == Gamma::SettingType::integer)
           || (set.metadata.setting_type == Gamma::SettingType::boolean)
           || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    if (set.metadata.flags.count("u32"))
      writeLong(m_baseAddress + set.metadata.address, set.value_int);
    else if (set.metadata.flags.count("u16"))
      writeShort(m_baseAddress + set.metadata.address, set.value_int);
    else {
      PL_DBG << "<IsegVHS> Setting " << set.id_ << " does not have a well defined hardware type";
      return false;
    }
    return true;
  }
  return false;
}

uint16_t QpxIsegVHSPlugin::readShort(int address) const
{
  if (m_controller) {
    return m_controller->read16(address, AddressModifier::A16_User);
  } else {
    return 0;
  }
}

void QpxIsegVHSPlugin::writeShort(int address, uint16_t data)
{
  if (m_controller) {
    m_controller->write16(address, AddressModifier::A16_User, data);
  }
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

//=============================================================================
// Special Commands
//=============================================================================

void QpxIsegVHSPlugin::programBaseAddress(uint32_t address)
{
  writeShort(m_baseAddress + VhsNewBaseAddressOFFSET, address);
  writeShort(m_baseAddress + VhsNewBaseAddressXorOFFSET, address ^ 0xFFFF);
}

uint32_t QpxIsegVHSPlugin::verifyBaseAddress(void) const
{
  uint32_t newAddress = -1;
  uint16_t temp;

  newAddress = readShort(m_baseAddress + VhsNewBaseAddressAcceptedOFFSET);
  temp = readShort(m_baseAddress + VhsNewBaseAddressOFFSET);
  if (newAddress != temp) {
    newAddress = -1;
  }

  return newAddress;
}


//=============================================================================
// Module Commands
//=============================================================================

bool QpxIsegVHSPlugin::connected() const
{
  if (!m_controller)
    return false;

  Gamma::Setting vendID(this->device_name() + "/VendorID");
  vendID.enrich(setting_definitions_);
  read_setting(vendID);

  if (vendID.value_int != ISEG_VENDOR_ID)
    return false;

  Gamma::Setting devClass(this->device_name() + "/DeviceClass");
  devClass.enrich(setting_definitions_);
  read_setting(devClass);

  return (devClass.value_int == ISEG_DEVCLASS_V12C0);
}

std::string QpxIsegVHSPlugin::firmwareName() const
{
  if (!m_controller)
    return std::string();

  Gamma::Setting fwaddress(this->device_name() + "/FirmwareRelease");
  fwaddress.enrich(setting_definitions_);
  read_setting(fwaddress);

  uint32_t version = fwaddress.value_int;

  return std::to_string(version >> 24) + std::to_string(version >> 16) + "." +
    std::to_string(version >>  8) + std::to_string(version >>  0);
}


}
