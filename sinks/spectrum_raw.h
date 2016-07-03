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
 *      Qpx::SinkRaw to output custom list mode to file
 *
 ******************************************************************************/

#ifndef SPECTRUM_RAW_H
#define SPECTRUM_RAW_H

#include "spectrum.h"

namespace Qpx {

class SpectrumRaw : public Spectrum
{
protected:
  std::string file_dir_;
  std::string file_name_bin_;
  std::string file_name_txt_;

  std::ofstream file_bin_;
  std::streampos bin_begin_;

  bool open_xml_, open_bin_;
  pugi::xml_document xml_doc_;
  pugi::xml_node xml_root_;

  uint64_t hits_this_spill_, total_hits_;

  bool ignore_patterns_;

public:
  SpectrumRaw();
  SpectrumRaw(const SpectrumRaw&other)
    : Spectrum(other)
    , file_dir_(other.file_dir_)
    , file_name_bin_(other.file_name_bin_)
    , file_name_txt_(other.file_name_txt_)
    , hits_this_spill_(0)
    , total_hits_(0)
    , open_xml_(false)
    , open_bin_(false)
  {}

  SpectrumRaw* clone() const override { return new SpectrumRaw(*this); }

  ~SpectrumRaw();

protected:
  std::string my_type() const override {return "Raw";}

  bool _initialize() override;

  PreciseFloat _data(std::initializer_list<uint16_t> list) const
    { return Sink::_data(list);}
  std::unique_ptr<std::list<Entry>> _data_range(std::initializer_list<Pair> range)
    { return Sink::_data_range(range); }

  //event processing
  void _push_spill(const Spill&) override;
  void _push_hit(const Hit&) override;

  void addEvent(const Event&) override;
  void _flush() override;

  bool init_text();
  bool init_bin();
  void writeHit(const Hit&);

  std::string _data_to_xml() const override {return "written to file";}
  uint16_t _data_from_xml(const std::string&) override {return 0;}
};

}

#endif // SPECTRUM1D_H
