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
*/

#include <config.h>
#include "CRingItem.h"
#include "DataFormat.h"

//#include <CRingBuffer.h>
//#include <CRingSelectionPredicate.h>
#include <string.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

////////////////////////////////////////////////////////////////////////////////
//
// Constructors and other canonicals.
//

/*!
   Construct the ring item:
   - If the maxbody is larger than CRingItemStaticBufferSize, allocate
     a new buffer and point the item at that, otherwise point it at 
     m_staticBuffer.
   - Pointer m_cursor at the body of the ring item.
   - Calculate and fill in the storage size.
   - Set m_swapNeeded to false.
   - Set the ring item type.

   \param type - The ring item type.  This is only 16 bits wide but stored in a
                 32 bit word so receivers can determine if bytes were swapped.
   \param maxBody - Largest body we can hold

*/
CRingItem::CRingItem(uint16_t type, size_t maxBody) :
  m_pItem(reinterpret_cast<RingItem*>(&m_staticBuffer)),
  m_storageSize(maxBody),
  m_swapNeeded(false)
{

  // If necessary, dynamically allocate (big max item).

  newIfNecessary(maxBody);
  m_pItem->s_header.s_type = type;
  m_pItem->s_header.s_size = 0;
  m_pItem->s_body.u_noBodyHeader.s_mbz = 0;
  setBodyCursor(m_pItem->s_body.u_noBodyHeader.s_body);
  updateSize();
  
}
/**
 * constructor with body header.
 * This is basically the same as the prior constructor, however a body header
 * is pre-created in the event body.  The size and cursor are updated to
 * reflect the new body start location.
 *
 * @param type        - Ring Item type.
 * @param timestamp   - Event timestamp for the body header.
 * @param sourceid    - Id of the event source.
 * @param barrierType - Type of barrier being created (0 if not a barrier)
 * @param maxbody     - Maximum body size required.
 */
CRingItem::CRingItem(uint16_t type, uint64_t timestamp, uint32_t sourceId,
                     uint32_t barrierType, size_t maxBody) :
  m_pItem(reinterpret_cast<RingItem*>(&m_staticBuffer)),
  m_storageSize(maxBody),
  m_swapNeeded(false)
{
  // If necessary, dynamically allocate (big max item).

  newIfNecessary(maxBody);
  m_pItem->s_header.s_type = type;
  m_pItem->s_header.s_size = 0;
  
  pBodyHeader pHeader = &(m_pItem->s_body.u_hasBodyHeader.s_bodyHeader);
  pHeader->s_size      = sizeof(BodyHeader);
  pHeader->s_timestamp = timestamp;
  pHeader->s_sourceId  = sourceId;
  pHeader->s_barrier   = barrierType;
  
  setBodyCursor(m_pItem->s_body.u_hasBodyHeader.s_body);
  updateSize();
  
}
/*!
  Copy construct.  This is actually the same as the construction above, 
  however the item contents get memcpied into our ring body. The caller has to
  ensure that m_pCursor reflects the amount of data copied into the ring body.

  \param rhs  - The source of the copy.
*/
CRingItem::CRingItem(const CRingItem& rhs)
{
  // If the storage size is big enough, we need to dynamically allocate
  // our storage

  newIfNecessary(rhs.m_storageSize);

  copyIn(rhs);
}
/*!
    Destroy the item. If the storage size was big, we need to delete the 
    storage as it was dynamically allocated.
*/
CRingItem::~CRingItem()
{
  deleteIfNecessary();
}

/*!
   Assignment.  If necessary destroy the body.  If necessary re-create the body.
   After that it's just a copy in.

   \param rhs  - The object that we are duplicating into *this.
*/
CRingItem&
CRingItem::operator=(const CRingItem& rhs)
{
  if (this != &rhs) {
    deleteIfNecessary();
    newIfNecessary(rhs.m_storageSize);
    copyIn(rhs);
  }

  return *this;
}

