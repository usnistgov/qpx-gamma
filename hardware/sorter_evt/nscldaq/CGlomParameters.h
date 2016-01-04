#ifndef __CGLOMPARAMETERS_H
#define __CGLOMPARAMETERS_H
/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2005.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt
*/

/**
 * @file CGlomParameters.h
 * @brief  Encapsulates a ring item that contains glom parametrs.
 * @author  Ron Fox <fox@nscl.msu.edu>
 */

#ifndef __CRINGITEM_H
#include "CRingItem.h"
#endif

#ifndef __CPPRTL_TYPEINFO
#include <typeinfo>
#ifndef __CPPRTL_TYPEINFO
#define __CPPRTL_TYPEINFO
#endif
#endif

/**
 * @class CGlomParameters
 *
 * Encapsulates a ring item of type EVB_GLOM_INFO.  The structure of this ring
 * item is given by the GlomParameters struct.  No body header is required for
 * this type..though the mbz field is present in case later we decide that
 * was a mistake (e.g. cascaded event building we may have gloms with different
 * parameters at different levels and knowing that by assigning each glom an
 * event source id may be needed).
 */
class CGlomParameters : public CRingItem
{
public:
    // Note the enum values below _must_ match those in DataFormat.h:
    
    typedef enum _TimestampPolicy {
        first = 0, last = 1, average = 2
    } TimestampPolicy;
    
    // Canonicals:
    
public:
    CGlomParameters(uint64_t interval, bool isBuilding, TimestampPolicy policy);
    virtual ~CGlomParameters();
    CGlomParameters(const CGlomParameters& rhs);
    CGlomParameters(const CRingItem& rhs) throw(std::bad_cast);
    
    CGlomParameters& operator=(const CGlomParameters& rhs);
    int operator==(const CGlomParameters& rhs) const;
    int operator!=(const CGlomParameters& rhs) const;
    
    // Selectors:
public:
   uint64_t coincidenceTicks() const;
   bool     isBuilding() const;
   TimestampPolicy timestampPolicy() const;
   
   // Object methods:
public:
    virtual std::string typeName() const;
    virtual std::string toString() const;

};


#endif
