#ifndef VMEMODULE_H
#define VMEMODULE_H

#include "basemodule.h"
#include "vmecontroller.h"
#include "generic_setting.h"


#define ISEG_VENDOR_ID									0x69736567
#define VHS_ADDRESS_SPACE_LENGTH				0x0400
#define VDS_ADDRESS_SPACE_LENGTH				0x0800
#define VME_ADDRESS_MODIFIER						0x29

class VmeModule : public BaseModule
{

public:
	VmeModule(BaseController *controller, int baseAddress);
	~VmeModule();

	bool    connected(void);

  bool read_setting(Gamma::Setting& set, int address_modifier);
  bool write_setting(Gamma::Setting& set, int address_modifier);

  void    programBaseAddress(uint16_t address);
  uint16_t verifyBaseAddress(void);

  std::string address() {   std::stringstream stream;
                            stream << "VME BA 0x"
                                   << std::setfill ('0') << std::setw(sizeof(uint16_t)*2)
                                   << std::hex << m_baseAddress;
                            return stream.str();}
  uint16_t baseAddress() { return m_baseAddress; }
  void    setBaseAddress(uint16_t baseAddress);


  // --- Module commands ----------------------------------------------------
  std::string firmwareRelease(void);

private:
  uint16_t m_baseAddress;
	VmeController *m_controller;


  uint16_t readShort(int address);
  void    writeShort(int address, uint16_t data);
  uint16_t readShortBitfield(int address);
  void    writeShortBitfield(int address, uint16_t data);
  uint32_t readLong(int address);
  void    writeLong(int address, uint32_t data);
  uint32_t readLongBitfield(int address);
  void    writeLongBitfield(int address, uint32_t data);
  float   readFloat(int address);
  void    writeFloat(int address, float data);

  uint16_t mirrorShort(uint16_t data);
  uint32_t mirrorLong(uint32_t data);
};


// --- VHS12 ------------------------------------------------------------------
#define VhsFirmwareReleaseOFFSET					56
#define VhsDeviceClassOFFSET						62

#define VhsVendorIdOFFSET							92

#define VhsNewBaseAddressOFFSET						0x03A0
#define VhsNewBaseAddressXorOFFSET					0x03A2
#define VhsNewBaseAddressAcceptedOFFSET				0x03A6

#endif // VMEMODULE_H


/*

typedef enum {
  Volt, KiloVolt
} EnumVoltageUnit;

typedef enum {
  NanoAmpere, MicroAmpere, MilliAmpere, Ampere
} EnumCurrentUnit;

ctor :
  m_voltageUnit(Volt),
  m_currentUnit(MilliAmpere)

  EnumVoltageUnit m_voltageUnit;
  EnumCurrentUnit m_currentUnit;
  float   siVoltageToUserVoltage(float value);
  float   userVoltageToSiVoltage(float value);
  float   siCurrentToUserCurrent(float siCurrent);
  float   userCurrentToSiCurrent(float userCurrent);
  virtual std::string voltageUnit(int channel) const
  {
    std::string result("V");
    if (m_voltageUnit == KiloVolt)
      result = "kV";
    return result;
  }

  virtual void setVoltageUnit(std::string unit)
  {
    if (unit == "kV")
      m_voltageUnit = KiloVolt;
    else
      m_voltageUnit = Volt;
  }

  virtual std::string currentUnit(int channel) const
  {
    std::string result;
    switch (m_currentUnit) {
    case MilliAmpere: result = "mA"; break;
    case MicroAmpere: result = "\u03BCA"; break;
    case NanoAmpere:  result = "nA"; break;
    default:          result = "A";  break;
    }
    return result;
  }

  virtual void setCurrentUnit(std::string unit)
  {
    if (unit == "nA")
      m_currentUnit = NanoAmpere;
    else if (unit == "uA" || unit == "\u03BCA")
      m_currentUnit = MicroAmpere;
    else if (unit == "mA")
      m_currentUnit = MilliAmpere;
    else
      m_currentUnit = Ampere;
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

*/
