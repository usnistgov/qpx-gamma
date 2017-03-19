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

#include "coord.h"
//#include "custom_logger.h"

void Coord::set_energy(double nrg, Qpx::Calibration cali)
{
  bin_ = nan("");
  bits_ = 0;
  energy_ = nrg;
  if (!std::isnan(nrg))
  {
    bits_ = cali.bits();
    bin_ = cali.inverse_transform(nrg);
  }
}

void Coord::set_bin(double bin, uint16_t bits, Qpx::Calibration cali)
{
  bin_ = bin;
  bits_ = bits;
  energy_ = nan("");
  if (!std::isnan(bin)/* && cali.valid()*/)
    energy_ = cali.transform(bin_, bits_);
//  DBG << "made pos bin" << bin_ << " bits" << bits_ << " nrg" << energy_;
}

double Coord::energy() const
{
  return energy_;
}

double Coord::bin(const uint16_t to_bits) const
{
  if (!to_bits || !bits_)
    return bin_;

  if (bits_ > to_bits)
    return bin_ / pow(2, bits_ - to_bits);
  if (bits_ < to_bits)
    return bin_ * pow(2, to_bits - bits_);
  else
    return bin_;
}

