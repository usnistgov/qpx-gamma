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
#include "CRingScalerItem.h"
#include <time.h>
#include <string.h>
#include <sstream>
#include <stdio.h>

using namespace std;

uint32_t CRingScalerItem::m_ScalerFormatMask(0xffffffff); // by default scalers are 32 bits wide.

///////////////////////////////////////////////////////////////////////////////////////
//
// Constructors and other canonicals.
//

/*!
  Construct a scaler item with the scalers all zeroed and the times not set...
  except for the absolute timestamp which is set to now.
  \param numScalers - Number of scalers that will be put in the item.
*/
CRingScalerItem::CRingScalerItem(size_t numScalers) :
  CRingItem(PERIODIC_SCALERS,
	    bodySize(numScalers))
{
  init(numScalers);
  pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
  
  pScalers->s_intervalStartOffset = 0;
  pScalers->s_intervalEndOffset   = 0;
  pScalers->s_timestamp = static_cast<uint32_t>(time(NULL));
  pScalers->s_scalerCount = numScalers;
  pScalers->s_isIncremental = 1;
  pScalers->s_intervalDivisor = 1;
  memset(pScalers->s_scalers, 0, numScalers*sizeof(uint32_t));
}
/*!
  Construct a scaler item with all the info specified.
  \param startTime - incremental scaler interval start time.
  \param endTime   - incremental scaler interval end time
  \param timestamp - Absolute system time at which the item was created.
  \param scalers   - Vector of scaler values.
  \param isIncremental - value of the incremental flag (defaults to true).
  \param timestampDivisor - Value of the timestamp divisor - defaults to 1.


*/
CRingScalerItem::CRingScalerItem(uint32_t startTime,
				 uint32_t stopTime,
				 time_t   timestamp,
				 std::vector<uint32_t> scalers,
                                 bool isIncremental,
                                 uint32_t timeOffsetDivisor) :
  CRingItem(PERIODIC_SCALERS, bodySize(scalers.size()))
{
  init(scalers.size());

  pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
    
  pScalers->s_intervalStartOffset = startTime;
  pScalers->s_intervalEndOffset   = stopTime;
  pScalers->s_timestamp           = timestamp;
  pScalers->s_scalerCount         = scalers.size();
  pScalers->s_isIncremental = isIncremental ? 1: 0;
  pScalers->s_intervalDivisor = timeOffsetDivisor;

  for (int i = 0; i  < scalers.size(); i++) {
    pScalers->s_scalers[i] = scalers[i];
  }
  updateSize();
}
/**
 * constructor - with body header.
 *
 * Construcs a scaler ring item that has a body header.
 *
 * @param eventTimestamp - The event timestamp for the body header.
 * @param source         - id of the source that created this item.
 * @param barrier        - barrier type or 0 if this was not a barrier.
  \param startTime - incremental scaler interval start time.
  \param endTime   - incremental scaler interval end time
  \param timestamp - Absolute system time at which the item was created.
  \param scalers   - Vector of scaler values.
  @param timeDivisor - The divisor for the start/end times that yields seconds.
                       defaults to 1.
  @param incremental - True (default) if the scaler item is incremental.
 */
CRingScalerItem::CRingScalerItem(
    uint64_t eventTimestamp, uint32_t source, uint32_t barrier, uint32_t startTime,
    uint32_t stopTime, time_t   timestamp, std::vector<uint32_t> scalers,
    uint32_t timeDivisor, bool incremental) :
    CRingItem(
        PERIODIC_SCALERS, eventTimestamp,  source, barrier,
        bodySize(scalers.size())
    )
{
    init(scalers.size());
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
      
    pScalers->s_intervalStartOffset = startTime;
    pScalers->s_intervalEndOffset   = stopTime;
    pScalers->s_timestamp           = timestamp;
    pScalers->s_scalerCount         = scalers.size();
    pScalers->s_isIncremental = incremental ? 1 : 0;
    pScalers->s_intervalDivisor = timeDivisor;

    for (int i = 0; i  < scalers.size(); i++) {
      pScalers->s_scalers[i] = scalers[i];
    }    
    updateSize();
}

