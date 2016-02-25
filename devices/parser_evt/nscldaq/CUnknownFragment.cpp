/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2005.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt
*/

/**
 * @file CUnknownFragment.cpp
 * @brief Implements the CUnknownFragment calss for EVB_UNKNOWN_PAYLOAD ring items.
 * @author Ron Fox <fox@nscl.msu.edu>
 */

#include "CUnknownFragment.h"
#include "DataFormat.h"
#include <string.h>
#include <sstream>


/*-----------------------------------------------------------------------------
 * Canonical methods.
 *---------------------------------------------------------------------------*/

/**
 * constructor
 *
 * This is the primary constructor.
 *
 * @param timestamp - Ticks that identify when this fragment was triggered.
 * @param sourceId  - Id of the source that created this fragment.
 * @param barrier   - Barrier id of the fragment o 0 if this was not part of a
 *                    barrier.
 * @param size      - Number of _bytes_ in the payload.
 * @param pPayload  - Pointer to the payload.
 */
CUnknownFragment::CUnknownFragment(
    uint64_t timestamp, uint32_t sourceId, uint32_t barrier, uint32_t size,
    void* pPayload) :
       CRingFragmentItem(timestamp, sourceId, size, pPayload, barrier)
{
    // The only thing left to do is fill in the correct type
    
    pRingItem pItem  = reinterpret_cast<pRingItem>(getItemPointer());
    pItem->s_header.s_type = EVB_UNKNOWN_PAYLOAD;
        
}
/**
 * destructor
 */
CUnknownFragment::~CUnknownFragment()  {}

/**
 * copy constructor (specific)
 *
 * @param rhs - The object item we are copying in construction.
 */
CUnknownFragment::CUnknownFragment(const CUnknownFragment& rhs) :
    CRingFragmentItem(rhs)
{
    
}
/**
 * copy constuctor (generic)
 *
 * @param rhs - The object item we are copying in construction.
 * @throw std::bad_cast if the rhs object is ot a EVB_UNKNOWN_PAYLOAD type.
 */
CUnknownFragment::CUnknownFragment(const CRingItem& rhs) throw(std::bad_cast) :
    CRingFragmentItem(rhs)
{
    if (type() != EVB_UNKNOWN_PAYLOAD) throw std::bad_cast();        
}
/**
 * operator=
 *
 * @param rhs - The object that we are being assigned from.
 * @return CUnknownFragment& reference to *this.
 */
CUnknownFragment&
CUnknownFragment::operator=(const CUnknownFragment& rhs)
{
    CRingItem::operator=(rhs);
    return *this;
}
/**
 * operator==
 *    @param rhs - object of comparison.
 *    @return int - nonzero if equality.
 */
int
CUnknownFragment::operator==(const CUnknownFragment& rhs) const
{
    return CRingItem::operator==(rhs);
}
/**
 * operator!=
 * @param rhs - Object of comparison.
 * @return int - nonzero if items don't compare for equality.
 */
int
CUnknownFragment::operator!=(const CUnknownFragment& rhs) const
{
    return CRingItem::operator!=(rhs);
}
/*----------------------------------------------------------------------------
 * Virtual method overrides;
 *--------------------------------------------------------------------------*/

/**
 * typeName
 *
 * @return std::string textual version of item type.
 */
std::string
CUnknownFragment::typeName() const
{
    return "Fragment with unknown payload";
}
