#ifndef VMEMODULE_H
#define VMEMODULE_H

#include <sstream>
#include <iomanip>
#include "daq_source.h"
#include "qpx_util.h"
#include "vme_stack.h"

class VmeController;

class VmeModule : public Qpx::Source
{

public:
  VmeModule() : m_controller(nullptr), m_baseAddress(0)  {}
  virtual ~VmeModule(){}

  bool boot() override {
    if (!(status_ & Qpx::SourceStatus::can_boot))
      return false;

    status_ = Qpx::SourceStatus::loaded | Qpx::SourceStatus::can_boot;

    if (!connected())
      return false;

    status_ = Qpx::SourceStatus::loaded | Qpx::SourceStatus::booted;
    return true;
  }

  bool die() override {
    disconnect();
    status_ = Qpx::SourceStatus::loaded | Qpx::SourceStatus::can_boot;
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
    return "0x" + itohex32(m_baseAddress);
  }

  //implement these for derived classes
  virtual uint32_t baseAddressSpaceLength() const = 0;
  virtual bool connected() const { return false; }
  virtual std::string firmwareName() const {return std::string();}

  virtual void addReadout(VmeStack& stack, int param) {}

  //probably does not apply to many...
  virtual void    programBaseAddress(uint32_t address) {}
  virtual uint16_t verifyBaseAddress() { return 0; }

protected:
  uint32_t       m_baseAddress;
  VmeController *m_controller;
};

#endif // VmeModule_H
