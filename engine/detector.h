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
 *      Gamma::Calibration defines calibration with units and math model
 *      Gamma::Detector defines detector with name, calibration, DSP parameters
 *
 ******************************************************************************/


#ifndef GAMMA_DETECTOR
#define GAMMA_DETECTOR

#include <vector>
#include <string>
#include <boost/date_time.hpp>
#include "calibration.h"
#include "generic_setting.h"

namespace Gamma {

class Detector : public XMLable2 {
 public:
  Detector()
      : energy_calibrations_("Calibrations")
      , gain_match_calibrations_("GainMatchCalibrations")
      , settings_("Optimization")
      , fwhm_calibration_("FWHM", 0)
      , name_("none")
      , type_("none")
  {settings_.metadata.setting_type = SettingType::stem;}
  

  Detector(std::string name) : Detector() {name_ = name;}
  std::string xml_element_name() const override {return "Detector";}
  
  bool shallow_equals(const Detector& other) const {return (name_ == other.name_);}
  bool operator!= (const Detector& other) const {return !operator==(other);}
  bool operator== (const Detector& other) const {
    if (name_ != other.name_) return false;
    if (type_ != other.type_) return false;
    if (settings_ != other.settings_) return false;
    if (energy_calibrations_ != other.energy_calibrations_) return false;
    if (gain_match_calibrations_ != other.gain_match_calibrations_) return false;
    if (fwhm_calibration_ != other.fwhm_calibration_) return false;
    return true;
  }

  void from_xml(const pugi::xml_node &) override;
  void to_xml(pugi::xml_node &) const override;
  void to_xml_options(pugi::xml_node &root, bool options) const;

  Calibration highest_res_calib();
  Calibration get_gain_match(uint16_t bits, std::string todet);
  
  std::string name_, type_;
  XMLable2DB<Calibration> energy_calibrations_;
  XMLable2DB<Calibration> gain_match_calibrations_;

  Calibration fwhm_calibration_;
  Setting settings_;
  
};

}

#endif
