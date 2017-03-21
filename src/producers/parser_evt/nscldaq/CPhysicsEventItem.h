#ifndef __CPHYSICSEVENTITEM_H
#define __CPHYSICSEVENTITEM_H
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
#include <CRingItem.h>		/* Base class */
#endif

#ifndef __CRT_UNISTD_H
#include <unistd.h>
#ifndef __CRTUNISTD_H
#define __CRTUNISTD_H
#endif
#endif

#ifndef __CRT_STDINT_H
#include <stdint.h>
#ifndef __CRT_STDINT_H
#define __CRT_STDINT_H
#endif
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
#define __CPPRTL_TYPEINFO
#endif
#endif

struct _RingItem;

/**
 *  This class is a wrapper for physics events.
 *  It's mainly provided so that textual dumps
 *  can be performed as typeName and toString
 *  are the only substantive methods...everything
 *  else just delegates to the base class.
 */

class CPhysicsEventItem : public CRingItem
{
public:
  CPhysicsEventItem(size_t maxBody=8192);
  CPhysicsEventItem(uint64_t timestamp, uint32_t source, uint32_t barrier,
                    size_t maxBody=8192);

  CPhysicsEventItem(const CRingItem& rhs) throw(std::bad_cast);
  CPhysicsEventItem(const CPhysicsEventItem& rhs);
  virtual ~CPhysicsEventItem();

  CPhysicsEventItem& operator=(const CPhysicsEventItem& rhs);
  int operator==(const CPhysicsEventItem& rhs) const;
  int operator!=(const CPhysicsEventItem& rhs) const;

  // Virtual methods that all ring items must provide:

  virtual std::string typeName() const;	// Textual type of item.
  virtual std::string toString() const; // Provide string dump of the item.

 
  
};


#endif
