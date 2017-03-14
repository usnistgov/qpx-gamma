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
 *      Qpx::Calibration defines calibration with units and math model
 *      Qpx::Detector defines detector with name, calibration, DSP parameters
 *
 ******************************************************************************/

#pragma once

#include <vector>
#include <string>
#include <boost/date_time.hpp>
#include "calibration.h"
#include "generic_setting.h"

#include "json.hpp"
using namespace nlohmann;

namespace Qpx {

class Detector : public XMLable
{
 public:
  Detector();
  Detector(std::string name);

  std::string xml_element_name() const override {return "Detector";}
  
  bool shallow_equals(const Detector& other) const {return (name_ == other.name_);}
  bool operator== (const Detector& other) const;
  bool operator!= (const Detector& other) const;

  std::string debug(std::string prepend) const;

  void from_xml(const pugi::xml_node &) override;
  void to_xml(pugi::xml_node &) const override;
  void to_xml_options(pugi::xml_node &root, bool options) const;

  Calibration highest_res_calib() const;
  Calibration best_calib(int bits) const;
  Calibration get_gain_match(uint16_t bits, std::string todet) const;
  
  std::string name_ {"none"};
  std::string type_ {"none"};
  XMLableDB<Calibration> energy_calibrations_ {"Calibrations"};
  XMLableDB<Calibration> gain_match_calibrations_ {"GainMatchCalibrations"};

  Calibration fwhm_calibration_ {"FWHM", 0};
  Calibration efficiency_calibration_ {"Efficiency", 0};
  Setting settings_ {"Optimization"};
};

void to_json(json& j, const Detector &s);
void to_json_options(json& j, const Detector &s, bool options);
void from_json(const json& j, Detector &s);

}
