#ifndef _CUNKNOWNFRAGMENT_H
#define _CUNKNOWNFRAGMENT_H

/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2005.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt
*/

/**
 * @file CUnknownFragment.h
 * @brief Defines the CUnknownFragment calss for EVB_UNKNOWN_PAYLOAD ring items.
 * @author Ron Fox <fox@nscl.msu.edu>
 */

#ifndef __CRINGFRAGMENT_H
#include "CRingFragmentItem.h"
#endif

#ifndef __CPPRTL_TYPEINFO
#include <typeinfo>
#ifndef __CPPRTL_TYPEINFO
#define __CPPRTL_TYPEINFO
#endif
#endif

/**
 * @class CUnknownFragment
 *
 * This class encapsulates ring items of type EVB_UNKNOWN_PAYLOAD. These are
 * event builder fragments where the payloads are pretty clearly not ring items.
 */
class CUnknownFragment : public CRingFragmentItem
{
    // Canonical methods:
    
public:
    CUnknownFragment(uint64_t timestamp, uint32_t sourceid, uint32_t barrier,
                     uint32_t size, void* pPayload);
    virtual ~CUnknownFragment();
    CUnknownFragment(const CUnknownFragment& rhs);
    CUnknownFragment(const CRingItem& rhs) throw(std::bad_cast);
    
    CUnknownFragment& operator=(const CUnknownFragment& rhs);
    int operator==(const CUnknownFragment& rhs) const;
    int operator!=(const CUnknownFragment& rhs) const;
    
    // Selectors:
public:

    std::string typeName() const;
    
};


#endif