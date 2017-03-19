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

#include <usb.h>
#include "vmusb2.h"
#include "custom_logger.h"
#include <boost/utility/binary.hpp>
#include "custom_timer.h"
#include "qpx_util.h"

#define XXUSB_ACTION_STOP                  BOOST_BINARY(00000)
#define XXUSB_ACTION_START                 BOOST_BINARY(00001)
#define XXUSB_ACTION_USB_TRIGGER           BOOST_BINARY(00010)
#define XXUSB_ACTION_CLEAR                 BOOST_BINARY(00100)
#define XXUSB_ACTION_SYSRES                BOOST_BINARY(01000)
#define XXUSB_ACTION_SCALER_DUMP           BOOST_BINARY(10000)

static const short USB_WIENER_VENDOR_ID(0x16dc);
static const short USB_VMUSB_PRODUCT_ID(0xb);

static const unsigned int FIDRegister(0);       // Firmware id.
static const unsigned int GMODERegister(4);     // Global mode register.

static const int DEFAULT_TIMEOUT(2000);	// ms.
static const int DRAIN_RETRIES(5);

// Bulk transfer endpoints
static const int ENDPOINT_OUT(2);
static const int ENDPOINT_IN(0x86);


bool VmUsb2::daq_init() {
  //  First do a bulk read just to flush any crap that's in the VM-USB
  // output fifo..as it appears to sometimes leave crap there.
  // ignore any error status, and use a short timeout:

  DBG << "<VmUsb> Flushing any garbage in the VM-USB fifo...\n";

  char junk[1000];
  size_t moreJunk;
  usbRead(junk, sizeof(junk), &moreJunk, 1*1000); // One second timeout.

  DBG << "<VmUsb> Starting VM-USB initialization\n";


  // Now we can start preparing to read...
  systemReset();

  wait_ms(300);
  writeActionRegister(0);

//  m_pVme->writeBulkXferSetup(0 << CVMUSB::TransferSetupRegister::timeoutShift); // don't want multibuffering...1sec timeout is fine.

  // The global mode:
  //   13k buffer
  //   Single event seperator.
  //   Aligned on 16 bits.
  //   Single header word.
  //   Bus request level 4.
  //   Flush scalers on a single event.
  //
//  m_pVme->writeGlobalMode((4 << CVMUSB::GlobalModeRegister::busReqLevelShift) |
                            //        CVMUSB::GlobalModeRegister::flushScalers |
                            // CVMUSB::GlobalModeRegister::mixedBuffers        |
                            // CVMUSB::GlobalModeRegister::spanBuffers         |
//                            (CVMUSB::GlobalModeRegister::bufferLen13K <<
//                                  CVMUSB::GlobalModeRegister::bufferLenShift));


}

void VmUsb2::daq_start() {
  writeActionRegister(XXUSB_ACTION_START);
}

void VmUsb2::daq_stop() {
  writeActionRegister(XXUSB_ACTION_STOP);
}

void VmUsb2::systemReset()
{
  writeActionRegister(XXUSB_ACTION_SYSRES);
}

uint32_t VmUsb2::read32(uint32_t address, AddressModifier amodifier) {
  uint32_t d32 =0;
  a_vmeRead32(address, amodifier, &d32);
  return d32;
}

uint16_t VmUsb2::read16(uint32_t address,AddressModifier amodifier) {
  uint16_t d16 = 0;
  a_vmeRead16(address, amodifier, &d16);
  return d16;
}

uint8_t VmUsb2::read8(uint32_t address, AddressModifier amodifier) {
  uint8_t d8 = 0;
  a_vmeRead8(address, amodifier, &d8);
  return d8;
}

void VmUsb2::write32(uint32_t vmeAddress, AddressModifier am, uint32_t data)
{
  return;

  int result = a_vmeWrite32(vmeAddress, am, data);
  if (result < 0)
    ERR << "vme_write32 failed";
}

void VmUsb2::write16(uint32_t vmeAddress, AddressModifier am, uint16_t data)
{
  return;

  int result = a_vmeWrite16(vmeAddress, am, data);
  if (result < 0)
    ERR << "vme_write16 failed";
}

