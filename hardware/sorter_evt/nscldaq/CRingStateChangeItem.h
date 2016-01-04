#ifndef __CRINGSTATECHANGEITEM_H
#define __CRINGSTATECHANGEITEM_H
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


#ifndef __CRINGITEM_H
#include <CRingItem.h>
#endif


#ifndef __DATAFORMAT_H
#include <DataFormat.h>
#endif


#ifndef __STL_STRING
#include <string>
#ifndef __STL_STRING
#define __STL_STRING
#endif
#endif

#ifndef __CPPRTL_TYPEINFO
#include <typeinfo>
#ifndef __CPPRTL_TYPEINFO
#endif
#endif

#ifndef __RANGEERROR_H
#include <RangeError.h>
#endif

/*!
  This class represents a state change item.
  State change items are items in the buffer that indicate a change in the state of
  data taking.  The set of state changes recognized by this class are:

  - BEGIN_RUN  - the run has become active after being stopped.
  - END_RUN    - The run has become inactive after being ended.
  - PAUSE_RUN  - The run has become inactive after being temporarily paused.
  - RESUME_RUN - The run has become active after being resumed while paused.

  This object is suitable for use by both producers (who must create state change
  items), and consumers (who must inspect the contents of such items after getting
  them from a ring).


*/
class CRingStateChangeItem : public CRingItem
{

  // construction and other canonicals
public:
  CRingStateChangeItem(uint16_t reason = BEGIN_RUN);
  CRingStateChangeItem(uint16_t reason,
		       uint32_t runNumber,
		       uint32_t timeOffset,
		       time_t   timestamp,
		       std::string title) throw(CRangeError);
  CRingStateChangeItem(uint64_t eventTimestamp, uint32_t sourceId, uint32_t barrierType,
                       uint16_t reason,
		       uint32_t runNumber,
		       uint32_t timeOffset,
		       time_t   timestamp,
		       std::string title,
                       uint32_t offsetDivisor = 1);
  
  CRingStateChangeItem(const CRingItem& item) throw(std::bad_cast);
  CRingStateChangeItem(const CRingStateChangeItem& rhs);
  virtual ~CRingStateChangeItem();

  CRingStateChangeItem& operator=(const CRingStateChangeItem& rhs);
  int operator==(const CRingStateChangeItem& rhs) const;
  int operator!=(const CRingStateChangeItem& rhs) const;

  // Accessors for elements of the item (selectors and mutators both).

  void setRunNumber(uint32_t run);
  uint32_t getRunNumber() const;

  void setElapsedTime(uint32_t offset);
  uint32_t getElapsedTime() const;
  float    computeElapsedTime() const;

  void setTitle(std::string title) throw(CRangeError);
  std::string getTitle() const;

  void setTimestamp(time_t stamp);
  time_t getTimestamp() const;

  // Virtual method overrides.

  virtual std::string typeName() const;
  virtual std::string toString() const;

  // Utitlity functions..

private:
  void init();
  bool isStateChange();
  pStateChangeItemBody getStateChangeBody();
};
#endif
