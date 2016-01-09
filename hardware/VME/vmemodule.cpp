#include "vmemodule.h"
#include "boost/endian/conversion.hpp"
#include "custom_logger.h"

typedef union {
	long    l;
	float   f;
  uint16_t  w[2];
  uint8_t   b[4];
} TFloatWord;

VmeModule::VmeModule(BaseController *controller, int baseAddress)
{
	m_controller = static_cast<VmeController *>(controller);
  setBaseAddress(baseAddress);
}

VmeModule::~VmeModule()
{
}

bool VmeModule::connected()
{
  uint32_t vendorId = readLong(m_baseAddress + VhsVendorIdOFFSET);

	return vendorId == ISEG_VENDOR_ID;
}

bool VmeModule::read_setting(Gamma::Setting& set, int address_modifier) {
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

bool VmeModule::write_setting(Gamma::Setting& set, int address_modifier) {
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

uint16_t VmeModule::readShort(int address)
{
	if ((m_controller)) {
		return m_controller->readShort(address, VME_ADDRESS_MODIFIER);
	} else {
		return 0;
	}
}

void VmeModule::writeShort(int address, uint16_t data)
{
	if ((m_controller)) {
		m_controller->writeShort(address, VME_ADDRESS_MODIFIER, data);
	}
}

uint16_t VmeModule::readShortBitfield(int address)
{
	return readShort(address);
}

void VmeModule::writeShortBitfield(int address, uint16_t data)
{
	writeShort(address, data);
}

float VmeModule::readFloat(int address)
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

void VmeModule::writeFloat(int address, float data)
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

uint32_t VmeModule::readLong(int address)
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

void VmeModule::writeLong(int address, uint32_t data)
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

uint32_t VmeModule::readLongBitfield(int address)
{
	return readLong(address);
}

void VmeModule::writeLongBitfield(int address, uint32_t data)
{
	writeLong(address, data);
}

/**
 * Mirrors the Bit positions in a 16 bit word.
 */
uint16_t VmeModule::mirrorShort(uint16_t data)
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
uint32_t VmeModule::mirrorLong(uint32_t data)
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


std::string VmeModule::firmwareRelease(void)
{
  uint32_t version = readLong(m_baseAddress + VhsFirmwareReleaseOFFSET);

  return std::to_string(version >> 24) + std::to_string(version >> 16) + "." +
    std::to_string(version >>  8) + std::to_string(version >>  0);
}


void VmeModule::setBaseAddress(uint16_t baseAddress)
{
  m_firmwareName.clear();
	m_baseAddress = baseAddress;
  //m_vendorId    = readLong(m_baseAddress + VhsVendorIdOFFSET);

  uint32_t tmp = readLong(m_baseAddress + 60);

  int V12C0 = 20;		// IsegVHS 12 channel common ground VME

  if (tmp == V12C0)
    m_firmwareName = "V12C0";
  // re-scan of module here
}

//=============================================================================
// Special Commands
//=============================================================================

void VmeModule::programBaseAddress(uint16_t address)
{
  writeShort(m_baseAddress + VhsNewBaseAddressOFFSET, address);
  writeShort(m_baseAddress + VhsNewBaseAddressXorOFFSET, address ^ 0xFFFF);
}

uint16_t VmeModule::verifyBaseAddress(void)
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