void VmUsb2::write8(uint32_t vmeAddress, AddressModifier am, uint8_t data)
{
  return;

  int result = a_vmeWrite8(vmeAddress, am, data);
  if (result < 0)
    ERR << "vme_write8 failed";
}


std::string VmUsb2::controllerName(void)
{
  return "VM-USB (nscl hack)";
}


/*!
   Write the interrupt mask.  This 8 bit register has bits that are set for
   each interrupt level that should be ignored by the VM-USB.
   @param mask Interrupt mask to write.
*/
void VmUsb2::writeIrqMask(uint8_t mask)
{
  // Well this is a typical VM-USB abomination:
  // - First save the global (mode?) register as we're going to wipe it out
  // - Next write the irqmask | 0x8000 to the glboal mode register.
  // - Write a USB trigger to the action register.
  // - Clear the action register(?)
  // - Write 0 to the global mode register.
  // - restore the saved global mode register.

  uint16_t oldGlobalMode = readRegister(GMODERegister);
  uint16_t gblmask      = mask;
  gblmask              |= 0x8000;

  writeRegister(GMODERegister, static_cast<uint32_t>(gblmask));
  writeActionRegister(2);	// Hopefully bit 1 numbered from zero not one.
  uint8_t maskValue  = readRegister(FIDRegister); // should have the mask.

  writeRegister(GMODERegister, 0);  // Turn off this nonesense
  writeRegister(GMODERegister, static_cast<uint32_t>(oldGlobalMode)); // restore the old globalmode.

  if (mask != maskValue)
    DBG << "<VmUsb> write IRQ mask failed: " << (int)mask << "!=" << (int)maskValue;

  m_irqMask = maskValue;
}
/*!
   Read the interrupt mask.  This 8 bit register has bits set for each
   interrupt level that should be ignoered by the VM-USB.
   @return uint8_t
   @retval contents of the mask register.
*/
uint8_t VmUsb2::readIrqMask()
{
  // Since the interrupt mask register appears cleverly crafted so that you
  // can't actually read it without destroying it, we're just going to use
  // a copy of the value:

  return m_irqMask;
}




/////////////////////////////////////////////////////////////////////////
/////////////////////////// VME Transfer Ops ////////////////////////////
/////////////////////////////////////////////////////////////////////////

// Write a 32 bit word to the VME for some specific address modifier.
// This is done by creating a list, inserting a single 32bit write into
// it and executing the list.
//   returns:
//    0  - Success.
//   -1  - USB write failed.
//   -2  - USB read failed.
//   -3  - VME Bus error.

int VmUsb2::a_vmeWrite32(uint32_t address, AddressModifier aModifier, uint32_t data)
{
  VmUsbStack list;
  list.addWrite32(address, aModifier, data);
  return doVMEWrite(list);
}


//   Write a 16 bit word to the VME.  This is identical to vmeWrite32,
//   however the data is 16 bits wide.
int VmUsb2::a_vmeWrite16(uint32_t address, AddressModifier aModifier, uint16_t data)
{
  VmUsbStack list;
  list.addWrite16(address, aModifier, data);
  return doVMEWrite(list);
}


//  Do an 8 bit write to the VME bus.
int VmUsb2::a_vmeWrite8(uint32_t address, AddressModifier aModifier, uint8_t data)
{
  VmUsbStack list;
  list.addWrite8(address, aModifier, data);
  return doVMEWrite(list);
}


// Read a 32 bit word from the VME.  This is done by creating a list,
// inserting a single VME read operation in it and executing the list in
// immediate mode.
//   returns
//     0    - All went well.
//    -1    - The usb_bulk_write failed.
//    -2    - The usb_bulk_read failed.
int VmUsb2::a_vmeRead32(uint32_t address, AddressModifier aModifier, uint32_t* data)
{
  VmUsbStack list;
  list.addRead32(address, aModifier);
  uint32_t      lData;
  int status = doVMERead(list, &lData);
  *data      = lData;
  return status;
}

