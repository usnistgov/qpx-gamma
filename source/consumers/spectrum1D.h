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
 ******************************************************************************/

#pragma once

#include "spectrum.h"

namespace Qpx {

class Spectrum1D : public Spectrum
{
public:
  Spectrum1D();
  Spectrum1D* clone() const override { return new Spectrum1D(*this); }

protected:
  std::string my_type() const override {return "1D";}

  bool _initialize() override;

  PreciseFloat _data(std::initializer_list<size_t> list) const override;
  std::unique_ptr<std::list<Entry>> _data_range(std::initializer_list<Pair> list) override;
  void _append(const Entry&) override;
  void _set_detectors(const std::vector<Qpx::Detector>& dets) override;

  //event processing
  void addEvent(const Event&) override;
  virtual void addHit(const Hit&);

  //save/load
  bool _write_file(std::string, std::string) const override;
  bool _read_file(std::string, std::string) override;

  std::string _data_to_xml() const override;
  uint16_t _data_from_xml(const std::string&) override;

  #ifdef H5_ENABLED
  void _load_data(H5CC::Group&) override;
  void _save_data(H5CC::Group&) const override;
  #endif

  bool channels_from_string(std::istream &data_stream, bool compression);
  void init_from_file(std::string filename);

  //format-specific stuff
  void write_tka(std::string) const;
  void write_n42(std::string) const;
  void write_spe(std::string) const;

  bool read_xylib(std::string, std::string);
  bool read_tka(std::string);
  bool read_n42(std::string);
  bool read_ava(std::string);
  bool read_dat(std::string);
  bool read_spe_radware(std::string);
  bool read_spe_gammavision(std::string);

  std::vector<PreciseFloat> spectrum_;
  uint32_t cutoff_bin_;
  uint16_t maxchan_;
};

}
