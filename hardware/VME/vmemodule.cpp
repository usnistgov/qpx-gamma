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

	m_deviceClass = 0;
	m_baseAddress = baseAddress;
  m_vendorId    = readLong(m_baseAddress + VhsVendorIdOFFSET);

  //PL_DBG << "device class load : " << deviceClass();

	m_channelNumber = -1;
}

VmeModule::~VmeModule()
{
	// TODO: close VME controller
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

int VmeModule::deviceClass(void)
{
  //PL_DBG << "vendorID " << m_vendorId;

  if (m_vendorId != ISEG_VENDOR_ID) { // iseg
    m_vendorId = readLong(m_baseAddress + VhsVendorIdOFFSET);
    //PL_DBG << "new vendorID " << m_vendorId;
  }

  uint32_t tmp = readLong(m_baseAddress + 60/*VhsDeviceClassOFFSET*/);
  m_deviceClass = (uint16_t)tmp;

  //PL_DBG << "VendorID=" << m_vendorId << " deviceClass=" << m_deviceClass;


	if (m_deviceClass == V12C0) {
    //PL_DBG << "loading V12C0";

    m_firmwareName = "V12C0";
		m_channelNumber = 0;
		for (int chn = 0; chn < 16; ++chn)
      if (tmp & (1 << ((uint32_t)chn + 16)))
				++m_channelNumber;

		m_channelOffset[0] = VhsChannel00OFFSET;
		m_channelOffset[1] = VhsChannel01OFFSET;
		m_channelOffset[2] = VhsChannel02OFFSET;
		m_channelOffset[3] = VhsChannel03OFFSET;
		m_channelOffset[4] = VhsChannel04OFFSET;
		m_channelOffset[5] = VhsChannel05OFFSET;
		m_channelOffset[6] = VhsChannel06OFFSET;
		m_channelOffset[7] = VhsChannel07OFFSET;
		m_channelOffset[8] = VhsChannel08OFFSET;
		m_channelOffset[9] = VhsChannel09OFFSET;
		m_channelOffset[10] = VhsChannel10OFFSET;
		m_channelOffset[11] = VhsChannel11OFFSET;

		m_flexGroupOffset[0] = VhsFlexGroup00OFFSET;
		m_flexGroupOffset[1] = VhsFlexGroup01OFFSET;
		m_flexGroupOffset[2] = VhsFlexGroup02OFFSET;
		m_flexGroupOffset[3] = VhsFlexGroup03OFFSET;
		m_flexGroupOffset[4] = VhsFlexGroup04OFFSET;
		m_flexGroupOffset[5] = VhsFlexGroup05OFFSET;
		m_flexGroupOffset[6] = VhsFlexGroup06OFFSET;
		m_flexGroupOffset[7] = VhsFlexGroup07OFFSET;
		m_flexGroupOffset[8] = VhsFlexGroup08OFFSET;
		m_flexGroupOffset[9] = VhsFlexGroup09OFFSET;
		m_flexGroupOffset[10] = VhsFlexGroup10OFFSET;
		m_flexGroupOffset[11] = VhsFlexGroup11OFFSET;
		m_flexGroupOffset[12] = VhsFlexGroup12OFFSET;
		m_flexGroupOffset[13] = VhsFlexGroup13OFFSET;
		m_flexGroupOffset[14] = VhsFlexGroup14OFFSET;
		m_flexGroupOffset[15] = VhsFlexGroup15OFFSET;
		m_flexGroupOffset[16] = VhsFlexGroup16OFFSET;
		m_flexGroupOffset[17] = VhsFlexGroup17OFFSET;
		m_flexGroupOffset[18] = VhsFlexGroup18OFFSET;
		m_flexGroupOffset[19] = VhsFlexGroup19OFFSET;
		m_flexGroupOffset[20] = VhsFlexGroup20OFFSET;
		m_flexGroupOffset[21] = VhsFlexGroup21OFFSET;
		m_flexGroupOffset[22] = VhsFlexGroup22OFFSET;
		m_flexGroupOffset[23] = VhsFlexGroup23OFFSET;
		m_flexGroupOffset[24] = VhsFlexGroup24OFFSET;
		m_flexGroupOffset[25] = VhsFlexGroup25OFFSET;
		m_flexGroupOffset[26] = VhsFlexGroup26OFFSET;
		m_flexGroupOffset[27] = VhsFlexGroup27OFFSET;
		m_flexGroupOffset[28] = VhsFlexGroup28OFFSET;
		m_flexGroupOffset[29] = VhsFlexGroup29OFFSET;
		m_flexGroupOffset[30] = VhsFlexGroup30OFFSET;
		m_flexGroupOffset[31] = VhsFlexGroup31OFFSET;

		m_voltageMaxSetOffset[0] = VhsVoltageMaxSet00OFFSET;
		m_voltageMaxSetOffset[1] = VhsVoltageMaxSet01OFFSET;
		m_voltageMaxSetOffset[2] = VhsVoltageMaxSet02OFFSET;
		m_voltageMaxSetOffset[3] = VhsVoltageMaxSet03OFFSET;
		m_voltageMaxSetOffset[4] = VhsVoltageMaxSet04OFFSET;
		m_voltageMaxSetOffset[5] = VhsVoltageMaxSet05OFFSET;
		m_voltageMaxSetOffset[6] = VhsVoltageMaxSet06OFFSET;
		m_voltageMaxSetOffset[7] = VhsVoltageMaxSet07OFFSET;
		m_voltageMaxSetOffset[8] = VhsVoltageMaxSet08OFFSET;
		m_voltageMaxSetOffset[9] = VhsVoltageMaxSet09OFFSET;
		m_voltageMaxSetOffset[10] = VhsVoltageMaxSet10OFFSET;
		m_voltageMaxSetOffset[11] = VhsVoltageMaxSet11OFFSET;

		m_currentMaxSetOffset[0] = VhsCurrentMaxSet00OFFSET;
		m_currentMaxSetOffset[1] = VhsCurrentMaxSet01OFFSET;
		m_currentMaxSetOffset[2] = VhsCurrentMaxSet02OFFSET;
		m_currentMaxSetOffset[3] = VhsCurrentMaxSet03OFFSET;
		m_currentMaxSetOffset[4] = VhsCurrentMaxSet04OFFSET;
		m_currentMaxSetOffset[5] = VhsCurrentMaxSet05OFFSET;
		m_currentMaxSetOffset[6] = VhsCurrentMaxSet06OFFSET;
		m_currentMaxSetOffset[7] = VhsCurrentMaxSet07OFFSET;
		m_currentMaxSetOffset[8] = VhsCurrentMaxSet08OFFSET;
		m_currentMaxSetOffset[9] = VhsCurrentMaxSet09OFFSET;
		m_currentMaxSetOffset[10] = VhsCurrentMaxSet10OFFSET;
		m_currentMaxSetOffset[11] = VhsCurrentMaxSet11OFFSET;

  }

	return m_deviceClass;
}

int VmeModule::serialNumber(void)
{
	if (m_deviceClass == V12C0) {
		return readLong(m_baseAddress + VhsSerialNumberOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

std::string VmeModule::firmwareRelease(void)
{
  uint32_t version = 0;

	if (m_deviceClass == V12C0) {
    version = readLong(m_baseAddress + VhsFirmwareReleaseOFFSET);
	} else {
		deviceClass();
	}

  return std::to_string(version >> 24) + std::to_string(version >> 16) + "." +
    std::to_string(version >>  8) + std::to_string(version >>  0);
}

uint32_t VmeModule::decoderRelease(void)
{
	if (m_deviceClass == V12C0) {
		return 0;
	} else {
		deviceClass();
		return 0;
	}
}

float VmeModule::temperature(int index)
{
	if (index != 0)
		return 0.0;

	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsTemperatureOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

float VmeModule::supplyP5(void)
{
	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsSupplyP5OFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

float VmeModule::supplyP12(void)
{
	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsSupplyP12OFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

float VmeModule::supplyN12(void)
{
	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsSupplyN12OFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

uint32_t VmeModule::adcSamplesPerSecond(void)
{
	if (m_deviceClass == V12C0) {
		return readShort(m_baseAddress + VhsADCSamplesPerSecondOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setAdcSamplesPerSecond(uint32_t sps)
{
	if (m_deviceClass == V12C0) {
		return writeShort(m_baseAddress + VhsADCSamplesPerSecondOFFSET, sps);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::digitalFilter(void)
{
	if (m_deviceClass == V12C0) {
		return readShort(m_baseAddress + VhsDigitalFilterOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setDigitalFilter(uint32_t filter)
{
	if (m_deviceClass == V12C0) {
		return writeShort(m_baseAddress + VhsDigitalFilterOFFSET, filter);
	} else {
		deviceClass();
	}
}

float VmeModule::voltageLimit(void)
{
	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsVoltageMaxOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

float VmeModule::currentLimit(void)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + VhsCurrentMaxOFFSET);
	} else {
		deviceClass();
		return 0;
	}

	return result;
}

uint32_t VmeModule::status(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleStatusOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

uint32_t VmeModule::eventStatus(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleEventStatusOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setEventStatus(uint32_t status)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + VhsModuleEventStatusOFFSET, status);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::eventMask(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleEventMaskOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setEventMask(uint32_t mask)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + VhsModuleEventMaskOFFSET, mask);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::control(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleControlOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setControl(uint32_t control)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + VhsModuleControlOFFSET, control);
	} else {
		deviceClass();
	}
}

float VmeModule::voltageRampSpeed(void)
{
	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsVoltageRampSpeedOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setVoltageRampSpeed(float vramp)
{
	if (m_deviceClass == V12C0) {
		return writeFloat(m_baseAddress + VhsVoltageRampSpeedOFFSET, vramp);
	} else {
		deviceClass();
	}
}

float VmeModule::currentRampSpeed(void)
{
	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsCurrentRampSpeedOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setCurrentRampSpeed(float iramp)
{
	if (m_deviceClass == V12C0) {
		return writeFloat(m_baseAddress + VhsCurrentRampSpeedOFFSET, iramp);
	} else {
		deviceClass();
	}
}

bool VmeModule::killEnable(void)
{
	ModuleControlSTRUCT result;

	result.Word = control();
	return result.Bit.SetKillEnable;
}

void VmeModule::setKillEnable(bool enable)
{
	ModuleControlSTRUCT result;

	result.Word = control();
	result.Bit.SetKillEnable = enable;

	setControl(result.Word);
}

void VmeModule::setSelectedChannelsOn(uint32_t mask)
{
  if (mask == static_cast<uint32_t>(-1)) {
		if (m_deviceClass == V12C0) {
			writeLong(m_baseAddress + VhsSetOnOffAllChannelsOFFSET, 1);
		} else {
			deviceClass();
		}
	} else {
		for (int chn = 0; chn < 32; ++chn)
			if (mask & (1 << chn))
				setChannelOn(chn);
	}
}

void VmeModule::setSelectedChannelsOff(uint32_t mask)
{
  if (mask == static_cast<uint32_t>(-1)) {
		if (m_deviceClass == V12C0) {
			writeLong(m_baseAddress + VhsSetOnOffAllChannelsOFFSET, 0);
		} else {
			deviceClass();
		}
	} else {
		for (int chn = 0; chn < 32; ++chn)
			if (mask & (1 << chn))
				setChannelOff(chn);
	}
}

void VmeModule::setAllChannelsVoltageSet(float vset)
{
	vset = userVoltageToSiVoltage(vset);

	if (m_deviceClass == V12C0) {
		writeFloat(m_baseAddress + VhsSetVoltageAllChannelsOFFSET, vset);
	} else {
		deviceClass();
	}
}

void VmeModule::setAllChannelsCurrentSet(float iset)
{
	iset = userCurrentToSiCurrent(iset);

	if (m_deviceClass == V12C0) {
		writeFloat(m_baseAddress + VhsSetCurrentAllChannelsOFFSET, iset);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::restartTime(void)
{
	if (m_deviceClass == V12C0) {
		return readShort(m_baseAddress + VhsRestartTimeOFFSET);
	} else {
		deviceClass(); return 0;
	}
}

void VmeModule::setRestartTime(uint32_t restartTime)
{
	if (m_deviceClass == V12C0) {
		writeShort(m_baseAddress + VhsRestartTimeOFFSET, restartTime);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::eventChannelStatus(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleEventChannelStatusOFFSET);
	} else {
		deviceClass(); return 0;
	}
}

void VmeModule::clearEventChannelStatus(void)
{
	if (m_deviceClass == V12C0) {
		writeShort(m_baseAddress + VhsModuleEventChannelStatusOFFSET, -1);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::eventChannelMask(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleEventChannelMaskOFFSET);
	} else {
		deviceClass(); return 0;
	}
}

void VmeModule::setEventChannelMask(uint32_t mask)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + VhsModuleEventChannelMaskOFFSET, mask);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::eventGroupStatus(void)
{
	if (m_deviceClass == V12C0) {
		return readLongBitfield(m_baseAddress + VhsModuleEventGroupStatusOFFSET);
	} else {
		deviceClass(); return 0;
	}
}

uint32_t VmeModule::eventGroupMask(void)
{
	if (m_deviceClass == V12C0) {
		return readLongBitfield(m_baseAddress + VhsModuleEventGroupMaskOFFSET);
	} else {
		deviceClass(); return 0;
	}
}

void VmeModule::setEventGroupMask(uint32_t mask)
{
	if (m_deviceClass == V12C0) {
		writeLongBitfield(m_baseAddress + VhsModuleEventGroupMaskOFFSET, mask);
	} else {
		deviceClass();
	}
}

void VmeModule::clearEventGroupStatus(void)
{
	if (m_deviceClass == V12C0) {
		writeLong(m_baseAddress + VhsModuleEventGroupStatusOFFSET, -1);
	} else {
		deviceClass();
	}
}

void VmeModule::doClear(void)
{
	ModuleControlSTRUCT result;

	result.Word = control();
	result.Bit.DoClear = 1;
	setControl(result.Word);
}

void VmeModule::setBaseAddress(uint16_t baseAddress)
{
	m_baseAddress = baseAddress;
	m_vendorId    = 0; // force re-scan of module at next access
}

//=============================================================================
// Special Commands
//=============================================================================

void VmeModule::setSpecialControlCommand(uint32_t command)
{
	if (m_deviceClass == V12C0) {
		writeShort(m_baseAddress + VhsSpecialControlCommandOFFSET, command);
	} else {
		deviceClass();
	}

	if (
		(command == SPECIALCONTROL_COMMAND_ENDSPECIAL) ||
		(command == SPECIALCONTROL_COMMAND_RESTART)
	) {
		m_vendorId = 0; // Force re-scan of all module data
	}

}

uint32_t VmeModule::specialControlCommand(void)
{
	if (m_deviceClass == V12C0) {
		return readShort(m_baseAddress + VhsSpecialControlCommandOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

uint32_t VmeModule::specialControlStatus(void)
{
	if (m_deviceClass == V12C0) {
		return readShort(m_baseAddress + VhsSpecialControlStatusOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::sendHexLine(std::vector<uint8_t> record)
{
  uint32_t val;

	if (m_deviceClass == V24D0) {
    while ((record.size() % 4) != 0)
      //record.push_back('0');
      record.push_back(0);

    for (int i = 0; i < record.size(); i += 4) {
			val = (record[i] << 24) | (record[i + 1] << 16) | (record[i + 2] << 8) | (record[i + 3]);
			writeLong(m_baseAddress + m_channelOffset[0] + i, val);
		}

		//writeLong(BaseAddress + VdsSpecialControlCommandOFFSET, SPECIALCONTROL_COMMAND_DATASENT);
	} else if (m_deviceClass == V12C0) {
    for (int i = 0; i < record.size(); i += 2) {
			val = (record[i] << 8) | (record[i + 1]);
			writeShort(m_baseAddress + m_channelOffset[0] + i, val);
		}

		writeShort(m_baseAddress + VhsSpecialControlCommandOFFSET, SPECIALCONTROL_COMMAND_DATASENT);
	} else {
		deviceClass();
	}

	//qDebug() << "Data sent:" << record << "\n";
}

void VmeModule::programBaseAddress(uint16_t address)
{
	if (m_deviceClass == V12C0) {
		writeShort(m_baseAddress + VhsNewBaseAddressOFFSET, address);
		writeShort(m_baseAddress + VhsNewBaseAddressXorOFFSET, address ^ 0xFFFF);
	} else {
		deviceClass();
	}
}

uint16_t VmeModule::verifyBaseAddress(void)
{
  uint16_t newAddress = -1;
  uint16_t temp;

	if (m_deviceClass == V12C0) {
		newAddress = readShort(m_baseAddress + VhsNewBaseAddressAcceptedOFFSET);
		temp = readShort(m_baseAddress + VhsNewBaseAddressOFFSET);
		if (newAddress != temp) {
			newAddress = -1;
		}
	} else {
		deviceClass();
	}

	return newAddress;
}

//=============================================================================
// Channel Commands
//=============================================================================

uint32_t VmeModule::channelStatus(int channel)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelStatusOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

uint32_t VmeModule::channelEventStatus(int channel)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelEventStatusOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setChannelEventStatus(int channel, uint32_t status)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelEventStatusOFFSET, status);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::channelEventMask(int channel)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelEventMaskOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setChannelEventMask(int channel, uint32_t mask)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelEventMaskOFFSET, mask);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::channelControl(int channel)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelControlOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setChannelControl(int channel, uint32_t control)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelControlOFFSET, control);
	} else {
		deviceClass();
	}
}

float VmeModule::channelVoltageNominal(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelVoltageNominalOFFSET);
	} else {
		deviceClass();
		return 0;
	}

	return siVoltageToUserVoltage(result);
}

float VmeModule::channelCurrentNominal(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelCurrentNominalOFFSET);
	} else {
		deviceClass();
		return 0;
	}

	return siCurrentToUserCurrent(result);
}

float VmeModule::channelVoltageSet(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelVoltageSetOFFSET);
	} else {
		deviceClass();
		return 0;
	}

	return siVoltageToUserVoltage(result);
}

void VmeModule::setChannelVoltageSet(int channel, float vset)
{
	vset = userVoltageToSiVoltage(vset);

	if (m_deviceClass == V12C0) {
		writeFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelVoltageSetOFFSET, vset);
	} else {
		deviceClass();
	}
}

float VmeModule::channelVoltageMeasure(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelVoltageMeasureOFFSET);
	} else {
		deviceClass();
		return 0;
	}

	return siVoltageToUserVoltage(result);
}

float VmeModule::channelCurrentSet(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelCurrentSetOFFSET);
	} else {
		deviceClass();
		return 0;
	}

	return siCurrentToUserCurrent(result);
}

void VmeModule::setChannelCurrentSet(int channel, float iset)
{
	iset = userCurrentToSiCurrent(iset);

	if (m_deviceClass == V12C0) {
		writeFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelCurrentSetOFFSET, iset);
	} else {
		deviceClass();
	}
}

float VmeModule::channelCurrentMeasure(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelCurrentMeasureOFFSET);
	} else {
		deviceClass();
		return 0;
	}

	return siCurrentToUserCurrent(result);
}

bool VmeModule::channelDelayedTripEnabled(int channel)
{
	GroupTimeout1PropertySTRUCT property;
	property.Word = flexGroupType(channel);

	return (property.Bit.GroupType == GROUP_TYPE_TIMEOUT);
}

bool VmeModule::channelDelayedTripRampDown(int channel)
{
	GroupTimeout1PropertySTRUCT property;
	property.Word = flexGroupType(channel);

	return (property.Bit.Action == GROUP_ACTION_RAMP_DOWN);
}

int VmeModule::channelDelayedTripTime(int channel)
{
	GroupTimeout1PropertySTRUCT property;
	property.Word = flexGroupType(channel);

	if (property.Bit.GroupType == GROUP_TYPE_TIMEOUT)
		return property.Bit.TimeoutTime;
	else
		return -1;
}

void VmeModule::setChannelDelayedTripAction(int channel, OutputSupervisionAction action, int delay)
{
	int member = 0;
	GroupTimeout1PropertySTRUCT property = { 0 };

	if (action != IgnoreFailure) {
		if (action == RampDownChannel)
			property.Bit.Action = GROUP_ACTION_RAMP_DOWN;
		else if (action == ShutDownChannel)
			property.Bit.Action = GROUP_ACTION_SHUT_GROUP;
		else if (action == ShutDownModule)
			property.Bit.Action = GROUP_ACTION_SHUT_MODULE;

		property.Bit.GroupType = GROUP_TYPE_TIMEOUT;
		property.Bit.TimeoutTime = delay;
		member = (1 << channel);
	}

	setFlexGroup(channel, member, property.Word);
}

float VmeModule::channelVoltageInterlockMaxSet(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelVoltageIlkMaxSetOFFSET);
	} else {
		deviceClass();
		return 0;
	}

	return siVoltageToUserVoltage(result);
}

void VmeModule::setChannelVoltageInterlockMaxSet(int channel, float ilkmax)
{
	ilkmax = userVoltageToSiVoltage(ilkmax);

	if (m_deviceClass == V12C0) {
		writeFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelVoltageIlkMaxSetOFFSET, ilkmax);
	} else {
		deviceClass();
	}
}

