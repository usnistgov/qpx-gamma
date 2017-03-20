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
 *      Qpx::Peak
 *
 ******************************************************************************/

#pragma once

#include <vector>
#include <set>
#include "polynomial.h"
#include "hypermet.h"
#include "detector.h"
#include "sum4.h"
#include "UncertainDouble.h"

#include "xmlable.h"

#include "json.hpp"
using namespace nlohmann;

namespace Qpx {

class Peak {
public:
  Peak(){}

  Peak(const json& j, const Finder &fs,
       const SUM4Edge& LB, const SUM4Edge& RB);

  Peak(const Hypermet &hyp, const SUM4 &s4, const FitSettings &fs);

  void reconstruct(FitSettings fs);

  //get rid of these
  std::vector<double> hr_peak_, hr_fullfit_;
  double intensity_theoretical_ {0.0};
  double efficiency_relative_ {0.0};

  const SUM4     &sum4() const { return sum4_;}
  const Hypermet &hypermet() const { return hypermet_;}

  const UncertainDouble &center() const {return center_;}
  const UncertainDouble &energy() const {return energy_;}
  const UncertainDouble &fwhm() const {return fwhm_;}
  const UncertainDouble &area_sum4() const {return area_sum4_;}
  const UncertainDouble &area_hyp() const {return area_hyp_;}
  const UncertainDouble &area_best() const {return area_best_;}
  const UncertainDouble &cps_sum4() const  {return cps_sum4_;}
  const UncertainDouble &cps_hyp() const {return cps_hyp_;}
  const UncertainDouble &cps_best() const  {return cps_best_;}

  void override_energy(double);

  int quality_energy() const;
  int quality_fwhm() const;
  bool good() const;

  bool operator<(const Peak &other) const;
  bool operator==(const Peak &other) const;
  bool operator!=(const Peak &other) const;
  bool operator>(const Peak &other) const;

  void to_xml(pugi::xml_node &node) const;
  void from_xml(const pugi::xml_node &node);
  std::string xml_element_name() const {return "Peak";}

  friend void to_json(json& j, const Peak &s);

private:
  SUM4 sum4_;
  Hypermet hypermet_;

  UncertainDouble center_, energy_;
  UncertainDouble fwhm_;
  UncertainDouble area_sum4_, area_hyp_, area_best_;
  UncertainDouble cps_sum4_, cps_hyp_, cps_best_;
  
};

}
