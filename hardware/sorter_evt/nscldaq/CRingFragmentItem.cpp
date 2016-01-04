/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2009.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Author:
             Ron Fox
	     NSCL
	     Michigan State University
	     East Lansing, MI 48824-1321
*/

#include "CRingFragmentItem.h"
#include "DataFormat.h"
#include "CRingItemFactory.h"

#include <string.h>
#include <sstream>

/*-------------------------------------------------------------------------------------
 *   Canonical methods
 */

/**
 * constructor 
 *
 *  Create a new ring fragmen from the data supplied:
 *
 * @param timestamp   - The event builder timestamp.
 * @param source      - Source id of fragment creator.
 * @param payloadSize - Size of the fragment payload.
 * @param pBody       - Pointer to payload.
 * @param barrier     - Barrier type (defaults to 0 which is not a barrier).
 */
CRingFragmentItem::CRingFragmentItem(uint64_t timestamp, uint32_t source, uint32_t payloadSize, 
				     const void* pBody, uint32_t barrier) :
  CRingItem(EVB_FRAGMENT, timestamp, source, barrier, bodySize(payloadSize)) 

{
  init(payloadSize);
  pEventBuilderFragment pFrag =
    reinterpret_cast<pEventBuilderFragment>(getItemPointer());
  pBodyHeader pHeader = &(pFrag->s_bodyHeader);
  
  pHeader->s_size        = sizeof(BodyHeader);
  pHeader->s_timestamp   = timestamp;
  pHeader->s_sourceId    = source;
  pHeader->s_barrier = barrier;
  
  copyPayload(pBody, payloadSize);
  updateSize();
  
}

/**
 * constructor 
 * 
 * Given a reference to a ring item create a CRingFragment item:
 * - If the type of the ring item is not EVB_FRAGMENT, throw std::bad_cast.
 * - Otherwise create this from the ring item.
 *
 * @param item - Reference to a generic ring item.
 */
CRingFragmentItem::CRingFragmentItem(const CRingItem& rhs) throw(std::bad_cast) :
  CRingItem(rhs)
{
  if ((type() != EVB_FRAGMENT) && (type() != EVB_UNKNOWN_PAYLOAD)) {
    throw std::bad_cast();
  }
  updateSize();

}

/**
 * Copy constructor.
 *
 * Construct this as an identical copy of an existing item.
 * 
 * @param rhs - const reference to what we are copying.
 */
CRingFragmentItem::CRingFragmentItem(const CRingFragmentItem& rhs) :
  CRingItem(rhs)
{
    updateSize();
}
/**
 * destructor
 *
 * Base class does everything we need:
 */
CRingFragmentItem::~CRingFragmentItem() {}

/**
 * assignment
 *
 *  This is pretty much like copy construction.
 *
 *
 * @param rhs - const reference to what we are copying.
 * @return CRingFragmentItem& reference to *this.
 *  
 */
CRingFragmentItem&
CRingFragmentItem::operator=(const CRingFragmentItem& rhs) 
{
  if (&rhs != this) {
    CRingItem:: operator=(rhs);
    updateSize();
  }
  return *this;
}
/**
 * operator==
 *
 *   There's a really good chance taht two items are equal if they have
 *   the same timestamp, sourcid, size and barrier value
 *
 * @param rhs - Const reference to the item to compare to *this.
 * @return int - nonzero for equality zero otherwise.
 */
int
CRingFragmentItem::operator==(const CRingFragmentItem& rhs) const
{
  return CRingItem::operator==(rhs);


}
/**
 * operator!=
 *   
 * This is just the logical inverse of operator==
 * @param rhs - Const reference to the item to compare to *this.
 * @return int - nonzero for inequality zero otherwise.
 */
int
CRingFragmentItem::operator!=(const CRingFragmentItem& rhs) const
{
  return !(operator==(rhs));
}

/*------------------------------------------------------------------------------
 * getters:
 */

/**
 * Return the current object's timestamp.
 *
 * @return uint64_t
 */
uint64_t
CRingFragmentItem::timestamp() const
{
  return getEventTimestamp();
}
/**
 * source
 *
 * Return the source id.
 *
 * @return uint32_t
 */
uint32_t 
CRingFragmentItem::source() const
{
  return getSourceId();
}
/**
 * return the size of thefragment payload
 * 
 * @return size_t
 */
size_t
CRingFragmentItem::payloadSize() 
{
  pEventBuilderFragment pItem =
    reinterpret_cast<pEventBuilderFragment>(getItemPointer());
    
  return pItem->s_header.s_size - sizeof(RingItemHeader) - sizeof(BodyHeader);

}
/**
 * Return a const pointer to the payload.
 *
 * @return const void*
 */