float VmeModule::channelVoltageInterlockMinSet(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelVoltageIlkMinSetOFFSET);
	} else {
		deviceClass();
		return 0;
	}

	return siVoltageToUserVoltage(result);
}

void VmeModule::setChannelVoltageInterlockMinSet(int channel, float ilkmin)
{
	ilkmin = userVoltageToSiVoltage(ilkmin);

	if (m_deviceClass == V12C0) {
		writeFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelVoltageIlkMinSetOFFSET, ilkmin);
	} else {
		deviceClass();
	}
}

float VmeModule::channelCurrentInterlockMaxSet(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelCurrentIlkMaxSetOFFSET);
	} else {
		deviceClass();
		return 0;
	}

	return siCurrentToUserCurrent(result);
}

void VmeModule::setChannelCurrentInterlockMaxSet(int channel, float ilkmax)
{
	ilkmax = userCurrentToSiCurrent(ilkmax);

	if (m_deviceClass == V12C0) {
		writeFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelCurrentIlkMaxSetOFFSET, ilkmax);
	} else {
		deviceClass();
	}
}

float VmeModule::channelCurrentInterlockMinSet(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelCurrentIlkMinSetOFFSET);
	} else {
		deviceClass();
		return 0;
	}

	return siCurrentToUserCurrent(result);
}

