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
 *      FitParam -
 *
 ******************************************************************************/

#ifndef FIT_PARAM_H
#define FIT_PARAM_H

#include "fityk.h"
#include "calibration.h"
#include <string>
#include <limits>

class FitParam {

public:
  FitParam(std::string name, double v) :
    FitParam(name, v,
             std::numeric_limits<double>::min(),
             std::numeric_limits<double>::max())
  {}

  FitParam(std::string name, double v, double lower, double upper) :
    name_(name),
    val(v),
    uncert(std::numeric_limits<double>::infinity()),
    lbound(lower),
    ubound(upper)
  {}

  std::string name() {return name_;}

  const std::string def_bounds();
  bool extract(fityk::Fityk* f, fityk::Func* func);

  double val, uncert, lbound, ubound;

private:
  std::string name_;

  double get_err(fityk::Fityk* f,
                 std::string funcname);

};

class FitSettings {
public:
  bool override_;

  double finder_cutoff_kev;

  uint16_t KON_width;
  double   KON_sigma_spectrum;
  double   KON_sigma_resid;

  uint16_t ROI_max_peaks;
  double   ROI_extend_peaks;
  double   ROI_extend_background;

  uint16_t background_edge_samples;

  bool     resid_auto;
  uint16_t resid_max_iterations;
  uint64_t resid_min_amplitude;

  bool     small_simplify;
  uint64_t small_max_amplitude;

  bool     step_enable;
  FitParam step_amplitude;

  bool     tail_enable;
  FitParam tail_amplitude;
  FitParam tail_slope;

  double   lateral_slack;

  bool     width_common;
  FitParam width_common_bounds;
  FitParam width_variable_bounds;
  bool     width_at_511_variable;
  double   width_at_511_tolerance;

  bool     Lskew_enable;
  FitParam Lskew_amplitude;
  FitParam Lskew_slope;

  bool     Rskew_enable;
  FitParam Rskew_amplitude;
  FitParam Rskew_slope;

  uint16_t fitter_max_iter;

  //specific to spectrum
  Gamma::Calibration cali_nrg_, cali_fwhm_;
  uint16_t bits_;

  FitSettings();
};

#endif
