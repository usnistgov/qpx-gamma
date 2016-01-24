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

#include "vmusb_stack.h"
#include <iostream>
#include <sstream>
using namespace std;

// The following are bits in the mode word that leads off each stack line.
//

static const int modeAMMask(0x3f); // Address modifier bits.
static const int modeAMShift(0);

static const int modeDSMask(0xc0);
static const int modeDSShift(6);

static const int modeNW(0x100);
static const int modeNA(0x400);
static const int modeMB(0x800);
static const int modeSLF(0x1000);
static const int modeMarker(0x2000);
static const int modeDelay(0x8000); 
static const int modeBE(0x10000);
static const int modeHD(0x20000);
static const int modeND(0x40000);
static const int modeHM(0x80000);
static const int modeNTMask(0xc00000);
static const int modeNTShift(22);
static const int modeBLTMask(0xf0000000);
static const int modeBLTShift(24);



// The following bit must be set in the address stack line for non long
// word transfers:

static const int addrNotLong(1);

/////////////////////////////////////////////////////////////////
//  Constructors and canonicals.


VmUsbStack&
VmUsbStack::operator=(const VmUsbStack& rhs)
{
  if (this != &rhs) {
    m_list = rhs.m_list;
  }
  return *this;
}
/*!
   Comparison is based on  comparison of the list contents.
*/
int
VmUsbStack::operator==(const VmUsbStack& rhs) const
{
  return m_list == rhs.m_list;
}
int
VmUsbStack::operator!=(const VmUsbStack& rhs) const
{
  return !(*this == rhs);
}

////////////////////////////////////////////////////////////////////
//    Operations on the whole of the list:

/*!
   Clear the list.
*/
void
VmUsbStack::clear()
{
  m_list.clear();
}
/*!
   return the size of the list in longwords.
*/
size_t
VmUsbStack::size() const
{
  return m_list.size();
}
/*!
     Return a copy of the list itself:
*/

vector<uint32_t>
VmUsbStack::get() const
{
  return m_list;
}

/*!
  Append the contents of another readoutlist to this one
*/
void VmUsbStack::append(const VmUsbStack& list)
{
  const std::vector<uint32_t>& other = list.get();
  m_list.insert(m_list.end(), other.begin(), other.end());
}

/////////////////////////////////////////////////////////////////////
//  register operations.

/*!
    Add a register read to a stack.  Register reads are just like
    single shot 32 bit reads but the SLF bit must be set in the
    mode line.  I am assuming that we don't need stuff like the
    DS bits either, as those are significant only for VME bus.
    address  : unsigned int
       Register address, see 3.4 of the VM-USB manual for a table
       of addresses.
*/
void
VmUsbStack::addRegisterRead(unsigned int address)
{
  uint32_t mode = modeNW  | modeSLF;
  m_list.push_back(mode);
  m_list.push_back(address);

}
/*!
   Add a register write to a stack. Register writes are the same
   as register reads except:
   - There is a data longword that follows the address.
   - The mode word does not have modeNW set.

    address : unsigned int
       Address of the register.  See VM-USB manual section 3.4 for
       a table of addresses.
    data    : uint32_t
      The data to write.  Note that some registers are only 16 bits
      wide.  In these cases, set the low order 16 bits of the data.
*/
void
VmUsbStack::addRegisterWrite(unsigned int address,
                             uint32_t     data)
{
  uint32_t mode = modeSLF;
  m_list.push_back(mode);
  m_list.push_back(address);
  m_list.push_back(data);

}


////////////////////////////////////////////////////////////////////////////////
// 
// Single shot VME write operations:

/*!
   Add a single 32 bit write to the list.  Any legitimate non-block mode
   address modifier is acceptable.   This version has the following restrictions:
   - No checking for suitability of the address modifier is done.  E.g. doing a
     MBLT64 transfer amod is allowed although will probably not work as expected
     when the list is executed.
   - The address must be longword aligned, however this is not checked.
     Instead, the address will have the bottom 2 bits set to 0.
     The VM-USB does not support marshalling the longword into an appropriate
     multi-transfer UAT, and neither do we.

    address : uint32_t
       The address to which the data are transferred. It is the caller's
       responsibility to ensure that this is longword aligned.
    amod   : uint8_t
       The 6 bit address modifier for the transfer.  Extraneous upper bits
       will be silently masked off.
     datum : uint32_t
       The data to transfer to the DTB slave.
*/
void
VmUsbStack::addWrite32(uint32_t address, AddressModifier amod, uint32_t datum)
{
  // First we need to build up the stack transfer mode longword:

  uint32_t mode = (static_cast<uint32_t>(amod) << modeAMShift) & modeAMMask;
  m_list.push_back(mode);

  // Now the address and data.. the LWORD* bit will not be set in the address

  m_list.push_back(address & 0xfffffffc); // The longword aligned address.
  m_list.push_back(datum);	          // data to write.
}
/*!
   Add a single 16 bit word write to the list.  Any legitimate non-block mode
   address modifier is acceptalbe.   This version has the following restrictions:
   - No checking for address modifier suitability is done.
   - The address must be word aligned.  The bottom bit of the address will be
     silently zeroed if this is not the case.

    address : uint32_t
       Address to which the data are transferred.
    amod : uint8_t
       The 6 bit address modifier for the transfer.  Extraneous bits are
       silently masked off.
    datum : uint16_t
       The data word to transfer.
*/
void
VmUsbStack::addWrite16(uint32_t address, AddressModifier amod, uint16_t datum)
{
  // Build up the mode word... no need to diddle with DS0/DS1 yet.

  uint32_t mode = (static_cast<uint32_t>(amod) << modeAMShift) & modeAMMask;
  m_list.push_back(mode);

  // Now the address and data.  The thing that characterizes a non-longword
  // transfer is that the LWORD* bit must be set in the address.
  // Both data strobes firing is what makes this a word transfer as opposed
  // to a byte transfer (see addWrite8 below).

  m_list.push_back((address & 0xfffffffe) | addrNotLong);
  m_list.push_back(static_cast<uint32_t>(datum));

}
/*!
   Add a single 8 bit write to the list.
   - No checking for the address modifier is done, and any extraneous
     bits in it are silently discarded.

  Note that since this is a byte write, no alignment restrictions apply.
  The data strobes that fire reflect the byte number.
   address : uint32_t
      The address to which the data will be written.
   amod : uint8_t
      The 6 bit address modifier code.
   datum : uint8_t
      The byte to write.
*/
void
VmUsbStack::addWrite8(uint32_t address, AddressModifier amod, uint8_t datum)
{
  // The data strobes depend on the bottom 2 bits of the address.
  // for an even address, DS1 is disabled.
  // for an odd address, DS0 is disabled.

  uint32_t laddr = (address & 0xfffffffe) | addrNotLong;
  uint32_t mode  = (static_cast<uint32_t>(amod) << modeAMShift) & modeAMMask;
  mode |= dataStrobes(address);

  m_list.push_back(mode);
  m_list.push_back(laddr);

  // The claim is that the data must be shifted to the correct lane.
  // some old code I have does not do this.. What I'm going to do so I
  // don't have to think too hard about whether or not this is correct
  // is to put the data byte on both D0-D7 and D8-D15, so it does not
  // matter if
  // - an even byte is being written, or odd
  // - the data has to or does not have to be in the appropriate data lanes.
  //

  uint32_t datum16 = (static_cast<uint32_t>(datum)); // D0-D7.
  datum16         |= (datum16 << 8);               // D8-D15.

  m_list.push_back(datum16);
  
}

////////////////////////////////////////////////////////////////////////////////
//
// Single shot VME Read operations.