/*!
   Comparison for equality.. note that true equality may be time consuming
   to determine, as it requires the contents of the items to be equal which will
   require linear time in the size of the item.
   \param rhs  - refers to the item to be compared with *this>
   \return int
   \retval 0   - Not equal\
   \retval 1   - equal

*/
int
CRingItem::operator==(const CRingItem& rhs) const
{
  // short cut by looking at storage size and swap characteristics first:

  if (m_storageSize != rhs.m_storageSize) return 0;
  if (m_swapNeeded  != rhs.m_swapNeeded ) return 0;

  // Now there's nothing for it but to compare the contents:

  return (memcmp(m_pItem, rhs.m_pItem, m_pItem->s_header.s_size) == 0);
}
/*!
  Inequality is just the logical inverse of equality.  This can take time, see
  operator==

  \param rhs  - Reference to the object *this will be compared to.
  \return int
  \retval 0   - Objects are not unequal
  \retval 1   - Objects are unequal
*/
int
CRingItem::operator!=(const CRingItem& rhs) const
{
  return !(*this == rhs);
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Selectors:

/*!

   \return size_t
   \retval The size allocated to the ring item's body.
*/
size_t
CRingItem::getStorageSize() const
{
  return m_storageSize;
}

/*!
   \return size_t
   \retval Amount of data in the body.  This is the difference between the 
           cursor and the start of the body.  This does not include the
           body header if one exists.
*/
size_t
CRingItem::getBodySize() const
{
  return (m_pCursor - reinterpret_cast<uint8_t*>(getBodyPointer()));
}
/*!
  \return void*
  \retval Pointer to the body of the ring item. To get the next insertion point,
          see getBodyCursor.
*/
void*
CRingItem::getBodyPointer() const
{
    // The result depends on whether or not the item has a body header:
    
    if(hasBodyHeader()) {
        return (m_pItem->s_body.u_hasBodyHeader.s_body);
    } else {
        return (m_pItem->s_body.u_noBodyHeader.s_body);
    }
}
/*!
   \return void*
   \retval Pointer to the next insertion point of the body
*/
void*
CRingItem::getBodyCursor()
{
  return m_pCursor;
}
/*!
   \return void*
   \retval Pointer to the body.  To be usually used by derived classes but...
*/
pRingItem
CRingItem::getItemPointer()
{
  return m_pItem;
}

/*!
   \return void*
   \retval Pointer to the body.  To be usually used by derived classes but...
*/
const _RingItem*
CRingItem::getItemPointer() const
{
  return m_pItem;
}
/*!
   \return uint32_t
   \retval Current type of the item.
*/
uint32_t
CRingItem::type() const
{
  uint32_t rawType = m_pItem->s_header.s_type;
  if (mustSwap()) {
    return swal(rawType);
  }
  else {
    return rawType;
  }
}

/**
 * hasBodyHeader
 *
 * @return bool - true if the item has a body header, false otherwise.
 */
bool
CRingItem::hasBodyHeader() const
{
    return (m_pItem->s_body.u_noBodyHeader.s_mbz != 0);
}
/**
 * getEventTimestamp
 *
 * @return uint64_t - returns the timestamp from the body header.
 * @throws std::string - if the item has no body header.
 */
uint64_t
CRingItem::getEventTimestamp() const
{
    throwIfNoBodyHeader(
        "Attempted to get a timestamp from an event that does not have one"
    );

    return m_pItem->s_body.u_hasBodyHeader.s_bodyHeader.s_timestamp;
}
/**
 * getSourceId
 *
 * @return uint32_t the id of the data source that contributed this event.
 *
 * @throw std::string - if this event does ot have a body header.
 */
uint32_t
CRingItem::getSourceId() const
{
    throwIfNoBodyHeader(
        "Attempted to get the source ID from an event that does not have one"
    );
    return m_pItem->s_body.u_hasBodyHeader.s_bodyHeader.s_sourceId;
}
/**
 * getBarrierType
 *
 * @return uint32_t - Barrier type in the event.
 * @throw std::string - If this event does not have an event header.
 */
uint32_t
CRingItem::getBarrierType() const
{
    throwIfNoBodyHeader(
        "Attempted to get the barrier type from an event that does not have one"
    );
    return m_pItem->s_body.u_hasBodyHeader.s_bodyHeader.s_barrier;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// Mutators.

/*!
  Update the body cursor to reflect data that was put in the item.
  \param pNewCursor - New vlaue for the body cursor.
 
*/
void
CRingItem::setBodyCursor(void* pNewCursor)
{
  m_pCursor = reinterpret_cast<uint8_t*>(pNewCursor);
}

/*!
** Given the current item cursor set the size of the item.
*/
void
CRingItem::updateSize()
{
  size_t s = sizeof(RingItemHeader) + getBodySize();
  
  // That body size does not count the body header if it's there.
  
  if (hasBodyHeader()) {
    s += sizeof(BodyHeader);
  } else {
    s += sizeof(uint32_t);
  }

  m_pItem->s_header.s_size = s;
}

/**
 * setBodyHeader
 *
 * Sets a body header to the desired values.  If the event does not yet
 * have a body header, space is created for by sliding the existing data
 * down in the buffer. Clearly then, for large events, it is much quicker to
 * construct the ring item with the wrong header and then call this to get the
 * header right than it is to use this method to add a header to an event that
 * has none
 *
 * @param timestamp   - The event timestamp
 * @param sourceId    - Id of the source that contributed this item.
 * @param barrierType - Type of the item.
 */

void
CRingItem::setBodyHeader(uint64_t timestamp, uint32_t sourceId,
    uint32_t barrierType)
{
    if (!hasBodyHeader()) {
        // Make space for the body header.
        
        uint8_t* pBody = (m_pItem->s_body.u_noBodyHeader.s_body);
        size_t moveSize = sizeof(BodyHeader) - sizeof(uint32_t);
        size_t moveCount= m_pItem->s_header.s_size - sizeof(RingItemHeader) - sizeof(uint32_t);
        memmove(pBody + moveSize, pBody, moveCount);
        m_pCursor += moveSize;

    }
    pBodyHeader pHeader = &(m_pItem->s_body.u_hasBodyHeader.s_bodyHeader);
    pHeader->s_size = sizeof(BodyHeader);
    pHeader->s_timestamp = timestamp;
    pHeader->s_sourceId  = sourceId;
    pHeader->s_barrier   = barrierType;
    updateSize();
    
}

///////////////////////////////////////////////////////////////////////////////////////
//
//   Object operations.

/*!
   Commit the current version of the ring data to a ring.  
   - Calculates the size field of the header
   - puts the data in the ring buffer.

   \param ring  - Reference to the ring buffer in which the item will be put.

   \note The invoking process must already be the producing process for the ring.
   \note This implementation has no mechanism to timeout the put, however that could be
         added later.
   \note This function is non-destructive. There's nothing to stop the caller from
         issuing it on several rings.
   
*/
/*void
CRingItem::commitToRing(CRingBuffer& ring)
{
  updateSize();
  ring.put(m_pItem, m_pItem->s_header.s_size);
}*/

/*!
   This is primarily intended for items that have been constructed via
   the getFromRing static member.
   \return bool
   \retval true - The byte order on the generating system is different from that of
                  this system.
   \retval false - The byte order on the generating system is the same as that of this
                  system.
*/
bool
CRingItem::mustSwap() const
{
  return m_swapNeeded;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// Default implementations of virtual methods:
//

/**
 * typeName
 *
 *  Return an std::string that contains the name of the ring item type
 *  (e.g. "Physics data").  The default produces:
 *  "Unknown (0xnn) where 0xnn is the hexadecimal ring item type.
 *
 * @return std::string containing the type as described above.
 */

std::string
CRingItem::typeName() const
{
  std::stringstream typeStr;
  typeStr << "Unknown (" << std::hex << type() << ")"; 
  return typeStr.str();
}   
/**
 * toString
 *
 * Return an std::string that contains a formatted dump of the ring item
 * body. Default implementation just produces a hex-dump of the data
 * the dump has 8 elements per line with spaces between each element
 * and the format of each element is %02x.
 *
 * @return std::string - the dump described above.
 */
std::string
CRingItem::toString() const
{
  std::stringstream  dump;
  const uint8_t*      p     = reinterpret_cast<uint8_t*>(getBodyPointer());
  size_t              n     = getBodySize(); 
  int                 nPerLine(8);

  dump << bodyHeaderToString();
  
  dump << std::hex << std::setfill('0');

  for (int i = 0; i < n; i++) {
    if ( ((i % nPerLine) == 0)) {
      dump << std::endl;
    }
    dump   << std::setw(2)   << static_cast<unsigned int>(*p++) << " ";

  }
  // If there's no trailing endl put one in.

  if (n % nPerLine) {
    dump << std::endl;
  }
  

  return dump.str();
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  Static class methods.

/*!
   Fetches the next item in the ring that matches the
   conditions described by the predicate and wraps it in a CRingItem.
   The m_swapNeeded flag will be set in accordance with the byte order of the underlying
   item.  This is determined by knowing that the high 16 bits of the type field
   shall always be zero.
   \param ring  - Reference to the ring from which to get the item
   \param predicate - an object that skips unwanted items in the ring.

   \return CRingItem*
   \retval Pointer to a new, dynamically constructed ring item that has been wrapped around
           the message fetched.

   \note There is no method for specifying a timeout on the wait for a desirable
         message.
*/
/*CRingItem*
CRingItem::getFromRing(CRingBuffer& ring, CRingSelectionPredicate& predicate)
{
  predicate.selectItem(ring);
  
  // look at the header, figure out the byte order and count so we can
  // create the item and fill it in.
  //

  RingItemHeader header;
  blockUntilData(ring, sizeof(header));	// Wait until we have at least a header.
  ring.peek(&header, sizeof(header)); 

  bool otherOrder(false);
  uint32_t size = header.s_size;
  if ((header.s_type & 0xffff0000) != 0) {
    otherOrder = true;
    size = swal(size);
  }
  // Create the item and fill it in:

  CRingItem* pItem = new CRingItem(header.s_type, size);
  blockUntilData(ring, size);	// Wait until all data in.
  size_t gotSize = ring.get(pItem->m_pItem, size, size);// Read the item from the ring.
  if(gotSize  != size) {  
    std::cerr << "Mismatch in CRingItem::getItem required size: sb " << size << " was " << gotSize 
	      << std::endl;
  }
  
  // The ring item was constructed with the cursor pointing as if there's
  // no body header...therefore the arithmetic below is correct whether there
  // is or isn't a body header.
  //
  
  pItem->m_pCursor  +=  (size - sizeof(RingItemHeader) - sizeof(uint32_t));

  
  pItem->m_swapNeeded = otherOrder;

  return pItem;

}*/

///////////////////////////////////////////////////////////////////////////////////////
//
// Private utilities.
//


/*
 * Common code for copy construction and assignment,
 * copies the contents of some source item into *this.
 * We assume that the final values for m_pCursor and m_pItem
 * have already been set.
 */
void
CRingItem::copyIn(const CRingItem& rhs)
{
  m_storageSize   = rhs.m_storageSize;
  newIfNecessary(m_storageSize);
  
  m_swapNeeded  = rhs.m_swapNeeded;
  memcpy(m_pItem, rhs.m_pItem, 
	 rhs.m_pItem->s_header.s_size); 

  
  
  // where copyin is used, our cursor is already pointing at the body of the item.
  // therefore when updating it we need to allow for that in the arithmetic below.

  m_pCursor    = reinterpret_cast<uint8_t*>(m_pItem);
  m_pCursor   += m_pItem->s_header.s_size;
                 
  
  updateSize();
}


/*
 *   If necessary, delete dynamically allocated buffer space.
 */
void 
CRingItem::deleteIfNecessary()
{
  if (m_pItem != (pRingItem)m_staticBuffer) {
    delete [](reinterpret_cast<uint8_t*>(m_pItem));
  }
}
/*
 *  If necessary, create dynamically allocated buffer space and point
 * m_pItem at it.
 */
void
CRingItem::newIfNecessary(size_t size)
{
  if (size > CRingItemStaticBufferSize) {
    m_pItem  = reinterpret_cast<RingItem*>(new uint8_t[size + sizeof(RingItemHeader) + 100]);
  }
  else {
    m_pItem = reinterpret_cast<RingItem*>(m_staticBuffer);
  }
  m_pCursor= reinterpret_cast<uint8_t*>(&(m_pItem->s_body));

}
/*
** Swap the bytes of a longword:
*/
uint32_t
CRingItem::swal(uint32_t datum)
{
  union {
    uint32_t along;
    uint8_t  bytes[4];
  } swapper;
  swapper.along = datum;

  uint32_t result = swapper.bytes[3] | (swapper.bytes[2] << 8) | 
    (swapper.bytes[1] << 16) | (swapper.bytes[0] << 24);

}
/**
 * bodyHeaderToString
 *
 * return a string representation of the body header.  If the body header
 * does not exist in this ring item returnes "No body header\n"
 *
 * @return std::string
 */
std::string
CRingItem::bodyHeaderToString() const
{
    std::stringstream result;
    
    if (hasBodyHeader()) {
        pBodyHeader pHeader = &(m_pItem->s_body.u_hasBodyHeader.s_bodyHeader);
        result << "Body Header:\n";
        result << "Timestamp:    " << pHeader->s_timestamp << std::endl;
        result << "SourceID:     " << pHeader->s_sourceId  << std::endl;
        result << "Barrier Type: " << pHeader->s_barrier << std::endl;
        
    } else {
        result << "No body header\n";
    }
    return result.str();
}

/*
 *  Blocks the caller until a ring has at least the minimum required
 * amound of data.. note the use of a local predicate
 */

/*class RingHasNoMoreThan : public CRingBuffer::CRingBufferPredicate
{
private:
  size_t   m_requiredBytes;
public:
  RingHasNoMoreThan(size_t required) :
    m_requiredBytes(required)
  {}

  bool operator()(CRingBuffer& ring) {
    return ring.availableData() < m_requiredBytes;
  }
};*/

/*void 
CRingItem::blockUntilData(CRingBuffer& ring, size_t nbytes)
{
  RingHasNoMoreThan p(nbytes);
  ring.blockWhile(p);
}*/
/**
 * timeString
 *
 * Given a time_t time, returns a string that is the textual time (ctime()).
 *
 * @param theTime - time gotten from e.g. time(2).
 * 
 * @return std::string textified time
 */
std::string
CRingItem::timeString(time_t theTime) 
{

  std::string result(ctime(&theTime));
  
  // For whatever reason, ctime appends a '\n' on the end.
  // We need to remove that.

  result.erase(result.size()-1);

  return result;
}
/**
 * throwIfNoBodyHeader
 *
 * Throw an exception if the event does not have a body header.
 *
 * @param msg - The message to throw.
 */
void
CRingItem::throwIfNoBodyHeader(std::string msg) const
{
    if (m_pItem->s_body.u_noBodyHeader.s_mbz == 0) {
        throw msg;
    }
}
