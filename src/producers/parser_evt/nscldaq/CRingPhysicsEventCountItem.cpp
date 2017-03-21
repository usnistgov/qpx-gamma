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
#include "CRingPhysicsEventCountItem.h"
#include <sstream>

using namespace std;

///////////////////////////////////////////////////////////////////////////////////
//
// Constructors and other canonicals.

/*!
   Default constructor has a timestamp of now, a time offset and
   event count of 0.
*/
CRingPhysicsEventCountItem::CRingPhysicsEventCountItem() :
  CRingItem(PHYSICS_EVENT_COUNT)
{
  init();
  pPhysicsEventCountItemBody pItem =
    reinterpret_cast<pPhysicsEventCountItemBody>(getBodyPointer());
    
  pItem->s_timeOffset = 0;
  pItem->s_timestamp = static_cast<uint32_t>(time(NULL));
  pItem->s_eventCount = 0;
  pItem->s_offsetDivisor = 1;
  setBodyCursor(&(pItem[1]));
  updateSize();
}
/*!
   Creates an event count item timestamped to now with a specified
   number of events and run offset.

   \param count       - number of events.
   \param timeOffset  - Seconds into the active run time at which this item
                        was produced.
*/
CRingPhysicsEventCountItem::CRingPhysicsEventCountItem(uint64_t count,
						       uint32_t timeOffset) :
  CRingItem(PHYSICS_EVENT_COUNT)
{

  pPhysicsEventCountItemBody pItem =
    reinterpret_cast<pPhysicsEventCountItemBody>(getBodyPointer());
  
  pItem->s_timeOffset  = timeOffset;
  pItem->s_timestamp = static_cast<uint32_t>(time(NULL));
  pItem->s_eventCount = count;
  pItem->s_offsetDivisor = 1;
  setBodyCursor(&(pItem[1]));
  updateSize();
}
/*!
  Creates an event count item that is fully specified.
   \param count       - number of events.
   \param timeOffset  - Seconds into the active run time at which this item
   \param stamp       - Timestamp at which the event was produced.
*/
CRingPhysicsEventCountItem::CRingPhysicsEventCountItem(uint64_t count,
						       uint32_t timeOffset,
						       time_t   stamp) :
  CRingItem(PHYSICS_EVENT_COUNT)
{
  init();
  pPhysicsEventCountItemBody pItem =
    reinterpret_cast<pPhysicsEventCountItemBody>(getBodyPointer());

  pItem->s_timeOffset  = timeOffset;
  pItem->s_timestamp  = stamp;
  pItem->s_eventCount = count;
  pItem->s_offsetDivisor = 1;
  setBodyCursor(&(pItem[1]));
  updateSize();
}

/**
 * construtor
 *
 * Construct an event count item that includes a body header.
 *
 * @param timestamp - Event timestamp value.
 * @param source    - Id of the data source.
 * @param barrier   - Barrier type or 0 if not a barrier.
 * @param count     - Number of physics events.
 * @param timeoffset  - How long into the run we are.
 * @param stamp      - Unix timestamp.
 * @param divisor    - timeoffset/divisor = seconds.
 */
CRingPhysicsEventCountItem::CRingPhysicsEventCountItem(
    uint64_t timestamp, uint32_t source, uint32_t barrier,
    uint64_t count, uint32_t timeoffset, time_t stamp,
    int divisor) :
  CRingItem(PHYSICS_EVENT_COUNT, timestamp, source, barrier)
{
  pPhysicsEventCountItemBody pBody =
    reinterpret_cast<pPhysicsEventCountItemBody>(getBodyPointer());
  pBody->s_timeOffset    = timeoffset;
  pBody->s_offsetDivisor = divisor;
  pBody->s_timestamp     = stamp;
  pBody->s_eventCount    = count;

  setBodyCursor(&(pBody[1]));
  updateSize();
}

 
/*!
  Construction from an existing ring item.
  \param rhs - an existing ring item.

  \throw std::bad_cast if rhs is not a PHYSICS_EVENT_COUNT item.
*/
CRingPhysicsEventCountItem::CRingPhysicsEventCountItem(const CRingItem& rhs) throw(std::bad_cast)  :
  CRingItem(rhs)
{
  if (type() != PHYSICS_EVENT_COUNT) {
    throw bad_cast();
  }
  init();

}
/*!
  Construction from an existing physics event count item.
  \param rhs   the existing item.
*/
CRingPhysicsEventCountItem::CRingPhysicsEventCountItem(const CRingPhysicsEventCountItem& rhs) :
  CRingItem(rhs)
{
  init();
}
/*!
  Destructor chaining:
*/
CRingPhysicsEventCountItem::~CRingPhysicsEventCountItem()
{}

