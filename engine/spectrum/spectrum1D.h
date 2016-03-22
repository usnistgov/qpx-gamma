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
 *      Qpx::Spectrum::Spectrum1D one-dimensional spectrum
 *
 ******************************************************************************/

#ifndef SPECTRUM1D_H
#define SPECTRUM1D_H

#include "spectrum.h"

namespace Qpx {
namespace Spectrum {

class Spectrum1D : public Spectrum
{
public:
  Spectrum1D();

protected:
  std::string my_type() const override {return "1D";}

  //1D is ok with all patterns
  bool initialize() override {
    cutoff_bin_ = get_attr("cutoff_bin").value_int;
    maxchan_ = 0;

    spectrum_.resize(pow(2, metadata_.bits), 0);

    Spectrum::initialize();

    return true;
  }

  PreciseFloat _get_count(std::initializer_list<uint16_t> list) const;
  std::unique_ptr<std::list<Entry>> _get_spectrum(std::initializer_list<Pair> list);
  void _add_bulk(const Entry&) override;
  void _set_detectors(const std::vector<Qpx::Detector>& dets) override;

  //event processing
  void addEvent(const Event&) override;
  virtual void addHit(const Hit&);

  //save/load
  bool _write_file(std::string, std::string) const override;
  bool _read_file(std::string, std::string) override;

  std::string _channels_to_xml() const override;
  uint16_t _channels_from_xml(const std::string&) override;

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
  int32_t cutoff_bin_;
  uint16_t maxchan_;
};

}}

#endif // SPECTRUM1D_H
