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
 *      Qpx::Sink1D one-dimensional spectrum
 *
 ******************************************************************************/

#pragma once

#include "spectrum.h"

namespace Qpx {

class Delayometer : public Spectrum
{
  struct SpectrumItem
  {
    SpectrumItem() {}
    SpectrumItem(const PreciseFloat& t, const PreciseFloat& c)
      : ns(t), counts(c) {}

    PreciseFloat counts {0};
    PreciseFloat ns {0};
  };

public:
  Delayometer();
  Delayometer* clone() const override { return new Delayometer(*this); }

protected:
  std::string my_type() const override {return "Delayometer";}

  bool _initialize() override;

  PreciseFloat _data(std::initializer_list<uint16_t> list) const;
  std::unique_ptr<std::list<Entry>> _data_range(std::initializer_list<Pair> list) override;
  void _append(const Entry&) override;
  void _set_detectors(const std::vector<Qpx::Detector>& dets) override;

  //event processing
  void _push_hit(const Hit&) override;

  void addEvent(const Event&) override;

  std::string _data_to_xml() const override;
  uint16_t _data_from_xml(const std::string&) override;

  #ifdef H5_ENABLED
  void _load_data(H5CC::Group&) override;
  void _save_data(H5CC::Group&) const override;
  #endif

  std::map<int64_t, SpectrumItem> spectrum_;

  double maxchan_;
  TimeStamp timebase;
};

}
