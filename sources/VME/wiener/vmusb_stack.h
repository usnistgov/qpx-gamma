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

#pragma once

#include <vector>
#include <stdint.h>
#include <sys/types.h>
#include <iostream>
#include "vmecontroller.h"
#include "vme_stack.h"


// Bits in the list target address word:

static const uint16_t TAVcsID0(1); // Bit mask of Stack id bit 0.
static const uint16_t TAVcsSel(2); // Bit mask to select list dnload
static const uint16_t TAVcsWrite(4); // Write bitmask.
static const uint16_t TAVcsIMMED(8); // Target the VCS immediately.
static const uint16_t TAVcsID1(0x10);
static const uint16_t TAVcsID2(0x20);
static const uint16_t TAVcsID12MASK(0x30); // Mask for top 2 id bits
static const uint16_t TAVcsID12SHIFT(4);


/*!
   The best way to use the VM-USB involves building lists of VME
   operations, called \em stacks.  These stacks can either be submitted
   for immediate execution or stored inside the VM-USB for triggered
   execution.  In this way, several events will be autonomously handled
   by the VM-USB with no computer intervention.

   This class allows application programs to painlessly build a stack.
   Stacks are built up by creating an instance of this class, 
   invoking menber functions  to add list elements, and then 
   passing the list to either CVMUSB::loadList or CVMUSB::executeList

   There is nothing sacred about a list with respect to copy construction,
   assignment, or comparison.  Those operations are simply delegated to 
   member data.

	MGS mod: eliminated SWIG junk, address modifier consts

*/

class VmUsbStack : public VmeStack
{
public:
  static void* addToPacket16(void* packet,   uint16_t datum);
  static void* addToPacket32(void* packet,   uint32_t datum);
  static void* getFromPacket16(void* packet, uint16_t* datum);
  static void* getFromPacket32(void* packet, uint32_t* datum);

private:
  std::vector<uint32_t> m_list; // Stack lines are all 32 bits wide.
public:
  VmUsbStack() {}
  ~VmUsbStack() {}
  
  VmUsbStack& operator=(const VmUsbStack& rhs);
  int operator==(const VmUsbStack& rhs) const;
  int operator!=(const VmUsbStack& rhs) const;
  
  
  // Operations on the list as a whole:
  
  virtual void            clear();
  virtual size_t          size() const;
  std::vector<uint32_t>   get() const;
  std::vector<uint16_t>   packet(uint16_t ta, off_t offset = 0) const;

  // Append readout list
  virtual void append(const VmUsbStack& list);

  // Register operations 
  
public:
  virtual void addRegisterRead(unsigned int address);
  virtual void addRegisterWrite(unsigned int address, uint32_t data);

    // Single shot VME operations.  Note that these are only supported
    // in natural alignments, as otherwise it is not so easy to let the
    // application know how to marshall the multiple transers appropriately.
    
public:
  // Writes:

  virtual void addWrite32(uint32_t address, AddressModifier amod, uint32_t datum);
  virtual void addWrite16(uint32_t address, AddressModifier amod, uint16_t datum);
  virtual void addWrite8(uint32_t address,  AddressModifier amod, uint8_t datum);

  // Reads:

  virtual void addRead32(uint32_t address, AddressModifier amod);
  virtual void addRead16(uint32_t address, AddressModifier amod);
  virtual void addRead8(uint32_t address, AddressModifier amod);

  // Block transfer operations. 
  // These must meet the restrictions of the VMUSB on block transfers.
  //
  virtual void addBlockRead32(uint32_t baseAddress, AddressModifier amod, size_t transfers);
  virtual void addFifoRead32(uint32_t  baseAddress, AddressModifier amod, size_t transfers);
  virtual void addFifoRead16(uint32_t baseAddress, AddressModifier amod, size_t transfers);
  virtual void addBlockWrite32(uint32_t baseAddresss, AddressModifier amod, void* data,
		       size_t transfers);


  // VMEUSB supports getting the block readout count from a constant mask and
  // a count read from a VME module.  This is supported in this software by
  // providing functions to set the mask from a list, 
  // to specify the readout of the count (8,16 or 32 bits).
  // and to initiate the block transfer:
  // as with all block reads, only 32 bit transfers are supported.
  //

  virtual void addBlockCountRead8(uint32_t  address, uint32_t mask, AddressModifier amod);
  virtual void addBlockCountRead16(uint32_t address, uint32_t mask, AddressModifier amod);
  virtual void addBlockCountRead32(uint32_t address, uint32_t mask, AddressModifier amod);
  virtual void addMaskedCountBlockRead32(uint32_t address, AddressModifier amod);
  virtual void addMaskedCountFifoRead32(uint32_t address, AddressModifier amod);


  // Miscellaneous:

  virtual void addDelay(uint8_t clocks);
  virtual void addMarker(uint16_t value);


  // Debugging:
  std::string to_string();	/* Dump contents of stack. */

private:
  uint32_t dataStrobes(uint32_t address);
  void     addBlockRead(uint32_t base, size_t transfers,
			uint32_t startingMode,
			size_t   width = sizeof(uint32_t));
  void     lastTransferIsNumberData(uint32_t mask);

};
