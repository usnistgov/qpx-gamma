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
#include "CFileDataSource.h"


#include <URL.h>
#include <CRingItem.h>
#include <DataFormat.h>
#include <ErrnoException.h>
#include <CInvalidArgumentException.h>
#include <io.h>

#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using std::vector;
using std::set;
using std::string;


/////////////////////////////////////////////////////////////////////////////////////////////
//
//  Constructors and implemented canonicals:


/*!
  The constructor must:
  - Decode the URL to a filename and open the file (see openFile).
  - Save a copy of the URL just for the hell of it.

  \param url           - Specifies the file to open.
  \param exclusionList - Specifies the list of item types to not return to the caller.

*/
CFileDataSource::CFileDataSource(URL& url, vector<uint16_t> exclusionList) :
  m_fd(-1),
  m_url(*(new URL(url)))
{
  for (int i=0; i < exclusionList.size(); i++) {
    m_exclude.insert(exclusionList[i]);
  }
  openFile();			// May throw exceptions.
}
/**
 * construtor from fd:
 */
CFileDataSource::CFileDataSource(int fd, vector<uint16_t> exclusionlist) :
  m_fd(fd),  m_url(*(new URL("file://stdin/junk")))
{
  for (int i=0; i < exclusionlist.size(); i++) {
    m_exclude.insert(exclusionlist[i]);
  }
}

/*!
   The destructor must close the file (ignoring errors so that if it's open
   nothing happens).
   The url must be deleted as well.
*/
CFileDataSource::~CFileDataSource()
{
  delete &m_url;
  close(m_fd);
}
/////////////////////////////////////////////////////////////////////////////////////////
//
//  Mandatory interface:

/*!
  Provide the caller with the next item from the ring source.
  This is done by getting the ring item from the file and then ensuring
  that it is acceptable.  The first acceptable item is returned to the caller.
  
  \return CRingItem*
  \retval NULL - end of file reached without an acceptable item being found.
  \retval other - Pointer to a dynamically allocated CRingItem that is the
                  next acceptable item from the ring.  The caller is responsible for
		  deleting this storage when done with it.
*/
CRingItem*
CFileDataSource::getItem()
{
  while (1) {
    CRingItem* pItem = getItemFromFile();
    if (!pItem) {
      return pItem;
    }
    if (acceptable(pItem)) {
      return pItem;
    }
    // Skip the item, it's not acceptable.

    delete pItem;
  }
}
//////////////////////////////////////////////////////////////////////////////////////////
//
// Private utilties.

/*
**  Get the next item from the file:
**  First we fetch the header.  Using that we can create storage of the right size
**  and read the item into that.
**  We then allocate a ring item and fill its body in from the file.
**  Returns:
**    Pointer to the item read from file or NULL if we hit the end of file or an error.
*/
CRingItem*
CFileDataSource::getItemFromFile()
{
  // First read a header:

  RingItemHeader header;

  int nRead = io::readData(m_fd, &header, sizeof(header));
  if (nRead != sizeof(header)) {
    return reinterpret_cast<CRingItem*>(NULL);
  }
  uint32_t itemsize = getItemSize(header);
  uint32_t bodysize = itemsize - sizeof(header);

  // Read the remainder of the data:

  uint8_t* pBody = new uint8_t[bodysize];
  nRead          = io::readData(m_fd, pBody, bodysize);
  if (nRead != bodysize) {
    delete []pBody;
    return reinterpret_cast<CRingItem*>(NULL);
  }

  CRingItem* pItem = new CRingItem(1, itemsize); //  Type will get overwritten:


  // The ring item is filled in this way to preserve the initial byte order.

  pRingItemHeader pStorage = reinterpret_cast<pRingItemHeader>(pItem->getItemPointer());
  memcpy(pStorage, &header, sizeof(RingItemHeader));
  memcpy(pStorage+1, pBody, bodysize);

  delete []pBody;

  // Set the cursor:

  char* pCursor = reinterpret_cast<char*>(pStorage+1); // start of body.
  pCursor      += bodysize;	                       // end of body.
  pItem->setBodyCursor(pCursor);

  return pItem;
  
}
/*
** Determines if an item is acceptable.
**
** Parameters:
**   item - Pointer to the item.
** Returns:
**   True if acceptable, false if not.
*/
bool
CFileDataSource::acceptable(CRingItem* item) const
{
  uint16_t type             = item->type(); // Byte swaps as needed.
  set<uint16_t>::iterator i = m_exclude.find(type);
  return (i == m_exclude.end()); // Not found is good.
}
/*
** Opens the file that corresponds to the URL m_url.
** The resulting fd is placed in m_fd.
** errors are reported via exceptions which include:
**  CErrnoException - For open failures
**  CInvalidArgumentException  - for protocols that are not file:
**
*/
void
CFileDataSource::openFile()
{
  string p = m_url.getProto();
  if (p != string("file")) {
    throw CInvalidArgumentException(string(m_url), "A file URL only",
				    "Opening a file data source");
  }
  string fullPath= m_url.getPath();


  m_fd = open(fullPath.c_str(), O_RDONLY);
  if (m_fd == -1) {
    throw CErrnoException("Opening file data source");
  }
}
/*
**  Return the size of an item.  This does the right thing in the presence
**  of a creating system that is byte backwards than the executing system.
**
** Parameters:
**    header - refers to the item header to analyze.
** Returns:
**    number of bytes in the item.
*/
uint32_t
CFileDataSource::getItemSize(RingItemHeader& header)
{
  uint32_t size = header.s_size;
  uint32_t type = header.s_type;
  
  // If necessary, byte swap the size:

  if ((type & 0xffff0000) != 0) {
    union {
      uint8_t  bytes[4];
      uint32_t l;
    } lsize;
    lsize.l = size;
    size = 0;
    for (int i=0; i < 4; i++) {
      size |= (lsize.bytes[i] << 4*(3-i));
    }
  }


  return size;
}
