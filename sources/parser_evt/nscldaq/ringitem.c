/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2008

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Author:
             Ron Fox
	     NSCL
	     Michigan State University
	     East Lansing, MI 48824-1321
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "DataFormat.h"

/*-----------------------------------------------------------------------------
 *  Static utilities:
 *---------------------------------------------------------------------------*/
static uint32_t swal(uint32_t l)
{
    uint32_t result = 0;
    int      i;
    for (i = 0; i < 4; i++) {
        result = (result << 8) | (l & 0xff);
        l = l >> 8;
    }
    return result;
}

 static void fillHeader(pRingItem pItem, uint32_t size, uint32_t type)
 {
    pItem->s_header.s_size = size;
    pItem->s_header.s_type = type;
 }
 
/**
* sizeStringArray
*
* Determine the number of bytes required to hold a string array.
*
* @param nStrings  - Number of strings.
* @param ppStrings - Array of pointers to null terminated strings.
*
* @return size_t Number of bytes required.
*/
static size_t
sizeStringArray(unsigned nStrings, const char** ppStrings)
{
    int    i;
    size_t result = 0;
    
    for (i = 0; i < nStrings; i++) {
        result += strlen(ppStrings[i]);
        result++;                  /* Count the null terminator too. */
    }
    return result;
 
}
/**
* fillRingHeader
*
*  Fills in the ring item header of an item.
* @param pItem - Pointer to the ring item.
* @param size  - Size of the ring item.
* @param type  - Type of the ring item.
*/
static void
fillRingHeader(pRingItem pItem, uint32_t size, uint32_t type)
{
    pItem->s_header.s_size = size;
    pItem->s_header.s_type = type;
}

/**
 * fillBodyHeader
 *
 *    Fills in the body header of an event given a pointer to the item
 *    and the field values:
 *
 *    @param pItem     - Pointer to the ring item.
 *    @param timestamp - s_timestamp value.
 *    @param sourceId  - Id of source of data.
 *    @param barrier   - Barrier type.
 */
static void
fillBodyHeader(
    pRingItem pItem, uint64_t timestamp, uint32_t sourceId, uint32_t barrier)
{
    /* Locate the body header */
    
    
    pBodyHeader pH = &(pItem->s_body.u_hasBodyHeader.s_bodyHeader);
    
    /*( Fill it in: */
    
    pH->s_size      = sizeof(BodyHeader);
    pH->s_timestamp = timestamp;
    pH->s_sourceId  = sourceId;
    pH->s_barrier   = barrier;
}

/**
 * fillPhysicsBody
 *
 *   Given a pointer to the body of a physics item (uint16_t*) fills in the
 *   body.
 *
 *   @param pBody     - Pointer to the physics body/
 *   @param wordCount - Number of words in the event.
 *   @param pEvent    - Pointer to the event body.
 *
 *   @note that wordCount, corrected for itself is prepended to the event.
 *   @note the body must already have been properly sized.
 */
static void
fillPhysicsBody(void* pBody, uint32_t wordCount, const void* pEvent)
{
    uint32_t* d = (uint32_t*)pBody;
    
    *d++ = wordCount + sizeof(uint32_t)/sizeof(uint16_t); /* Counting itself... */
    if (wordCount) {
        memcpy(d, pEvent, wordCount*sizeof(uint16_t));
    }
}
/**
 * fillEventCountBody
 *
 *  Fills the body of an event count item given the itemPointer and the
 *  stuff to put in it.
 *
 *  @param pItem   - Pointer to the item.
 *  @param offset  - Time offset into the run.
 *  @param divisor - offset/divisor = seconds.
 *  @param unixTime- Unix time of day.
 *  @param count   - Number of events.
 */
