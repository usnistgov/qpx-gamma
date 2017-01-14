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

#include "fit_settings.h"

FitSettings::FitSettings()
  : overriden (false)
  , bits_ (0)

  , finder_cutoff_kev(100)

  , KON_width (4.0)
  , KON_sigma_spectrum (3.0)
  , KON_sigma_resid (3.0)

  , ROI_max_peaks (10)
  , ROI_extend_peaks (3.5)
  , ROI_extend_background (0.6)

  , background_edge_samples(7)
  , sum4_only(false)

  , resid_auto (true)
  , resid_max_iterations (5)
  , resid_min_amplitude  (5.0)
  , resid_too_close (0.2)

  , small_simplify (true)
  , small_max_amplitude (500)


  , lateral_slack(0.5)

  , width_common (true)
  , width_common_bounds ("w", 1.0, 0.7, 1.3)
  , width_variable_bounds ("w", 1.0, 0.7, 4)
  , width_at_511_variable (true)
  , width_at_511_tolerance (5.0)

  , gaussian_only(false)
  , step_amplitude ("step_h", 1.0e-10, 1.0e-10, 0.75)
  , tail_amplitude ("tail_h", 1.0e-10, 1.0e-10, 0.015)
  , tail_slope ("tail_s", 2.75, 2.5, 50)
  , Lskew_amplitude ("lskew_h", 1.0e-10, 1.0e-10, 0.75)
  , Lskew_slope ("lskew_s", 0.5, 0.3, 2)
  , Rskew_amplitude ("rskew_h", 1.0e-10, 1.0e-10, 0.75)
  , Rskew_slope ("rskew_s", 0.5, 0.3, 2)

  , fitter_max_iter (3000)
{
  step_amplitude.set_enabled(true);
  tail_amplitude.set_enabled(true);
  Lskew_amplitude.set_enabled(true);
  Rskew_amplitude.set_enabled(true);
}

//bool FitSettings::gaussian_only()
//{
//  return
//    !(step_amplitude.enabled
//      || tail_amplitude.enabled
//      || Lskew_amplitude.enabled
//      || Rskew_amplitude.enabled);
//}

void FitSettings::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  pugi::xml_node kon_node = node.append_child("KON");
  kon_node.append_attribute("width").set_value(std::to_string(KON_width).c_str());
  kon_node.append_attribute("sigma_spectrum").set_value(std::to_string(KON_sigma_spectrum).c_str());
  kon_node.append_attribute("sigma_resid").set_value(std::to_string(KON_sigma_resid).c_str());

  pugi::xml_node roi_node = node.append_child("ROI");
  roi_node.append_attribute("max_peaks").set_value(std::to_string(ROI_max_peaks).c_str());
  roi_node.append_attribute("extend_peaks").set_value(std::to_string(ROI_extend_peaks).c_str());
  roi_node.append_attribute("extend_background").set_value(std::to_string(ROI_extend_background).c_str());
  roi_node.append_attribute("edge_samples").set_value(std::to_string(background_edge_samples).c_str());
  roi_node.append_attribute("sum4_only").set_value(sum4_only);

  pugi::xml_node resid_node = node.append_child("Residuals");
  resid_node.append_attribute("auto").set_value(resid_auto);
  resid_node.append_attribute("max_iterations").set_value(std::to_string(resid_max_iterations).c_str());
  resid_node.append_attribute("min_amplitude").set_value(std::to_string(resid_min_amplitude).c_str());
  resid_node.append_attribute("too_close").set_value(std::to_string(resid_too_close).c_str());

  pugi::xml_node small_node = node.append_child("SmallPeaks");
  small_node.append_attribute("enabled").set_value(small_simplify);
  small_node.append_attribute("max_amplitude").set_value(std::to_string(small_max_amplitude).c_str());

  pugi::xml_node width_node = node.append_child("WidthOptions");
  width_node.append_attribute("use_common").set_value(width_common);
  width_node.append_attribute("exception511").set_value(width_at_511_variable);
  width_node.append_attribute("tolerance511").set_value(std::to_string(width_at_511_tolerance).c_str());
  width_common_bounds.to_xml(width_node);

  pugi::xml_node hyp_node = node.append_child("Hypermet");
  hyp_node.append_attribute("gaussian_only").set_value(gaussian_only);
  hyp_node.append_attribute("lateral_slack").set_value(std::to_string(lateral_slack).c_str());
  hyp_node.append_attribute("fitter_max_iterations").set_value(std::to_string(fitter_max_iter).c_str());
  width_variable_bounds.to_xml(hyp_node);
  step_amplitude.to_xml(hyp_node);
  tail_amplitude.to_xml(hyp_node);
  tail_slope.to_xml(hyp_node);
  Lskew_amplitude.to_xml(hyp_node);
  Lskew_slope.to_xml(hyp_node);
  Rskew_amplitude.to_xml(hyp_node);
  Rskew_slope.to_xml(hyp_node);
}