/*!
  Casting construction. 
  \param rhs - const reference to a RingItem.  We are doing an upcast to 
  a scaler buffer.
*/ 
CRingScalerItem::CRingScalerItem(const CRingItem& rhs) throw(bad_cast) :
  CRingItem(rhs)
{
  if (type() != PERIODIC_SCALERS) {
    throw bad_cast();
  }
  pScalerItemBody pItem = reinterpret_cast<pScalerItemBody>(getBodyPointer());
  
  init(pItem->s_scalerCount);
  updateSize();
  
}

/*!
  Ordinary copy construction
  \param rhs - The item we are copying into *this
*/
CRingScalerItem::CRingScalerItem(const CRingScalerItem& rhs) :
  CRingItem(rhs)
{
    
}

/*!
  Destruction just delegates.
*/
CRingScalerItem::~CRingScalerItem()
{}

/*!
   Assignment.. just the base class assignment with init():

  \param rhs - The item being assigned to *this.
  \return CRingScalerItem&
  \retval *this
*/
CRingScalerItem&
CRingScalerItem::operator=(const CRingScalerItem& rhs) 
{
  if (this != &rhs) {
    CRingItem::operator=(rhs);
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());

    init(pScalers->s_scalerCount);
    
  }
  return *this;
}
/*!
    Equality comparison.. there's no point in looking a the pointers.
    so the base class comparison is just fine.
    \param rhs - Item being compared to *this.
    \return int
    \retval 0 - Not equal
    \retval 1 - Equal
*/
int
CRingScalerItem::operator==(const CRingScalerItem& rhs) const
{
  return CRingItem::operator==(rhs);
}
/*!
   Inequality is the logical inverse of equality.
   \rhs - Item being compared to *thisl
   \return int
   \retval 0 - not unequal
   \retval 1 - unequal
*/
int
CRingScalerItem::operator!=(const CRingScalerItem& rhs) const
{
  return !(*this == rhs);
}
    
/////////////////////////////////////////////////////////////////////////////////////
//
//  Accessors - selectors and mutators.
//

/*!
   Set the scaler interval start time.

   \param startTime - the new interval start time.
*/
void
CRingScalerItem::setStartTime(uint32_t startTime)
{
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
    
    pScalers->s_intervalStartOffset = startTime;
}
/*!
   \return uint32_t
   \retval the current interval start time.
*/
uint32_t
CRingScalerItem::getStartTime() const
{
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());

    return pScalers->s_intervalStartOffset;
}
/**
 * computeStartTime
 *
 * Determine the floating point start timein seconds using the btime and
 * divisor values.
 *
 * @return float
 */
float
CRingScalerItem::computeStartTime() const
{
     pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
     float start   = pScalers->s_intervalStartOffset;
     float divisor = pScalers->s_intervalDivisor;
     return start/divisor;
}

/*!
   Set the interval ent time
   \pram endTime - the new interval end time.
*/
void
CRingScalerItem::setEndTime(uint32_t endTime) 
{
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
    pScalers->s_intervalEndOffset = endTime;
}
/*!
   \return uint32_t
   \retval The current interval end time.
*/
uint32_t
CRingScalerItem::getEndTime() const
{
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
    return pScalers->s_intervalEndOffset;
}
/**
 * computeEndTime
 *
 * Compute the interval end time using both the etime and the divisor.
 *
 * @return float - time in seconds.
 */
float
CRingScalerItem::computeEndTime() const
{
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
    float end   = pScalers->s_intervalEndOffset;
    float divisor = pScalers->s_intervalDivisor;
    return end/divisor;   
}
/**
 * getTimestampDivisor
 *   Get the divisor that must be applied to get a real time offset.
 *
 * @return uint32_t
 */
uint32_t
CRingScalerItem::getTimeDivisor() const
{
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
    return pScalers->s_intervalDivisor;
}

/**
 * isIncremental
 *
 * @return bool true if s_isIncremental is true etc.
 */
bool
CRingScalerItem::isIncremental() const
{
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
    return (pScalers->s_isIncremental != 0);
}

/*!
   set the absolute timestamp of the event.

   \param stamp - the time stampe.
*/
void
CRingScalerItem::setTimestamp(time_t  stamp)
{
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
    pScalers->s_timestamp = stamp;
}
/*!
   \return time_t
   \retval the current timestamp.
*/
time_t
CRingScalerItem::getTimestamp() const
{
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
    return pScalers->s_timestamp;
}