static void
fillEventCountBody(
    pRingItem pItem, uint32_t offset, uint32_t divisor, uint32_t unixTime,
    uint64_t count)
{
    pPhysicsEventCountItemBody pBody = (pPhysicsEventCountItemBody)bodyPointer(pItem);
    
    pBody->s_timeOffset    = offset;
    pBody->s_offsetDivisor = divisor;
    pBody->s_timestamp     = unixTime;
    pBody->s_eventCount    = count;
}
/**
 * fillScalerBody
 *
 * Fills the body of a scaler item.
 *
 * @param pItem - Pointer to the item.  The assumption is that the item
 *      is big enough to hold the statically sized data and the scaler counters.
 *
 * @param start       - Interval start time
 * @param end         - Interval end time.
 * @param divisor     - divide start or end by this to get seconds into the run.
 * @param unixTime    - time_t at which this was created.
 * @param count       - Number of scalers.
 * @param incremental - True if these are incremental scalers.
 * @param pScalers    - Pointer to scalers. The assumption is that each scaler
 *                      is a uint32_t.
 */
static void
fillScalerBody(
    pRingItem pItem, uint32_t start, uint32_t end, uint32_t divisor,
    uint32_t unixTime, uint32_t count, int incremental, uint32_t* pScalers)
{
    pScalerItemBody pBody = (pScalerItemBody)bodyPointer(pItem);
    pBody->s_intervalStartOffset = start;
    pBody->s_intervalEndOffset   = end;
    pBody->s_timestamp           = unixTime;
    pBody->s_intervalDivisor     = divisor;
    pBody->s_scalerCount         = count;
    pBody->s_isIncremental       = incremental;
    
    if(count > 0) {
        memcpy(pBody->s_scalers, pScalers, count*sizeof(uint32_t));
    }
}
/**
 * fillTextItemBody
 *
 * Fills in a text item body
 *
 * @param pItem  - Pointer to the text ring item.
 * @param offset - Time offset into run.
 * @param divisor  - offset/divisor = seconds
 * @param unixTime - time_t at which this is being emitted.
 * @param nStrings - Number of strings
 * @param ppStrings - Pointer to the strings.
 */
static void
fillTextItemBody(
    pRingItem pItem, uint32_t offset, uint32_t divisor, uint32_t unixTime,
    uint32_t nStrings, const char** ppStrings)
{
    int i;
    pTextItemBody pBody = (pTextItemBody)bodyPointer(pItem);
    
    pBody->s_timeOffset    = offset;
    pBody->s_timestamp     = unixTime;
    pBody->s_stringCount   = nStrings;
    pBody->s_offsetDivisor = divisor;
    
    char* pDest = pBody->s_strings;
    for (i =0; i < nStrings; i++) {
        size_t copySize = strlen(ppStrings[i]) + 1;
        memcpy(pDest, ppStrings[i], copySize);
        pDest += copySize;
    }
}

/**
 * fillStateChangeBody
 *
 *  Given an item and the stuff that goes into a statechange item's body,
 *  fill in the state change item's body.
 *
 *  @param pItem    - Pointer to the ring item.
 *  @param run      - The run number.
 *  @param offset   - Time offset.
 *  @param divisor  - offset/divisor = seconds.
 *  @param unixTime - Unix time_t at which this is being emitted.
 *  @param title    - Pointer to the run title.
 */
static void
fillStateChangeBody(
    pRingItem pItem, uint32_t run, uint32_t offset, uint32_t divisor,
    uint32_t unixTime, const char* pTitle)
{
    pStateChangeItemBody pBody = (pStateChangeItemBody)bodyPointer(pItem);
    
    pBody->s_runNumber     = run;
    pBody->s_timeOffset    = offset;
    pBody->s_Timestamp     = unixTime;
    pBody->s_offsetDivisor = divisor;
    
    /*
      Zero out the title and copy in at most TITLE_MAXSIZE bytes of title:
      Note that since the storage is TITLE_MAXSIZE+1 our code ensures the
      presence of a terminating null
    */
    
    memset(pBody->s_title, 0, TITLE_MAXSIZE+1);
    strncpy(pBody->s_title, pTitle, TITLE_MAXSIZE);
}
/*------------------------------------------------------------------------------------*/
/**
 * Implement the functions prototypes in DataFormat.h
 * These are functions that produce ring items.
 *
 */

