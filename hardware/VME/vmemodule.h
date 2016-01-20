#ifndef VMEMODULE_H
#define VMEMODULE_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include "daq_device.h"

class VmeController;

class VmeModule : public Qpx::DaqDevice
{

public:
  VmeModule() {}
  virtual ~VmeModule() {}

  virtual bool    connect(VmeController *controller, int baseAddress) = 0;
  virtual bool    connected() const = 0;
  virtual void    disconnect() {}

  virtual std::string firmwareName() const {return std::string();}

  virtual uint32_t baseAddress() const { return 0; }
  virtual uint32_t baseAddressSpaceLength() const = 0;
  virtual bool    setBaseAddress(uint32_t baseAddress) {}
  virtual std::string address() const = 0;

  virtual void    programBaseAddress(uint32_t address) {}
  virtual uint16_t verifyBaseAddress() { return 0; }
};

#endif // VmeModule_H
