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
 *      Qpx::Spectrum::Spectrum2D spectrum type for true coincidence with
 *      two dimensions of energy.
 *
 ******************************************************************************/


#ifndef PIXIE_SPECTRUM2D_H
#define PIXIE_SPECTRUM2D_H

#include "spectrum.h"
#include <unordered_map>

namespace Qpx {
namespace Spectrum {

class Spectrum2D : public Spectrum
{
public:
  Spectrum2D() {}

  static Template get_template() {
    Template new_temp;
    new_temp.type = "2D";
    new_temp.output_types = {"m", "esc", "spn"};
    new_temp.input_types = {"esc", "spn"};
    new_temp.description = "Matrix-type coincidence spectrum for two detectors";

    Gamma::Setting buf;
    buf.id_ = "buffered";
    buf.metadata.setting_type = Gamma::SettingType::boolean;
    buf.metadata.unit = "T/F";
    buf.metadata.description = "Buffered output for efficient plotting (more memory)";
    buf.metadata.writable = true;
    new_temp.generic_attributes.add(buf);

    //have option to not buffer
    return new_temp;
  }
  
protected:
  typedef std::map<std::pair<uint16_t,uint16_t>, uint64_t> SpectrumMap2D;
  
  bool initialize() override;
  void init_from_file(std::string filename);

  std::string my_type() const override {return "2D";}

  uint64_t _get_count(std::initializer_list<uint16_t> list ) const;
  std::unique_ptr<EntryList> _get_spectrum(std::initializer_list<Pair> list);
  void _set_detectors(const std::vector<Gamma::Detector>& dets) override;

  void addEvent(const Event&) override;
  void _add_bulk(const Entry&) override;

  //save/load
  bool _write_file(std::string, std::string) const override;
  bool _read_file(std::string, std::string) override;
  
  std::string _channels_to_xml() const override;
  uint16_t _channels_from_xml(const std::string&) override;
  
  //export to matlab script
  void write_m(std::string) const;
  void write_esc(std::string) const;
  void write_spn(std::string) const;

  bool read_esc(std::string);
  bool read_spn(std::string);

  //indexes of the two chosen channels
  std::vector<int8_t> pattern_;

  //the data itself
  SpectrumMap2D spectrum_;
  SpectrumMap2D temp_spectrum_;
  bool buffered_;
  
};

}}

#endif