/**
 * bodyPointer
 *
 * Given a ring item that has been filled in up through its body header or mbz
 * field, returns a pointer to the body of the item.
 *
 * @param pItem - Pointer to any type of ring item that has the union format.
 *
 * @return void* - Pointer to the internal payload of the ring item.
 */
void*
bodyPointer(pRingItem pItem)
{
    void* pResult;
    
    /* What we return depends on whether or not there's a zero length for the
        body header:
    */

    
    if (pItem->s_body.u_noBodyHeader.s_mbz) {
        pResult = (void*)pItem->s_body.u_hasBodyHeader.s_body;
    } else {
        pResult = (void*)pItem->s_body.u_noBodyHeader.s_body;
    }
    
    return pResult;
}


/**
 * Format a physics event item.  The payload is put in the event body preceded by the uin32-T
 * nWords. Note that this event will not have  abody header.

 *
 * @param nWords - Number of uint16_t items in the event payload.
 * @param pPayload - The payload of the event.
 *
 *  
 * @return pPhysicsEventitem - Pointer to the new item.
 *         at some point the caller must free(3) the item.
 */
 pPhysicsEventItem  formatEventItem(size_t nWords, void* pPayload)
 {
   /* The size of the resulting physics even item will be
      Size of a RingItemHeader +
      size of a uint32_t (leading word count) +
      size of the mbz word in the empty body header +
      nWords*sizeof(uint16_t):
   */
   size_t itemSize = sizeof(RingItemHeader) + sizeof(uint32_t)*2
    + nWords*sizeof(uint16_t);
   pRingItem pItem;
   void*             pBody;

   pItem = (pRingItem)malloc(itemSize);
   if (!pItem) {
     return (pPhysicsEventItem)pItem;
   }
   
   fillHeader(pItem, itemSize, PHYSICS_EVENT);

   pItem->s_body.u_noBodyHeader.s_mbz = 0;           /* No body header. */
   
   pBody = bodyPointer(pItem);
   
   fillPhysicsBody(pBody, nWords, pPayload);
   return (pPhysicsEventItem)pItem;
 }

/**
 * Create/Format a trigger count item.
 * 
 * @param  runTime - Number of seconds into the run represented by this item.
 * @param  stamp   - Unix timestamp at which the item was emitted.
 * @param  triggerCount - Number of triggers accepted this run.
 *
 * @return pPhysicsEventCount - Pointer; to the formatted item.  This must be passed to
 *                               free(3) to reclaim storage.
*/
pPhysicsEventCountItem
formatTriggerCountItem(uint32_t runTime, time_t stamp, uint64_t triggerCount)
{
  /* Figure out the size of an event count item without a body header:
     Ring item header + PhysicsCountItemBody + mbz longword.
  */
  size_t itemSize =
    sizeof(RingItemHeader) + sizeof(PhysicsEventCountItemBody) + sizeof(uint32_t);
  
  pRingItem pItem = (pRingItem)malloc(itemSize);
  if (!pItem) {
    return (pPhysicsEventCountItem)pItem;
  }

  fillHeader(pItem, itemSize, PHYSICS_EVENT_COUNT);

  pItem->s_body.u_noBodyHeader.s_mbz = 0;
  
  fillEventCountBody(pItem, runTime, 1, stamp, triggerCount);

  return (pPhysicsEventCountItem)pItem;
}
/**
 * Create/format a scaler item.
 * @param scalerCount - Number of 32 bit scaler items in the item.
 * @param timestamp   - Absolute timestamp.
 * @param btime       - Offset to beginning of interval from start of run.
 * @param etime       - Offset to end of interval from start of run.
 * @param pCounters   - The counters.
 *
 * @return pScalerItem Pointer to the scaler aitme we are going to return.  Must be passed to
 *                     free(3) to release storage.
 */
