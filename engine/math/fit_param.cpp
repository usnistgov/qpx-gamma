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
 *      FitykUtil -
 *
 ******************************************************************************/

#include "fit_param.h"
#include <boost/lexical_cast.hpp>
#include "custom_logger.h"

const std::string FitParam::def_bounds() {

  std::string ret = "~";
  ret += boost::lexical_cast<std::string>(val) +
         " [" + boost::lexical_cast<std::string>(lbound) +
          ":" + boost::lexical_cast<std::string>(ubound) + "]";
  return ret;
}

double FitParam::get_err(fityk::Fityk* f,
                          std::string funcname)
{
  std::string command = "%" + funcname + "." + name_ + ".error";
//  PL_DBG << "<FitParam> " << command;
  return f->calculate_expr(command);
}

bool FitParam::extract(fityk::Fityk* f, fityk::Func* func)
{
  try {
    val = func->get_param_value(name_);
//    uncert = get_err(f, func->name);
//    PL_DBG << "<FitParam> " << name_ << " = " << val << " +/- " << uncert;
  } catch (...) {
    return false;
  }
  return true;
}

FitSettings::FitSettings()
  : override_ (false)
  , bits_ (0)

  , finder_cutoff_kev(100)

  , KON_width (4.0)
  , KON_sigma_spectrum (3.0)
  , KON_sigma_resid (3.0)

  , ROI_max_peaks (10)
  , ROI_extend_peaks (3.5)
  , ROI_extend_background (0.6)

  , background_edge_samples(5)

  , resid_auto (true)
  , resid_max_iterations (5)
  , resid_min_amplitude  (5.0)

  , small_simplify (true)
  , small_max_amplitude (500)

  , step_enable (true)
  , step_amplitude ("step_h", 1.0e-10, 1.0e-10, 0.75)

  , tail_enable (false)
  , tail_amplitude ("tail_h", 1.0e-10, 1.0e-10, 0.015)
  , tail_slope ("tail_s", 2.75, 2.5, 50)

  , lateral_slack(0.5)

  , width_common (true)
  , width_common_bounds ("w", 1.0, 0.7, 1.3)
  , width_variable_bounds ("w", 1.0, 0.7, 4)
  , width_at_511_variable (true)
  , width_at_511_tolerance (5.0)

  , Lskew_enable (true)
  , Lskew_amplitude ("lskew_h", 1.0e-10, 1.0e-10, 0.75)
  , Lskew_slope ("lskew_s", 0.5, 0.3, 2)

  , Rskew_enable (true)
  , Rskew_amplitude ("rskew_h", 1.0e-10, 1.0e-10, 0.75)
  , Rskew_slope ("rskew_s", 0.5, 0.3, 2)

  , fitter_max_iter (300)
{}
