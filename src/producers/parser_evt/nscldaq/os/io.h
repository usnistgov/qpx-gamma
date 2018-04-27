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

#include <unistd.h>
#include <stdint.h>

/**
 * @file io.h
 * @brief Commonly used I/O method definitions.
 * @author Ron Fox
 */

namespace io {
  void writeData (int fd, const void* pData , size_t size);
  size_t readData (int fd, void* pBuffer,  size_t nBytes);
}
