#ifndef DAQHWYNET_URL_H
#define DAQHWYNET_URL_H

/*=========================================================================*\
| Copyright (C) 2007 by the Board of Trustees of Michigan State University. |
| You may use this software under the terms of the GNU public license       |
| (GPL).  The terms of this license are described at:                       |
| http://www.gnu.org/licenses/gpl.txt                                       |
|                                                                           |
| Written by: E. Kasten,                                                    |
|             R. Fox                                                        |
\*=========================================================================*/

#include <sys/types.h>
#include <string.h>
#include <errno.h>


#ifndef __STL_STRING
#include <string>
#ifndef __STL_STRING
#define __STL_STRING
#endif
#endif


/**
* @class URL
* @brief URL Uniform Resource Parsing.
*
*  A URI is of the form:
*    protocol://host/path:port
*
*  Where path can be a compound path consisting of elements separated by /
*  Example:
*
* /verbatim

     tcp://thechad.nscl.msu.edu/myringbuffer:1280

* /endverbatim
*
*  In this implementation, ports must be numeric and cannot be service names.
*
* A URL class.
*
* @author  Eric Kasten, R. Fox
* @version 2.0.0
*/
class URL  {
  
  // Object private data:

private:
  int          my_port;		// Port after parse.
  std::string  my_proto;	// Protocol 
  std::string  my_path;		// Path.
  std::string  my_namestr;	// Hostname.


public: 
  
  URL(std::string);		// Constructor with stringified URL
  URL(const URL&);		// Copy constructor.
  
  URL& operator=(const URL&);	// Assign
  int operator==(const URL&) const; // Comparison for equality
  int operator!=(const URL&) const; // and inequality.

  // Get elements of the URI:

  std::string getProto()    const; // Get the protocol
  std::string getHostName() const; // Get the host name
  int getPort()             const; // Get the port
  std::string getPath()     const; // Get the path
  operator std::string()    const; // Stringify the url.

private:

  void parseString(std::string); // Parse a String into a URL


};


#endif
