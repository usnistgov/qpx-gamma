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
 *      Qpx::Sink1D_LFC for loss-free counting by statistical
 *      extrapolation from a preset minimum time sample.
 *
 ******************************************************************************/


#ifndef SPECTRUM1D_LFC_H
#define SPECTRUM1D_LFC_H

#include "spectrum1D.h"

namespace Qpx {

class Spectrum1D_LFC : public Spectrum1D
{
public:
  Spectrum1D_LFC();
  Spectrum1D_LFC* clone() const override { return new Spectrum1D_LFC(*this); }

protected:
  std::string my_type() const override {return "LFC1D";}
  bool _initialize() override;
  
  void _push_stats(const StatsUpdate&) override;

  void addHit(const Hit&) override;


  StatsUpdate time1_, time2_;
  double     time_sample_;
  
  std::vector<PreciseFloat> channels_all_;
  std::vector<PreciseFloat> channels_run_;
  PreciseFloat count_current_, count_total_;
  int my_channel_;
};


}
#endif // SPECTRUM_LFC_H