void FitSettings::from_xml(const pugi::xml_node &node) {
  if (node.child("KON")) {
    KON_width = node.child("KON").attribute("width").as_uint();
    KON_sigma_spectrum = node.child("KON").attribute("sigma_spectrum").as_double();
    KON_sigma_resid = node.child("KON").attribute("sigma_resid").as_double();
  }

  if (node.child("ROI")) {
    ROI_max_peaks = node.child("ROI").attribute("max_peaks").as_uint();
    ROI_extend_peaks = node.child("ROI").attribute("extend_peaks").as_double();
    ROI_extend_background = node.child("ROI").attribute("extend_background").as_double();
    background_edge_samples = node.child("ROI").attribute("edge_samples").as_uint();
    sum4_only = node.child("ROI").attribute("sum4_only").as_bool();
  }

  if (node.child("Residuals")) {
    resid_auto = node.child("Residuals").attribute("auto").as_bool();
    resid_max_iterations = node.child("Residuals").attribute("max_iterations").as_uint();
    resid_min_amplitude = node.child("Residuals").attribute("min_amplitude").as_ullong();
    resid_too_close = node.child("Residuals").attribute("too_close").as_double();
  }

  if (node.child("SmallPeaks")) {
    small_simplify = node.child("SmallPeaks").attribute("enabled").as_bool();
    small_max_amplitude = node.child("SmallPeaks").attribute("max_amplitude").as_ullong();
  }

  if (node.child("WidthOptions")) {
    width_common = node.child("WidthOptions").attribute("use_common").as_bool();
    width_at_511_variable = node.child("WidthOptions").attribute("exception511").as_bool();
    width_at_511_tolerance = node.child("WidthOptions").attribute("tolerance511").as_double();
    if (node.child("WidthOptions").child("FitParam"))
      width_common_bounds.from_xml(node.child("WidthOptions").child("FitParam"));
  }

  if (node.child("Hypermet")) {
    gaussian_only = node.child("Hypermet").attribute("gaussian_only").as_bool();
    lateral_slack = node.child("Hypermet").attribute("lateral_slack").as_double();
    fitter_max_iter = node.child("Hypermet").attribute("fitter_max_iterations").as_uint();
    for (auto &q : node.child("Hypermet").children()) {
      if (std::string(q.name()) == width_variable_bounds.xml_element_name())
      {
        FitParam param;
        param.from_xml(q);
        if (param.name() == width_variable_bounds.name())
          width_variable_bounds = param;
        else if (param.name() == step_amplitude.name())
          step_amplitude = param;
        else if (param.name() == tail_amplitude.name())
          tail_amplitude = param;
        else if (param.name() == tail_slope.name())
          tail_slope = param;
        else if (param.name() == Lskew_amplitude.name())
          Lskew_amplitude = param;
        else if (param.name() == Lskew_slope.name())
          Lskew_slope = param;
        else if (param.name() == Rskew_amplitude.name())
          Rskew_amplitude = param;
        else if (param.name() == Rskew_slope.name())
          Rskew_slope = param;
      }
    }
  }

}

void FitSettings::clear()
{
  cali_nrg_ = Qpx::Calibration();
  cali_fwhm_ = Qpx::Calibration();
  bits_ = 0;
  live_time = boost::posix_time::not_a_date_time;
}
void FitSettings::clone(FitSettings other)
{
  other.cali_nrg_ = cali_nrg_;
  other.cali_fwhm_ = cali_fwhm_;
  other.bits_ = bits_;
  other.live_time = live_time;
  (*this) = other;
}

double FitSettings::nrg_to_bin(double energy) const
{
  return cali_nrg_.inverse_transform(energy, bits_);
}

double FitSettings::bin_to_nrg(double bin) const
{
  return cali_nrg_.transform(bin, bits_);
}

double FitSettings::bin_to_width(double bin) const
{
  double nrg = bin_to_nrg(bin);
  double fwhm = nrg_to_fwhm(nrg);
  return (nrg_to_bin(nrg + fwhm/2.0) - nrg_to_bin(nrg - fwhm/2.0));
}

double FitSettings::nrg_to_fwhm(double energy) const
{
  if (cali_fwhm_.valid())
    return cali_fwhm_.transform(energy);
  else
    return 1;
}


