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

#include "madc32.h"


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
                      unsigned char vector, unsigned char slot) {
  long Stack_read[1000];
  unsigned int reg;
  unsigned char stack_addr;
  long mask, Data = 0;
  int ret, i, shift;

  printf("Setup slot %d with irq %d and vector %x\n", slot, irq, vector);

  if ((irq == 0) && (slot > 1)) {
    printf("Only the NIM1 and scaler triggers can operate without an interrupt\n");
    return -1;
  }

  for (i = 0; i < 100; i++)
    Stack_read[i] = 0;

  switch(slot) {
  case 0:
    stack_addr = XXUSB_STACK_SLOT0;
    reg = XXUSB_ISV12_REGISTER;
    shift = 0;
    mask = 0xffff0000;
    break;
  case 1:
    stack_addr = XXUSB_STACK_SLOT1;
    reg = XXUSB_ISV12_REGISTER;
    shift = 16;
    mask = 0x0000ffff;
    break;
  case 2:
    stack_addr = XXUSB_STACK_SLOT2;
    reg = XXUSB_ISV34_REGISTER;
    shift = 0;
    mask = 0xffff0000;
    break;
  case 3:
    stack_addr = XXUSB_STACK_SLOT3;
    reg = XXUSB_ISV34_REGISTER;
    shift = 16;
    mask = 0x0000ffff;
    break;
  case 4:
    stack_addr = XXUSB_STACK_SLOT4;
    reg = XXUSB_ISV56_REGISTER;
    shift = 0;
    mask = 0xffff0000;
    break;
  case 5:
    stack_addr = XXUSB_STACK_SLOT5;
    reg = XXUSB_ISV56_REGISTER;
    shift = 16;
    mask = 0x0000ffff;
    break;
  case 6:
    stack_addr = XXUSB_STACK_SLOT6;
    reg = XXUSB_ISV78_REGISTER;
    shift = 0;
    mask = 0xffff0000;
    break;
  case 7:
    stack_addr = XXUSB_STACK_SLOT7;
    reg = XXUSB_ISV78_REGISTER;
    shift = 16;
    mask = 0x0000ffff;
    break;
  default:
    stack_addr = XXUSB_STACK_SLOT0;
    reg = XXUSB_ISV12_REGISTER;
    shift = 0;
    mask = 0xffff0000;
    break;
  }

  printf("Writing to stack address %x, register %x, and shift %x\n", stack_addr, reg, shift);

  // Put the Stack into the correct stack address
  ret = xxusb_stack_write(udev, stack_addr, Stack);

  // Read Stack back from VM_USB and compare
  xxusb_stack_read(udev, stack_addr, Stack_read);
  for(i = 0; i < (Stack[0] + 1); i++)
    if (Stack_read[i] != Stack[i])
      printf("Stack not written correctly for item %d: Write: %x Read: %x\n",
             i, Stack[i], Stack_read[i]);

  // Now set the interrupt and vector if one is chosen
  if (irq > 0) {
    VME_register_read(udev, reg, &Data);
    Data &= mask;
    Data |= (((slot << 12) | (irq << 8) | vector) << shift);
    VME_register_write(udev, reg, Data);
    VME_register_read(udev, reg, &Data);
    printf("Interrupt service vector = %x in register %x\n", Data, reg);

    // These must be set to 0x0000 for the CAEN, but I have no idea what they are
    //        VME_register_write(udev, XXUSB_ISV_HIGH_1_REGISTER, 0x0000);
    //        VME_register_write(udev, XXUSB_ISV_HIGH_2_REGISTER, 0x0000);
  }
  return 0;
}
void print_xxusb(usb_dev_handle *udev){
  long Data;

  printf("ID\t\tMODE\tDAQ\tLED\tDEV\tDGGA\tDGGB\tDGGX\n");
  VME_register_read(udev, XXUSB_FIRMWARE_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_GLOBAL_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_DAQ_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_LED_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_DEV_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_DGGA_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_DGGB_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_DGGEXT_REGISTER, &Data);
  printf("0x%x\n", Data);
  printf("SCLA\tSCLB\tMASK\tISV12\tISV34\tISV56\tISV78\tUSB\tISVH1\tISVH2\n");
  VME_register_read(udev, XXUSB_SCLRA_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_SCLRB_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_MASK_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_ISV12_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_ISV34_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_ISV56_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_ISV78_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, XXUSB_USB_BULK_REGISTER, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, 0x40, &Data);
  printf("0x%x\t", Data);
  VME_register_read(udev, 0x44, &Data);
  printf("0x%x\n", Data);
}

