/*=========================================================================*\
| Copyright (C) 2007 by the Board of Trustees of Michigan State University. |
| You may use this software under the terms of the GNU public license       |
| (GPL).  The terms of this license are described at:                       |
| http://www.gnu.org/licenses/gpl.txt                                       |
|                                                                           |
| Written by: E. Kasten                                                     |
\*=========================================================================*/

using namespace std;

#include "URL.h"

#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include <URIFormatException.h>

extern int h_errno;

using namespace std;

/*==============================================================*/
/** @fn URL::URL(string& rURLStr)
* @brief Constructor with a stringified URL.
*
* Constructor with a stringified URL.
*
* @param rURLStr The strigified URL.
* @return this
*/                                                             
URL::URL(string rURLStr) {
  parseString(rURLStr);
}

/*=============================================================*/
/**
*  @fn URL::URL(const URL&)
*  @brief Copy construction.
*
*  Costruct from an existing URL object
*
*  @param rhs  - the existing URL from which we will construct
*                an exact copy.
*/
URL::URL(const URL& rhs) :
  my_port(rhs.my_port),
  my_proto(rhs.my_proto),
  my_path(rhs.my_path),
  my_namestr(rhs.my_namestr)
{}




/*==============================================================*/
/** @fn URL& URL::operator=(const URL& rURL)
* @brief Assign a URL to this one.
*
* Assign an URL to this one.
*
* @param rURL The URL to assign.
* @return A reference to this.
*/                                                             
URL& URL::operator=(const URL& rURL) {
  my_port = rURL.my_port;
  my_path = rURL.my_path;
  my_proto = rURL.my_proto;
  my_namestr = rURL.my_namestr;
  return (*this);
}

/*==============================================================*/
/** @fn int  URL::operator==(const URL& rURL)
* @brief Check for equality with an URL.
*
* Check if this address is equivalent to the specified 
* URL.
*
* @param rURL The URL to compare.
* @return If the specified address is equal to this one.
*/                                                             
int
URL::operator==(const URL& rURL) const {
  return ((my_port    == rURL.my_port)&&
          (my_proto   == rURL.my_proto) &&
          (my_path    == rURL.my_path)&&
          (my_namestr == rURL.my_namestr));
}

/*==============================================================*/
/** @fn int URL::operator!=(const URL& rhs)
 *
 *  @brief Check for inequality with a URL.
 *
 *  This returns true if the equality check fails.
 *
 * @param rhs  - The URI we are comparing with.
 * @return int
 * @retval 1  - rhs is not the same as this.
 * @retval 0  - rhs is the same as this.
 */

int
URL::operator!=(const URL& rhs) const
{
  return !(this->operator==(rhs));
}


/*==============================================================*/
/** @fn const string& URL::getHostName() const
* @brief Get the hostname of this address.
*
* Get the hostname of this address or return a String of
* zero size.
*
* @param None
* @return The hostname of this address.
*/                                                             
string URL::getHostName() const {
  return my_namestr;
}

/*==============================================================*/
/** @fn int URL::getPort()
* @brief Get the port of this address.
*
* Get the port number of this address.
*
* @param None
* @return The port number of this address.
*/                                                             
int URL::getPort() const {
  return my_port;
}

/*==============================================================*/
/** @fn const string& URL::getProto() const
* @brief Get the protocol of this URL.
*
* Get the protocol of this URL.
*
* @param None
* @return The protocol of this URL.
*/                                                             
string URL::getProto() const {
  return my_proto;
}

/*==============================================================*/
/** @fn const string URL::getPath() const
* @brief Get the path of this URL.
*
* Get the path of this URL.
*
* @param None
* @return The path of this URL.
*/                                                             
string URL::getPath() const {
  return my_path;
}

/*==============================================================*/
/** operator std::string()
* @brief Stringify this URL.
*
* Stringify this URL in proto://host:port/path format. 
*
* @param None
* @return The stringified URL.
*/                                                             
URL::operator string() const
{

  char   urlString[PATH_MAX];
  
  snprintf(urlString, sizeof(urlString),
	   "%s://%s:%d/%s",
	   my_proto.c_str(), my_namestr.c_str(), my_port, my_path.c_str());

  return string(urlString);
}

