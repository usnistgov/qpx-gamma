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
 *      Qpx::Sink2D spectrum type for true coincidence with
 *      two dimensions of energy.
 *
 ******************************************************************************/

#pragma once

#include "spectrum.h"
#include <unordered_map>

namespace Qpx {

class Spectrum2D : public Spectrum
{
public:
  Spectrum2D();
  Spectrum2D* clone() const override { return new Spectrum2D(*this); }

protected:
  typedef std::map<std::pair<uint16_t,uint16_t>, PreciseFloat> SpectrumMap2D;
  
  bool _initialize() override;
  void init_from_file(std::string filename);

  std::string my_type() const override {return "2D";}

  PreciseFloat _data(std::initializer_list<size_t> list ) const override;
  std::unique_ptr<EntryList> _data_range(std::initializer_list<Pair> list) override;
  void _set_detectors(const std::vector<Qpx::Detector>& dets) override;

  void addEvent(const Event&) override;
  void _append(const Entry&) override;

  //save/load
  bool _write_file(std::string, std::string) const override;
  bool _read_file(std::string, std::string) override;
  
  std::string _data_to_xml() const override;
  uint16_t _data_from_xml(const std::string&) override;

  #ifdef H5_ENABLED
  void _load_data(H5CC::Group&) override;
  void _save_data(H5CC::Group&) const override;
  #endif

  //export to matlab script
  void write_m(std::string) const;
  void write_m4b(std::string) const;
  void write_mat(std::string) const;

  bool read_m4b(std::string);
  bool read_mat(std::string);

  //indexes of the two chosen channels
  std::vector<int8_t> pattern_;

  //the data itself
  SpectrumMap2D spectrum_;
  SpectrumMap2D temp_spectrum_;
  bool buffered_;

  bool check_symmetrization();
};

}