void VmeModule::setChannelCurrentInterlockMinSet(int channel, float ilkmin)
{
	ilkmin = userCurrentToSiCurrent(ilkmin);

	if (m_deviceClass == V12C0) {
		writeFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelCurrentIlkMinSetOFFSET, ilkmin);
	} else {
		deviceClass();
	}
}

float VmeModule::siVoltageToUserVoltage(float value)
{
	if (m_voltageUnit == KiloVolt)
		value *= 1E-3f;

	return value;
}

float VmeModule::userVoltageToSiVoltage(float value)
{
	if (m_voltageUnit == KiloVolt)
		value *= 1E3f;

	return value;
}

float VmeModule::siCurrentToUserCurrent(float siCurrent)
{
	if (m_currentUnit == NanoAmpere) {
		siCurrent *= 1E9f;
	} else if (m_currentUnit == MicroAmpere) {
		siCurrent *= 1E6f;
	} else if (m_currentUnit == MilliAmpere) {
		siCurrent *= 1E3f;
	}

	return siCurrent;
}

float VmeModule::userCurrentToSiCurrent(float userCurrent)
{
	if (m_currentUnit == NanoAmpere) {
		userCurrent *= 1E-9f;
	} else if (m_currentUnit == MicroAmpere) {
		userCurrent *= 1E-6f;
	} else if (m_currentUnit == MilliAmpere) {
		userCurrent *= 1E-3f;
	}

	return userCurrent;
}