/*!
   Add a 32 bit read to the list.  Pretty much like addWrite32, but there is no
   datum to transfer:
   - Address modifier is not checked for validity and extraneous bits are
     silently removed.
   - The transfer must be longword aligned, but is not error checked for that.
    address : uint32_t
      Address to which to transfer the data.
    amod   : uint8_t
      The address modifier to be associated with the transfer.
*/
void
VmUsbStack::addRead32(uint32_t address, AddressModifier amod)
{
  uint32_t mode = modeNW | ((static_cast<uint32_t>(amod) << modeAMShift) & modeAMMask);
  m_list.push_back(mode);
  m_list.push_back(address & 0xfffffffc);
}
/*!
   Add a 16 bit read to the list.  Pretty much like addWrite16, but there is no
   datum to transfer:
   - The address modifier is not checked for validity, and extraneous bits are
     silently discarded.
   - Data must be word aligned, but this is not checked for validity.
    address : uint32_t
      The transfer address.
    amod : uint8_t
      Address modifier gated on the bus for the transfer.
*/
void 
VmUsbStack::addRead16(uint32_t address, AddressModifier amod)
{
  uint32_t mode = modeNW | ((static_cast<uint32_t>(amod) << modeAMShift) & modeAMMask);
  m_list.push_back(mode);
  m_list.push_back((address & 0xfffffffe) | addrNotLong);
}
/*!
   Add an 8 bit read to the list.  Note that at this time, I am not 100%
   sure which data lanes will hold the result.  The VME spec is
   contradictory between the spec and the examples, I \em think,
   even bytes get returned in D0-D7, while odd bytes in D8-D15.
   When this is thoroughly tested, this comment must be fixed.
    address : uint32_t
      The transfer address
    amod : uint8_t
      The address modifier. No legality checking is done, and any
      extraneous bits are silently discarded.
*/
void
VmUsbStack::addRead8(uint32_t address, AddressModifier amod)
{
  uint32_t mode = modeNW | ((static_cast<uint32_t>(amod) << modeAMShift) & modeAMMask);
  mode         |= dataStrobes(address);
  m_list.push_back(mode);
  m_list.push_back((address & 0xfffffffe) | addrNotLong);
}

//////////////////////////////////////////////////////////////////////////////////
//
// Block transfer operations.
//  

/*!
   Add a 32 bit block read to the list.  There are several requirements
   in this version for simplicity.  None of these requirements are actively
   enforced.
   -  The base address must be longword aligned.
   - The address modifier must be one of the block transfer modes e.g.
     VmUsbStack::a32UserBlock

   This operation may generate more than one stack transaction, an initial
   one of complete 256 byte blocks in MB mode, and a final partial block
   transfer if necessary.
    baseAddress : uint32_t
      The addreess from which the first transfer occurs.
    amod : uint8_t
      The address modifier for this transfer.  Should be one of the block
      mode modifiers.
    transfers : size_t
      Number of \em longwords to transfer.
*/
void
VmUsbStack::addBlockRead32(uint32_t baseAddress, AddressModifier amod,
                           size_t transfers)
{
  addBlockRead(baseAddress,  transfers,
               static_cast<uint32_t>(((static_cast<uint8_t>(amod) << modeAMShift) & modeAMMask) | modeNW));
}

/*!
  Add a 32 block >write< to the list.
  - The base address must be longword aligned.
  - The address modifier must be one of the block transfer mode.
  - There must be at least 2 transfers specified. Using this to transfer 1 word will fail.
   baseAddress - Base of the target block.
   amod        - address modifier.
   data        - Data to transfer.
   transfers   - Number of transfers to perform.
*/
void
VmUsbStack::addBlockWrite32(uint32_t baseAddress, AddressModifier amod,
                            void* data, size_t transfers)
{

  // full universal MBLT -- doesn't seem to work
  uint32_t mode = (static_cast<uint32_t>(amod) << modeAMShift) & modeAMMask;
  mode         |= (0xff<<24);

  m_list.push_back(mode);
  m_list.push_back(transfers);
  m_list.push_back(baseAddress);
  
  // Put the data in the list too:
  uint32_t* src = reinterpret_cast<uint32_t*>(data);
  m_list.insert(m_list.end(), src, src+transfers);
  
}

