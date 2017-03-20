#define EXPORT extern "C" _declspec(dllexport)

#define XXUSB_WIENER_VENDOR_ID	0x16DC   /* Wiener, Plein & Baus */
#define XXUSB_VMUSB_PRODUCT_ID	0x000B	 /* VM-USB */
#define XXUSB_CCUSB_PRODUCT_ID	0x0001	 /* CC-USB */
#define XXUSB_ENDPOINT_OUT	2  /* Endpoint 2 Out*/
#define XXUSB_ENDPOINT_IN    0x86  /* Endpoint 6 In */
#define XXUSB_FIRMWARE_REGISTER 0
#define XXUSB_GLOBAL_REGISTER 1
#define XXUSB_ACTION_REGISTER 10
#define XXUSB_DELAYS_REGISTER 2
#define XXUSB_WATCHDOG_REGISTER 3
#define XXUSB_SELLEDA_REGISTER 6
#define XXUSB_SELNIM_REGISTER 7
#define XXUSB_SELLEDB_REGISTER 4
#define XXUSB_SERIAL_REGISTER 15
#define XXUSB_LAMMASK_REGISTER 8
#define XXUSB_LAM_REGISTER 12
#define XXUSB_READOUT_STACK 2
#define XXUSB_SCALER_STACK 3
#define XXUSB_NAF_DIRECT 12
#define LIBUSB_PATH_MAX 512
#define XXUSB_CC_NUMSEC 512
#define XXUSB_VM_NUMSEC 830

struct usb_dev_handle;
typedef struct usb_dev_handle usb_dev_handle;

struct usb_device_descriptor {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned short bcdUSB;
  unsigned char  bDeviceClass;
  unsigned char  bDeviceSubClass;
  unsigned char  bDeviceProtocol;
  unsigned char  bMaxPacketSize0;
  unsigned short idVendor;
  unsigned short idProduct;
  unsigned short bcdDevice;
  unsigned char  iManufacturer;
  unsigned char  iProduct;
  unsigned char  iSerialNumber;
  unsigned char  bNumConfigurations;
};

struct usb_device {
  struct usb_device *next, *prev;

  char filename[LIBUSB_PATH_MAX];

  struct usb_bus *bus;

  struct usb_device_descriptor descriptor;
  struct usb_config_descriptor *config;

  void *dev;		/* Darwin support */

  unsigned char devnum;

  unsigned char num_children;
  struct usb_device **children;
};

struct xxusb_device_typ
{
  struct usb_device *usbdev;
  char SerialString[7];
};


typedef struct xxusb_device_typ xxusb_device_type;

typedef struct usb_bus usb_busx;
typedef unsigned char UCHAR;

/*
short __stdcall xxusb_register_read(usb_dev_handle *hDev, short RegAddr, long *RegData);
short __stdcall xxusb_stack_read(usb_dev_handle *hDev, short StackAddr, long *StackData);
short __stdcall xxusb_stack_write(usb_dev_handle *hDev, short StackAddr, long *StackData);
short __stdcall xxusb_stack_execute(usb_dev_handle *hDev, long *StackData);
short __stdcall xxusb_register_write(usb_dev_handle *hDev, short RegAddr, long RegData);
short __stdcall xxusb_usbfifo_read(usb_dev_handle *hDev, long *DataBuffer, short lDataLen, int timeout);
short __stdcall xxusb_bulk_read(usb_dev_handle *hDev, char *DataBuffer, short lDataLen, int timeout);
short __stdcall xxusb_bulk_write(usb_dev_handle *hDev, char *DataBuffer, short lDataLen, int timeout);
short __stdcall xxusb_reset_toggle(usb_dev_handle *hDev);

short __stdcall xxusb_devices_find(xxusb_device_type *xxusbDev);
short __stdcall xxusb_device_close(usb_dev_handle *hDev);
usb_dev_handle* __stdcall xxusb_device_open(struct usb_device *dev);
short __stdcall xxusb_flash_program(usb_dev_handle *hDev, char *config, short nsect);
short __stdcall xxusb_flashblock_program(usb_dev_handle *hDev, UCHAR *config);
usb_dev_handle* __stdcall xxusb_serial_open(char *SerialString);

short __stdcall VME_register_write(usb_dev_handle *hdev, long VME_Address, long Data);
short __stdcall VME_register_read(usb_dev_handle *hdev, long VME_Address, long *Data);
short __stdcall VME_LED_settings(usb_dev_handle *hdev, int LED, int code, int invert, int latch);

short __stdcall VME_DGG(usb_dev_handle *hdev, unsigned short channel, unsigned short trigger,unsigned short output, long delay, unsigned short gate, unsigned short invert, unsigned short latch);

short __stdcall VME_Output_settings(usb_dev_handle *hdev, int Channel, int code, int invert, int latch);

short __stdcall VME_read_16(usb_dev_handle *hdev,short Address_Modifier, long VME_Address, long *Data);
short __stdcall VME_read_32(usb_dev_handle *hdev, short Address_Modifier, long VME_Address, long *Data);
short __stdcall VME_BLT_read_32(usb_dev_handle *hdev, short Address_Modifier, int count, long VME_Address, long Data[]);
short __stdcall VME_write_16(usb_dev_handle *hdev, short Address_Modifier, long VME_Address, long Data);
short __stdcall VME_write_32(usb_dev_handle *hdev, short Address_Modifier, long VME_Address, long Data);

short __stdcall CAMAC_DGG(usb_dev_handle *hdev, short channel, short trigger, short output, int delay, int gate, short invert, short latch);
short __stdcall CAMAC_register_read(usb_dev_handle *hdev, int A, long *Data);
short __stdcall CAMAC_register_write(usb_dev_handle *hdev, int A, long Data);
short __stdcall CAMAC_LED_settings(usb_dev_handle *hdev, int LED, int code, int invert, int latch);
short __stdcall CAMAC_Output_settings(usb_dev_handle *hdev, int Channel, int code, int invert, int latch);
short __stdcall CAMAC_read_LAM_mask(usb_dev_handle *hdev, long *Data);
short __stdcall CAMAC_write_LAM_mask(usb_dev_handle *hdev, long Data);

short __stdcall CAMAC_write(usb_dev_handle *hdev, int N, int A, int F, long Data, int *Q, int *X);
short __stdcall CAMAC_read(usb_dev_handle *hdev, int N, int A, int F, long *Data, int *Q, int *X);
short __stdcall CAMAC_Z(usb_dev_handle *hdev);
short __stdcall CAMAC_C(usb_dev_handle *hdev);
short __stdcall CAMAC_I(usb_dev_handle *hdev, int inhibit); 
*/
