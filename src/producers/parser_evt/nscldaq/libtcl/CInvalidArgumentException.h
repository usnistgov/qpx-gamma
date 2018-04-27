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

#pragma once

#include <Exception.h>


/*!
 *  This class is ressponsible for reporting exceptions that 
 *  occur because some function (member or otherwise) has been passed an invalid
 *  argument of some sort.
 *
 *  To use this exception type, the thrower must be able to create  a textual
 *  equivalent of the bad argument, a string that describes why the argument
 *  is bad as well as supplying the usual context string.
 */
class CInvalidArgumentException : public CException
{
    // Member data (private)
private:
    std::string         m_valuePassed;
    std::string         m_whyBad;
    mutable std::string m_reason;
    
    // Constructors and the various canonicals.
    
public:
    CInvalidArgumentException(std::string argument, std::string whybad,
                              std::string wasDoing);
    CInvalidArgumentException(const CInvalidArgumentException& rhs);
    virtual ~CInvalidArgumentException();
    
    CInvalidArgumentException& operator=(const CInvalidArgumentException& rhs);
    int operator==(const CInvalidArgumentException& rhs) const;
    int operator!=(const CInvalidArgumentException& rhs) const;
    
    // Operations that make this an exception class:
    
    virtual const char* ReasonText() const;
};