/*!
   Add a read from a fifo.  This is identical to addBlockRead32, however
   the NA bit is set in the initial mode word to ensure that the actual
   address is not incremented.
*/
void
VmUsbStack::addFifoRead32(uint32_t address, AddressModifier amod, size_t transfers)
{
  addBlockRead(address,  transfers, static_cast<uint32_t>(modeNA |
                                                          ((static_cast<uint8_t>(amod) << modeAMShift) & modeAMMask) |
                                                          modeNW));
}
// 16 bit version:

void
VmUsbStack::addFifoRead16(uint32_t address, AddressModifier amod, size_t transfers)
{
  addBlockRead(address,  transfers, static_cast<uint32_t>(modeNA |
                                                          ((static_cast<uint8_t>(amod) << modeAMShift) & modeAMMask) |
                                                          modeNW), sizeof(uint16_t));
}
///////////////////////////////////////////////////////////////////////////////////
//
// Private utility functions:

// Figure out the data strobes for a byte address:

uint32_t
VmUsbStack::dataStrobes(uint32_t address)
{
  uint32_t dstrobes;
  if (address & 1) {
    // odd address:

    dstrobes = 1;
  }
  else {
    dstrobes = 2;
  }
  return (dstrobes << modeDSShift);
}
// Add an block transfer.  This is common code for both addBlockRead32
// and addFifoRead32.. The idea is that we get a starting mode word to work
// with, within that mode we will fill in things like the BLT field, and the
// MB bit as needed.
//
void
VmUsbStack::addBlockRead(uint32_t base, size_t transfers,
                         uint32_t startingMode,
                         size_t   width)
{

  bool notlong = width != sizeof(uint32_t); // need to set addrNotLong in addres

  // There are several nasty edge cases cases to deal with.
  // If the base address is not block aligned, a partial transfer
  // of size min(remaining_blocksize, transfers) must first be dnoe.


  if ((base & 0xff) != 0) {
    size_t aligningTransfers = (base - (base & 0xff))/width;
    if (transfers < aligningTransfers) aligningTransfers = transfers;
    uint32_t mode  = startingMode;
    mode          |= (aligningTransfers) << modeBLTShift;
    m_list.push_back(mode);
    m_list.push_back(base | (notlong ? addrNotLong : 0));

    base      += aligningTransfers * width; //  This should align the base.
    transfers -= aligningTransfers;

  }
  if (transfers == 0) return;	// itty bitty  unaligned xfer or masked count.
  

  // There are two cases, remaining transfers are larger than a block,
  // or transfers are le a block.
  // The first case requires an MB and possibly a single BLT transfer.
  // the secod just a BLT...
  // The maximum number of transfers in a block is 256/width
  // by now the transfer  base address 'base' is block justified.:

  // base &= 0xffffff00;
  size_t  fullBlocks   = transfers/(256/width);
  size_t  partialBlock = transfers % (256/width);

  if (fullBlocks) {		// Multiblock transfer...
    uint32_t mode = startingMode;
    mode         |= modeMB;	// Multiblock transfer.
    mode         |= (256/width) << modeBLTShift; // Full block to xfer.
    m_list.push_back(mode);
    m_list.push_back(fullBlocks);
    m_list.push_back(base  | (notlong ? addrNotLong : 0));

    if ((startingMode & modeNA) == 0 ) {
      base += fullBlocks * 256;	// Adjust to end of MBLT if not FIFO read.
    }

    
  }
  if (partialBlock) {
    uint32_t   mode = startingMode;
    mode           |= (partialBlock) << modeBLTShift;
    m_list.push_back(mode);
    m_list.push_back(base  | (notlong ? addrNotLong : 0));
  }

}