pScalerItem
formatScalerItem(
    unsigned scalerCount, time_t timestamp, uint32_t btime, uint32_t etime,
    void* pCounters)
{
    /* Figure out the size of the item allocate it, fill in the headers dna get
       pointe to the body:
    */
    size_t itemSize =
        sizeof(RingItemHeader) + sizeof(ScalerItemBody) + sizeof(uint32_t)
        + scalerCount*sizeof(uint32_t);
    pRingItem pItem = (pRingItem)malloc(itemSize);
    
  
  
    if(!pItem) {
      return (pScalerItem)pItem;
    }
    
    fillHeader(pItem, itemSize, PERIODIC_SCALERS);

    pItem->s_body.u_noBodyHeader.s_mbz = 0;
    
    fillScalerBody(pItem, btime, etime, 1, timestamp, scalerCount, 1, pCounters);
   
    /* Return a pointer to the item. */
    return (pScalerItem)pItem;
}

/**
 * formatNonIncrTSScalerItem
 *
 *  Format a timestamped non incremental scaler item.  The timestamp in this case refers
 *  to a timestamp synched to the event trigger timestamps (e.g. for event building ordering
 *
 * @param scalerCount - Number of 32 bit scaler items in the item.
 * @param timestamp   - Absolute, unix, timestamp.
 * @param btime       - Offset to beginning of interval from start of run.
 * @param etime       - Offset to end of interval from start of run.
 * @param eventTimestamp - Event timestamp.
 * @param pCounters   - The counters.
 * @param timebasDivisor - What to divide the btime/etime values by to get seconds.
 *
 * @return pNonIncrTSScaleritem Pointer to the scaler aitme we are going to return.  Must be passed to
 *                     free(3) to release storage. 
 */
pScalerItem
formatNonIncrTSScalerItem(unsigned scalerCount, time_t timestamp, 
			  uint32_t btime, uint32_t etime, 
			  uint64_t eventTimestamp, void* pCounters,
			  uint32_t timebaseDivisor)
{
    /* This has been superseded by formatTimestampedScalerItem */
    
    return formatTimestampedScalerItem(
        eventTimestamp, 0, 0, 0, timebaseDivisor, timestamp, btime, etime,
        scalerCount, pCounters
    );

}


/**
 * Format a set of text strings into a Textitem.
 *
 * @param nStrings - number of strings to format.
 * @param stamp    - Absolute timestamp.
 * @param runTime  - Time into run.
 * @param pStrings - Pointer to the array of string pointers.
 * @param type     - Type of item to create (e.g. MONITORED_VARIABLES).
 *
 * @return pTextItem - pointer to the formatted text item.  This is dynamically allocated and must be
 *                     passed to free(3) to release the storage.
 */

pTextItem
formatTextItem(unsigned nStrings, time_t stamp, uint32_t runTime,  const char** pStrings, int type)
{
    pRingItem pItem;
    char*     pDest;
    int       i;
    /* Figure out the string storage size required: */
  
    size_t stringSize = sizeStringArray(nStrings, pStrings);

    /* Figure out the total size requirement, fill in the headers and get a
       body pointer
    */
    size_t itemSize =
        stringSize + sizeof(RingItemHeader) + sizeof(TextItemBody)
        + sizeof(uint32_t);  
    pItem = (pRingItem)malloc(itemSize);
    
    if (!pItem) {
      return (pTextItem)pItem;
    }
    fillHeader(pItem, itemSize, type);

    pItem->s_body.u_noBodyHeader.s_mbz = 0;
    
    
    fillTextItemBody(pItem, runTime, 1, stamp, nStrings, pStrings);
    
    return (pTextItem)pItem;
}

/**
 * Allocate and format a state change item
 *
 * @param stamp - Absolute timestamp.
 * @param offset - Seconds into the run of the state change.
 * @param runNumber - Number of the run.
 * @param pTitle  - Pointer to the null terminated title string.
 * @param type    - Type of item being created e.g. BEGIN_RUN.
 *
 * @return pStateChangeItem - Pointer to the storage allocated for the state change
 *                            item.. Must be passed to free(3) to reclaim storage.
 */
