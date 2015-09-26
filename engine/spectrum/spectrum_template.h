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
 *      Qpx::Spectrum::Template defines parameters for new spectrum
 *
 *      not thread-safe
 *
 ******************************************************************************/

#ifndef PIXIE_SPECTRUM_TEMPLATE_H
#define PIXIE_SPECTRUM_TEMPLATE_H

#include <string>
#include <vector>
#include "tinyxml2.h"
#include "generic_setting.h"
#include "xmlable.h"
#include "daq_types.h"

namespace Qpx {
namespace Spectrum {

class Template : public XMLable {
 public:
  Template(): //default appearance is blue in RGBA
  type("invalid"), name_("noname"), bits(14), appearance(4278190335), visible(false) {}

  std::string xml_element_name() const override {return "PixieSpectrumTemplate";}
  Template(std::string name) : Template() {name_ = name;}
  
  void from_xml(tinyxml2::XMLElement* el) override;
  void to_xml(tinyxml2::XMLPrinter&) const override;

  bool shallow_equals(const Template& other) const {return (name_ == other.name_);}
  bool operator!= (const Template& other) const {return !operator==(other);}
  bool operator== (const Template& other) const {
    if (name_ != other.name_) return false;
    if (type != other.type) return false; //assume other type info same
    if (bits != other.bits) return false;
    if (match_pattern != other.match_pattern) return false;
    if (add_pattern != other.add_pattern) return false;
    //appearance and visibility are trivial
    //    if (appearance != other.appearance) return false;
    //    if (visible != other.visible) return false;
    if (generic_attributes != other.generic_attributes) return false;
    return true;
  }
  
  std::string name_;
  uint8_t bits;
  std::vector<int16_t> match_pattern, add_pattern;
  uint32_t appearance;
  bool visible;
  std::vector<Gamma::Setting> generic_attributes;

  //this stuff from factory
  std::string type, description;
  std::list<std::string> input_types;
  std::list<std::string> output_types;
  //int dimensions?
};

}
}

#endif