//    Read a 16 bit word from the VME.  This is just like the previous
//    function but the data transfer width is 16 bits.
int VmUsb2::a_vmeRead16(uint32_t address, AddressModifier aModifier, uint16_t* data)
{
  std::unique_ptr<VmUsbStack> pList(new VmUsbStack()); //should vmecontroller do this
  VmUsbStack& list = *pList;
  list.addRead16(address, aModifier);
  uint32_t lData;
  int      status = doVMERead(list, &lData);
  *data = static_cast<uint16_t>(lData);
  return status;
}

//   Read an 8 bit byte from the VME... see vmeRead32 for information about this.
int VmUsb2::a_vmeRead8(uint32_t address, AddressModifier aModifier, uint8_t* data)
{
  VmUsbStack list;
  list.addRead8(address, aModifier);
  uint32_t lData;
  int      status = doVMERead(list, &lData);
  *data  = static_cast<uint8_t>(lData);
  return status;
}


// If the write list has already been created, this fires it off and returns
// the appropriate status:
int VmUsb2::doVMEWrite(VmUsbStack& list)
{
  uint16_t reply;
  size_t   replyBytes;
  int status = a_executeList(list, &reply, sizeof(reply), &replyBytes);
  // Bus error:
  if ((status == 0) && (reply == 0)) {
    status = -3;
  }
  return status;
}

// Common code to do a single shot vme read operation:
int VmUsb2::doVMERead(VmUsbStack& list, uint32_t* datum)
{
  size_t actualRead;
  int status = a_executeList(list, datum, sizeof(uint32_t), &actualRead);
  return status;
}


std::vector<uint32_t> VmUsb2::vmeBlockRead(int base, AddressModifier amod,  int xfercount) {
  uint32_t data[xfercount];
  size_t   xferred = 0;
  std::vector<uint32_t> result;

  a_vmeBlockRead((uint32_t)base, amod, (void*)data, (size_t)xfercount, &xferred);
  for (size_t i =0; i < xferred; i++) {
    result.push_back(data[i]);
  }
  return result;
}

std::vector<uint32_t> VmUsb2::vmeFifoRead(int base, AddressModifier amod, int xfercount) {
  uint32_t data[xfercount];
  size_t xferred = 0;
  std::vector<uint32_t> result;

  a_vmeFifoRead((uint32_t)base, amod, data, (size_t)xfercount, &xferred);
  for (size_t i = 0; i < xferred; i++) {
    result.push_back(data[i]);
  }
  return result;
}


uint32_t VmUsb2::readRegister(uint16_t address)
{
  VmUsbStack    list;
  uint32_t      data;
  size_t        count;
  list.addRegisterRead(address);

  int status = a_executeList(list, &data, sizeof(data), &count);
  if (status == -1)
    throw (std::string("<VmUsb> readRegister USB write failed: ")
      + std::to_string(errno));
  if (status == -2)
    throw (std::string("<VmUsb> readRegister USB read failed: ")
      + std::to_string(errno));

  return data;
}


void VmUsb2::writeRegister(uint16_t address, uint32_t data)
{
  VmUsbStack     list;
  uint32_t       rdstatus;
  size_t         rdcount;
  list.addRegisterWrite(address, data);

  int status = a_executeList(list, &rdstatus, sizeof(rdstatus), &rdcount);
  if (status == -1)
    throw (std::string("<VmUsb> writeRegister USB write failed: ")
      + std::to_string(errno));
  if (status == -2)
    throw (std::string("<VmUsb> writeRegister USB read failed: ")
      + std::to_string(errno));
}


//////////////////////////////////////////////////////////////////////////
/////////////////////////// Block read operations ////////////////////////
/////////////////////////////////////////////////////////////////////////

