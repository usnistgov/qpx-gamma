#ifndef IsegVHS_H
#define IsegVHS_H

#include "vmemodule.h"
#include "vmecontroller.h"
#include "generic_setting.h"

#define VHS_ADDRESS_SPACE_LENGTH				0x0400


class IsegVHS : public VmeModule
{

public:
  IsegVHS();
  ~IsegVHS();

  virtual bool    connect(VmeController *controller, int baseAddress);
  virtual bool    connected() const;
  virtual void    disconnect();

  std::string firmwareName() const;

  uint16_t    baseAddress() const { return m_baseAddress; }
  virtual uint16_t baseAddressSpaceLength() const {return VHS_ADDRESS_SPACE_LENGTH;}
  bool        setBaseAddress(uint16_t baseAddress);
  std::string address() const;

  void    programBaseAddress(uint16_t address);
  uint16_t verifyBaseAddress(void) const;

  bool read_setting(Gamma::Setting& set, int address_modifier) const;
  bool write_setting(Gamma::Setting& set, int address_modifier);

protected:
  uint16_t readShort(int address) const;
  void    writeShort(int address, uint16_t data);
  uint16_t readShortBitfield(int address) const;
  void    writeShortBitfield(int address, uint16_t data);
  uint32_t readLong(int address) const;
  void    writeLong(int address, uint32_t data);
  uint32_t readLongBitfield(int address) const;
  void    writeLongBitfield(int address, uint32_t data);
  float   readFloat(int address) const;
  void    writeFloat(int address, float data);

private:
  uint16_t m_baseAddress;
  VmeController *m_controller;

  static uint16_t mirrorShort(uint16_t data);
  static uint32_t mirrorLong(uint32_t data);

};


#endif // IsegVHS_H


