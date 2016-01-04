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


#include "CRingItemFactory.h"
#include "CRingItem.h"
#include "CPhysicsEventItem.h"
#include "CRingFragmentItem.h"
#include "CRingPhysicsEventCountItem.h"
#include "CRingScalerItem.h"
#include "CRingStateChangeItem.h"
#include "CRingTextItem.h"
#include "CPhysicsEventItem.h"
#include "CDataFormatItem.h"
#include "CUnknownFragment.h"
#include "CGlomParameters.h"
#include "CAbnormalEndItem.h"
#include "DataFormat.h"

#include <vector>
#include <string>
#include <string.h>
#include <set>

static std::set<uint32_t> knownItemTypes;

/**
 * Create a ring item of the correct underlying type as indicated by the
 * ring item type.  Note that the result is dynamically allocated and must be
 * freed by the caller (via delete).
 *
 * @param item - Reference to the item.
 * 
 * @return CRingItem*
 * @retval Pointer to an object that is some subclass of CRingItem.  Note that if the
 *         ring item type is not recognized, a true CRing Item is produced.
 *
 */
CRingItem*
CRingItemFactory::createRingItem(const CRingItem& item)
{
  CRingItem& Item (const_cast<CRingItem&>(item)); // We'll need this here&there
  switch (item.type()) {
    // State change:

  case BEGIN_RUN:
  case END_RUN:
  case PAUSE_RUN:
  case RESUME_RUN:
    {
      return new CRingStateChangeItem(item);
    }

    // String list.

  case PACKET_TYPES:
  case MONITORED_VARIABLES:
    {
      return new CRingTextItem(item);
    }
    // Scalers:

  case PERIODIC_SCALERS:
    {
      return new CRingScalerItem(item); 
    }

    // Physics trigger:

  case PHYSICS_EVENT:
    {
      CPhysicsEventItem* pItem;
      if(item.hasBodyHeader()) {
        pItem = new CPhysicsEventItem(
            item.getEventTimestamp(), item.getSourceId(), item.getBarrierType(),
            item.getStorageSize()
        );
      } else {
        pItem = new CPhysicsEventItem(item.getStorageSize());
      }
      uint8_t* pDest = reinterpret_cast<uint8_t*>(pItem->getBodyCursor());
      memcpy(pDest, 
	     const_cast<CRingItem&>(item).getBodyPointer(), item.getBodySize());
      pDest += item.getBodySize();
      pItem->setBodyCursor(pDest);
      pItem->updateSize();
      return pItem;
    }
    // trigger count.

  case PHYSICS_EVENT_COUNT:
    {
      return new CRingPhysicsEventCountItem(item);
      break;
    }
  // /Event builder fragment.
  case EVB_FRAGMENT:
    {
      return new CRingFragmentItem(item);
    }
  case RING_FORMAT:
    return new CDataFormatItem(item);
  case EVB_UNKNOWN_PAYLOAD:
    return new CUnknownFragment(item);
  case EVB_GLOM_INFO:
    return new CGlomParameters(item);
    break;

  case ABNORMAL_ENDRUN:
    return new CAbnormalEndItem(item);
    break;
    
   // Nothing we know about:

  default:
    return new CRingItem(item);
  }
}
/**
 *  createRingItem
 *
 *  Create a ring item object given a pointer that is believed to point to a 
 *  ring item structure (pRingItem).
 *
 * @param pItem - Pointer to the pRingItem.
 *
 * @return CRingItem* - dynamically allocated with the underlying type matching that
 *                      of s_header.s_type.
 * @throw std::string if the type does not match a known ring item type.
 */
CRingItem*
CRingItemFactory::createRingItem(const void* pItem)
{
  if (!isKnownItemType(pItem)) {
    throw std::string("CRingItemFactory::createRingItem - unknown ring item type");
  }
  /* Make a 'vanilla' CRing item that we can pass into the other creator */

  const RingItem* pRitem = reinterpret_cast<const RingItem*>(pItem);
  CRingItem baseItem(pRitem->s_header.s_type, pRitem->s_header.s_size);
  uint8_t*  pBody  = reinterpret_cast<uint8_t*>(baseItem.getItemPointer());
  memcpy(pBody, pRitem, pRitem->s_header.s_size);
  pBody += pRitem->s_header.s_size;
  baseItem.setBodyCursor(pBody);
  baseItem.updateSize();

  return createRingItem(baseItem);
}
/**
 * Determines if a  pointer points to something that might be a valid ring item.
 * - Item must have at least the size of a header.
 * - Item must be a recognized ring item.
 *
 * @param pItem - pointer to the data.
 * 
 * @return bool - true if we htink this is a valid ring item.
 */
bool
CRingItemFactory::isKnownItemType(const void* pItem)
{

  const RingItem* p = reinterpret_cast<const RingItem*>(pItem);
  if (itemSize(p) < sizeof(RingItemHeader)) {
    return false;
  }

  // if necessary stock the set of known item types:

  if(knownItemTypes.empty()) {
    knownItemTypes.insert(BEGIN_RUN);
    knownItemTypes.insert(END_RUN);
    knownItemTypes.insert(PAUSE_RUN);
    knownItemTypes.insert(RESUME_RUN);
    knownItemTypes.insert(RING_FORMAT);

    knownItemTypes.insert(PACKET_TYPES);
    knownItemTypes.insert(MONITORED_VARIABLES);

    knownItemTypes.insert(PERIODIC_SCALERS);
    knownItemTypes.insert(PHYSICS_EVENT);
    knownItemTypes.insert(PHYSICS_EVENT_COUNT);
    knownItemTypes.insert(EVB_FRAGMENT);
    knownItemTypes.insert(EVB_UNKNOWN_PAYLOAD);
    knownItemTypes.insert(EVB_GLOM_INFO);
    
    knownItemTypes.insert(ABNORMAL_ENDRUN);
  }

  return knownItemTypes.count(itemType(p)) > 0;

}