/*!
    Read a block of longwords from the VME.  It is the caller's responsibility
    to:
    - Ensure that the starting address of the transfer is block aligned
      (that is a multiple of 0x100).
    - That the address modifier is  block transfer modifier such as
      CVMUSBReadoutList::a32UserBlock.
    The list construction takes care of the case where the count spans block
    boundaries or requires a trailing partial block transfer.  Note that
    transfers are done in 32 bit width.

    \param baseAddress : uint32_t
      Fisrt transfer address of the transfer.
    \param aModifier   : uint8_t
      Address modifier of the transfer.  See above for restrictions.
    \param data        : void*
       Buffer to hold the data read.
    \param transferCount : size_t
       Number of \em Longwords to transfer.
    \param countTransferred : size_t*
        Pointer to a size_t into which will be written the number of longwords
        actually transferred.  This can be used to determine if the list
        exited prematurely (e.g. due to a bus error).

     \return int
     \retval 0  - Successful completion.  Note that a BERR in the middle
                  of the transfer is successful completion.  The countTransferred
                  will be less than transferCount in that case.
     \retval -1 - Failure on usb_bulk_write, the actual cause of the error is
                  in errno, countTransferred will be 0.
     \retval -2 - Failure on the usb_bulk_write, The actual error will be in
                  errno.  countTransferred will be 0.

*/
int VmUsb2::a_vmeBlockRead(uint32_t baseAddress, AddressModifier aModifier,
         void*    data,        size_t transferCount,
         size_t*  countTransferred)
{
  // Create the list:

  VmUsbStack list;
  list.addBlockRead32(baseAddress, aModifier, transferCount);


  *countTransferred = 0;
  int status = a_executeList(list, data, transferCount*sizeof(uint32_t),
         countTransferred);
  *countTransferred = *countTransferred/sizeof(uint32_t); // bytes -> transfers.
  return status;
}


/*!
   Do a 32 bit wide block read from a FIFO.  The only difference between
   this and vmeBlockRead is that the address read from is the same for the
   entire block transfer.
*/
int VmUsb2::a_vmeFifoRead(uint32_t address, AddressModifier aModifier,
        void*    data,    size_t transferCount, size_t* countTransferred)
{
  VmUsbStack list;
  list.addFifoRead32(address, aModifier, transferCount);

  *countTransferred = 0;
  int status =  a_executeList(list, data, transferCount*sizeof(uint32_t),
          countTransferred);
  *countTransferred = *countTransferred/sizeof(uint32_t); // bytes -> transferrs.
  return status;
}


/*!
    Execute a list immediately.  It is the caller's responsibility
    to ensure that no data taking is in progress and that data taking
    has run down (the last buffer was received).
    The list is transformed into an out packet to the VmUsb2 and
    transaction is called to execute it and to get the response back.
    \param list  : CVmUsb2ReadoutList&
       A reference to the list of operations to execute.
    \param pReadBuffer : void*
       A pointer to the buffer that will receive the reply data.
    \param readBufferSize : size_t
       number of bytes of data available to the pReadBuffer.
    \param bytesRead : size_t*
       Return value to hold the number of bytes actually read.

    \return int
    \retval  0    - All went well.
    \retval -1    - The usb_bulk_write failed.
    \retval -2    - The usb_bulk_read failed.

    In case of failure, the reason for failure is stored in the
    errno global variable.
*/
int VmUsb2::a_executeList(VmUsbStack&    list,
                       void*                  pReadoutBuffer,
                       size_t                 readBufferSize,
                       size_t*                bytesRead)
{
  size_t outSize;
  std::vector<uint16_t> outPacket = list.packet(TAVcsWrite | TAVcsIMMED);

  // Execute the transaction:
  int status = transaction(outPacket.data(), outPacket.size() * sizeof(short), pReadoutBuffer, readBufferSize);

  if(status >= 0)
    *bytesRead = status;
  else
    *bytesRead = 0;

  return (status >= 0) ? 0 : status;
}

std::vector<uint8_t> VmUsb2::executeList(VmUsbStack& list, int maxBytes)
{
  uint8_t data[maxBytes];
  size_t     nRead;
  std::vector<uint8_t> result;

  int status = this->a_executeList(list, data, maxBytes, &nRead);

  if (status == 0) {
    for (size_t i = 0; i < nRead; i++) {
      result.push_back(data[i]);
    }
  }

  return result;
}