/*!
  Set a scaler value.
  \param channel - Number of the channel to modify.
  \param value   - The new value for that channel.
  \throw CRangError if channel is too big.
*/
void
CRingScalerItem::setScaler(uint32_t channel, uint32_t value) throw(CRangeError)
{
    throwIfInvalidChannel(channel, "Attempting to set a scaler value");
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());

    pScalers->s_scalers[channel] = value;
}
/*!

  \param channel - Number of the channel we want.
  \return uint32_t
  \retval Value of the selected scaler channel
  \throw CRangeError - if the channel number is out of range.
*/
uint32_t
CRingScalerItem::getScaler(uint32_t channel) const throw(CRangeError) 
{
    throwIfInvalidChannel(channel, "Attempting to get a scaler value");
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
  
    return pScalers->s_scalers[channel];
}
/*!
    \return std::vector<uint32_t>
    \retval - the scalers.
*/
vector<uint32_t>
CRingScalerItem::getScalers() const
{
    vector<uint32_t> result;
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());

    for (int i =0; i < pScalers->s_scalerCount; i++) {
      result.push_back(pScalers->s_scalers[i]);
    }
    return result;
}

/*!
   \return uint32_t
   \retval number of scalers
*/
uint32_t
CRingScalerItem::getScalerCount() const
{
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());

    return pScalers->s_scalerCount;
}
  
/////////////////////////////////////////////////////////////////////////////////////
//
// Private utilities


/*
 * Do common initialization.. setting m_pScalers and the cursor.
 */
void
CRingScalerItem::init(size_t n)
{
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());
    size_t   size    = bodySize(n); 
    uint8_t* pCursor = reinterpret_cast<uint8_t*>(getBodyPointer());
    pCursor         += size;
    setBodyCursor(pCursor);
    updateSize();
    
}
/*-------------------------------------------------------
** Virtual method overrides
*-------------------------------------------------------*/

/**
 * typeName
 *
 *  Returns the string associated with this type of
 *  ring item ("Scaler").
 *
 * @return std::string - the item type name string.
 */
std::string
CRingScalerItem::typeName() const
{
  return std::string("Scaler");
}
/**
 * toString
 *
 *  Return a textual human readable version of the data
 *  in this item.
 *
 * @return std::string - the text string.
 */

std::string
CRingScalerItem::toString() const
{
  std::ostringstream out;

  float end   = computeEndTime();
  float start = computeStartTime();
  string   time  = timeString(getTimestamp());
  vector<uint32_t> scalers = getScalers();
  for (int i =0; i < scalers.size(); i++) {
    scalers[i] = scalers[i] & m_ScalerFormatMask; // Mask off unused bits.
  }

  float   duration = end - start;

  out << time << " : Scalers:\n";
  out << "Interval start time: " << start << " end: " << end << " seconds in to the run\n\n";
  out << bodyHeaderToString();
  out << (isIncremental() ? "Scalers are incremental" : "Scalers are not incremental") << std::endl;

  out << "Index         Counts                 Rate\n";
  for (int i=0; i < scalers.size(); i++) {
    char line[128];
    double rate = (static_cast<double>(scalers[i])/duration);

    sprintf(line, "%5d      %9d                 %.2f\n",
	    i, scalers[i], rate);
    out << line;
  }


  return out.str();
  
}

/*-------------------------------------------------------
** Private utilities:
*---------------------------------------------------------
/*
 *  Given the number of scalers determine the size of the body.
 */
size_t
CRingScalerItem::bodySize(size_t n)
{
  size_t  size    = sizeof(ScalerItemBody) + n*sizeof(uint32_t);
  return  size;
}
/*
** Throws a range error if the channel number is invalid.
*/
void
CRingScalerItem::throwIfInvalidChannel(uint32_t channel,
				       const char* message) const throw(CRangeError)
{
    pScalerItemBody pScalers = reinterpret_cast<pScalerItemBody>(getBodyPointer());

    if (channel >= pScalers->s_scalerCount) {
      throw CRangeError(0, pScalers->s_scalerCount, channel,
                        message);
    }
}