void VmeModule::setSelectedChannelsEmergencyOff(uint32_t mask)
{
  if (mask == static_cast<uint32_t>(-1)) {
		if (m_deviceClass == V12C0) {
			writeLong(m_baseAddress + VhsSetEmergencyAllChannelsOFFSET, 1);
		} else {
			deviceClass();
		}
	} else {
		for (int chn = 0; chn < 32; ++chn)
			if (mask & (1 << chn))
				setChannelEmergencyOff(chn);
	}
}

void VmeModule::clearSelectedChannelsEmergencyOff(uint32_t mask)
{
  if (mask == static_cast<uint32_t>(-1)) {
		if (m_deviceClass == V12C0) {
			writeLong(m_baseAddress + VhsSetEmergencyAllChannelsOFFSET, 0);
		} else {
			deviceClass();
		}
	} else {
		for (int chn = 0; chn < 32; ++chn)
			if (mask & (1 << chn))
				clearChannelEmergencyOff(chn);
	}
}

void VmeModule::setChannelOn(int channel)
{
	ChControlSTRUCT control;

	control.Word = channelControl(channel);
	control.Bit.setON = 1;
	setChannelControl(channel, control.Word);
}

void VmeModule::setChannelOff(int channel)
{
	ChControlSTRUCT control;

	control.Word = channelControl(channel);
	control.Bit.setON = 0;
	setChannelControl(channel, control.Word);
}

