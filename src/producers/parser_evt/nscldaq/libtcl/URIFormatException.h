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
#include <string>


/*!
    CURIFormatException defines an exception that's thrown when parsing an
    Universal Resource Identifier (URI).  The string indicates a human readable
    reason for the failure.  The error code is always -1.  The reason text
    encodes why the exception was thrown as well as where, e.g.:

    URI Format Incorrect: reason  : thrown at file:line

    where file is almost always something ending in URI.cpp, and line is the line in that
    file where the actual exception was constructed.

*/
class  CURIFormatException : public CException
{
private:
  std::string  m_Reason;


public:
  // Constructors:

  CURIFormatException(std::string uri, const char* file, int line); //!< uri is just not the right format.
  CURIFormatException(std::string uri, std::string port,
		      const char* file, int line);                  //!< Port is not a port.
  CURIFormatException(std::string uri, const char* host,
		      const char* file, int line);                   //!< Host fails validity check.
  CURIFormatException(const CURIFormatException& rhs);              // copy construction

  virtual ~CURIFormatException();

  // Remaining canonicals:

  CURIFormatException& operator=(const CURIFormatException& rhs);
  int operator==(const CURIFormatException& rhs) const;
  int operator!=(const CURIFormatException& rhs) const;

  // Exception interface:

  virtual const char* ReasonText() const;
  virtual int       ReasonCode() const;

};
