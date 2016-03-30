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



//  CErrnoException.h:
//
//    This file defines the CErrnoException class.
//
// Author:
//    Ron Fox
//    NSCL
//    Michigan State University
//    East Lansing, MI 48824-1321
//    mailto:fox@nscl.msu.edu
//
//  Copyright 1999 NSCL, All Rights Reserved.
//
/////////////////////////////////////////////////////////////

#ifndef __ERRNOEXCEPTION_H  //Required for current class
#define __ERRNOEXCEPTION_H
                               //Required for base classes
#ifndef __EXCEPTION_H
#include "Exception.h"
#endif                               
  
#ifndef __CRTL_ERRNO_H
#include <errno.h>
#define __CRTL_ERRNO_H
#endif
                           
#ifndef __STL_STRING
#include <string>
#define __STL_STRING
#endif

  
class CErrnoException  : public CException        
{
  int m_nErrno;  // // Snapshot of errno at construction time.
  
public:

			//Constructor with arguments

  CErrnoException(const char* pszAction) :
    m_nErrno(errno),
    CException(pszAction) {}
  CErrnoException(const std::string& rsAction) :
    m_nErrno(errno),
    CException(rsAction) {}
  ~CErrnoException ( ) { }       //Destructor

  // Copy Constructor:

  CErrnoException (const CErrnoException& aCErrnoException )   : 
    CException (aCErrnoException) 
  {   
    m_nErrno = aCErrnoException.m_nErrno;             
  }                                     
			//Operator= Assignment Operator
     
  CErrnoException& operator= (const CErrnoException& aCErrnoException)
  { 
    if (this == &aCErrnoException) return *this;          
    CException::operator= (aCErrnoException);
    m_nErrno = aCErrnoException.m_nErrno;
    return *this;                                                                                                 
  }                                     

			//Operator== Equality Operator

  int operator== (const CErrnoException& aCErrnoException) const
  { 
    return ((CException::operator== (aCErrnoException)) &&
	    (m_nErrno == aCErrnoException.m_nErrno) 
	    );
  }
  int operator!=(const CErrnoException& aCErrnoException) const {
    return !(this->operator==(aCErrnoException));
  }


  // Selectors:  Note typically these are not needed.
               
public:
  int getErrno() const
  {
    return m_nErrno;
  }
  // Mutating selectors:  For derived classes only.
protected:                   
  void setErrno(int am_nErrno)
  { 
    m_nErrno = am_nErrno;
  }                   

  // Functions of the class:
public:

  virtual   const char* ReasonText () const  ;
  virtual   int       ReasonCode () const  ;
 
};

#endif













