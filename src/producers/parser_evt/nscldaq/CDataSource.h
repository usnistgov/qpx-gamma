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


// forward definitions:

class CRingItem;


/*! 
   Abstract base class that defines an interface that all dumper data sources 
   must meet.  Data sources provide data that can be dumped in a formatted
   way to the dumper's stdout.
*/

class CDataSource
{
  // constructors and canonicals.  In general most canonicals are not allowed.
  //

public:
  CDataSource();
  virtual ~CDataSource();

private:
  CDataSource(const CDataSource& rhs);
  CDataSource& operator=(const CDataSource& rhs);
  int operator==(const CDataSource& rhs) const;
  int operator!=(const CDataSource& rhs) const;
public:

  // This method must be implemented in concrete
  // sublcasses.  It returns the next item from the'
  // source the dumper deserves to get.
  //

  virtual CRingItem* getItem() = 0;


};
