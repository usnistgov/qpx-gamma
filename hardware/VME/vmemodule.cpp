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
	m_vendorId    = 0;
	m_baseAddress = baseAddress;

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

    if (m_vendorId != ISEG_VENDOR_ID) { // iseg
		m_vendorId = readLong(m_baseAddress + VhsVendorIdOFFSET);
	}

  uint32_t tmp = readLong(m_baseAddress + VdsChannelNumberOFFSET);
  m_deviceClass = (uint16_t)tmp;

  //PL_DBG << "VendorID=" << m_vendorId << " deviceClass=" << m_deviceClass;


	if (m_deviceClass == V12C0) {
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

	} else if (m_deviceClass == V24D0) {
    m_firmwareName = "V24D0";
		m_channelNumber = tmp >> 16;

		m_channelOffset[0] = VdsChannel00OFFSET;
		m_channelOffset[1] = VdsChannel01OFFSET;
		m_channelOffset[2] = VdsChannel02OFFSET;
		m_channelOffset[3] = VdsChannel03OFFSET;
		m_channelOffset[4] = VdsChannel04OFFSET;
		m_channelOffset[5] = VdsChannel05OFFSET;
		m_channelOffset[6] = VdsChannel06OFFSET;
		m_channelOffset[7] = VdsChannel07OFFSET;
		m_channelOffset[8] = VdsChannel08OFFSET;
		m_channelOffset[9] = VdsChannel09OFFSET;
		m_channelOffset[10] = VdsChannel10OFFSET;
		m_channelOffset[11] = VdsChannel11OFFSET;
		m_channelOffset[12] = VdsChannel12OFFSET;
		m_channelOffset[13] = VdsChannel13OFFSET;
		m_channelOffset[14] = VdsChannel14OFFSET;
		m_channelOffset[15] = VdsChannel15OFFSET;
		m_channelOffset[16] = VdsChannel16OFFSET;
		m_channelOffset[17] = VdsChannel17OFFSET;
		m_channelOffset[18] = VdsChannel18OFFSET;
		m_channelOffset[19] = VdsChannel19OFFSET;
		m_channelOffset[20] = VdsChannel20OFFSET;
		m_channelOffset[21] = VdsChannel21OFFSET;
		m_channelOffset[22] = VdsChannel22OFFSET;
		m_channelOffset[23] = VdsChannel23OFFSET;

		m_flexGroupOffset[0] = VdsFlexGroup00OFFSET;
		m_flexGroupOffset[1] = VdsFlexGroup01OFFSET;
		m_flexGroupOffset[2] = VdsFlexGroup02OFFSET;
		m_flexGroupOffset[3] = VdsFlexGroup03OFFSET;
		m_flexGroupOffset[4] = VdsFlexGroup04OFFSET;
		m_flexGroupOffset[5] = VdsFlexGroup05OFFSET;
		m_flexGroupOffset[6] = VdsFlexGroup06OFFSET;
		m_flexGroupOffset[7] = VdsFlexGroup07OFFSET;
		m_flexGroupOffset[8] = VdsFlexGroup08OFFSET;
		m_flexGroupOffset[9] = VdsFlexGroup09OFFSET;
		m_flexGroupOffset[10] = VdsFlexGroup10OFFSET;
		m_flexGroupOffset[11] = VdsFlexGroup11OFFSET;
		m_flexGroupOffset[12] = VdsFlexGroup12OFFSET;
		m_flexGroupOffset[13] = VdsFlexGroup13OFFSET;
		m_flexGroupOffset[14] = VdsFlexGroup14OFFSET;
		m_flexGroupOffset[15] = VdsFlexGroup15OFFSET;
		m_flexGroupOffset[16] = VdsFlexGroup16OFFSET;
		m_flexGroupOffset[17] = VdsFlexGroup17OFFSET;
		m_flexGroupOffset[18] = VdsFlexGroup18OFFSET;
		m_flexGroupOffset[19] = VdsFlexGroup19OFFSET;
		m_flexGroupOffset[20] = VdsFlexGroup20OFFSET;
		m_flexGroupOffset[21] = VdsFlexGroup21OFFSET;
		m_flexGroupOffset[22] = VdsFlexGroup22OFFSET;
		m_flexGroupOffset[23] = VdsFlexGroup23OFFSET;
		m_flexGroupOffset[24] = VdsFlexGroup24OFFSET;
		m_flexGroupOffset[25] = VdsFlexGroup25OFFSET;
		m_flexGroupOffset[26] = VdsFlexGroup26OFFSET;
		m_flexGroupOffset[27] = VdsFlexGroup27OFFSET;
		m_flexGroupOffset[28] = VdsFlexGroup28OFFSET;
		m_flexGroupOffset[29] = VdsFlexGroup29OFFSET;
		m_flexGroupOffset[30] = VdsFlexGroup30OFFSET;
		m_flexGroupOffset[31] = VdsFlexGroup31OFFSET;

		m_voltageMaxSetOffset[0] = VdsVoltageMaxSet00OFFSET;
		m_voltageMaxSetOffset[1] = VdsVoltageMaxSet01OFFSET;
		m_voltageMaxSetOffset[2] = VdsVoltageMaxSet02OFFSET;
		m_voltageMaxSetOffset[3] = VdsVoltageMaxSet03OFFSET;
		m_voltageMaxSetOffset[4] = VdsVoltageMaxSet04OFFSET;
		m_voltageMaxSetOffset[5] = VdsVoltageMaxSet05OFFSET;
		m_voltageMaxSetOffset[6] = VdsVoltageMaxSet06OFFSET;
		m_voltageMaxSetOffset[7] = VdsVoltageMaxSet07OFFSET;
		m_voltageMaxSetOffset[8] = VdsVoltageMaxSet08OFFSET;
		m_voltageMaxSetOffset[9] = VdsVoltageMaxSet09OFFSET;
		m_voltageMaxSetOffset[10] = VdsVoltageMaxSet10OFFSET;
		m_voltageMaxSetOffset[11] = VdsVoltageMaxSet11OFFSET;
		m_voltageMaxSetOffset[12] = VdsVoltageMaxSet12OFFSET;
		m_voltageMaxSetOffset[13] = VdsVoltageMaxSet13OFFSET;
		m_voltageMaxSetOffset[14] = VdsVoltageMaxSet14OFFSET;
		m_voltageMaxSetOffset[15] = VdsVoltageMaxSet15OFFSET;
		m_voltageMaxSetOffset[16] = VdsVoltageMaxSet16OFFSET;
		m_voltageMaxSetOffset[17] = VdsVoltageMaxSet17OFFSET;
		m_voltageMaxSetOffset[18] = VdsVoltageMaxSet18OFFSET;
		m_voltageMaxSetOffset[19] = VdsVoltageMaxSet19OFFSET;
		m_voltageMaxSetOffset[20] = VdsVoltageMaxSet20OFFSET;
		m_voltageMaxSetOffset[21] = VdsVoltageMaxSet21OFFSET;
		m_voltageMaxSetOffset[22] = VdsVoltageMaxSet22OFFSET;
		m_voltageMaxSetOffset[23] = VdsVoltageMaxSet23OFFSET;

		m_currentMaxSetOffset[0] = VdsCurrentMaxSet00OFFSET;
		m_currentMaxSetOffset[1] = VdsCurrentMaxSet01OFFSET;
		m_currentMaxSetOffset[2] = VdsCurrentMaxSet02OFFSET;
		m_currentMaxSetOffset[3] = VdsCurrentMaxSet03OFFSET;
		m_currentMaxSetOffset[4] = VdsCurrentMaxSet04OFFSET;
		m_currentMaxSetOffset[5] = VdsCurrentMaxSet05OFFSET;
		m_currentMaxSetOffset[6] = VdsCurrentMaxSet06OFFSET;
		m_currentMaxSetOffset[7] = VdsCurrentMaxSet07OFFSET;
		m_currentMaxSetOffset[8] = VdsCurrentMaxSet08OFFSET;
		m_currentMaxSetOffset[9] = VdsCurrentMaxSet09OFFSET;
		m_currentMaxSetOffset[10] = VdsCurrentMaxSet10OFFSET;
		m_currentMaxSetOffset[11] = VdsCurrentMaxSet11OFFSET;
		m_currentMaxSetOffset[12] = VdsCurrentMaxSet12OFFSET;
		m_currentMaxSetOffset[13] = VdsCurrentMaxSet13OFFSET;
		m_currentMaxSetOffset[14] = VdsCurrentMaxSet14OFFSET;
		m_currentMaxSetOffset[15] = VdsCurrentMaxSet15OFFSET;
		m_currentMaxSetOffset[16] = VdsCurrentMaxSet16OFFSET;
		m_currentMaxSetOffset[17] = VdsCurrentMaxSet17OFFSET;
		m_currentMaxSetOffset[18] = VdsCurrentMaxSet18OFFSET;
		m_currentMaxSetOffset[19] = VdsCurrentMaxSet19OFFSET;
		m_currentMaxSetOffset[20] = VdsCurrentMaxSet20OFFSET;
		m_currentMaxSetOffset[21] = VdsCurrentMaxSet21OFFSET;
		m_currentMaxSetOffset[22] = VdsCurrentMaxSet22OFFSET;
		m_currentMaxSetOffset[23] = VdsCurrentMaxSet23OFFSET;
	}

	return m_deviceClass;
}