/*!
   Load a list into the VM-USB for later execution.
   It is the callers responsibility to:
   -  keep track of the lists and their  storage requirements, so that
      they are not loaded on top of or overlapping
      each other, or so that the available list memory is not exceeded.
   - Ensure that the list number is a valid value (0-7).
   - The listOffset is valid and that there is room in the list memory
     following it for the entire list being loaded.
   This code just load the list, it does not attach it to any specific trigger.
   that is done via register operations performed after all the required lists
   are in place.

   \param listNumber : uint8_t
      Number of the list to load.
   \param list       : CVmUsb2ReadoutList
      The constructed list.
   \param listOffset : off_t
      The offset in list memory at which the list is loaded.
      Question for the Wiener/Jtec guys... is this offset a byte or long
      offset... I'm betting it's a longword offset.
*/
int VmUsb2::loadList(uint8_t  listNumber, VmUsbStack& list, off_t listOffset)
{
  // Need to construct the TA field, straightforward except for the list number
  // which is splattered all over creation.

  uint16_t ta = TAVcsSel | TAVcsWrite;
  if (listNumber & 1)  ta |= TAVcsID0;
  if (listNumber & 2)  ta |= TAVcsID1; // Probably the simplest way for this
  if (listNumber & 4)  ta |= TAVcsID2; // few bits.

  std::vector<uint16_t> outPacket = list.packet(ta, listOffset);
  int status = usb_bulk_write(m_handle, ENDPOINT_OUT, reinterpret_cast<char*>(outPacket.data()),
                              outPacket.size() * sizeof(short), m_timeout);
  if (status < 0) {
    errno = -status;
    status= -1;
  }

  return (status >= 0) ? 0 : status;
}



/*!
  Execute a bulk read for the user.  The user will need to do this
  when the VmUsb2 is in autonomous data taking mode to read buffers of data
  it has available.
  \param data : void*
     Pointer to user data buffer for the read.
  \param buffersSize : size_t
     size of 'data' in bytes.
  \param transferCount : size_t*
     Number of bytes actually transferred on success.
  \param timeout : int [2000]
     Timeout for the read in ms.

  \return int
  \retval 0   Success, transferCount has number of bytes transferred.
  \retval -1  Read failed, errno has the reason. transferCount will be 0.

*/
int VmUsb2::usbRead(void* data, size_t bufferSize, size_t* transferCount, int timeout)
{
  //  CriticalSection s(*m_pMutex);
  int status = usb_bulk_read(m_handle, ENDPOINT_IN,
                             static_cast<char*>(data), bufferSize,
                             timeout);
  if (status >= 0) {
    *transferCount = status;
    status = 0;
  }
  else {
    errno = -status;
    status= -1;
    *transferCount = 0;
  }
  return status;
}

/*
   Utility function to perform a 'symmetric' transaction.
   Most operations on the VM-USB are 'symmetric' USB operations.
   This means that a usb_bulk_write will be done followed by a
   usb_bulk_read to return the results/status of the operation requested
   by the write.
   Parametrers:
   void*   writePacket   - Pointer to the packet to write.
   size_t  writeSize     - Number of bytes to write from writePacket.

   void*   readPacket    - Pointer to storage for the read.
   size_t  readSize      - Number of bytes to attempt to read.

   Returns:
     > 0 the actual number of bytes read into the readPacket...
         and all should be considered to have gone well.
     -1  The write failed with the reason in errno.
     -2  The read failed with the reason in errno.

   NOTE:  The m_timeout is used for both write and read timeouts.

*/
int VmUsb2::transaction(void* writePacket, size_t writeSize, void* readPacket,  size_t readSize)
{
  int status = usb_bulk_write(m_handle, ENDPOINT_OUT, static_cast<char*>(writePacket), writeSize, m_timeout);
  if (status < 0) {
    ERR << "<VmUsb> USB bulk write failed, errno=" << -status;
    return -1;		// Write failed!!
  }
  status    = usb_bulk_read(m_handle, ENDPOINT_IN, static_cast<char*>(readPacket), readSize, m_timeout);
  if (status < 0) {
    ERR << "<VmUsb> USB bulk read failed, errno=" << -status;
    return -2;
  }
  return status;
}

