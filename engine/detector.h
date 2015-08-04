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
#include "xmlable.h"
#include "calibration.h"

namespace Gamma {

class Detector : public XMLable {
 public:
  Detector()
      : energy_calibrations_("Calibrations")
      , fwhm_calibration_("FWHM", 0)
      , name_("none")
      , type_("none") {}
  
  Detector(tinyxml2::XMLElement* el) : Detector() {from_xml(el);}
  Detector(std::string name) : Detector() {name_ = name;}

  std::string xml_element_name() const override {return "Detector";}
  
  bool shallow_equals(const Detector& other) const {return (name_ == other.name_);}
  bool operator!= (const Detector& other) const {return !operator==(other);}
  bool operator== (const Detector& other) const {
    if (name_ != other.name_) return false;
    if (type_ != other.type_) return false;
    if (setting_values_ != other.setting_values_) return false;
    if (setting_names_ != other.setting_names_) return false;
    //if (energy_calibration_ != other.energy_calibration_) return false;
    return true;
  }

  void to_xml(tinyxml2::XMLPrinter&) const;
  void from_xml(tinyxml2::XMLElement*);
  Calibration highest_res_calib();
  
  std::string name_, type_;
  XMLableDB<Calibration> energy_calibrations_;
  Calibration fwhm_calibration_;
  std::vector<double> setting_values_;
  std::vector<std::string> setting_names_;
  
};

}

#endif