int VmeModule::serialNumber(void)
{
	if (m_deviceClass == V12C0) {
		return readLong(m_baseAddress + VhsSerialNumberOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLong(m_baseAddress + VdsSerialNumberOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		version = readLong(m_baseAddress + VdsFirmwareReleaseOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		return readLong(m_baseAddress + VdsDecoderReleaseOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		return readFloat(m_baseAddress + VdsTemperatureOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

float VmeModule::supplyP5(void)
{
	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsSupplyP5OFFSET);
	} else if (m_deviceClass == V24D0) {
		return readFloat(m_baseAddress + VdsSupplyP5OFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

float VmeModule::supplyP12(void)
{
	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsSupplyP12OFFSET);
	} else if (m_deviceClass == V24D0) {
		return readFloat(m_baseAddress + VdsSupplyP12OFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

float VmeModule::supplyN12(void)
{
	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsSupplyN12OFFSET);
	} else if (m_deviceClass == V24D0) {
		return readFloat(m_baseAddress + VdsSupplyN12OFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

uint32_t VmeModule::adcSamplesPerSecond(void)
{
	if (m_deviceClass == V12C0) {
		return readShort(m_baseAddress + VhsADCSamplesPerSecondOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLong(m_baseAddress + VdsADCSamplesPerSecondOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setAdcSamplesPerSecond(uint32_t sps)
{
	if (m_deviceClass == V12C0) {
		return writeShort(m_baseAddress + VhsADCSamplesPerSecondOFFSET, sps);
	} else if (m_deviceClass == V24D0) {
		return writeLong(m_baseAddress + VdsADCSamplesPerSecondOFFSET, sps);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::digitalFilter(void)
{
	if (m_deviceClass == V12C0) {
		return readShort(m_baseAddress + VhsDigitalFilterOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLong(m_baseAddress + VdsDigitalFilterOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setDigitalFilter(uint32_t filter)
{
	if (m_deviceClass == V12C0) {
		return writeShort(m_baseAddress + VhsDigitalFilterOFFSET, filter);
	} else if (m_deviceClass == V24D0) {
		return writeLong(m_baseAddress + VdsDigitalFilterOFFSET, filter);
	} else {
		deviceClass();
	}
}

float VmeModule::voltageLimit(void)
{
	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsVoltageMaxOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readFloat(m_baseAddress + VdsVoltageMaxOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		result = readFloat(m_baseAddress + VdsCurrentMaxOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + VdsModuleStatusOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

uint32_t VmeModule::eventStatus(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleEventStatusOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + VdsModuleEventStatusOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setEventStatus(uint32_t status)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + VhsModuleEventStatusOFFSET, status);
	} else if (m_deviceClass == V24D0) {
		writeLongBitfield(m_baseAddress + VdsModuleEventStatusOFFSET, status);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::eventMask(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleEventMaskOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + VdsModuleEventMaskOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setEventMask(uint32_t mask)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + VhsModuleEventMaskOFFSET, mask);
	} else if (m_deviceClass == V24D0) {
		writeLongBitfield(m_baseAddress + VdsModuleEventMaskOFFSET, mask);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::control(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleControlOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + VdsModuleControlOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setControl(uint32_t control)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + VhsModuleControlOFFSET, control);
	} else if (m_deviceClass == V24D0) {
		writeLongBitfield(m_baseAddress + VdsModuleControlOFFSET, control);
	} else {
		deviceClass();
	}
}

float VmeModule::voltageRampSpeed(void)
{
	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsVoltageRampSpeedOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readFloat(m_baseAddress + VdsVoltageRampSpeedOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setVoltageRampSpeed(float vramp)
{
	if (m_deviceClass == V12C0) {
		return writeFloat(m_baseAddress + VhsVoltageRampSpeedOFFSET, vramp);
	} else if (m_deviceClass == V24D0) {
		return writeFloat(m_baseAddress + VdsVoltageRampSpeedOFFSET, vramp);
	} else {
		deviceClass();
	}
}

float VmeModule::currentRampSpeed(void)
{
	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + VhsCurrentRampSpeedOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readFloat(m_baseAddress + VdsCurrentRampSpeedOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setCurrentRampSpeed(float iramp)
{
	if (m_deviceClass == V12C0) {
		return writeFloat(m_baseAddress + VhsCurrentRampSpeedOFFSET, iramp);
	} else if (m_deviceClass == V24D0) {
		return writeFloat(m_baseAddress + VdsCurrentRampSpeedOFFSET, iramp);
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
		} else if (m_deviceClass == V24D0) {
			writeLong(m_baseAddress + VdsSetOnOffAllChannelsOFFSET, 1);
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
		} else if (m_deviceClass == V24D0) {
			writeLong(m_baseAddress + VdsSetOnOffAllChannelsOFFSET, 0);
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
	} else if (m_deviceClass == V24D0) {
		writeFloat(m_baseAddress + VdsSetVoltageAllChannelsOFFSET, vset);
	} else {
		deviceClass();
	}
}

void VmeModule::setAllChannelsCurrentSet(float iset)
{
	iset = userCurrentToSiCurrent(iset);

	if (m_deviceClass == V12C0) {
		writeFloat(m_baseAddress + VhsSetCurrentAllChannelsOFFSET, iset);
	} else if (m_deviceClass == V24D0) {
		writeFloat(m_baseAddress + VdsSetCurrentAllChannelsOFFSET, iset);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::restartTime(void)
{
	if (m_deviceClass == V12C0) {
		return readShort(m_baseAddress + VhsRestartTimeOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLong(m_baseAddress + VdsRestartTimeOFFSET);
	} else {
		deviceClass(); return 0;
	}
}

void VmeModule::setRestartTime(uint32_t restartTime)
{
	if (m_deviceClass == V12C0) {
		writeShort(m_baseAddress + VhsRestartTimeOFFSET, restartTime);
	} else if (m_deviceClass == V24D0) {
		writeLong(m_baseAddress + VdsRestartTimeOFFSET, restartTime);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::eventChannelStatus(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleEventChannelStatusOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + VdsModuleEventChannelStatusOFFSET);
	} else {
		deviceClass(); return 0;
	}
}

void VmeModule::clearEventChannelStatus(void)
{
	if (m_deviceClass == V12C0) {
		writeShort(m_baseAddress + VhsModuleEventChannelStatusOFFSET, -1);
	} else if (m_deviceClass == V24D0) {
		writeLong(m_baseAddress + VdsModuleEventChannelStatusOFFSET, -1);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::eventChannelMask(void)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + VhsModuleEventChannelMaskOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + VdsModuleEventChannelMaskOFFSET);
	} else {
		deviceClass(); return 0;
	}
}

void VmeModule::setEventChannelMask(uint32_t mask)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + VhsModuleEventChannelMaskOFFSET, mask);
	} else if (m_deviceClass == V24D0) {
		writeLongBitfield(m_baseAddress + VdsModuleEventChannelMaskOFFSET, mask);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::eventGroupStatus(void)
{
	if (m_deviceClass == V12C0) {
		return readLongBitfield(m_baseAddress + VhsModuleEventGroupStatusOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + VdsModuleEventGroupStatusOFFSET);
	} else {
		deviceClass(); return 0;
	}
}

uint32_t VmeModule::eventGroupMask(void)
{
	if (m_deviceClass == V12C0) {
		return readLongBitfield(m_baseAddress + VhsModuleEventGroupMaskOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + VdsModuleEventGroupMaskOFFSET);
	} else {
		deviceClass(); return 0;
	}
}

void VmeModule::setEventGroupMask(uint32_t mask)
{
	if (m_deviceClass == V12C0) {
		writeLongBitfield(m_baseAddress + VhsModuleEventGroupMaskOFFSET, mask);
	} else if (m_deviceClass == V24D0) {
		writeLongBitfield(m_baseAddress + VdsModuleEventGroupMaskOFFSET, mask);
	} else {
		deviceClass();
	}
}

void VmeModule::clearEventGroupStatus(void)
{
	if (m_deviceClass == V12C0) {
		writeLong(m_baseAddress + VhsModuleEventGroupStatusOFFSET, -1);
	} else if (m_deviceClass == V24D0) {
		writeLong(m_baseAddress + VdsModuleEventGroupStatusOFFSET, -1);
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
	} else if (m_deviceClass == V24D0) {
		writeLong(m_baseAddress + VdsSpecialControlCommandOFFSET, command);
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
	} else if (m_deviceClass == V24D0) {
		return readLong(m_baseAddress + VdsSpecialControlCommandOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

uint32_t VmeModule::specialControlStatus(void)
{
	if (m_deviceClass == V12C0) {
		return readShort(m_baseAddress + VhsSpecialControlStatusOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLong(m_baseAddress + VdsSpecialControlStatusOFFSET);
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

	} else if (m_deviceClass == V24D0) {
		writeShort(m_baseAddress + VdsNewBaseAddressOFFSET, address);
		writeShort(m_baseAddress + VdsNewBaseAddressXorOFFSET, address ^ 0xFFFF);

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

	} else if (m_deviceClass == V24D0) {
		newAddress = readShort(m_baseAddress + VdsNewBaseAddressAcceptedOFFSET);
		temp = readShort(m_baseAddress + VdsNewBaseAddressOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + m_channelOffset[channel] + VdsChannelStatusOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

uint32_t VmeModule::channelEventStatus(int channel)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelEventStatusOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + m_channelOffset[channel] + VdsChannelEventStatusOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setChannelEventStatus(int channel, uint32_t status)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelEventStatusOFFSET, status);
	} else if (m_deviceClass == V24D0) {
		writeLongBitfield(m_baseAddress + m_channelOffset[channel] + VdsChannelEventStatusOFFSET, status);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::channelEventMask(int channel)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelEventMaskOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + m_channelOffset[channel] + VdsChannelEventMaskOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setChannelEventMask(int channel, uint32_t mask)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelEventMaskOFFSET, mask);
	} else if (m_deviceClass == V24D0) {
		writeLongBitfield(m_baseAddress + m_channelOffset[channel] + VdsChannelEventMaskOFFSET, mask);
	} else {
		deviceClass();
	}
}

uint32_t VmeModule::channelControl(int channel)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelControlOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + m_channelOffset[channel] + VdsChannelControlOFFSET);
	} else {
		deviceClass();
		return 0;
	}
}

void VmeModule::setChannelControl(int channel, uint32_t control)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + m_channelOffset[channel] + VhsChannelControlOFFSET, control);
	} else if (m_deviceClass == V24D0) {
		writeLongBitfield(m_baseAddress + m_channelOffset[channel] + VdsChannelControlOFFSET, control);
	} else {
		deviceClass();
	}
}

float VmeModule::channelVoltageNominal(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelVoltageNominalOFFSET);
	} else if (m_deviceClass == V24D0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelVoltageNominalOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelCurrentNominalOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelVoltageSetOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		writeFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelVoltageSetOFFSET, vset);
	} else {
		deviceClass();
	}
}

float VmeModule::channelVoltageMeasure(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelVoltageMeasureOFFSET);
	} else if (m_deviceClass == V24D0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelVoltageMeasureOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelCurrentSetOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		writeFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelCurrentSetOFFSET, iset);
	} else {
		deviceClass();
	}
}

float VmeModule::channelCurrentMeasure(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelCurrentMeasureOFFSET);
	} else if (m_deviceClass == V24D0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelCurrentMeasureOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelVoltageIlkMaxSetOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		writeFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelVoltageIlkMaxSetOFFSET, ilkmax);
	} else {
		deviceClass();
	}
}

float VmeModule::channelVoltageInterlockMinSet(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		return readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelVoltageIlkMinSetOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelVoltageIlkMinSetOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		writeFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelVoltageIlkMinSetOFFSET, ilkmin);
	} else {
		deviceClass();
	}
}

float VmeModule::channelCurrentInterlockMaxSet(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelCurrentIlkMaxSetOFFSET);
	} else if (m_deviceClass == V24D0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelCurrentIlkMaxSetOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		writeFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelCurrentIlkMaxSetOFFSET, ilkmax);
	} else {
		deviceClass();
	}
}

float VmeModule::channelCurrentInterlockMinSet(int channel)
{
	float result;

	if (m_deviceClass == V12C0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VhsChannelCurrentIlkMinSetOFFSET);
	} else if (m_deviceClass == V24D0) {
		result = readFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelCurrentIlkMinSetOFFSET);
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
	} else if (m_deviceClass == V24D0) {
		writeFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelCurrentIlkMinSetOFFSET, ilkmin);
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
		} else if (m_deviceClass == V24D0) {
			writeLong(m_baseAddress + VdsSetEmergencyAllChannelsOFFSET, 1);
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
		} else if (m_deviceClass == V24D0) {
			writeLong(m_baseAddress + VdsSetEmergencyAllChannelsOFFSET, 0);
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
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + m_flexGroupOffset[group] + VdsFlexGroupMember2OFFSET);
	} else {
		deviceClass(); return 0;
	}
}

uint32_t VmeModule::flexGroupType(int group)
{
	if (m_deviceClass == V12C0) {
		return readShortBitfield(m_baseAddress + m_flexGroupOffset[group] + VhsFlexGroupTypeOFFSET);
	} else if (m_deviceClass == V24D0) {
		return readLongBitfield(m_baseAddress + m_flexGroupOffset[group] + VdsFlexGroupType2OFFSET);
	} else {
		deviceClass(); return 0;
	}
}

void VmeModule::setFlexGroup(int group, uint32_t member, uint32_t type)
{
	if (m_deviceClass == V12C0) {
		writeShortBitfield(m_baseAddress + m_flexGroupOffset[group] + VhsFlexGroupMemberOFFSET, member);
		writeShortBitfield(m_baseAddress + m_flexGroupOffset[group] + VhsFlexGroupTypeOFFSET, type);
	} else if (m_deviceClass == V24D0) {
		writeLongBitfield(m_baseAddress + m_flexGroupOffset[group] + VdsFlexGroupMember2OFFSET, member);
		writeLongBitfield(m_baseAddress + m_flexGroupOffset[group] + VdsFlexGroupType2OFFSET, type);
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
	} else if (m_deviceClass == V24D0) {
		return writeFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelVoltageNominalOFFSET, maxset);
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
	} else if (m_deviceClass == V24D0) {
		return writeFloat(m_baseAddress + m_channelOffset[channel] + VdsChannelCurrentNominalOFFSET, maxset);
	} else {
		deviceClass();
	}
}
