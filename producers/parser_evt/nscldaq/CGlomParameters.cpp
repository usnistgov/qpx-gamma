/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2005.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt
*/

/**
 * @file CGlomParameters.cpp
 * @brief  Encapsulates a ring item that contains glom parameters.
 * @author  Ron Fox <fox@nscl.msu.edu>
 */
#include "CGlomParameters.h"
#include "DataFormat.h"
#include <sstream>


#include <sstream>


/**
 * @note the definition below requires that the order of array elements matches
 *       the timestamp policy values
 */

static const char* policyNames[4] = {
    "first", "last" , "average", "Error - undefined"
};


/*-------------------------------------------------------------------------
 * Canonical methods
 *-----------------------------------------------------------------------*/

/**
 * constructor
 *
 * This is the 'normal' constructor.  It builds a fully fledged glom parameters
 * data item.
 *
 * @param interval   - Number of ticks in the event building coincidence window.
 * @param isBuilding - If true the output of glom is built rather than just
 *                     ordered/formatted events.
 * @param policy     - The timestamp policy used by glom.
 */
CGlomParameters::CGlomParameters(
    uint64_t interval, bool isBuilding, CGlomParameters::TimestampPolicy policy
) :
    CRingItem(EVB_GLOM_INFO, sizeof(GlomParameters))
{
    // Fill in the body of the item:
    
    pGlomParameters pItem = reinterpret_cast<pGlomParameters>(getItemPointer());
    pItem->s_coincidenceTicks = interval;
    pItem->s_isBuilding       = (isBuilding ? 0xffff : 0);
    pItem->s_timestampPolicy  = policy;
    
    // Set the insertion cursor...and compute the final item size.
    
    setBodyCursor(&(pItem[1]));
    updateSize();
}
/**
 * destructor
 */
CGlomParameters::~CGlomParameters()
{
    
}
/**
 * copy constructor - specific
 *
 * @param rhs - The item used as a 'template' for our copy
 */
CGlomParameters::CGlomParameters(const CGlomParameters& rhs) :
    CRingItem(rhs)
{
        
}
/**
 * copy constructor - generic
 *
 * If the rhs is not an EVB_GLOM_INFO item, we'll throw a bad_cast.
 *
 * @param rhs - CRingItem reference from which we will construct.
 */
CGlomParameters::CGlomParameters(const CRingItem& rhs) throw(std::bad_cast) :
    CRingItem(rhs)
{
    if (type() != EVB_GLOM_INFO) throw std::bad_cast();        
}
/**
 * operator=
 *
 * @param rhs - The item being assighned to us.
 *
 * @return CGlomParameters& (*this)
 */
CGlomParameters&
CGlomParameters::operator=(const CGlomParameters& rhs)
{
    CRingItem::operator=(rhs);
    return *this;
}
/**
 * operator==
 *
 * @param rhs the item being compared to *this
 *
 * @return int - nonzero if equality.
 */
int
CGlomParameters::operator==(const CGlomParameters& rhs) const
{
    return CRingItem::operator==(rhs);
}
/**
 * operator!=
 *
 * @param rhs the item being compared to *this.
 *
 * @return int  non zero if not equal.
 */
int
CGlomParameters::operator!=(const CGlomParameters& rhs) const
{
    return CRingItem::operator!=(rhs);
}

/*----------------------------------------------------------------------------
 * Selectors
 *--------------------------------------------------------------------------*/

/**
 * coincidenceTicks
 *
 * @return uint64_t - the number of ticks glom used in its coincidence window
 *                    this is meaningful, however only if isBuilding() returns
 *                    true.
 */
uint64_t
CGlomParameters::coincidenceTicks() const
{
    CGlomParameters* This = const_cast<CGlomParameters*>(this);
    pGlomParameters pItem =
        reinterpret_cast<pGlomParameters>(This->getItemPointer());
    
    return pItem->s_coincidenceTicks;

    
}
/**
 * isBuilding
 *
 * @return bool - true if glom is glueing event fragments together.
 */
bool
CGlomParameters::isBuilding() const
{
    CGlomParameters* This = const_cast<CGlomParameters*>(this);
    pGlomParameters pItem =
        reinterpret_cast<pGlomParameters>(This->getItemPointer());
    
    return pItem->s_isBuilding;
}
/**
 * timestampPolicy
 *
 * @return CGlomParameters::TimestampPolicy - the timestamp policy from
 *         the ring item.
 */
CGlomParameters::TimestampPolicy
CGlomParameters::timestampPolicy() const
{
    CGlomParameters* This = const_cast<CGlomParameters*>(this);
    pGlomParameters pItem =
        reinterpret_cast<pGlomParameters>(This->getItemPointer());
        
    return static_cast<TimestampPolicy>(pItem->s_timestampPolicy);
}
/*---------------------------------------------------------------------------
 * Object methods
 *-------------------------------------------------------------------------*/

/**
 * typeName
 *
 * @return std::string - textual version of the ring type.
 */
std::string
CGlomParameters::typeName() const
{
    return std::string("Glom Parameters");
}
/**
 * toSTring
 *
 * @return std::string - a textual dump of the ring item contents.
 */
std::string
CGlomParameters::toString() const
{
    CGlomParameters* This = const_cast<CGlomParameters*>(this);
    pGlomParameters pItem =
        reinterpret_cast<pGlomParameters>(This->getItemPointer());
    std::stringstream    out;
    
    out << "Glom is " << (isBuilding() ? "" : " not ") << "building events\n";
    if (isBuilding()) {
        out << "Event building coincidence window is: "
            << coincidenceTicks() << " timestamp ticks\n";
    }
    unsigned tsPolicy = static_cast<unsigned>(timestampPolicy());
    if (tsPolicy >= sizeof(policyNames)/sizeof(char*)) {
        tsPolicy = sizeof(policyNames)/sizeof(char*) - 1;
    }
    out << "TimestampPolicy : policyNames[tsPolicy]\n";
    
    
    return out.str();
}
