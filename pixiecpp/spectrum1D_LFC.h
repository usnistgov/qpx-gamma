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
 *      Pixie::Spectrum::Spectrum1D_LFC for loss-free counting by statistical
 *      extrapolation from a preset minimum time sample.
 *
 ******************************************************************************/


#ifndef PIXIE_SPECTRUM1D_LFC_H
#define PIXIE_SPECTRUM1D_LFC_H

#include "spectrum1D.h"

namespace Pixie {
namespace Spectrum {

class Spectrum1D_LFC : public Spectrum1D
{
public:
  Spectrum1D_LFC() {};

  static Template get_template() {
    Template new_temp = Spectrum1D::get_template();
    new_temp.type = "LFC1D";
    new_temp.input_types = {};
    new_temp.description = "One detector loss-free spectrum";
    
    Setting t_sample;
    t_sample.name = "time_sample";
    t_sample.unit = "seconds";
    t_sample.value = 20.0;
    t_sample.description = "minimum \u0394t before compensating";
    t_sample.writable = true;
    new_temp.generic_attributes.push_back(t_sample);
    
    return new_temp;
  }

protected:
  std::string my_type() const override {return "LFC1D";}
  bool initialize() override;
  
  void addStats(const StatsUpdate&) override;
  void addRun(const RunInfo&) override;

  void addHit(const Hit&, int) override;


  StatsUpdate time1_, time2_;
  double     time_sample_;
  
  std::vector<PreciseFloat> channels_all_;
  std::vector<PreciseFloat> channels_run_;
  PreciseFloat count_current_, count_total_;
  int my_channel_;
};


}}
#endif // PIXIE_SPECTRUM_LFC_H
