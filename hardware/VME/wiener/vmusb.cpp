#include "vmusb.h"
#include "custom_logger.h"


VmUsb::VmUsb()
{
  PL_DBG << "Attempting to load xx_usb library";
  udev = NULL;

  boost::system::error_code ec;
  boost::shared_ptr<boost::dll::shared_library> xxlib = boost::make_shared<boost::dll::shared_library>();
  xxlib->load("xx_usb", boost::dll::load_mode::append_decorations, ec);

  if (xxlib->is_loaded()) {
    PL_DBG << "Library " << xxlib->location() << " loaded successfully";

    if (xxlib->has("xxusb_devices_find"))
      xxusbDevicesFind = boost::dll::import<short(xxusb_device_type *)> (xxlib, "xxusb_devices_find");
    else
      PL_ERR << "Failed to load function xxusb_devices_find";

    if (xxlib->has("xxusb_device_open"))
      xxusbDeviceOpen = boost::dll::import<usb_dev_handle*(struct usb_device *)> (xxlib, "xxusb_device_open");
    else
      PL_ERR << "Failed to load function xxusb_device_open";

    if (xxlib->has("xxusb_serial_open"))
      xxusbSerialOpen = boost::dll::import<usb_dev_handle*(char *)> (xxlib, "xxusb_serial_open");
    else
      PL_ERR << "Failed to load function xxusb_serial_open";

    if (xxlib->has("xxusb_device_close"))
      xxusbDeviceClose = boost::dll::import<short(usb_dev_handle *)> (xxlib, "xxusb_device_close");
    else
      PL_ERR << "Failed to load function xxusb_device_close";

    if (xxlib->has("xxusb_register_read"))
      xxusbRegisterRead = boost::dll::import<short(usb_dev_handle *, short, long *)> (xxlib, "xxusb_register_read");
    else
      PL_ERR << "Failed to load function xxusb_register_read";

    if (xxlib->has("xxusb_register_write"))
      xxusbRegisterWrite = boost::dll::import<short(usb_dev_handle *, short, long)> (xxlib, "xxusb_register_write");
    else
      PL_ERR << "Failed to load function xxusb_register_write";

    if (xxlib->has("xxusb_stack_read"))
      xxusbStackRead = boost::dll::import<short(usb_dev_handle *, short, long *)> (xxlib, "xxusb_stack_read");
    else
      PL_ERR << "Failed to load function xxusb_stack_read";

    if (xxlib->has("xxusb_stack_write"))
      xxusbStackWrite = boost::dll::import<short(usb_dev_handle *, short, long *)> (xxlib, "xxusb_stack_write");
    else
      PL_ERR << "Failed to load function xxusb_stack_write";

    if (xxlib->has("xxusb_stack_execute"))
      xxusbStackExecute = boost::dll::import<short(usb_dev_handle *, long *)> (xxlib, "xxusb_stack_execute");
    else
      PL_ERR << "Failed to load function xxusb_stack_execute";

    if (xxlib->has("xxusb_longstack_execute"))
      xxusbLongStackExecute = boost::dll::import<short(usb_dev_handle *, void *, short, short)> (xxlib, "xxusb_longstack_execute");
    else
      PL_ERR << "Failed to load function xxusb_longstack_execute";

    if (xxlib->has("xxusb_usbfifo_read"))
      xxusbUsbFifoRead = boost::dll::import<short(usb_dev_handle *, long *, short, short)> (xxlib, "xxusb_usbfifo_read");
    else
      PL_ERR << "Failed to load function xxusb_usbfifo_read";

    if (xxlib->has("xxusb_bulk_read"))
      xxusbBulkRead = boost::dll::import<short(usb_dev_handle *, char *, short, short)> (xxlib, "xxusb_bulk_read");
    else
      PL_ERR << "Failed to load function xxusb_bulk_read";

    if (xxlib->has("xxusb_bulk_write"))
      xxusbBulkWrite = boost::dll::import<short(usb_dev_handle *, char *, short, short)> (xxlib, "xxusb_bulk_write");
    else
      PL_ERR << "Failed to load function xxusb_bulk_write";

    if (xxlib->has("xxusb_reset_toggle"))
      xxusbResetToggle = boost::dll::import<short(usb_dev_handle *)> (xxlib, "xxusb_reset_toggle");
    else
      PL_ERR << "Failed to load function xxusb_reset_toggle";

    if (xxlib->has("xxusb_flash_program"))
      xxusbFlashProgram = boost::dll::import<short(usb_dev_handle *, char *, short)> (xxlib, "xxusb_flash_program");
    else
      PL_ERR << "Failed to load function xxusb_flash_program";

    if (xxlib->has("xxusb_flashblock_program"))
      xxusbFlashBlockProgram = boost::dll::import<short(usb_dev_handle *, UCHAR *)> (xxlib, "xxusb_flashblock_program");
    else
      PL_ERR << "Failed to load function xxusb_flashblock_program";




    if (xxlib->has("VME_register_read"))
      vmeRegisterRead = boost::dll::import<short(usb_dev_handle *, long, long *)> (xxlib, "VME_register_read");
    else
      PL_ERR << "Failed to load function VME_register_read";

    if (xxlib->has("VME_register_write"))
      vmeRegisterWrite = boost::dll::import<short(usb_dev_handle *, long, long)> (xxlib, "VME_register_write");
    else
      PL_ERR << "Failed to load function VME_register_write";

    if (xxlib->has("VME_DGG"))
      vmeDGG = boost::dll::import<short(usb_dev_handle *, unsigned short, unsigned short, unsigned short, long, unsigned short, unsigned short, unsigned short)> (xxlib, "VME_DGG");
    else
      PL_ERR << "Failed to load function VME_DGG";

    if (xxlib->has("VME_LED_settings"))
      vmeLED = boost::dll::import<short(usb_dev_handle *, int, int, int, int)> (xxlib, "VME_LED_settings");
    else
      PL_ERR << "Failed to load function VME_LED_settings";

    if (xxlib->has("VME_Output_settings"))
      vmeOutputSettings = boost::dll::import<short(usb_dev_handle *, int, int, int, int)> (xxlib, "VME_Output_settings");
    else
      PL_ERR << "Failed to load function VME_Output_settings";

    if (xxlib->has("VME_scaler_settings"))
      vmeScalerSettings = boost::dll::import<short(usb_dev_handle *, short, short, int, int)> (xxlib, "VME_scaler_settings");
    else
      PL_ERR << "Failed to load function VME_scaler_settings";

    if (xxlib->has("VME_read_16"))
      vmeRead16 = boost::dll::import<short(usb_dev_handle *, short, long, long *)> (xxlib, "VME_read_16");
    else
      PL_ERR << "Failed to load function VME_read_16";

    if (xxlib->has("VME_write_16"))
      vmeWrite16 = boost::dll::import<short(usb_dev_handle *, short, long, long)> (xxlib, "VME_write_16");
    else
      PL_ERR << "Failed to load function VME_write_16";

    if (xxlib->has("VME_read_32"))
      vmeRead32 = boost::dll::import<short(usb_dev_handle *, short, long, long*)> (xxlib, "VME_read_32");
    else
      PL_ERR << "Failed to load function VME_read_32";

    if (xxlib->has("VME_write_32"))
      vmeWrite32 = boost::dll::import<short(usb_dev_handle *, short, long, long)> (xxlib, "VME_write_32");
    else
      PL_ERR << "Failed to load function VME_write_32";

    if (xxlib->has("VME_BLT_read_32"))
      vmeReadBLT32 = boost::dll::import<short(usb_dev_handle *, short, int, long, long[])> (xxlib, "VME_BLT_read_32");
    else
      PL_ERR << "Failed to load function VME_BLT_read_32";

    if (
        !xxusbDevicesFind || !xxusbDeviceOpen || !xxusbSerialOpen || !xxusbDeviceClose ||
        !xxusbRegisterRead || !xxusbRegisterWrite || !xxusbStackRead || !xxusbStackWrite ||!xxusbStackExecute ||
        !xxusbUsbFifoRead || !xxusbBulkRead || !xxusbBulkWrite ||
        !vmeRegisterRead || !vmeRegisterWrite ||
        !vmeDGG || !vmeLED || !vmeOutputSettings || !vmeScalerSettings ||
        !vmeRead16 || !vmeWrite16 ||
        !vmeRead32 || !vmeWrite32 ||
        !vmeReadBLT32
        ) {

      m_errorString = "Error while loading shared library '" + xxlib->location().string() + "'";

      PL_ERR << "Error while functions from shared library '" << xxlib->location()
             << "'. Program will be terminated.";

      xxlib->unload();

      exit(1);
    }

    PL_DBG << "functions from xxusb library loaded successfully";

    xxusb_device_type devices[32]; // should be enough

    int deviceCount = xxusbDevicesFind(devices);

    int controllerNumber = 0;
    // no controllers found
    if (controllerNumber >= deviceCount) {
      m_errorString = "No VM-USB controller number " + std::to_string(controllerNumber) + " found!";
      return;
    }

    PL_DBG << "Found " << deviceCount << " devices";
    for (int i=0; i<deviceCount; ++i) {
      PL_DBG << "Device #" << i << "  s/n=" << devices[i].SerialString;
    }

    // Open the device
    udev = xxusbDeviceOpen(devices[controllerNumber].usbdev);

    m_serialNumber = std::string(devices[controllerNumber].SerialString);

    long fwrel;

    xxusbRegisterRead(udev, 0x00, &fwrel);
    PL_DBG << "Connected to VM-USB serial_nr=" << m_serialNumber << " firmware=" << fwrel;


  } else {
    PL_ERR << "Error while loading shared library.";
    PL_ERR << ec.message();

    m_errorString = ec.message();
  }
}

