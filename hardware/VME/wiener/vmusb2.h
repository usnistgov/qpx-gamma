/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2005.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Author:
             Ron Fox
       NSCL
       Michigan State University
       East Lansing, MI 48824-1321

   Modified by:
    Martin Shetty, NIST
*/

#ifndef VMUSB2_H
#define VMUSB2_H

#include <string>

#include "../vmecontroller.h"
#include "vmusb_stack.h"


struct usb_device;
struct usb_dev_handle;



class VmUsb2 : public VmeController
{
public:
  VmUsb2();
  ~VmUsb2();

  std::string controllerName(void);
  std::string serialNumber(void) { return m_serialNumber; }
  bool connected() { return m_handle; }
  std::list<std::string> controllers();

  bool connect(uint16_t target);
  bool connect(std::string target);
  void disconnect();
  bool reconnect(); /* Drop USB/open USb. */

  virtual void systemReset();

  void daq_start();
  void daq_stop();
  virtual bool daq_init();

  virtual VmeStack* newStack() { return new VmUsbStack(); }

  void      write8(uint32_t vmeAddress, AddressModifier am, uint8_t data);
  uint8_t   read8(uint32_t vmeAddress, AddressModifier am);

  void       write16(uint32_t vmeAddress, AddressModifier am, uint16_t data);
  uint16_t    read16(uint32_t vmeAddress, AddressModifier am);

  void        write32(uint32_t vmeAddress, AddressModifier am, uint32_t data);
  uint32_t     read32(uint32_t vmeAddress, AddressModifier am);

  void    writeRegister(uint16_t vmeAddress, uint32_t data);
  uint32_t readRegister(uint16_t vmeAddress);

  std::vector<uint32_t> vmeBlockRead(int base, AddressModifier amod,  int xfercount);
  std::vector<uint32_t> vmeFifoRead(int base, AddressModifier amod, int xfercount);

  // Once the interface is in DAQ auntonomous mode, the application
  // should call the following function to read acquired data.
  int usbRead(void* data, size_t bufferSize, size_t* transferCount, int timeout = 2000);

  void     writeIrqMask(uint8_t mask);
  uint8_t  readIrqMask();

private:
  std::string m_serialNumber;
  struct usb_dev_handle*  m_handle;	// Handle open on the device.
  struct usb_device*      m_device;   // Device we are open on.
  int                     m_timeout; // Timeout used when user doesn't give one.

  uint8_t m_irqMask;

  void writeActionRegister(uint16_t value);

  int   doVMEWrite(VmUsbStack& list);
  int   doVMERead(VmUsbStack&  list, uint32_t* datum);
  virtual std::vector<uint8_t> executeList(VmUsbStack& list, int maxBytes);
  virtual int loadList(uint8_t listNumber, VmUsbStack& list, off_t listOffset);
  int loadList(int listNumber, VmUsbStack& list, int offset) {
    return loadList((uint8_t)listNumber, list, (off_t)offset);
  }
  int transaction(void* writePacket, size_t writeSize, void* readPacket,  size_t readSize);


  int a_vmeWrite32(uint32_t address, AddressModifier aModifier, uint32_t data);
  int a_vmeRead32(uint32_t address, AddressModifier aModifier, uint32_t* data);

  int a_vmeWrite16(uint32_t address, AddressModifier aModifier, uint16_t data);
  int a_vmeRead16(uint32_t address, AddressModifier aModifier, uint16_t* data);

  int a_vmeWrite8(uint32_t address, AddressModifier aModifier, uint8_t data);
  int a_vmeRead8(uint32_t address, AddressModifier aModifier, uint8_t* data);

  virtual int a_vmeBlockRead(uint32_t baseAddress, AddressModifier aModifier,
                           void* data,  size_t transferCount, size_t* countTransferred);
  virtual int a_vmeFifoRead(uint32_t address, AddressModifier aModifier,
                          void* data, size_t transferCount, size_t* countTransferred);
  virtual int a_executeList(VmUsbStack& list, void* pReadBuffer, size_t readBufferSize, size_t* bytesRead);

  std::vector<struct usb_device*> enumerate();
  std::string serialNo(struct usb_device* dev);
  bool initVmUsb();
};

#endif // VMUSB_H
