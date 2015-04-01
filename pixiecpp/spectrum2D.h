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
 *      Pixie::Spectrum::Spectrum2D spectrum type for true coincidence with
 *      two dimensions of energy.
 *
 ******************************************************************************/


#ifndef PIXIE_SPECTRUM2D_H
#define PIXIE_SPECTRUM2D_H

#include "spectrum.h"
#include <unordered_map>

namespace Pixie {
namespace Spectrum {

class Spectrum2D : public Spectrum
{
public:
  Spectrum2D() {};

  static Template get_template() {
    Template new_temp;
    new_temp.type = "2D";
    new_temp.output_types = {"m"};
    new_temp.description = "Matrix-type coincidence spectrum for two detectors";

    Setting buf;
    buf.name = "buffered";
    buf.unit = "T/F";
    buf.value = 0;
    buf.description = "Buffered output for efficient plotting (more memory)";
    buf.writable = true;
    new_temp.generic_attributes.push_back(buf);

    //have option to not buffer
    return new_temp;
  }
  
protected:
  typedef std::map<std::pair<uint16_t,uint16_t>, uint64_t> SpectrumMap2D;
  
  bool initialize() override;
  std::string my_type() const override {return "2D";}

  uint64_t _get_count(std::initializer_list<uint16_t> list ) const;
  std::unique_ptr<EntryList> _get_spectrum(std::initializer_list<Pair> list);

  void addHit(const Hit&) override;
  virtual void addHit(const Hit&, int, int);

  //save/load
  bool _write_file(std::string, std::string) const override;
  
  std::string _channels_to_xml() const override;
  uint16_t _channels_from_xml(const std::string&) override;
  
  //export to matlab script
  void write_m(std::string) const;

  //indexes of the two chosen channels
  std::vector<int8_t> pattern_;

  //the data itself
  SpectrumMap2D spectrum_;
  SpectrumMap2D temp_spectrum_;
  bool buffered_;
  
};

}}

#endif