VmUsb::~VmUsb()
{
  if (udev) {
    xxusbDeviceClose(udev);
    udev = 0;
  }
}

//std::list<std::string> VmUsb::enumControllers(void)
//{
//  std::string controllerName;
//  std::list<std::string> controllerList;
//  xxusb_device_type devices[32]; // should be enough

//  if ( (udev) && (libxxusb->isLoaded())) {

//    // Find XX_USB devices and open the first one found
//    int deviceCount = xxusbDevicesFind(devices);

//    for (int i = 0; i < deviceCount; i++) {
//      controllerName = std::string(devices[i].SerialString);
//      controllerList.push_back(controllerName);
//    }
//  }

//  return controllerList;
//}

void VmUsb::writeShort(int vmeAddress, int addressModifier, int data, int *errorCode)
{
  int result = -1;

  if ((udev))
    result = vmeWrite16(udev, addressModifier, vmeAddress, data);

  if ((errorCode))
    *errorCode = result;
}

int VmUsb::readShort(int vmeAddress, int addressModifier, int *errorCode)
{
  long data = 0;
  int  result = -1;

  if ((udev))
    result = vmeRead16(udev, addressModifier, vmeAddress, &data);

  if ((errorCode))
    *errorCode = result;

  return data;
}


std::string VmUsb::errorString(void)
{
  return m_errorString;
}

std::string VmUsb::controllerName(void)
{
  return "Wiener VM-USB";
}

std::string VmUsb::information(void)
{
  std::string result;

  if (!udev)
    result = "Not connected to Wiener VME controller";
  else
    result = "Connected to Wiener VME controller " + m_serialNumber + ".";

  return result;
}

void VmUsb::systemReset(int *errorCode)
{
  //Q_UNUSED(errorCode);
  // not supported by VM-USB yet
}