/*!
   Add a delay in stack execution
    clocks : uint8_t
    The number of 200ns clocks to delay the  stack execution for.
*/
void 
VmUsbStack::addDelay(uint8_t clocks)
{
  uint32_t line = modeDelay | clocks;
  m_list.push_back(line);	// Add the delay to the stack.

}
/*!
  Add a marker word to the output buffer.   Marker words are just
  16 bits of literal data.
   uint16_t value    - the value to put in the buffer.
*/
void 
VmUsbStack::addMarker(uint16_t value)
{
  uint32_t line = modeMarker;
  m_list.push_back(line);
  m_list.push_back(static_cast<uint32_t>(value));

}

//////////////////////////////////////////////////////////////////////////

/* Variable block setup and readout instructions: */



/*!
   Add an 8 bit transfer that reads the number data for a variable
   length transfer:
    address (uint32_t) - Address from which the byte  is read.
    mask : uint32_t
      count extraction mask.
    amod    (uint8_t)  - Address modifier of the read..

*/
void
VmUsbStack::addBlockCountRead8(uint32_t address, uint32_t mask, AddressModifier amod)
{
  addRead8(address, amod);
  lastTransferIsNumberData(mask);
}
/*!
   Add a 16 bit transfer that reads number data for a variable length
   transfer:
    address (uint32_t) - VME address from which the 16 bit word is read.
    mask : uint32_t
      count extraction mask.
    amod    (uint8_t)  - address modifier.

*/
void 
VmUsbStack::addBlockCountRead16(uint32_t address, uint32_t mask, AddressModifier amod)
{
  addRead16(address, amod);
  lastTransferIsNumberData(mask);
}
/*!
   Add a 32 bit tranfer that reads number data for a variable length
   transfer:
    address (uint32_t)  - Vme address from which the 32 bit long is read.
    mask : uint32_t
      count extraction mask.
    amod    (uint8_t)   - address modifier.
*/
void
VmUsbStack::addBlockCountRead32(uint32_t address, uint32_t mask,  AddressModifier amod)
{
  addRead32(address, amod);
  lastTransferIsNumberData(mask);
}

/*!
   Add a variable length block transfer.  The length of this transfer is
   determined by the number data last read and the current value of the
   Number extract mask register. The stack setup is for a normal block transfer
   followed by the mask that specifies the bit field that contains the transfer count.


    address (uint32_t) Address of the transfer.
    amod    (uint8_t_) address modifier to use for the transfers.
*/
void
VmUsbStack::addMaskedCountBlockRead32(uint32_t address, AddressModifier amod)
{
  addBlockRead32(address, amod, (size_t)1); // Actual count comes from ND and mask.
  // nonzero is required to ensure stack words actually
  // get generated
}
/*!
   Add a variable length block transfer from a FIFO. The list must contain a prior read of
   the number data (e.g. addBlockCountReadxx) prior to this with no intervening block transfers
    address(uint32_t) address of the fifo.
    amod   (uint8_t)  transfer address modifier.

*/
void
VmUsbStack::addMaskedCountFifoRead32(uint32_t address, AddressModifier amod)
{
  addFifoRead32(address, amod, (size_t)1);  // Actual count comes from ND and mask.
  // nonzero is required to ensure stack words actually
  // get generated
}


/*
 * Utility function used to turn a single shot read in the list into
 * a number data read.  The last two words in the stack are assumed to describe
 * the transfer.  The mode word will get the modeND bit set,
 * The mask word will be inserted between the mode and the address.
 */
void
VmUsbStack::lastTransferIsNumberData(uint32_t mask)
{
  size_t modeIndex = m_list.size() - 2;
  m_list[modeIndex] |= modeND;
  modeIndex++;             // Points to the address.
  uint32_t address = m_list[modeIndex];
  m_list[modeIndex] = mask;

  m_list.push_back(address);
}