///////////////////////////////////////////
///CLEARED///CLEARED///CLEARED///CLEARED///
///////////////////////////////////////////
///CLEARED///CLEARED///CLEARED///CLEARED///
///////////////////////////////////////////
///CLEARED///CLEARED///CLEARED///CLEARED///
///////////////////////////////////////////
///CLEARED///CLEARED///CLEARED///CLEARED///
///////////////////////////////////////////
///CLEARED///CLEARED///CLEARED///CLEARED///
///////////////////////////////////////////
///CLEARED///CLEARED///CLEARED///CLEARED///
///////////////////////////////////////////

VmUsb2::VmUsb2()
{
  m_device = nullptr;
  m_handle = nullptr;
  m_timeout = DEFAULT_TIMEOUT;
  m_irqMask = 0xFF;
}

VmUsb2::~VmUsb2()
{
  disconnect();
}


std::list<std::string> VmUsb2::controllers()
{
  std::list<std::string> controllerList;
  for (auto &q : enumerate())
    controllerList.push_back(serialNo(q));
  return controllerList;
}

bool VmUsb2::connect(uint16_t target) {
  m_device = nullptr;
  m_handle = nullptr;
  m_serialNumber.clear();

  std::vector<struct usb_device*> devices = enumerate();
  if (devices.empty()) {
    ERR << "<VmUsb> no VmUsb devices found";
    return false;
  }
  if (target > devices.size()) {
    ERR << "<VmUsb> target VmUsb device " << target <<
              " out of bounds (tot=" << devices.size() << ")";
    return false;
  }

  m_device = devices[target];
  m_handle  = usb_open(m_device);
  if (!m_handle) {
    ERR << "<VmUsb> Unable to open VmUsb device at " << target;
    return false;
  }

  m_serialNumber = serialNo(m_device);
  return initVmUsb();
}


bool VmUsb2::connect(std::string target) {
  m_device = nullptr;
  m_handle = nullptr;
  m_serialNumber.clear();

  std::vector<struct usb_device*> devices = enumerate();
  if (devices.empty()) {
    ERR << "<VmUsb> no VmUsb devices found";
    return false;
  }

  for (auto &q : devices) {
    if (serialNo(q) == target) {
      m_device = q;
      break;
    }
  }

  if (!m_device) {
    ERR << "The VM-USB with serial number " << target
           << " could not be enumerated";
    return false;
  }

  m_handle  = usb_open(m_device);
  if (!m_handle) {
    ERR << "<VmUsb> Unable to open VmUsb device at " << target;
    return false;
  }

  m_serialNumber = serialNo(m_device);
  return initVmUsb();
}


// Reconnect by closing device and invoking connect
// by serial number
bool VmUsb2::reconnect()
{
  usb_release_interface(m_handle, 0);
  usb_close(m_handle);
  wait_ms(1000);	// Let this all happen
  return connect(m_serialNumber);
}

void VmUsb2::disconnect() {
  if (m_handle) {
    usb_release_interface(m_handle, 0);
    usb_close(m_handle);
    wait_ms(5000);
  }

  m_device = nullptr;
  m_handle = nullptr;
  m_serialNumber.clear();
  m_irqMask = 0xFF;
}


