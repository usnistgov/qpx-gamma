/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * This software can be redistributed and/or modified freely provided that
 * any derivative works bear some notice that they are derived from it, and
 * any modified versions bear some notice that they have been modified.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      Common includes for integration with Pixie API.
 *
 ******************************************************************************/

#ifndef PIXIE_COMMON
#define PIXIE_COMMON

#include "madc32.h"
#include <stdint.h>

const int PRESET_MAX_MODULES = 10;
const int NUMBER_OF_CHANNELS = 32;

typedef char S8;
typedef uint8_t U8;
typedef int16_t S16;
typedef uint16_t U16;
typedef int32_t S32;
typedef uint32_t U32;
typedef int64_t S64;
typedef uint64_t U64;

#endif