pStateChangeItem
formatStateChange(time_t stamp, uint32_t offset, uint32_t runNumber,
		  const char* pTitle, int type)
{
    
  /* Figure out the size of the item and attempt to allocate it. */
  
  size_t itemSize =
    sizeof(RingItemHeader) + sizeof(StateChangeItemBody) + sizeof(uint32_t);
  pRingItem pItem = (pRingItem)malloc(itemSize);
  
  /* Only fill in stuff if the allocations worked. */
  
  if (pItem) {
    /* Fill in the headers and get a body pointer: */
    
    fillHeader(pItem, sizeof(StateChangeItem), type);

    pItem->s_body.u_noBodyHeader.s_mbz   = 0;
    fillStateChangeBody(pItem, runNumber, offset, 1, stamp, pTitle);
 
  }
  /* Either return the null pointer we got or the filled in beast: */
  
  return (pStateChangeItem)pItem;
  
}
/*---------------------------------------------------------------------------
 * Format timestamped items... since version 11.0.
 */

/**
 * formatDataFormat
 *
 * Return a pointer to a filled in data format ring item.  These ring items
 * are used to indicate to consumers the format of ring items that follow.
 * The major and minor versions put into that item are those defined in the
 * header.
 *
 * @return pDataFormat -  Pointer to a new data format item that must be
 *                        freed usung free(3).
 */
pDataFormat
formatDataFormat()
{
    pDataFormat pItem = malloc(sizeof(DataFormat));
    if (pItem) {
      fillHeader((pRingItem)pItem, sizeof(DataFormat), RING_FORMAT);
        
        pItem->s_mbz           = 0;
        pItem->s_majorVersion  = FORMAT_MAJOR;
        pItem->s_minorVersion  = FORMAT_MINOR;
    }
    
    return pItem;
}
/**
 * formatEVBFragment.
 *
 * Fills in an undifferentiated event builder fragment.  These are used
 * to allow software to snoop on the output of the event orderer.
 *
 * @param timestamp - Timestamp associated with the fragment payload.
 * @param sourceId  - Event fragment source that contributed the data.
 * @param barrier   - Barrier id of the fragment.
 * @param payloadSize - Number of _bytes_ in the payload.
 * @param pPayload    - Pointer to the payload.
 *
 * @return pEventBuilderFragment - pointer to an event fragment ring item.
 *  The ultimate caller must deallocate the storage associated with this item
 *  via a call to free(3).
 *  @retval null - if the item could not be allocated.
 */
pEventBuilderFragment
formatEVBFragment(uint64_t timestamp, uint32_t sourceId, uint32_t barrier,
    uint32_t payloadSize, const void* pPayload)
{
    /* Figure out how big the item is going to be and allocate it. */
    
    size_t itemSize =
        sizeof(EventBuilderFragment) + (payloadSize);
    pEventBuilderFragment pItem = (pEventBuilderFragment)malloc(itemSize);
    
    /* Only fill in the item if we could allocate it. */
    
    if (pItem) {
        fillHeader((pRingItem)pItem, itemSize, EVB_FRAGMENT);
        
        pItem->s_bodyHeader.s_size        = sizeof(BodyHeader);
        pItem->s_bodyHeader.s_timestamp   = timestamp;
        pItem->s_bodyHeader.s_sourceId    = sourceId;
        pItem->s_bodyHeader.s_barrier     = barrier;
        
        if (payloadSize) {
            memcpy(pItem->s_body, pPayload, payloadSize);
        }
    }
    /* Return the item or null if allocation failed. */
    return pItem;
}
/**
 * formatEVBFragmentUnknown
 *
 * same as above but the item type is set to EVB_UNKNOWN_PAYLOAD
 *
 *
 * @param timestamp - Timestamp associated with the fragment payload.
 * @param sourceId  - Event fragment source that contributed the data.
 * @param barrier   - Barrier id of the fragment.
 * @param payloadSize - Number of _bytes_ in the payload.
 * @param pPayload    - Pointer to the payload.
 *
 * @return pEventBuilderFragment - pointer to an event fragment ring item.
 *  The ultimate caller must deallocate the storage associated with this item
 *  via a call to free(3).
 *  @retval null - if the item could not be allocated.
 */