void VmeModule::setChannelEmergencyOff(int channel)
{
	ChControlSTRUCT control;

	control.Word = channelControl(channel);
	control.Bit.setEmergency = 1;
	setChannelControl(channel, control.Word);
}

void VmeModule::clearChannelEmergencyOff(int channel)
{
	ChControlSTRUCT control;

	control.Word = channelControl(channel);
	control.Bit.setEmergency = 0;
	setChannelControl(channel, control.Word);
}

int VmeModule::channelNumber(void)
{
	return m_channelNumber;
}

uint32_t VmeModule::GetModuleIlkOutStatus(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleIlkOutStatusOFFSET);
	} else if (m_deviceClass == V24D0) {
		return 0; // FIXME
	} else {
		deviceClass(); return 0;
	}
}

uint32_t VmeModule::GetModuleIlkOutControl(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleIlkOutControlOFFSET);
	} else if (m_deviceClass == V24D0) {
		return 0; /// FIXME
	} else {
		deviceClass(); return 0;
	}
}

void VmeModule::SetModuleIlkOutControl(uint32_t control)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + VhsModuleIlkOutControlOFFSET, control);
	} else if (m_deviceClass == V24D0) {
		/// FIXME
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::GetModuleIlkOutCount(void)
{
	if (m_deviceClass == V12C0) {
		return readShort(m_baseAddress + VhsModuleIlkCountOFFSET);
	} else if (m_deviceClass == V24D0) {
		return 0; // FIXME
	} else {
		deviceClass(); return 0;
	}
}

uint32_t VmeModule::GetModuleIlkOutLastTrigger(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleIlkLastTriggerOFFSET);
	} else if (m_deviceClass == V24D0) {
		return 0; // FIXME
	} else {
		deviceClass(); return 0;
	}
}

