#ifndef __CRINGTEXTITEM_H
#define __CRINGTEXTITEM_H

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
#include "CRingItem.h"
#endif

#ifndef __DATAFORMAT_H
#include "DataFormat.h"
#endif

#ifndef __RANGEERROR_H
#include <RangeError.h>
#endif

#ifndef __CRT_STDINT_H
#include <stdint.h>
#ifndef __CRT_STDINT_H
#define __CRT_STDINT_H
#endif
#endif

#ifndef __CRT_TIME_H
#include <time.h>
#ifndef __CRT_TIME_H
#define __CRT_TIME_H
#endif
#endif

#ifndef __STL_STRING
#include <string>
#ifndef __STL_STRING
#define __STL_STRING
#endif
#endif

#ifndef __STL_VECTOR
#include <vector>
#ifndef __STL_VECTOR
#define __STL_VECTOR
#endif
#endif


#ifndef __CPPRTL_TYPEINFO
#include <typeinfo>
#ifndef __CPPRTL_TYPEINFO
#define __CPPRTL_TYPEINFO
#endif
#endif


/*!
  The text ring item provides a mechanism to put an item in/take an item out of 
  a ring buffer that consists of null terminated text strings.  
*/
class CRingTextItem : public CRingItem
{
  // Private data:


public:
  // Constructors and other canonicals:

  CRingTextItem(uint16_t type,
		std::vector<std::string> theStrings);
  CRingTextItem(uint16_t type,
		std::vector<std::string> theStrings,
		uint32_t                 offsetTime,
		time_t                   timestamp) ;
  CRingTextItem(
    uint16_t type, uint64_t eventTimestamp, uint32_t source, uint32_t barrier,
    std::vector<std::string> theStrings, uint32_t offsetTime, time_t timestamp,
    int offsetDivisor = 1
  );
  CRingTextItem(const CRingItem& rhs) throw(std::bad_cast);
  CRingTextItem(const CRingTextItem& rhs);

  virtual ~CRingTextItem();

  CRingTextItem& operator=(const CRingTextItem& rhs);
  int operator==(const CRingTextItem& rhs) const;
  int operator!=(const CRingTextItem& rhs) const;

  // Public interface:
public:
  std::vector<std::string>  getStrings() const;

  void     setTimeOffset(uint32_t offset);
  uint32_t getTimeOffset() const;
  float    computeElapsedTime() const;
  uint32_t getTimeDivisor() const;

  void     setTimestamp(time_t stamp);
  time_t   getTimestamp() const;

  // Virtual methods all ring overrides.

  virtual std::string typeName() const;
  virtual std::string toString() const;

  //private utilities:
private:
  size_t bodySize(std::vector<std::string> strings) const;
  bool   validType() const;
  void   copyStrings(std::vector<std::string> strings);
  void   init();
};


#endif