pEventBuilderFragment
formatEVBFragmentUnknown(uint64_t timestamp, uint32_t sourceId, uint32_t barrier,
    uint32_t payloadSize, const void* pPayload)
{
    pEventBuilderFragment pFrag = formatEVBFragment(
        timestamp, sourceId, barrier, payloadSize, pPayload
    );
    pFrag->s_header.s_type = EVB_UNKNOWN_PAYLOAD;
    
    return pFrag;
}
/**
 * formatTimestampedEventItem
 *
 *   Format a Physics data item that has a timestamp/data source header.
 *
 *   @param timestamp - the event timestamp
 *   @param sourceId  - the data source id.
 *   @param barrier   - The barrier type.
 *   @param payloadSize - Size of the physics payload.
 *   @param pPayload   - The physics event payload in uint16_t.
 *
 *  @return pPhyscisEventItem that has been dynamically created and which the
 *          caller must deallocate using free(3).
 *  @retval NULL - indicates the data allocation failed.
 *
 *  @note The payload will get its size prepended to it...and the actual payload
 *         size accordingly adjusted.
 */
pPhysicsEventItem
formatTimestampedEventItem(
    uint64_t timestamp, uint32_t sourceId, uint32_t barrier,
    uint32_t payloadSize, const void* pPayload)
{
    /* Figure out the size of the event and allocate it: */
    
    size_t itemSize = sizeof(RingItemHeader) + sizeof(BodyHeader)
        + payloadSize*sizeof(uint16_t) + sizeof(uint32_t);
    pRingItem pItem = (pRingItem)malloc(itemSize);
    
    /* Only bother to fill it in if it wass successfully allocated. */
    
    if (pItem) {
        /* Fill in the headers: */
        
       
        fillHeader(pItem, itemSize, PHYSICS_EVENT);
        fillBodyHeader(pItem, timestamp, sourceId, barrier);
        
        void* pBody = bodyPointer(pItem);
        fillPhysicsBody(pBody, payloadSize , pPayload);
    }
    
    /* Return the item pointer or null if malloc failed: */
    
    return (pPhysicsEventItem)pItem;
}
/**
 * formatTimestampedTriggerCountItem
 *
 * Creates a ring item that records trigger count information but
 * and has a body header to provide timestamp and event source information.
 *
 * @param timestamp - Event clock timestamp to associate with this item.
 * @param sourceId  - Id of the source that generated this item.
 * @param barrier   - Barrier type of the item.
 * @param runTime   - Length of time into the run.
 * @param offsetDivisor - What to divide run time by to get seconds
 * @param stamp     - Unix time_t defining when this was emitted.
 * @param triggerCount - Number of triggers received this run.
 * 
 * @return pPhysicsEventCountItem  - pointer to the ring item.  The item was
 *      dynamicaly allocated and must be released via a call to free(3).
 * @retval NULL - the item could not be allocated.
 */
pPhysicsEventCountItem
formatTimestampedTriggerCountItem (
    uint64_t timestamp, uint32_t sourceId, uint32_t barrier,uint32_t runTime,
    uint32_t offsetDivisor, time_t stamp, uint64_t triggerCount)
{
    /* Figure out how big the item is and allocated it: */
    
    size_t itemSize =
        sizeof(RingItemHeader) + sizeof(BodyHeader) + sizeof(PhysicsEventCountItemBody);
    pRingItem pItem = (pRingItem)malloc(itemSize);
    
    /* Only fill in the item if we got it: */
    
    if (pItem) {
        /* Fill in the header: */
        
        fillHeader((pRingItem)pItem, itemSize, PHYSICS_EVENT_COUNT);
        
        
        /* Fill in the body header */
        
        fillBodyHeader(pItem, timestamp, sourceId, barrier);
        
        /* Fill in the body */
        
        fillEventCountBody(pItem, runTime, offsetDivisor, stamp, triggerCount);
    }
    
    /* Return either the item or null if allocation failed. */
    
    return (pPhysicsEventCountItem)pItem;
}
/**
 * formatTimestampedScalerItem
 *
 * Allocates and fills in a ScalerItem that has a full body header.
 *
 * @param timestamp           - Event timestamp associated with the item.
 * @param sourceId            - Id of source that emitted this item.
 * @param barrier             - Barrier type of the item.
 * @param isIncremental       - Non zero if the scaler counters are incremental
 * @param timeInternalDivisor - Divide time offsets by this to get seconds
 * @param timeofDay           - Unix timestamp at which the item was emitted.
 * @param btime               - Interval start time.
 * @param etime               - Interval end time
 * @param nScalers            - Number of scalers.
 * @param pCounters           - Pointer to the counters.
 *
 * @return pScalerItem pointer to the scaler item.  This was dynamically allocated
 *      the caller must use free(3) to release its storage.
 */
