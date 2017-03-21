/**

#    This software is Copyright by the Board of Trustees of Michigan
#    State University (c) Copyright 2013.
#
#    You may use this software under the terms of the GNU public license
#    (GPL).  The terms of this license are described at:
#
#     http://www.gnu.org/licenses/gpl.txt
#
#    Author:
#            Ron Fox
#            NSCL
#            Michigan State University
#            East Lansing, MI 48824-1321

##
# @file   CAbnormalEndItem.cpp
# @brief  Implements the abnormal end run ring item class.
# @author <fox@nscl.msu.edu>
*/

#include "CAbnormalEndItem.h"
#include "DataFormat.h"
#include <typeinfo>



/**
 * default constructor
 *    Create the ring item.
 */
CAbnormalEndItem::CAbnormalEndItem() :
    CRingItem(ABNORMAL_ENDRUN) {}
    
/**
 * destructor is null.
 */
CAbnormalEndItem::~CAbnormalEndItem() {}


/*
 * copy costructor
 */
CAbnormalEndItem::CAbnormalEndItem(const CAbnormalEndItem& rhs) :
    CRingItem(rhs)
{}
CAbnormalEndItem::CAbnormalEndItem(const CRingItem& rhs) :
    CRingItem(rhs)
{
    if(type() != ABNORMAL_ENDRUN) {
        throw std::bad_cast();
    }
}

/**
 * assignment
 *  @param rhs the item assigned to us.
 *  @return *this
 */
CAbnormalEndItem&
CAbnormalEndItem::operator=(const CAbnormalEndItem& rhs)
{
    CRingItem::operator=(rhs);
    return *this;
}
/**
 * operator==
 *   all abnormal end items are the same.
 * @return true
 */
int
CAbnormalEndItem::operator==(const CAbnormalEndItem& rhs) const
{
    return 1;
}
/**
 * operator==
 *   We are equal to any ring item that has our type.
 * @param rhs - what this is being compared with.
 * @return true if rhs is an abnormal end item.
 */
int
CAbnormalEndItem::operator==(const CRingItem& rhs) const
{
    const pRingItem pItem = const_cast<const pRingItem>(rhs.getItemPointer());
    return (pItem->s_header.s_type == ABNORMAL_ENDRUN);
}


/**
 * operator!=
 *  @return !=-
 */
int
CAbnormalEndItem::operator!=(const CAbnormalEndItem& rhs) const
{
    return !operator==(rhs);
}
int
CAbnormalEndItem::operator!=(const CRingItem& rhs) const
{
    return !operator==(rhs);
}




/* Formatting interface */

/**
 * typeName
 *    Returns a textual name of the item typ
 * @return std::string
 */
std::string
CAbnormalEndItem::typeName() const
{
    return "Abnormal End";
}
/**
 * toString
 *
 *   Return a nicely formatted rendition of the ring item.
 * @return std::string
 */
std::string
CAbnormalEndItem::toString() const
{
    std::string result = typeName();
    result += "\n";
    return result;
}