// Initialize VmUsb
// Assume m_handle has already been opened
bool VmUsb2::initVmUsb()
{
    // Claim the interface

    usb_set_configuration(m_handle, 1);
    int status = usb_claim_interface(m_handle, 0);
    if (status == -EBUSY) {
      ERR << "<VmUsb> some other process has already claimed device";
      disconnect();
      return false;
    }
    if (status == -ENOMEM) {
      ERR << "<VmUsb> claim failed due to lack of memory";
      disconnect();
      return false;
    }

    // Errors we don't know about:
    if (status < 0) {
      ERR << "<VmUsb> failed to claim interface "  << (-status);
      disconnect();
      return false;
    }

    usb_clear_halt(m_handle, ENDPOINT_IN);
    usb_clear_halt(m_handle, ENDPOINT_OUT);
    wait_ms(100);

    // Turn off DAQ mode and flush any data that might be trapped in the VMUSB
    // FIFOS.  To write the action register may require at least one read of the FIFO.
    int retriesLeft = DRAIN_RETRIES;
    uint8_t buffer[1024*13*2];  // Biggest possible VM-USB buffer.
    size_t  bytesRead;

    while (retriesLeft) {
        try {
            usbRead(buffer, sizeof(buffer), &bytesRead, 1);
            writeActionRegister(0);     // Turn off data taking.
            break;                      // done if success.
        } catch (...) {
            retriesLeft--;
        }
    }

    if (!retriesLeft) {
      ERR << "<VmUsb> not able to stop data taking VM-USB may need to be power cycled";
      disconnect();
      return false;
    }

    while(usbRead(buffer, sizeof(buffer), &bytesRead) == 0)
      DBG << "<VmUsb> Flushing VM-USB buffer";

    // Now set the irq mask so that all bits are set..that:
    // - is the only way to ensure the m_irqMask value matches the register.
    // - ensures m_irqMask actually gets set:

   // writeIrqMask(0xff);   /* Seems to like to timeout :-( */)

  uint32_t fwrel = readRegister(0x00);
  DBG << "<VmUsb> Connected to VM-USB serial_nr=" << m_serialNumber << " firmware=" << itohex32(fwrel);
  return true;
}



//  Enumerate the Wiener/JTec VM-USB devices.
//  This function returns a vector of usb_device descriptor pointers
//  for each Wiener/JTec device on the bus.
std::vector<struct usb_device*> VmUsb2::enumerate() {
  usb_init();           // assume not inistialized
  usb_find_busses();		// re-enumerate the busses
  usb_find_devices();		// re-enumerate the devices.

  // Now we are ready to start the search:
  std::vector<struct usb_device*> devices;	// Result vector.
  struct usb_bus* pBus = usb_get_busses();

  while(pBus) {
    struct usb_device* pDevice = pBus->devices;
    while(pDevice) {
      usb_device_descriptor* pDesc = &(pDevice->descriptor);
      if ((pDesc->idVendor  == USB_WIENER_VENDOR_ID) && (pDesc->idProduct == USB_VMUSB_PRODUCT_ID))
        devices.push_back(pDevice);
      pDevice = pDevice->next;
    }
    pBus = pBus->next;
  }

  return devices;
}

// return serial number of usb device
std::string VmUsb2::serialNo(struct usb_device* dev)
{
  usb_dev_handle* pDevice = usb_open(dev);

  if (pDevice) {
    char szSerialNo[256];	// actual string is only 6chars + null.
    int nBytes = usb_get_string_simple(pDevice, dev->descriptor.iSerialNumber, szSerialNo, sizeof(szSerialNo));
    usb_close(pDevice);

    if (nBytes > 0)
      return std::string(szSerialNo);
    else
      throw std::string("usb_get_string_simple failed in VmUsb::serialNo");

  } else
    throw std::string("usb_open failed in VmUsb::serialNo");
}

//  Writing a value to the action register.
//  Special case, see section 4.2, 4.3 in VM-USB manual
void VmUsb2::writeActionRegister(uint16_t data)
{
  // Build up the output packet:
  char outPacket[100];
  char* pOut = outPacket;
  pOut = static_cast<char*>(VmUsbStack::addToPacket16(pOut, 5)); // Select Register block for transfer.
  pOut = static_cast<char*>(VmUsbStack::addToPacket16(pOut, 10)); // Select action register wthin block.
  pOut = static_cast<char*>(VmUsbStack::addToPacket16(pOut, data));

  // This operation is write only.
  int outSize = pOut - outPacket;
  int status = usb_bulk_write(m_handle, ENDPOINT_OUT,
      outPacket, outSize, m_timeout);

  if (status < 0)
    throw (std::string("<VmUsb> Error in usb_bulk_write, writing action register ")
      + std::to_string(-status));
  if (status != outSize)
    throw "usb_bulk_write wrote different size than expected";
}