pScalerItem
formatTimestampedScalerItem(
    uint64_t timestamp, uint32_t sourceId, uint32_t barrier,
    int isIncremental, uint32_t timeIntervalDivisor, uint32_t timeofday,
    uint32_t btime, uint32_t etime, uint32_t nScalers, void* pCounters    
)
{
    /* Figure out how big the item is and allocated storage for it: */
    
    size_t itemSize =
        sizeof(RingItemHeader) + sizeof(BodyHeader) + sizeof(ScalerItemBody)
        + nScalers * sizeof(uint32_t);
    pRingItem pItem = (pRingItem)malloc(itemSize);
    
    if (pItem) {    
        /* Fill in the variousl chunk of the item: */
        
        fillRingHeader(pItem, itemSize, PERIODIC_SCALERS);
        fillBodyHeader(pItem, timestamp, sourceId, barrier);
        fillScalerBody(
            pItem, btime, etime, timeIntervalDivisor, timeofday, nScalers,
            isIncremental, pCounters
        );
    }
    return (pScalerItem)pItem;
}
/**
 * formatTimestampedTextItem
 *
 *   Formats a text item that has a full body header.
 *
 * @param timestamp    - Event timestamp.
 * @param sourceId     - Id of the source.
 * @param barrier      - Barrier type of the item.
 * @param nStrings     - Number of strings to pack.
 * @param stamp        - Unix time at which this item was emitted.
 * @param runTime      - Offset into the run.
 * @param ppStrings    - Pointer to the strings.
 * @param type         - The actual title string.
 * @param timeInternalDivisor - runTime/timeInternalDivisor  = seconds.
 *
 * @return pTextItem A text item which has been dynamically allocated
 *      and filled in from the parameter data.  The storage for this item
 *      type must be released with free(3).
 */
pTextItem
formatTimestampedTextItem(
    uint64_t timestamp, uint32_t sourceId, uint32_t barrier, unsigned nStrings,
    time_t stamp, uint32_t runTime, const char** ppStrings, int type,
    uint32_t timeIntervalDivisor    
)
{
    /* Figuer out how big this item is: */
    
    size_t itemSize = sizeof(RingItemHeader) + sizeof(BodyHeader) + sizeof(TextItemBody) +
        sizeStringArray(nStrings, ppStrings);
    pRingItem pItem = (pRingItem)malloc(itemSize);
    
    /* Fill in the stuff:  */
    
    if(pItem) {
        
        fillRingHeader(pItem, itemSize, type);
        fillBodyHeader(pItem, timestamp, sourceId, barrier);
        fillTextItemBody(
            pItem, runTime, timeIntervalDivisor, stamp, nStrings, ppStrings
        );
    }
    return (pTextItem)pItem;
}
/**
 * formatTimestampedStateChange
 *
 *  Allocates and formats a state change item that has a full body header.
 *
 *  @param timestamp - Event timestamp
 *  @param sourceId  - Id of emitting source.
 *  @param barrier   - type of barrier event.
 *  @param stamp     - unix timestamp
 *  @param offset    - Run time offset.
 *  @param runNumber - The run number that's making the transition.
 *  @param offsetDivisor - offset/offsetDivisor = seconds.
 *  @param pTitle    - Run title.
 *  @param type      - Ring item type.
 *
 *  @return pStateChangeItem that has been dynamically allocated and must be
 *       released with free(3).
 */
