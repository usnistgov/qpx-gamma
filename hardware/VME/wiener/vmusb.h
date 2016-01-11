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



  //  short __stdcall xxusb_devices_find(xxusb_device_type *xxusbDev);
  boost::function<short(xxusb_device_type *)> xxusbDevicesFind;
  //  usb_dev_handle* __stdcall xxusb_device_open(struct usb_device *dev);
  boost::function<usb_dev_handle*(struct usb_device *)> xxusbDeviceOpen;
  //  usb_dev_handle* __stdcall xxusb_serial_open(char *SerialString);
  boost::function<usb_dev_handle*(char *)> xxusbSerialOpen;
  //  short __stdcall xxusb_device_close(usb_dev_handle *hDev);
  boost::function<short(usb_dev_handle *)> xxusbDeviceClose;
  //  short __stdcall xxusb_register_read(usb_dev_handle *hDev, short RegAddr, long *RegData);
  boost::function<short(usb_dev_handle *, short, long *)> xxusbRegisterRead;
  //  short __stdcall xxusb_register_write(usb_dev_handle *hDev, short RegAddr, long RegData);
  boost::function<short(usb_dev_handle *, short, long)> xxusbRegisterWrite;
  //  short __stdcall xxusb_stack_read(usb_dev_handle *hDev, short StackAddr, long *StackData);
  boost::function<short(usb_dev_handle *, short, long *)> xxusbStackRead;
  //  short __stdcall xxusb_stack_write(usb_dev_handle *hDev, short StackAddr, long *StackData);
  boost::function<short(usb_dev_handle *, short, long *)> xxusbStackWrite;
  //  short __stdcall xxusb_stack_execute(usb_dev_handle *hDev, long *StackData);
  boost::function<short(usb_dev_handle *, long *)> xxusbStackExecute;
  //  short __stdcall xxusb_longstack_execute(usb_dev_handle *hDev, void *DataBuffer, int lDataLen, int timeout);
  boost::function<short(usb_dev_handle *, void *, int, int)> xxusbLongStackExecute;
  //  short __stdcall xxusb_usbfifo_read(usb_dev_handle *hDev, long *DataBuffer, short lDataLen, int timeout);
  boost::function<short(usb_dev_handle *, long *, short, short)> xxusbUsbFifoRead;
  //  short __stdcall xxusb_bulk_read(usb_dev_handle *hDev, char *DataBuffer, short lDataLen, int timeout);
  boost::function<short(usb_dev_handle *, char *, short, short)> xxusbBulkRead;
  //  short __stdcall xxusb_bulk_write(usb_dev_handle *hDev, char *DataBuffer, short lDataLen, int timeout);
  boost::function<short(usb_dev_handle *, char *, short, short)> xxusbBulkWrite;
  //  short __stdcall xxusb_reset_toggle(usb_dev_handle *hDev);
  boost::function<short(usb_dev_handle *)> xxusbResetToggle;
  //  short __stdcall xxusb_flash_program(usb_dev_handle *hDev, char *config, short nsect);
  boost::function<short(usb_dev_handle *, char *, short)> xxusbFlashProgram;
  //  short __stdcall xxusb_flashblock_program(usb_dev_handle *hDev, UCHAR *config);
  boost::function<short(usb_dev_handle *, UCHAR*)> xxusbFlashBlockProgram;



  //  short __stdcall VME_register_read(usb_dev_handle *hdev, long VME_Address, long *Data);
  boost::function<short(usb_dev_handle *, long, long *)> vmeRegisterRead;
  //  short __stdcall VME_register_write(usb_dev_handle *hdev, long VME_Address, long Data);
  boost::function<short(usb_dev_handle *, long, long)> vmeRegisterWrite;
  //  short __stdcall VME_DGG(usb_dev_handle *hdev, unsigned short channel, unsigned short trigger,unsigned short output, long delay, unsigned short gate, unsigned short invert, unsigned short latch);
  boost::function<short(usb_dev_handle *, unsigned short, unsigned short, unsigned short, long, unsigned short, unsigned short, unsigned short)> vmeDGG;
  //  short __stdcall VME_LED_settings(usb_dev_handle *hdev, int LED, int code, int invert, int latch);
  boost::function<short(usb_dev_handle *, int, int, int, int)> vmeLED;
  //  short __stdcall VME_Output_settings(usb_dev_handle *hdev, int Channel, int code, int invert, int latch);
  boost::function<short(usb_dev_handle *, int, int, int, int)> vmeOutputSettings;
  //  short __stdcall VME_scaler_settings(usb_dev_handle *hdev, short Channel, short trigger, int enable, int reset);
  boost::function<short(usb_dev_handle *, short, short, int, int)> vmeScalerSettings;
  //  short __stdcall VME_read_16(usb_dev_handle *hdev,short Address_Modifier, long VME_Address, long *Data);
  boost::function<short(usb_dev_handle *, short, long, long *)> vmeRead16;
  //  short __stdcall VME_write_16(usb_dev_handle *hdev, short Address_Modifier, long VME_Address, long Data);
  boost::function<short(usb_dev_handle *, short, long, long)> vmeWrite16;
  //  short __stdcall VME_read_32(usb_dev_handle *hdev, short Address_Modifier, long VME_Address, long *Data);
  boost::function<short(usb_dev_handle *, short, long, long*)> vmeRead32;
  //  short __stdcall VME_write_32(usb_dev_handle *hdev, short Address_Modifier, long VME_Address, long Data);
  boost::function<short(usb_dev_handle *, short, long, long)> vmeWrite32;
  //  short __stdcall VME_BLT_read_32(usb_dev_handle *hdev, short Address_Modifier, int count, long VME_Address, long Data[]);
  boost::function<short(usb_dev_handle *, short, int, long, long[])> vmeReadBLT32;

};

#endif // VMUSB_H