std::string VmUsbStack::to_string()
{
  std::stringstream str;
  str << "---- Stack Dump: " << m_list.size() << " 32 bit elements ----\n";
  str << hex;
  for (int i=0; i < m_list.size(); i++) {
    str << m_list[i] << endl;
  }
  str << dec;
  return str.str();
}


//  Utility to create a stack from a transfer address word and
//  a CVMUSBReadoutList and an optional list offset (for non VCG lists).
//  Parameters:
//     uint16_t ta               The transfer address word.
//     CVMUSBReadoutList& list:  The list of operations to create a stack from.
//     size_t* outSize:          Pointer to be filled in with the final out packet size
//     off_t   offset:           If VCG bit is clear and VCS is set, the bottom
//                               16 bits of this are put in as the stack load
//                               offset. Otherwise, this is ignored and
//                               the list lize is treated as a 32 bit value.
//  Returns:
//     A uint16_t* for the list. The result is dynamically allocated
//     and must be released via delete []p e.g.
//
std::vector<uint16_t> VmUsbStack::packet(uint16_t ta, off_t offset) const
{
  int listLongwords = m_list.size();
  int listShorts    = listLongwords*sizeof(uint32_t)/sizeof(uint16_t);
  int packetShorts    = (listShorts + 3);
  std::vector<uint16_t> outPacket(packetShorts, 0);
  uint16_t* p         = outPacket.data();

  // Fill the outpacket:

  p = static_cast<uint16_t*>(addToPacket16(p, ta));
  //
  // The next two words depend on which bits are set in the ta
  //
  if(ta & TAVcsIMMED) {
    p = static_cast<uint16_t*>(addToPacket32(p, listShorts+1)); // 32 bit size.
  }
  else {
    p = static_cast<uint16_t*>(addToPacket16(p, listShorts+1)); // 16 bits only.
    p = static_cast<uint16_t*>(addToPacket16(p, offset));       // list load offset.
  }

  for (int i = 0; i < listLongwords; i++)
    p = static_cast<uint16_t*>(addToPacket32(p, m_list[i]));

  return outPacket;
}


//   Build up a packet by adding a 16 bit word to it;
//   the datum is packed low endianly into the packet.
void* VmUsbStack::addToPacket16(void* packet, uint16_t datum)
{
  uint8_t* pPacket = static_cast<uint8_t*>(packet);

  *pPacket++ = (datum  & 0xff); // Low byte first...
  *pPacket++ = (datum >> 8) & 0xff; // then high byte.

  return static_cast<void*>(pPacket);
}


//  Build up a packet by adding a 32 bit datum to it.
//  The datum is added low-endianly to the packet.
void* VmUsbStack::addToPacket32(void* packet, uint32_t datum)
{
  uint8_t* pPacket = static_cast<uint8_t*>(packet);

  *pPacket++    = (datum & 0xff);
  *pPacket++    = (datum >> 8) & 0xff;
  *pPacket++    = (datum >> 16) & 0xff;
  *pPacket++    = (datum >> 24) & 0xff;

  return static_cast<void*>(pPacket);
}


//    Retrieve a 16 bit value from a packet; packet is little endian
//    by usb definition. datum will be retrieved in host byte order.
void* VmUsbStack::getFromPacket16(void* packet, uint16_t* datum)
{
  uint8_t* pPacket = static_cast<uint8_t*>(packet);

  uint16_t low = *pPacket++;
  uint16_t high= *pPacket++;

  *datum =  (low | (high << 8));

  return static_cast<void*>(pPacket);

}

//   Same as above but a 32 bit item is returned.
void* VmUsbStack::getFromPacket32(void* packet, uint32_t* datum)
{
  uint8_t* pPacket = static_cast<uint8_t*>(packet);

  uint32_t b0  = *pPacket++;
  uint32_t b1  = *pPacket++;
  uint32_t b2  = *pPacket++;
  uint32_t b3  = *pPacket++;

  *datum = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);


  return static_cast<void*>(pPacket);
}
