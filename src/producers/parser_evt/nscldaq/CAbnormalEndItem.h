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
# @file   CAbnormalEndItem.h
# @brief  Define the wrapper class for the abnormal end run ring item.
# @author <fox@nscl.msu.edu>
*/
#pragma once

#include "CRingItem.h"

class CAbnormalEndItem : public CRingItem
{
public:
    CAbnormalEndItem();
    virtual ~CAbnormalEndItem();
    
    CAbnormalEndItem(const CAbnormalEndItem& rhs);
    CAbnormalEndItem(const CRingItem& rhs);
    
    CAbnormalEndItem& operator=(const CAbnormalEndItem& rhs);
    int operator==(const CAbnormalEndItem& rhs) const;
    int operator==(const CRingItem& rhs) const;
    
    int operator!=(const CAbnormalEndItem& rhs) const;
    int operator!=(const CRingItem& rhs) const;
    
    // Formatting:
    
public:
    virtual std::string typeName() const;
    virtual std::string toString() const;
};

