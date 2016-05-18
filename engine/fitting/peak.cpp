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

#include "peak.h"
#include "custom_logger.h"

namespace Qpx {

Peak::Peak(const Hypermet &hyp, const SUM4 &s4, const FitSettings &fs)
  : intensity_theoretical_(0.0)
  , efficiency_relative_(0.0)
  , hypermet_(hyp)
  , sum4_(s4)
{
  reconstruct(fs);
}


void Peak::reconstruct(FitSettings fs) {

  if (hypermet_.height().value.finite() && (hypermet_.height().value.value() > 0)) {
    center_ = hypermet_.center().value;
  }
  else
    center_ = sum4_.centroid();

  if (!std::isfinite(center_.uncertainty())) {
    DBG << "<Peak> overriding peak center uncert with sum4 for " << center_.to_string() << " with " << sum4_.centroid().to_string();
    center_.setUncertainty(sum4_.centroid().uncertainty());
  }


  double energyval = fs.cali_nrg_.transform(center_.value(), fs.bits_);
  double emin = fs.cali_nrg_.transform(center_.value() - center_.uncertainty(), fs.bits_);
  double emax = fs.cali_nrg_.transform(center_.value() + center_.uncertainty(), fs.bits_);
  energy_ = UncertainDouble::from_double(energyval, 0.5 * (emax - emin));
  energy_.setSigFigs(center_.sigfigs());

  if (hypermet_.width().value.finite())
  {
    double L = hypermet_.center().value.value() - hypermet_.width().value.value() * sqrt(log(2));
    double R = hypermet_.center().value.value() + hypermet_.width().value.value() * sqrt(log(2));
    double dmax = (hypermet_.width().value.value() + hypermet_.width().value.uncertainty()) * sqrt(log(2));
    double dmin = (hypermet_.width().value.value() - hypermet_.width().value.uncertainty()) * sqrt(log(2));
    double Lmax = hypermet_.center().value.value() - dmax;
    double Rmax = hypermet_.center().value.value() + dmax;
    double Lmin = hypermet_.center().value.value() - dmin;
    double Rmin = hypermet_.center().value.value() + dmin;
    double val = fs.cali_nrg_.transform(R, fs.bits_) - fs.cali_nrg_.transform(L, fs.bits_);
    double max = fs.cali_nrg_.transform(Rmax, fs.bits_) - fs.cali_nrg_.transform(Lmax, fs.bits_);
    double min = fs.cali_nrg_.transform(Rmin, fs.bits_) - fs.cali_nrg_.transform(Lmin, fs.bits_);
    double uncert = (max - min);
    fwhm_ = UncertainDouble::from_double(val, uncert, 1);
  } else {
    double L = sum4_.centroid().value() - 0.5 * sum4_.fwhm().value();
    double R = sum4_.centroid().value() + 0.5 * sum4_.fwhm().value();
    fwhm_ = UncertainDouble::from_double(fs.cali_nrg_.transform(R, fs.bits_) - fs.cali_nrg_.transform(L, fs.bits_),
                           std::numeric_limits<double>::quiet_NaN(),
                           1);
  }

  area_hyp_ =  hypermet_.area();
  area_sum4_ = sum4_.peak_area();

//  DBG << "<Peak> hyparea = " << area_hyp.debug();

  area_best_ = area_sum4_;

  cps_best_ = cps_hyp_ = cps_sum4_ = UncertainDouble::from_double(0,0);

  double live_seconds = fs.live_time.total_milliseconds() * 0.001;

  if (live_seconds > 0) {
    cps_hyp_  = area_hyp_ / live_seconds;
    cps_sum4_ = area_sum4_ / live_seconds;
//    if (hypermet_.height_.value.finite() && (hypermet_.height_.value.value() > 0))
//      cps_best = cps_hyp;
//    else
      cps_best_ = cps_sum4_;
  }
}

bool Peak::good() const
{
  return ((sum4_.quality() == 1)
          && (quality_energy() == 1)
          && (quality_fwhm() == 1));
}

int Peak::quality_energy() const
{
  if (energy_.error() > 50)
    return 3;
  else if (!std::isfinite(energy_.uncertainty()) || !energy_.uncertainty())
    return 2;
  else
    return 1;
}

int Peak::quality_fwhm() const
{
  if (fwhm_.error() > 50)
    return 3;
  else if (!std::isfinite(fwhm_.uncertainty()) || !fwhm_.uncertainty())
    return 2;
  else
    return 1;
}

void Peak::override_energy(double newval)
{
  energy_.setValue(newval);
}

bool Peak::operator<(const Peak &other) const
{
  return (center_ < other.center_);
}

bool Peak::operator==(const Peak &other) const
{
  return (center_ == other.center_);
}

bool Peak::operator!=(const Peak &other) const
{
  return !(center_ == other.center_);
}

bool Peak::operator>(const Peak &other) const
{
  return (center_ > other.center_);
}

void Peak::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  if (sum4_.peak_width()) {
    pugi::xml_node s4 = node.append_child("SUM4");
    s4.append_attribute("left").set_value(sum4_.left());
    s4.append_attribute("right").set_value(sum4_.right());
  }
  if (hypermet_.height().value.value() > 0)
    hypermet_.to_xml(node);
}


void Peak::from_xml(const pugi::xml_node &node) {
  if (node.child("Hypermet"))
    hypermet_.from_xml(node.child("Hypermet"));
  if (node.child("SUM4"))
  {
    double L = node.child("SUM4").attribute("left").as_double();
    double R = node.child("SUM4").attribute("right").as_double();
  }
}

}
