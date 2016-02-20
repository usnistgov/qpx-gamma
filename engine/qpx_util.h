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
 *      TableChanSettings - tree for displaying and manipulating
 *      channel settings and chosing detectors.
 *
 ******************************************************************************/

#ifndef QPX_UTIL_H_
#define QPX_UTIL_H_

#include <string>
#include <sstream>
#include "custom_logger.h"

const std::vector<std::string> k_UTF_superscripts = {
  "\u2070", "\u00B9", "\u00B2",
  "\u00B3", "\u2074", "\u2075",
  "\u2076", "\u2077", "\u2078",
  "\u2079"
};

inline std::string to_str_precision(double number, int precision = -1) {
  std::ostringstream ss;
  if (precision < 0)
    ss << number;
  else
    ss << std::fixed << std::setprecision(precision) << number;
  return ss.str();
}

inline std::string UTF_superscript(uint16_t value) {
  std::string output;
  if (value < 10)
    output = k_UTF_superscripts[value];
  else
    output = UTF_superscript(value / 10) + k_UTF_superscripts[value % 10];
  return output;
}

inline std::string itobin16 (uint16_t bin)
{
  std::stringstream ss;
  for (int k = 0; k < 16; ++k) {
    if (bin & 0x8000)
      ss << "1";
    else
      ss << "0";
    bin <<= 1;
  }
  return ss.str();
}

inline std::string itobin32 (uint32_t bin)
{
  uint16_t lo = bin & 0x0000FFFF;
  uint16_t hi = (bin >> 16) & 0x0000FFFF;

  return (itobin16(hi) + " " + itobin16(lo));
}

inline std::string itobin64 (uint64_t bin)
{
  uint32_t lo = bin & 0x00000000FFFFFFFF;
  uint32_t hi = (bin >> 32) & 0x00000000FFFFFFFF;

  return (itobin32(hi) + " " + itobin32(lo));
}


inline std::string itohex64 (uint64_t bin)
{
  std::stringstream stream;
  stream << std::uppercase << std::setfill ('0') << std::setw(sizeof(uint64_t)*2)
         << std::hex << bin;
  return stream.str();
}

inline std::string itohex32 (uint32_t bin)
{
  std::stringstream stream;
  stream << std::uppercase << std::setfill ('0') << std::setw(sizeof(uint32_t)*2)
         << std::hex << bin;
  return stream.str();
}

inline std::string itohex16 (uint16_t bin)
{
  std::stringstream stream;
  stream << std::uppercase << std::setfill ('0') << std::setw(sizeof(uint16_t)*2)
         << std::hex << bin;
  return stream.str();
}

#endif