/*==============================================================*/
/** @fn URL URL::parseString(string& rStr)
* @brief Parse a String into a URL.
*
* Parse a String into a URL.
*
* @param rStr The String to parse.
* @return A URL parsed from the String.
* @throw StringFormatException If the String is not a URL.
*/                                                             
void URL::parseString(string rStr) {
  if (rStr.size() <= 0) 
    throw CURIFormatException(rStr, __FILE__, __LINE__);

  // The string is of the form:
  //   protocol://host:port/path
  // where the path may have /'s in it.
  // We are going to require that all elements of the URI are present.
  //
  // We are not going to worry about the special case of
  // file: protocols where there is no host.
  //

  // note that rStr.c_str() returns a const char* so it's against the
  // rules of the game to modify the string pointed to.. even via
  // aliased pointers..



  // The protocol is the token prior to the first : in the URI:

  string protocol;
  const char* protocolStart;
  const char* protocolEnd;
  size_t      protocolLength;

  protocolStart = rStr.c_str();
  protocolEnd   = strchr(protocolStart, ':');

  // There must be a colon, and there must be text after the :

  if (!protocolEnd  || (strlen(protocolEnd) == 0)) {
    throw CURIFormatException(rStr, __FILE__, __LINE__);
  }
  protocolLength = protocolEnd - protocolStart;

  protocol.assign(rStr, 0, protocolLength);

  // A bit of relaxation.  The 'host; starts after the
  // : and two / characters.
  //

  string host;
  const char*  slash = "/";		// For strspn.
  const char*  hostStart;
  const char*  hostEnd;
  off_t  nhostStart;
  size_t hostLength;
  size_t numSlashes;

  hostStart = protocolEnd + 1;	// Skip the colon.
  numSlashes= strspn(hostStart, slash);	// count the slashes...
  if (numSlashes < 2) {
    throw CURIFormatException(rStr, __FILE__, __LINE__);	// need at least 2 slashes.
  }
  hostStart += 2;		       // The real start of the host.
  hostEnd    = strchr(hostStart, ':');                  // We require a port, and thus a ':'.
  if (!hostEnd) {
    hostEnd = strchr(hostStart, '/'); // Might not be a port... but must be a trailing slash.
  }
  if (!hostEnd) {
    throw CURIFormatException(rStr, __FILE__, __LINE__);
  }
  nhostStart = hostStart - rStr.c_str();
  hostLength = hostEnd - hostStart;
  host.assign(rStr, nhostStart, hostLength);

  // Next might be the  the port.  It must follow exactly on the heels of the 
  // host, and terminate either in a / or in the 
  // end of the string.

  string portString;
  const char*  portStart;
  const char*  portEnd;
  const char*  portIntegerEnd;
  off_t  nPortStart;
  size_t nPortSize;
  int    port(-1);
  bool   havePath(true);

  if (*hostEnd == ':') {
    portStart = hostEnd+1;	// Past colon...
    portEnd   = strchr(portStart, '/');
    if (!portEnd) {		// no path...
      havePath = false;
      portEnd = rStr.c_str() + rStr.length(); // Ends in trailing null..
    }
    nPortStart = portStart - rStr.c_str();
    nPortSize  = portEnd   - portStart;
    portString.assign(rStr, nPortStart, nPortSize);
    
    char* pEnd;
    port = strtol(portString.c_str(), &pEnd, 0);
    portEnd++;

  // The entire string must have been gobbled up...when portEnd -> 0.

    if (*pEnd) {
      throw CURIFormatException(rStr, portString, __FILE__, __LINE__);
    }
  }
  else {
    portEnd = hostEnd;
    portEnd++;
    havePath = (*portEnd != '\0');
  }


  
  // The path is either empty, in which case it gets filled with
  // a "/" or it is the remainder of the string.  We don't do any
  // validity checking on the path.

  string path;
  const char*  pathStart;

  if(havePath) {
    path = portEnd;		// includes all leading '/'-es there may be.
  }
  else {
    path = "/";
  }

  // If the protocol is file.. then the 
  // path is the host + path and host does not exist nor does it
  // require validation:
  // TODO: This is patently false...but I'm not sure what depends on it :-(

  if (protocol == string("file")) {
    my_port = 0;
    my_proto= protocol;
    my_path = host;
    my_path += "/";
    my_path += path;
    my_namestr = "";
    return;
  }

  // One last error check.  The host must translate.
  // to dotted ips or be dotted ips.d
  //

  int n1,n2,n3,n4;
  if (sscanf(host.c_str(), "%d.%d.%d.%d", &n1, &n2, &n3, &n4) != 4) {

    // Host is a DNS or bad:

    struct hostent entry;
    struct hostent *pEntry;
    char            buffer[1024];
    int             error;
    int status  = gethostbyname_r(host.c_str(),
				  &entry, buffer, sizeof(buffer),
				  &pEntry, &error);
    if (status) {
      throw
	CURIFormatException(rStr, host.c_str(),
			    __FILE__, __LINE__);
    }

  }
  // Parse is successful;
  // protocol, host, path, port contain the chunks we need:

  
  my_port    = port;
  my_proto   = protocol;
  my_path    = path;
  my_namestr = host;

}