/*!
   Assignment:
   \param rhs - Item assigned to *this
   \return CRingPhysicsEventCountItem&
   \retval *this
*/
CRingPhysicsEventCountItem&
CRingPhysicsEventCountItem::operator=(const CRingPhysicsEventCountItem& rhs)
{
  if (this != &rhs) {
    CRingItem::operator=(rhs);
    init();
  }
  return *this;
}
/*!
   Equality comparison.
   \param rhs - item being compared to *this.
   \return int
   \retval 0  - Not equal
   \retval 1  - Equal.
*/
int
CRingPhysicsEventCountItem::operator==(const CRingPhysicsEventCountItem& rhs) const
{
  return CRingItem::operator==(rhs);
}
/*!
  Inequality compare
  \param rhs - item being compared to *this.
  \return int
  \retval !(operator==(rhs)).
*/
int
CRingPhysicsEventCountItem::operator!=(const CRingPhysicsEventCountItem& rhs) const
{
  return !(*this == rhs);
}
//////////////////////////////////////////////////////////////////////////////////
//
//  object interface:
//

/*!
    \return uint32_t
    \retval The curren value of the time offset.
*/
uint32_t
CRingPhysicsEventCountItem::getTimeOffset() const
{
  pPhysicsEventCountItemBody pItem =
    reinterpret_cast<pPhysicsEventCountItemBody>(getBodyPointer());

  return pItem->s_timeOffset;
}
/**
 * computeTimeOffset
 *
 * Get the floating point seconds into the run using the time offset and
 * divisor fields:
 *
 * @return float
 */
float
CRingPhysicsEventCountItem::computeElapsedTime() const
{
    pPhysicsEventCountItemBody pItem =
    reinterpret_cast<pPhysicsEventCountItemBody>(getBodyPointer());
    
    float timeOffset = pItem->s_timeOffset;
    float divisor    = pItem->s_offsetDivisor;
    
    return timeOffset/divisor;
}
/**
 * getTimeDivisor
 *   Return the divisor used to compute the offset.
 *
 * @return uint32_t
 */
uint32_t
CRingPhysicsEventCountItem::getTimeDivisor() const
{
  pPhysicsEventCountItemBody pItem =
    reinterpret_cast<pPhysicsEventCountItemBody>(getBodyPointer());
  return pItem->s_offsetDivisor;
}
/*!
   set the time offset.
   \param offset - new value of time offset.
*/
void
CRingPhysicsEventCountItem::setTimeOffset(uint32_t offset)
{
  pPhysicsEventCountItemBody pItem =
    reinterpret_cast<pPhysicsEventCountItemBody>(getBodyPointer());

  pItem->s_timeOffset = offset;
}

/*!
  \return time_t
  \retval current value of the timestamp.
*/
time_t
CRingPhysicsEventCountItem::getTimestamp() const
{
  pPhysicsEventCountItemBody pItem =
    reinterpret_cast<pPhysicsEventCountItemBody>(getBodyPointer());

  return pItem->s_timestamp;
}
/*!
  \param stamp - New value for the timestamp.
*/
void
CRingPhysicsEventCountItem::setTimestamp(time_t stamp)
{
  pPhysicsEventCountItemBody pItem =
    reinterpret_cast<pPhysicsEventCountItemBody>(getBodyPointer());

  pItem->s_timestamp = stamp;
}

/*!
  \return uint64_t
  \retval number of events submitted  to the ring this run.
*/
uint64_t
CRingPhysicsEventCountItem::getEventCount() const
{
  pPhysicsEventCountItemBody pItem =
    reinterpret_cast<pPhysicsEventCountItemBody>(getBodyPointer());

  return pItem->s_eventCount;
}
/*!
  \param count - new value for number of events submitted.
*/
void
CRingPhysicsEventCountItem::setEventCount(uint64_t count)
{
  pPhysicsEventCountItemBody pItem =
    reinterpret_cast<pPhysicsEventCountItemBody>(getBodyPointer());

  pItem->s_eventCount = count;
}
//////////////////////////////////////////////////////////
//
// Virtual method overrides.


/**
 * typeName
 * 
 *   Provides the type name for this ring item.
 *
 * @return std::string type name.
 */
std::string
CRingPhysicsEventCountItem::typeName() const
{
  return std::string("Trigger count");
}
/**
 * toString
 *
 * Returns a stringified version of the item.
 *
 * @return item - the string.
 */
std::string
CRingPhysicsEventCountItem::toString() const
{
  std::ostringstream out;

  string   time   = timeString(getTimestamp());
  uint32_t offset = getTimeOffset();
  uint64_t events = getEventCount();

  out << bodyHeaderToString();
  out << time << " : " << events << " Triggers accepted as of " 
      << offset << " seconds into the run\n";
  out << " Average accepted trigger rate: " 
      <<  (static_cast<double>(events)/static_cast<double>(offset))
      << " events/second \n";

  return out.str();
}

/////////////////////////////////////////////////////////////////////////////////
//
// Private utilities.
//


/*
** initialize by setting the item pointer and body cursor
** appropriatly.
*/
void
CRingPhysicsEventCountItem::init()
{


}