pStateChangeItem
formatTimestampedStateChange(
    uint64_t timestamp, uint32_t sourceId, uint32_t barrier, time_t stamp,
    uint32_t offset, uint32_t runNumber, uint32_t offsetDivisor,
    const char* pTitle, int type
)
{
    /* Figure out the item size and allocate it: */
    
    size_t itemSize =
        sizeof(RingItemHeader) + sizeof(BodyHeader) + sizeof(StateChangeItemBody);
    pRingItem pItem = (pRingItem)malloc(itemSize);
    
    if (pItem) {
        fillHeader(pItem, itemSize, type);
        fillBodyHeader(pItem, timestamp, sourceId, barrier);
        fillStateChangeBody(
            pItem, runNumber, offset, offsetDivisor, stamp, pTitle
        );
    }
    return (pStateChangeItem)pItem;
}
/**
 * formatGlomParameters
 *
 * Format a EVB_GLOM_INFO ring item.  This items describes the parameters
 * of the current GLOM event builder.
 *
 * @param interval - Ticks that define a coincidence interval if building.
 * @param isBuilding - on zero if glom is building events.
 * @param timestampPolicy - value to put in timestamp policy field.
 * 
 * @return pGlomParameters - pointer to a malloc()'d GlomParameters ring item
 *                           that was filled in by this function
 * @retval null is returned if the allocation of the ring item failed.
 */
pGlomParameters
formatGlomParameters(uint64_t interval, int isBuilding, int timestampPolicy)
{
    pGlomParameters pResult = (pGlomParameters)malloc(sizeof(GlomParameters));
    if (pResult) {
        pResult->s_header.s_size = sizeof(GlomParameters);
        pResult->s_header.s_type = EVB_GLOM_INFO;
        
        pResult->s_mbz = 0;
        pResult->s_coincidenceTicks = interval;
        pResult->s_isBuilding = isBuilding;
        pResult->s_timestampPolicy = timestampPolicy;
    }
    
    return pResult;
}
/**
 * formatAbnormalEndItem
 *    Creates an abnormal end item.
 *
 * @return pAbnormalEndItem  dynamically allocated.
 */
pAbnormalEndItem
formatAbnormalEndItem()
{
    pAbnormalEndItem p = (pAbnormalEndItem)(malloc(sizeof(AbnormalEndItem)));
    p->s_mbz = 0;
    p->s_header.s_size = sizeof(AbnormalEndItem);
    p->s_header.s_type = ABNORMAL_ENDRUN;
    
    return p;
}
/**
 * itemSize
 *    Return the size of an item regardless of the byte swapping.
 *
 *  @param item - Pointer to the item.
 *
 *  @return uint32_t the ring item size.
 */
uint32_t
itemSize(const RingItem* pItem)
{
    if (mustSwap(pItem)) {
        return swal(pItem->s_header.s_size);
    } else {
        return pItem->s_header.s_size;
    }
}
/**
 * itemType
 *   Return the type of the ring item regardless of the byte swappingh.
 *
 * @param pItem - Pointer to the item.
 *
 * @return uint16_t the ring item type
 */
uint16_t
itemType(const RingItem* pItem)
{
    if (mustSwap(pItem)) {
        return swal(pItem->s_header.s_type);
    } else {
        return pItem->s_header.s_type;
    }
}
/**
 * mustSwap
 *    Returns true (nonzero) if the item is byteswapped relative to the
 *    host.
 *
 * @param pItem Pointer to a ring item.
 * @return int  - nonzero if the item is byte swapped relative to the
 *                host.
 */
int
mustSwap(const RingItem* pItem)
{
    return ((pItem->s_header.s_type & 0x0000ffff) == 0);
}
