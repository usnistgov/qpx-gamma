#pragma once

#include <string>
#include <list>
#include "vme_stack.h"

class VmeController
{
public:
  VmeController()	{}
  virtual ~VmeController() {}

  virtual void daq_start() {}
  virtual void daq_stop() {}
  virtual bool daq_init() { return false; }

  virtual std::string controllerName(void) { return "VmeController"; }
  virtual std::string serialNumber(void) { return std::string(); }

  virtual std::list<std::string> controllers(void) { return std::list<std::string>(); }
  virtual bool connect(uint16_t target) { return false; }
  virtual bool connect(std::string target) { return false; }
	virtual bool connected() { return false; }
  virtual void systemReset() = 0;

  virtual std::string address() const	{ return std::string();	}
  virtual bool setAddress(std::string address) { return false; }

  virtual VmeStack* newStack() { return new VmeStack(); }

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
