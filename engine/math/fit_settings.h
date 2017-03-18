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
 *      FitSettings -
 *
 ******************************************************************************/

#pragma once

#include "fit_param.h"
#include "calibration.h"

#include "xmlable.h"

#include "json.hpp"
using namespace nlohmann;

class FitSettings : public XMLable
{
public:
  bool overriden;

  double finder_cutoff_kev;

  uint16_t KON_width;
  double   KON_sigma_spectrum;
  double   KON_sigma_resid;

  uint16_t ROI_max_peaks;
  double   ROI_extend_peaks;
  double   ROI_extend_background;
  uint16_t background_edge_samples;
  bool     sum4_only;

  bool     resid_auto;
  uint16_t resid_max_iterations;
  uint64_t resid_min_amplitude;
  double   resid_too_close;

  bool     small_simplify;
  uint64_t small_max_amplitude;

  bool     width_common;
  FitParam width_common_bounds;
  bool     width_at_511_variable;
  double   width_at_511_tolerance;

  //hypermet
  bool gaussian_only;
  double   lateral_slack;
  FitParam width_variable_bounds;
  //variable bounds
  FitParam step_amplitude;
  FitParam tail_amplitude;
  FitParam tail_slope;
  FitParam Lskew_amplitude;
  FitParam Lskew_slope;
  FitParam Rskew_amplitude;
  FitParam Rskew_slope;
  uint16_t fitter_max_iter;

  //specific to spectrum
  Qpx::Calibration cali_nrg_, cali_fwhm_;
  uint16_t bits_;
  boost::posix_time::time_duration real_time ,live_time;

  FitSettings();
  void clone(FitSettings other);
  void clear();

  double nrg_to_bin(double energy) const;
  double bin_to_nrg(double bin) const;
  double bin_to_width(double bin) const;
  double nrg_to_fwhm(double energy) const;

  void to_xml(pugi::xml_node &node) const override;
  void from_xml(const pugi::xml_node &node) override;
  std::string xml_element_name() const override {return "FitSettings";}
};

void to_json(json& j, const FitSettings& s);
void from_json(const json& j, FitSettings& s);

