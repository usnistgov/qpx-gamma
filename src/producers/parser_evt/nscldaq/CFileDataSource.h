#pragma once
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


#include "CDataSource.h"
#include <set>
#include <vector>
#include <stdint.h>


// Forward class definitions:

class URL;
class CRingItem;
struct _RingItemHeader;

/*!
  Provide a data source from an event file.  This allows users to directly dump
  an event file to stdout.  The data source returns sequential ring items
  that are not in the excluded set of data types.

*/

class CFileDataSource : public CDataSource
{
  // Private per-object data:

private:
  int                  m_fd;	  // File descriptor open on the event source.
  std::set<uint16_t>   m_exclude; // item types to exclude from the return set.
  URL&                 m_url;	  // URI that points to the file.

  // Constructors and other canonicals:

public:
  CFileDataSource(URL& url,  std::vector<uint16_t> exclusionlist);
  CFileDataSource(int fd, std::vector<uint16_t> exclusionlist);
  virtual ~CFileDataSource();

private:
  CFileDataSource(const CFileDataSource& rhs);
  CFileDataSource& operator=(const CFileDataSource& rhs);
  int operator==(const CFileDataSource& rhs) const;
  int operator!=(const CFileDataSource& rhs) const;
public:

  // Mandatory interface:

  virtual CRingItem* getItem();

  // utilities:

private:
  CRingItem* getItemFromFile();
  bool       acceptable(CRingItem* item) const;
  void       openFile();
  uint32_t   getItemSize(_RingItemHeader& header);
};
