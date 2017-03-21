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
 *      Qpx::Consumer generic spectrum type.
 *                       All public methods are thread-safe.
 *                       When deriving override protected methods.
 *
 *      Qpx::ConsumerFactory creates spectra of appropriate type
 *                               by type name, from template, or
 *                               from file.
 *
 *      Qpx::ConsumerRegistrar for registering new spectrum types.
 *
 ******************************************************************************/

#pragma once

#include "detector.h"

#include "json.hpp"
using namespace nlohmann;

namespace Qpx {

class ConsumerMetadata : public XMLable
{
public:
  ConsumerMetadata();
  ConsumerMetadata(std::string tp, std::string descr, uint16_t dim,
           std::list<std::string> itypes, std::list<std::string> otypes);

  // read & write
  Setting get_attribute(std::string setting) const;
  Setting get_attribute(std::string setting, int32_t idx) const;
  Setting get_attribute(Setting setting) const;
  Setting get_all_attributes() const;
  void set_attribute(const Setting &setting);

  Setting attributes() const;
  void set_attributes(const Setting &settings);
  void overwrite_all_attributes(Setting settings);

  //read only
  std::string type() const {return type_;}
  std::string type_description() const {return type_description_;}
  uint16_t dimensions() const {return dimensions_;}
  std::list<std::string> input_types() const {return input_types_;}
  std::list<std::string> output_types() const {return output_types_;}


  void disable_presets();
  void set_det_limit(uint16_t limit);
  bool chan_relevant(uint16_t chan) const;

  std::string debug(std::string prepend) const;

  bool shallow_equals(const ConsumerMetadata& other) const;
  bool operator!= (const ConsumerMetadata& other) const;
  bool operator== (const ConsumerMetadata& other) const;

  //XMLable
  std::string xml_element_name() const override {return "SinkMetadata";}
  void to_xml(pugi::xml_node &node) const override;
  void from_xml(const pugi::xml_node &node) override;

  friend void to_json(json& j, const ConsumerMetadata &s);
  friend void from_json(const json& j, ConsumerMetadata &s);

private:
  //this stuff from factory, immutable upon initialization
  std::string type_ {"invalid"};
  std::string type_description_;
  uint16_t dimensions_ {0};
  std::list<std::string> input_types_;
  std::list<std::string> output_types_;

  //can change these
  Setting attributes_ {"Options"};

public:
  std::vector<Qpx::Detector> detectors;

};

}
