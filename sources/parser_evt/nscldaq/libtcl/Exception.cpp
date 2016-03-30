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


//  CException.cpp
// Base class for exceptions thrown 
// by the histogrammign subsystem.
// (could be used for any other subsystem
// for that matter).
//
//
//   Author:
//      Ron Fox
//      NSCL
//      Michigan State University
//      East Lansing, MI 48824-1321
//      mailto:fox@nscl.msu.edu
//
//////////////////////////.cpp file//////////////////////////////////////////

//
// Header Files:
//

#include <config.h>
#include "Exception.h"                               
#include <string.h>

static const char*  Copyright = 
"CException.cpp: Copyright 1999 NSCL, All rights reserved\n";

// Functions for class CException

//////////////////////////////////////////////////////////////////////////
//
// Function:
//   CException(const char* pszAction)
//   CException(const std::string& rsAction)
// Operation Type:
//   Parmeterized constructors.
// 
CException::CException(const char* pszAction)
{
  setAction(pszAction);
}
CException::CException(const std::string& rsAction)
{
  setAction(rsAction);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:
//   CException(const CException& rException)
// Operation Type:
//   Copy Constructor.
//
CException::CException(const CException& rException)
{
  DoAssign(rException);
}
//////////////////////////////////////////////////////////////////////////
//
// Function:
//   CException& operator=(const CException& rException)
// Operation Type:
//   Assignment Operator.
//
CException&
CException::operator=(const CException& rException)
{
  if(this != &rException) {
    DoAssign(rException);
  }
  return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:
//    int operator==(const CException& rException)
// Operation Type:
//    equality comparison.
//
int
CException::operator==(const CException& rException) const
{
  return (strcmp(m_szAction, rException.m_szAction) == 0);
}


//////////////////////////////////////////////////////////////////////////
//
//  Function:   
//    const Textsz_t ReasonText (  )
//  Operation Type:
//     Selector.
//
const char*
CException::ReasonText() const 
{
// Returns a const pointer to text which
// describes the reason the exception was
// thrown.  This is exception type
// specific.   The default action returns
// a pointer to the constant string:
//      "Unspecified Exception"
//

  return "Unspecified Exception";

}
//////////////////////////////////////////////////////////////////////////
//
//  Function:   
//    int ReasonCode (  )
//  Operation Type:
//     Selector
//
int 
CException::ReasonCode() const 
{
// Returns a code which describes the reason for the
// exception .  This is exception type specific and may
// be used to do detailed exception analysis and recovery.
// For example in the CErrnoException class, the errno
// at the time of instantiation of the object is returned.
// The default returns -1
//

  return -1;

}
//////////////////////////////////////////////////////////////////////////
//
//  Function:   
//    const Textsz_t WasDoing (  )
//  Operation Type:
//     Selector.
//
const char* 
CException::WasDoing() const 
{
// Returns a const pointer to  m_szAction.
// This is intended to describe the action 
// which was occuring when the exception
// was thrown.
// 

  return getAction();

}
//////////////////////////////////////////////////////////////////////////
//
//  Function:
//    void setAction(const char* pszAction)
//    void setAction(const std::string& rsAction)
//  Operation type:
//    mutator (write selector).
//
void
CException::setAction(const char* pszAction)
{
  memset(m_szAction, 0, sizeof(m_szAction));
  strncpy(m_szAction, pszAction, sizeof(m_szAction)-1);
}
void CException::setAction(const std::string& rsAction)
{
  setAction(rsAction.c_str());
}

/////////////////////////////////////////////////////////////////////////
//
// Function:
//    void DoAssign(const CException& rException)
// Operation Type:
//    Protected utility.
//
void
CException::DoAssign(const CException& rException)
{
  // This is virtual to allow derived classes to override
  // and chain calls to.
  //
  setAction(rException.m_szAction);
}