uint32_t VmeModule::GetModuleIlkOutChnActualActive(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleIlkChnActualActiveOFFSET);
	} else if (m_deviceClass == V24D0) {
		return 0; // FIXME
	} else {
		deviceClass(); return 0;
	}
}

uint32_t VmeModule::GetModuleIlkOutChnEverTriggered(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleIlkChnEverTriggeredOFFSET);
	} else if (m_deviceClass == V24D0) {
		return 0; // FIXME
	} else {
		deviceClass(); return 0;
	}
}

uint32_t VmeModule::flexGroupMemberList(int group)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + m_flexGroupOffset[group] + VhsFlexGroupMemberOFFSET);
	} else {
		deviceClass(); return 0;
	}
}

uint32_t VmeModule::flexGroupType(int group)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + m_flexGroupOffset[group] + VhsFlexGroupTypeOFFSET);
	} else {
		deviceClass(); return 0;
	}
}

void VmeModule::setFlexGroup(int group, uint32_t member, uint32_t type)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + m_flexGroupOffset[group] + VhsFlexGroupMemberOFFSET, member);
		writeShortBitfield(m_baseAddress + m_flexGroupOffset[group] + VhsFlexGroupTypeOFFSET, type);
	} else {
		deviceClass();
	}
}

float VmeModule::channelVoltageHardwareNominal(int channel)
{
	if ( (m_deviceClass == V12C0) || (m_deviceClass == V24D0) ) {
		return readFloat(m_baseAddress + m_voltageMaxSetOffset[channel]);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setChannelVoltageNominal(int channel, float maxset)
{
	if (m_deviceClass == V12C0) {
		return writeFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelVoltageNominalOFFSET, maxset);
	} else {
		deviceClass();
	}
}

float VmeModule::channelCurrentHardwareNominal(int channel)
{
	if ( (m_deviceClass == V12C0) || (m_deviceClass == V24D0) ) {
		return siCurrentToUserCurrent(readFloat(m_baseAddress + m_currentMaxSetOffset[channel]));
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setChannelCurrentNominal(int channel, float maxset)
{
	maxset = userCurrentToSiCurrent(maxset);

	if (m_deviceClass == V12C0) {
		return writeFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelCurrentNominalOFFSET, maxset);
	} else {
		deviceClass();
	}
}
