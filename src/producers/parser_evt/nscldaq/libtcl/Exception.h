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


//  CException.h:
//
//    This file defines the CException class.
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

#pragma once
#include <string>

#define kACTIONSIZE 1024 // Size of action message.
                               
class CException      
{
	// Attributes:
private:
  char m_szAction[kACTIONSIZE] ;   // Saved action in progress when
					               // exception was thrown.
public:  
			//Default constructor

  virtual ~ CException ( ) { }       //Destructor

			//Constructors with arguments

  CException (const char* pszAction  );
  CException (const std::string& rsAction);
  
			//Copy constructor

  CException (const CException& aCException );

			//Operator= Assignment Operator

  CException& operator= (const CException& aCException);
  
			//Operator== Equality Operator
  int operator== (const CException& aCException) const;
  int operator!= (const CException& rException) const{
    return !(operator==(rException));
  }
                       //Get accessor function for attribute

  // Selectors:

public:
	  const char*  getAction() const
  {
	  return m_szAction;
  }
                       
  //Set accessor function for attribute (protected)

protected:
  void setAction (const char* pszAction);
  void setAction (const std::string& rsAction);

  // Selectors which depend on the actual exception type:

public:
  virtual const char* ReasonText () const  ;
  virtual int ReasonCode () const  ;
  const char* WasDoing () const  ;

  // Utility functions:
protected:

  virtual void DoAssign(const CException& rhs);
};