void*
CRingFragmentItem::payloadPointer()
{
  pEventBuilderFragment pItem =
    reinterpret_cast<pEventBuilderFragment>(getItemPointer());

  return pItem->s_body;  

}
/**
 * return the barrier type:
 *
 * @return int32_t
 */
uint32_t
CRingFragmentItem::barrierType() const
{
  return getBarrierType();
}
/**
 * typeName 
 *   Provide the type of the ringitem as a string.
 *
 * @return std::string "Event fragment ".
 */
std::string
CRingFragmentItem::typeName() const
{
  return "Event fragment";
}
/**
 * toString
 *
 *  Dumps the contents of a ring fragment.
 * @return std::string - the stringified version of the fragment.
 */
std::string
CRingFragmentItem::toString() const
{
  static const int perLine = 16;
  std::ostringstream out;
  CRingFragmentItem* This = const_cast<CRingFragmentItem*>(this);
  
  out << typeName() << ':' << std::endl;
  out << "Fragment timestamp:    " << timestamp()   << std::endl;
  out << "Source ID         :    " << source()      << std::endl;
  out << "Payload size      :    " << This->payloadSize() << std::endl;
  out << "Barrier type      :    " << barrierType() << std::endl;


  out << "- - - - - -  Payload - - - - - - -\n";
  if (CRingItemFactory::isKnownItemType(This->payloadPointer())) {
    
    // Make a low level base class ring item and then invoke the factory
    // to make it the right type of object:
    const _RingItemHeader* pHeader = reinterpret_cast<const _RingItemHeader*>(This->payloadPointer());
    CRingItem       rawItem(pHeader->s_type, pHeader->s_size);

    uint8_t* pBody = reinterpret_cast<uint8_t*>(rawItem.getBodyCursor());
    memcpy(pBody, pHeader+1, pHeader->s_size - sizeof(RingItemHeader));
    pBody += pHeader->s_size - sizeof(RingItemHeader);
    rawItem.setBodyCursor(pBody);

    CRingItem* pRingItem = CRingItemFactory::createRingItem(rawItem);

    out << " Seems to be a ring item: " <<  pRingItem->typeName() << std::endl;
    out << pRingItem->toString();
    delete pRingItem;


  } else {
    out << "   Does not look like a ring item";
    out << std::hex << std::endl;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(This->payloadPointer());
    for (int i = 0; i < This->payloadSize(); i++) {
      out << *p++ << ' ';
      if (((i % perLine) == 0) && (i != 0)) {
	out << std::endl;
      }
    }
    if (This->payloadSize() % perLine) {
      out << std::endl;		// if needed a trailing endl.
    }
  }
    

  return out.str();
}

/*---------------------------------------------------------------------------
 * Private utilities:
 */

/**
 * bodySize
 *
 *  Returns the full size of the body of the event given the size of the payload.
 *
 * @param payloadSize Number of bytes in the payload.
 *
 * @return size_t
 */
size_t
CRingFragmentItem::bodySize(size_t payloadSize) const
{
  return sizeof(EventBuilderFragment) + (payloadSize-sizeof(uint32_t)) -
    sizeof(RingItemHeader);
}
/**
 * copyPayload
 *
 *  Copies the payload source to our payload destination.
 *  - It is assumed the destination size is big enough.
 *  - The cursor is reset after the copy.
 *  - The size is updated after the copy.
 *  - The payload size element is used to determine the data size.
 *
 * @param pPayloadSource - Pointer to the buffer containing the payload.
 * @param size           - Number of bytes in the payload:
 */
void
CRingFragmentItem::copyPayload(const void* pPayloadSource, size_t size)
{
    if (size) {        
      pEventBuilderFragment pFragment =
        reinterpret_cast<pEventBuilderFragment>(getItemPointer());
      memcpy(pFragment->s_body, pPayloadSource, size);
    }
}
/**
 * init
 *
 * Initialize the size and various pointers in the base class.  Ensures the buffer is big enough
 * to take the data.  This should not be done with a fragment that has any data/meaning.
 *
 *  @param size Size of payload
 */
void
CRingFragmentItem::init(size_t size)
{

  deleteIfNecessary();
  newIfNecessary(size);

  uint8_t* pCursor = reinterpret_cast<uint8_t*>(getBodyPointer());
  pCursor         += size;
  setBodyCursor(pCursor);
  updateSize();

}
