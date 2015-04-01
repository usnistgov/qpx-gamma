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
 *      Pixie::Spectrum::Spectrum1D one-dimensional spectrum
 *
 ******************************************************************************/

#ifndef PIXIE_SPECTRUM1D_H
#define PIXIE_SPECTRUM1D_H

#include "spectrum.h"

namespace Pixie {
namespace Spectrum {

class Spectrum1D : public Spectrum
{
public:
  Spectrum1D() {};

  static Template get_template() {
    Template new_temp;
    new_temp.type = "1D";
    new_temp.input_types = {"cnf", "tka", "n42", "ava"};
    new_temp.output_types = {"n42", "tka"};
    new_temp.description = "Traditional MCA spectrum";
    return new_temp;
  }
  

protected:
  std::string my_type() const override {return "1D";}

  //1D is ok with all patterns
  bool initialize() override {
    dimensions_ = 1;
    spectrum_.resize(resolution_, 0);
    detectors_.resize(kNumChans);
    recalc_energies();
    return true;
  }

  uint64_t _get_count(std::initializer_list<uint16_t> list) const;
  std::unique_ptr<std::list<Entry>> _get_spectrum(std::initializer_list<Pair> list);

  //event processing
  void addHit(const Hit&) override;
  virtual void addHit(const Hit&, int);

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
  
  bool read_cnf(std::string);
  bool read_tka(std::string);
  bool read_n42(std::string);
  bool read_ava(std::string);

  std::vector<uint64_t> spectrum_;
};

}}

#endif // PIXIE_SPECTRUM1D_H
