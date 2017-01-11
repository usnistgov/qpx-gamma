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
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      Marker - for marking a channel or energt in a plot
 *
 ******************************************************************************/

#ifndef QPX_COORD_H
#define QPX_COORD_H

#include "calibration.h"

class Coord {
public:
  Coord() : bits_(0), energy_(nan("")), bin_(nan("")) {}

  void set_energy(double nrg, Qpx::Calibration cali);
  void set_bin(double bin, uint16_t bits, Qpx::Calibration cali);

  uint16_t bits() const {return bits_;}
  double energy() const;
  double bin(const uint16_t to_bits) const;


  bool null() { return ((bits_ == 0) && std::isnan(bin_) && std::isnan(energy_) ); }
  bool operator!= (const Coord& other) const { return (!operator==(other)); }
  bool operator== (const Coord& other) const {
    if (energy_ != other.energy_) return false;
    if (bin(bits_) != other.bin(bits_)) return false;
    return true;
  }

private:
  double energy_;
  double bin_;
  uint16_t bits_;
};


#endif
