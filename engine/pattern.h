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
 *        Qpx::Pattern        
 *
 ******************************************************************************/

#ifndef PATTERN_H
#define PATTERN_H

#include <vector>
#include <string>
#include <cstdint>
#include <stddef.h>
#include "event.h"

namespace Qpx {

class Pattern {
private:
  std::vector<bool> gates_;
  size_t threshold_;

public:
  inline Pattern()
      : threshold_(0)
  {}

  inline Pattern(const std::string &s) {
    from_string(s);
  }

  void resize(size_t);
  std::vector<bool> gates() const { return gates_; }
  size_t threshold() const { return threshold_; }
  void set_gates(std::vector<bool>);
  void set_theshold(size_t);

  bool relevant(int16_t) const;
  bool validate(const Event &e) const;
  bool antivalidate(const Event &e) const;

  std::string to_string() const;
  void from_string(std::string s);

  std::string gates_to_string() const;
  std::vector<bool> gates_from_string(std::string s);

  bool operator==(const Pattern other) const;
  bool operator!=(const Pattern other) const {return !operator ==(other);}

};

}

#endif
