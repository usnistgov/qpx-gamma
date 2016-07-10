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


#ifndef SpectrumAddback1D_H
#define SpectrumAddback1D_H

#include "spectrum1D.h"

namespace Qpx {

class SpectrumAddback1D : public Spectrum1D
{
public:
  SpectrumAddback1D();
  SpectrumAddback1D* clone() const override { return new SpectrumAddback1D(*this); }

protected:
  std::string my_type() const override {return "Addback 1D";}
  void addEvent(const Event&) override;
};


}
#endif // SPECTRUM_LFC_H
