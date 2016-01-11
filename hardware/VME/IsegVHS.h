#ifndef IsegVHS_H
#define IsegVHS_H

#include "basemodule.h"
#include "vmecontroller.h"
#include "generic_setting.h"


#define ISEG_VENDOR_ID									0x69736567
#define VHS_ADDRESS_SPACE_LENGTH				0x0400
#define VME_ADDRESS_MODIFIER						0x29

class IsegVHS : public BaseModule
{

public:
  IsegVHS(BaseController *controller, int baseAddress);
  ~IsegVHS();

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

#endif // IsegVHS_H