/*Prints out the settings of the MADC-32 module*/
void print_madc(usb_dev_handle *udev) {
  long Data;

  printf("ADDR\tIRQ\tVEC\tLEN\tDATA\n");
  printf("0x%x\t", MADC_BADDR);
  VME_read_16(udev, AM_MADC, MADC_ADDR(MADC_IRQ_LEVEL), &Data);
  printf("0x%x\t", Data);
  VME_read_16(udev, AM_MADC, MADC_ADDR(MADC_IRQ_VEC), &Data);
  printf("0x%x\t", Data);
  VME_read_16(udev, AM_MADC, MADC_ADDR(MADC_DATA_LEN), &Data);
  printf("0x%x\t", Data);
  VME_read_16(udev, AM_MADC, MADC_ADDR(MADC_DATA_RDY), &Data);
  printf("0x%x\t", (Data & 0x0001));
  printf("\n");
}

//**********************************************************************
/*Sets up the two stacks for data acquisition */
void set_stack(struct usb_dev_handle *udev, unsigned char irq, 
               unsigned char vector, unsigned char slot){
  long Stack[STACK_LEN];
  int i, len = 0;

  // build VM-USB stack with flexible data size
  len = 0;
  Stack[0] = 0x0009; // number of lines after this
  Stack[1] = 0x0000; // address of place to store in stack memory
  Stack[2] = 0x010b;	//BLT
  Stack[3] = 0x2200;	// number of cycles = 34 reads header + 32 data + EOB
  Stack[4] = 0x0000;	// lower bits of address
  Stack[5] = (MADC_BADDR & 0xffff0000) >> 16; // 16-32 bit portion of address = 0x4000 for BADR 4000000
  Stack[6] = 0x0209; //write marker
  Stack[7] = 0x0000; //
  Stack[8] = 0xffff; //marker = oxffff to identify events in data stream
  Stack[9] = 0x0000;
  for (i = 0; i <= Stack[0]; i++) printf("Stack[%d] = 0x%x\n", i, Stack[i]);
  xxusb_stack_setup(udev, Stack, irq, vector, slot);
  //Set IRQ Level for IRQ mode
  //  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_IRQ_LEVEL), irq);
  //Set IRQ Vector for IRQ mode
  //  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_IRQ_VEC), vector);
}

//**********************************************************************
/*Configure the VM-USB module*/     
void set_vm_usb(struct usb_dev_handle *udev){
  long Data = 0;

  // Read the VM-USB firmware version
  VME_register_read(udev, XXUSB_FIRMWARE_REGISTER, &Data);
  printf("Firmware version of the VM-USB 0x%x\n", Data);

  // Set top yellow LED to light with infifo not empty
  VME_LED_settings(udev, 0, 1, 0, 0);
  // Set Red LED to light with NIM1
  VME_LED_settings(udev, 1, 1, 0, 0);
  // Set green LED to light with VME DTACK
  VME_LED_settings(udev, 2, 2, 0, 0);
  // Set bottom yellow LED to light with VME BERR
  VME_LED_settings(udev, 3, 5, 0, 0);
  // Set DGG channel B to trigger on NIM2, output on O1,
  //     with delay =500 x 12.5ns,
  //     and width =  500 x 12.5ns,
  //     not latching or inverting
  //  VME_DGG(udev, 0, 2, 0, 0, 0x200, 0, 0);

  //	0=13k, 1=8k, 2=4k, 5=512
  Data = 0;
  VME_write_32(udev,0x100D,4,Data);
  // 0 us delay before read due to trigger
  VME_register_write(udev, XXUSB_DAQ_REGISTER, 0);
}
