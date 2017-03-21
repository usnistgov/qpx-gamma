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

//  CRangeError.h:
//
//    This file defines the CRangeError class.
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

/********************** WARNING - this file is obsolete, include 
                        CrangeError.h from now on
*/


#ifndef __CRANGEERROR_H  //Required for current class
#define __CRANGEERROR_H
                               //Required for base classes
#ifndef __CEXCEPTION_H
#include "Exception.h"
#endif                             
#ifndef __STL_STRING
#include <string>
#define __STL_STRING
#endif  
                               
class CRangeError  : public CException        
{
  int m_nLow;			// Lowest allowed value for range (inclusive).
  int m_nHigh;		// Highest allowed value for range.
  int m_nRequested;		// Actual requested value which is outside
				// of the range.
  std::string m_ReasonText;            // Reason text will be built up  here.
public:
  //   The type below is intended to allow the client to categorize the
  //   exception:

  enum {
    knTooLow,			// CRangeError::knTooLow  - below m_nLow
    knTooHigh			// CRangeError::knTooHigh - above m_nHigh
  };
			//Constructors with arguments

  CRangeError (  int nLow,  int nHigh,  int nRequested,
		 const char* pDoing) :       
    CException(pDoing),
    m_nLow (nLow),  
    m_nHigh (nHigh),  
    m_nRequested (nRequested)
  { UpdateReason(); }
  CRangeError(int nLow, int nHigh, int nRequested,
	  const std::string& rDoing) :
    CException(rDoing),
    m_nLow(nLow),
    m_nHigh(nHigh),
    m_nRequested(nRequested)
  { UpdateReason(); }
  virtual ~ CRangeError ( ) { }       //Destructor

			//Copy constructor

  CRangeError (const CRangeError& aCRangeError )   : 
    CException (aCRangeError) 
  {
    m_nLow = aCRangeError.m_nLow;
    m_nHigh = aCRangeError.m_nHigh;
    m_nRequested = aCRangeError.m_nRequested;
    UpdateReason();
  }                                     

			//Operator= Assignment Operator

  CRangeError operator= (const CRangeError& aCRangeError)
  { 
    if (this != &aCRangeError) {
      CException::operator= (aCRangeError);
      m_nLow = aCRangeError.m_nLow;
      m_nHigh = aCRangeError.m_nHigh;
      m_nRequested = aCRangeError.m_nRequested;
      UpdateReason();
    }

    return *this;
  }                                     

			//Operator== Equality Operator

  int operator== (const CRangeError& aCRangeError) const
  { 
    return (
	    (CException::operator== (aCRangeError)) &&
	    (m_nLow == aCRangeError.m_nLow) &&
	    (m_nHigh == aCRangeError.m_nHigh) &&
	    (m_nRequested == aCRangeError.m_nRequested) 
	    );
  }
  int operator!=  (const CRangeError& aCRangeError) const
  { 
    return !(this->operator==(aCRangeError));
  }

  // Selectors - Don't use these unless you're a derived class
  //             or you need some special exception type specific
  //             data.  Generic handling should be based on the interface
  //             for CException.
public:                             

  int getLow() const
  {
    return m_nLow;
  }
  int getHigh() const
  {
    return m_nHigh;
  }
  int getRequested() const
  {
    return m_nRequested;
  }
  // Mutators - These can only be used by derived classes:

protected:
  void setLow (int am_nLow)
  { 
    m_nLow = am_nLow;
    UpdateReason();
  }
  void setHigh (int am_nHigh)
  { 
    m_nHigh = am_nHigh;
    UpdateReason();
  }
  void setRequested (int am_nRequested)
  { 
    m_nRequested = am_nRequested;
    UpdateReason();
  }
  //
  //  Interfaces implemented from the CException class.
  //
public:                    
  virtual   const char* ReasonText () const  ;
  virtual   int ReasonCode () const  ;
 
  // Protected utilities:
  //
protected:
  void UpdateReason();
};

#endif
