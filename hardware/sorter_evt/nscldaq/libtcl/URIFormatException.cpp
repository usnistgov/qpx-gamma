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
#include <config.h>

#include <URIFormatException.h>
#include <stdio.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////////////////
//   Constructors:


/*!
   Construct the exception for a URI that does not have the right format.
   \param URI    - The bad URI.
   \param file   - File in which the exception was constructed.
   \param line   - Line number at which the exception was constructed.
*/
CURIFormatException::CURIFormatException(string URI, const char* file, int line) :
  CException("")
{
  char reason[1000];
  snprintf(reason, sizeof(reason), 
	   "URI Format Incorrect: Cannot parse %s as a URI : thrown at %s:%d", 
	   URI.c_str(), file, line);
  m_Reason = reason;
  setAction("Parsing URI");
  m_Reason += " while ";
  m_Reason += WasDoing();
}
/*!
  Construct the exception for a port that is not valid (probably not an integer).
  \param URI  - The full URI string.
  \param port - The string that is supposed to be a port number.
   \param file   - File in which the exception was constructed.
   \param line   - Line number at which the exception was constructed.
*/
CURIFormatException::CURIFormatException(string URI, string port, 
					 const char* file, int line) :
  CException("")
{
  char reason[1000];
  snprintf(reason, sizeof(reason),
	   "URI Format Incorrect: URI %s,  %s is not a valid port : thrown at %s:%d",
	   URI.c_str(), port.c_str(), file, line);
  m_Reason = reason;
  setAction("Parsing URI Port string");
  m_Reason += " while ";
  m_Reason += WasDoing();

}
/*!
  Construct the exception for a host that fails  validity check. The validity check
  consists of ensuring that either the host is a dotted IP address, or alternatively,
  it's a DNS host name that could be resolved.

  \param URI  - The full URI string.
  \param host - The hostname. string.
  \param file - The file in which the exception was thrown.
  \param line  - The line number.
*/
CURIFormatException::CURIFormatException(string URI, const char* host, 
					 const char* file, int line) :
  CException("")
{
  char reason[1000];
  snprintf(reason, sizeof(reason),
	   "URI Format Incorrect: %s has an invalid host %s : thrown at %s:%d",
	   URI.c_str(), host, file, line);
  m_Reason = reason;
  setAction("Validating host");
  m_Reason += " while ";
  m_Reason += WasDoing();
}

/*!
   Copy construction:
*/
CURIFormatException::CURIFormatException(const CURIFormatException& rhs) :
  CException(*this),
  m_Reason(rhs.m_Reason)
{
}

CURIFormatException::~CURIFormatException() {}

/////////////////////////////////////////////////////////////////////////////////
// Additional canonical operators an methods.
//


/*!  Assignment */

CURIFormatException&
CURIFormatException::operator=(const CURIFormatException& rhs) 
{
  if (this != &rhs) {
    CException::operator=(rhs);
    m_Reason = rhs.m_Reason;
    
  }
  return *this;
}

/*!  Equality comparison.. all the members must be the same. */

int
CURIFormatException::operator==(const CURIFormatException& rhs) const
{
  return ((CException::operator==(rhs))               &&
	  (m_Reason == rhs.m_Reason));

}

/*!  Inequality if ! equal.  */

int
CURIFormatException::operator!=(const CURIFormatException& rhs) const
{
  return !(*this == rhs);
}

////////////////////////////////////////////////////////////////////////////////////////
//   Implementation of the abstract Exception interface:


/*!   

  \return const char*
  \retval Return the full text of the error

*/
const char*
CURIFormatException::ReasonText() const
{
  return m_Reason.c_str();
}
/*!
  \return Int_t
  \retval -1
*/
int
CURIFormatException::ReasonCode() const
{
  return -1;
}
