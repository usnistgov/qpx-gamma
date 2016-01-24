#ifndef VMECONTROLLER_H
#define VMECONTROLLER_H

#include <string>
#include <list>

enum class AddressModifier : uint8_t {
  A16_Lock         = 0x2C,
  A16_User         = 0x29,
  A16_Priv         = 0x2D,

  A24_Lock         = 0x32,
  A24_UserData     = 0x39,
  A24_UserProgram  = 0x3A,
  A24_UserBlock    = 0x3B,
  A24_UserBlock64  = 0x38,

  A24_PrivData     = 0x3D,
  A24_PrivProgram  = 0x3E,
  A24_PrivBlock    = 0x3F,
  A24_PrivBlock64  = 0x3C,

  A32_Lock         = 0x05,
  A32_UserData     = 0x09,
  A32_UserProgram  = 0x0A,
  A32_UserBlock    = 0x0B,
  A32_UserBlock64  = 0x08,
  
  A32_PrivData     = 0x0D,
  A32_PrivProgram  = 0x0E,
  A32_PrivBlock    = 0x0F,
  A32_PrivBlock64  = 0x0C,

  A64_Lock         = 0x04,
  A64_SingleAccess = 0x01,
  A64_Block        = 0x03,
  A64_Block64      = 0x00,

};


class VmeController
{
public:
  VmeController()	{}
  virtual ~VmeController() {}

  virtual std::string controllerName(void) { return "VmeController"; }
  virtual std::string serialNumber(void) { return std::string(); }

  virtual std::list<std::string> controllers(void) { return std::list<std::string>(); }
  virtual bool connect(uint16_t target) { return false; }
  virtual bool connect(std::string target) { return false; }
	virtual bool connected() { return false; }
  virtual void systemReset() = 0;

  virtual std::string address() const	{ return std::string();	}
  virtual bool setAddress(std::string address) { return false; }

//  virtual void setLiveInsertion(bool liveInsertion) {}
//	virtual bool liveInsertion() const { return false; }
//  virtual std::string busStatus() { return std::string(); }

  virtual std::list<std::string> moduleList() { return std::list<std::string>(); }

//  virtual void      write8(uint32_t vmeAddress, AddressModifier am, uint8_t data) = 0;
//  virtual uint8_t   read8(uint32_t vmeAddress, AddressModifier am) = 0;

  virtual void      write16(uint32_t vmeAddress, AddressModifier am, uint16_t data) = 0;
  virtual uint16_t   read16(uint32_t vmeAddress, AddressModifier am) = 0;

  virtual void      write32(uint32_t vmeAddress, AddressModifier am, uint32_t data) = 0;
  virtual uint32_t   read32(uint32_t vmeAddress, AddressModifier am) = 0;

  virtual void    writeRegister(uint16_t vmeAddress, uint32_t data) = 0;
  virtual uint32_t readRegister(uint16_t vmeAddress) = 0;

  virtual void start_daq() {}
  virtual void stop_daq() {}

  virtual void     writeIrqMask(uint8_t mask) {}
  virtual uint8_t      readIrqMask() {return 0xFF;}

};

#endif // VMECONTROLLER_H
