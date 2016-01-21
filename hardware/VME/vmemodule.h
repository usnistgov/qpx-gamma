#ifndef VMEMODULE_H
#define VMEMODULE_H

#include <sstream>
#include <iomanip>
#include "daq_device.h"

class VmeController;

class VmeModule : public Qpx::DaqDevice
{

public:
  VmeModule() : m_controller(nullptr), m_baseAddress(0)  {}
  virtual ~VmeModule(){}

  bool boot() override {
    if (!(status_ & Qpx::DeviceStatus::can_boot))
      return false;

    status_ = Qpx::DeviceStatus::loaded | Qpx::DeviceStatus::can_boot;

    if (!connected())
      return false;

    status_ = Qpx::DeviceStatus::loaded | Qpx::DeviceStatus::booted;
    return true;
  }

  bool die() override {
    disconnect();
    status_ = Qpx::DeviceStatus::loaded | Qpx::DeviceStatus::can_boot;
    return true;
  }

  virtual bool connect(VmeController *controller, int baseAddress) {
    m_controller = controller;
    return setBaseAddress(baseAddress);
  }

  virtual void disconnect() {
    m_controller = nullptr;
    m_baseAddress = 0;
  }

  virtual uint32_t baseAddress() const { return m_baseAddress; }

  virtual bool    setBaseAddress(uint32_t baseAddress) {
    m_baseAddress = baseAddress;
    return connected();
  }

  virtual std::string address() {
      std::stringstream stream;
      stream << "VME BA 0x"
             << std::setfill ('0') << std::setw(sizeof(uint16_t)*2)
             << std::hex << m_baseAddress;
      return stream.str();
  }

  //implement these for derived classes
  virtual uint32_t baseAddressSpaceLength() const = 0;
  virtual bool connected() const { return false; }
  virtual std::string firmwareName() const {return std::string();}

  //probably does not apply to many...
  virtual void    programBaseAddress(uint32_t address) {}
  virtual uint16_t verifyBaseAddress() { return 0; }

protected:
  uint32_t       m_baseAddress;
  VmeController *m_controller;
};

#endif // VmeModule_H
