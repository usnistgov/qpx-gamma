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
 *      Types for organizing data aquired from device
 *        Qpx::Hit        single energy event with coincidence flags
 *
 ******************************************************************************/

#ifndef QPX_DIGITIZED_VALUE
#define QPX_DIGITIZED_VALUE

#include <string>
#include <fstream>

namespace Qpx {

class DigitizedVal {
public:

  inline DigitizedVal()
  {
    val_ = 0;
    bits_ = 16;
  }

  inline DigitizedVal(uint16_t v, uint16_t b)
  {
    val_ = v;
    bits_ = b;
  }

  inline void set_val(uint16_t v)
  {
    val_ = v;
  }

  inline uint16_t bits() const
  {
    return bits_;
  }

  inline uint16_t val(uint16_t bits) const
  {
    if (bits == bits_)
      return val_;
    else if (bits < bits_)
      return val_ >> (bits_ - bits);
    else
      return val_ << (bits - bits_);
  }

  inline bool operator==(const DigitizedVal other) const
  {
    return ((val_ == other.val_) && (bits_ == other.bits_));
  }

  inline bool operator!=(const DigitizedVal other) const
  {
    return !operator==(other);
  }

  inline void write_bin(std::ofstream &outfile) const
  {
    outfile.write((char*)&val_, sizeof(val_));
  }

  inline void read_bin(std::ifstream &infile)
  {
    infile.read(reinterpret_cast<char*>(&val_), sizeof(val_));
  }

  std::string to_string() const;

private:
  uint16_t  val_;
  uint16_t  bits_;
};

}

#endif
