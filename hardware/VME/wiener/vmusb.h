#ifndef VMUSB_H
#define VMUSB_H

#include <string>
#include "../boostdll/dll/import.hpp"
#include "../boostdll/dll/shared_library.hpp"
#include <boost/make_shared.hpp>

#include "../vmecontroller.h"
#include "xxusbdll.h"


class VmUsb : public VmeController
{
public:
  VmUsb();
  ~VmUsb();
//  std::list<std::string> enumControllers(void);

  virtual std::string errorString(void);
  virtual std::string controllerName(void);
  virtual std::string information(void);
  virtual void systemReset(int *errorCode = NULL);

  virtual bool connected() { return (udev); }

  virtual void writeShort(int vmeAddress, int addressModifier, int data, int *errorCode = NULL);
  virtual int  readShort(int vmeAddress, int addressModifier, int *errorCode = NULL);

private:
  usb_dev_handle *udev;

  std::string m_serialNumber;
  std::string m_errorString;

  boost::function<short(xxusb_device_type *)> xxusbDevicesFind;
  boost::function<usb_dev_handle*(struct usb_device *)> xxusbDeviceOpen;
  boost::function<short(usb_dev_handle *)> xxusbDeviceClose;
  boost::function<short(usb_dev_handle *, short, long *)> xxusbRegisterRead;
  boost::function<short(usb_dev_handle *, short, long, long *)> vmeRead16;
  boost::function<short(usb_dev_handle *, short, long, long)> vmeWrite16;

};

#endif // VMUSB_H
