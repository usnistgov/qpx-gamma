#include <usb.h>

#ifndef libxxusb_h
#define libxxusb_h

#ifdef __cplusplus
extern "C" {
#endif

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

struct XXUSB_STACK
{
long Data;
short Hit;
short APatt;
short Num;
short HitMask;
};

struct XXUSB_CC_COMMAND_TYPE
{
short Crate;
short F;
short A;
short N;
long Data;
short NoS2;
short LongD;
short HitPatt;
short QStop;
short LAMMode;
short UseHit;
short Repeat;
short AddrScan;
short FastCam;
short NumMod;
short AddrPatt;
long HitMask[4];
long Num;
};

struct xxusb_device_typ
{
  struct usb_device *usbdev;
  char SerialString[7];
};

typedef struct xxusb_device_typ xxusb_device_type;
typedef unsigned char UCHAR;
typedef struct usb_bus usb_busx;


int xxusb_longstack_execute(usb_dev_handle *hDev, void *DataBuffer, int lDataLen, int timeout);
int xxusb_bulk_read(usb_dev_handle *hDev, void *DataBuffer, int lDataLen, int timeout);
int xxusb_bulk_write(usb_dev_handle *hDev, void *DataBuffer, int lDataLen, int timeout);
int xxusb_usbfifo_read(usb_dev_handle *hDev, int *DataBuffer, int lDataLen, int timeout);

short xxusb_register_read(usb_dev_handle *hDev, short RegAddr, long *RegData);
short xxusb_stack_read(usb_dev_handle *hDev, short StackAddr, long *StackData);
short xxusb_stack_write(usb_dev_handle *hDev, short StackAddr, long *StackData);
short xxusb_stack_execute(usb_dev_handle *hDev, long *StackData);
short xxusb_register_write(usb_dev_handle *hDev, short RegAddr, long RegData);
short xxusb_reset_toggle(usb_dev_handle *hDev);

short xxusb_devices_find(xxusb_device_type *xxusbDev);
short xxusb_device_close(usb_dev_handle *hDev);
usb_dev_handle* xxusb_device_open(struct usb_device *dev);
short xxusb_flash_program(usb_dev_handle *hDev, char *config, short nsect);
short xxusb_flashblock_program(usb_dev_handle *hDev, UCHAR *config);
usb_dev_handle* xxusb_serial_open(char *SerialString);

short VME_register_write(usb_dev_handle *hdev, long VME_Address, long Data);
short VME_register_read(usb_dev_handle *hdev, long VME_Address, long *Data);
short VME_LED_settings(usb_dev_handle *hdev, int LED, int code, int invert, int latch);
short VME_DGG(usb_dev_handle *hdev, unsigned short channel, unsigned short trigger,unsigned short output, long delay, unsigned short gate, unsigned short invert, unsigned short latch);

short VME_Output_settings(usb_dev_handle *hdev, int Channel, int code, int invert, int latch);

short VME_read_16(usb_dev_handle *hdev,short Address_Modifier, long VME_Address, long *Data);
short VME_read_32(usb_dev_handle *hdev, short Address_Modifier, long VME_Address, long *Data);
short VME_BLT_read_32(usb_dev_handle *hdev, short Address_Modifier, int count, long VME_Address, long Data[]);
short VME_write_16(usb_dev_handle *hdev, short Address_Modifier, long VME_Address, long Data);
short VME_write_32(usb_dev_handle *hdev, short Address_Modifier, long VME_Address, long Data);

short CAMAC_DGG(usb_dev_handle *hdev, short channel, short trigger, short output, int delay, int gate, short invert, short latch);
short CAMAC_register_read(usb_dev_handle *hdev, int A, long *Data);
short CAMAC_register_write(usb_dev_handle *hdev, int A, long Data);
short CAMAC_LED_settings(usb_dev_handle *hdev, int LED, int code, int invert, int latch);
short CAMAC_Output_settings(usb_dev_handle *hdev, int Channel, int code, int invert, int latch);
short CAMAC_read_LAM_mask(usb_dev_handle *hdev, long *Data);
short CAMAC_write_LAM_mask(usb_dev_handle *hdev, long Data);

short CAMAC_write(usb_dev_handle *hdev, int N, int A, int F, long Data, int *Q, int *X);
short CAMAC_read(usb_dev_handle *hdev, int N, int A, int F, long *Data, int *Q, int *X);
short CAMAC_Z(usb_dev_handle *hdev);
short CAMAC_C(usb_dev_handle *hdev);
short CAMAC_I(usb_dev_handle *hdev, int inhibit); 

#ifdef __cplusplus
}
#endif

#endif /* libxxusb.h */
