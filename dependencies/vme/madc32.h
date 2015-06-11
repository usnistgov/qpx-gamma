/* VM-USB Demo program Version 1.1
 * 
 * VM-usb_EASYVME library included, WIENER Plein & Baus - Andreas Ruben (aruben@wiener-us.com)
 *
 *     This program is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation, version 2.
 *
 * The program calls first usb_init(), usb_find_busses(), and usb_find_devices(),
 * passing relevant arguments to structures bus and dev.
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "libxxusb.h"
#include "registers.h"
#include </usr/include/sys/io.h> 

#include "addr_mesytec.h"
#include "mscf_reg.h"


//#define SCALER_MODE
#undef SCALER_MODE

//#define EXT_OSCILLATOR
#undef EXT_OSCILLATOR

//Maximum number of commands in a STACK
#define STACK_LEN 30

//The base address of the MADC-32
#define MADC_BADDR 0x20000000

#ifdef SCALER_MODE
#warning- SCALER MODE

//The base address of the Struck SIS-3820
//#define SIS3820_ADDRESS 0x38000000
#endif



/*
 ******** xxusb_stack_setup ************************

 Writes a stack into a given slot and sets up the
 appropriate interrupts.
 
 Parameters:
 hdev: USB device handle returned from an open function
 Stack: Values to write into the stack space
 irq: VME IRQ value to use for this stack.  0 disables interrupts
 vector: VME IRQ vector to use for this stack.
 slot: Stack slot to use (0-7) 0 is for NIM1 triggers and 1 for scaler triggers.
       They can also be reused with interrupts if needed.
 
 Returns:  0 on success and negative on error
 */
int xxusb_stack_setup(usb_dev_handle *udev, long Stack[], unsigned char irq, 
		      unsigned char vector, unsigned char slot);

void print_xxusb(usb_dev_handle *udev);

/*Prints out the settings of the MADC-32 module*/
void print_madc(usb_dev_handle *udev);

//**********************************************************************
/*Sets up the two stacks for data acquisition */
void set_stack(struct usb_dev_handle *udev, unsigned char irq, 
               unsigned char vector, unsigned char slot);

//**********************************************************************
/*Configure the VM-USB module*/     
void set_vm_usb(struct usb_dev_handle *udev);
