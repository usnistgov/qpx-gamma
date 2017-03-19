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

#include "calibration.h"
#include "setting.h"

#include "json.hpp"
using namespace nlohmann;

namespace Qpx {

class Detector : public XMLable
{
private:
  std::string name_ {"unknown"};
  std::string type_ {"unknown"};

  XMLableDB<Calibration> energy_calibrations_ {"Calibrations"};
  XMLableDB<Calibration> gain_match_calibrations_ {"GainMatchCalibrations"};
  Calibration fwhm_calibration_;
  Calibration efficiency_calibration_;

  Setting settings_ {"Optimization"};

public:
  Detector();
  Detector(std::string name);

  std::string name() const;
  std::string type() const;

  Calibration resolution() const;
  Calibration efficiency() const;

  Calibration get_gain_match(uint16_t bits, std::string todet) const;
  XMLableDB<Calibration> gain_calibrations() const;
  void remove_gain_calibration(size_t pos);

  bool has_energy_calib(int bits) const;
  Calibration get_energy_calib(int bits) const;
  Calibration best_calib(int bits) const;
  Calibration highest_res_calib() const;
  XMLableDB<Calibration> energy_calibrations() const;
  void remove_energy_calibration(size_t pos);

  std::list<Setting> optimizations() const;
  Setting get_setting(std::string id) const;

  void set_name(const std::string&);
  void set_type(const std::string&);
  void set_energy_calibration(const Calibration&);
  void set_gain_calibration(const Calibration&);
  void set_resolution_calibration(const Calibration&);
  void set_efficiency_calibration(const Calibration&);
  void add_optimizations(const std::list<Setting>&, bool writable_only = false);
  void clear_optimizations();

  std::string debug(std::string prepend) const;


  bool shallow_equals(const Detector& other) const {return (name_ == other.name_);}
  bool operator== (const Detector& other) const;
  bool operator!= (const Detector& other) const;
  
  //XMLable
  std::string xml_element_name() const override {return "Detector";}
  void from_xml(const pugi::xml_node &) override;
  void to_xml(pugi::xml_node &) const override;
  void to_xml_options(pugi::xml_node &root, bool options) const;

  friend void to_json(json& j, const Detector &s);
  friend void from_json(const json& j, Detector &s);
  json to_json(bool options) const;
};

}
