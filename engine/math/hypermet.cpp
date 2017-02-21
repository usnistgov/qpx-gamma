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
 *      Hypermet function ensemble
 *
 ******************************************************************************/

#include "hypermet.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <boost/lexical_cast.hpp>

#include "custom_logger.h"
#include "qpx_util.h"

Hypermet::Hypermet(Gaussian gauss, FitSettings settings)
  : height_("h", gauss.height().value().value())
  , center_("c", gauss.center().value().value())
  , width_("w", gauss.hwhm().value().value() / sqrt(log(2)))
  , Lskew_amplitude_(settings.Lskew_amplitude), Lskew_slope_(settings.Lskew_slope)
  , Rskew_amplitude_(settings.Rskew_amplitude), Rskew_slope_(settings.Rskew_slope)
  , tail_amplitude_(settings.tail_amplitude), tail_slope_(settings.tail_slope)
  , step_amplitude_(settings.step_amplitude)
{
  if (settings.gaussian_only)
  {
    Lskew_amplitude_.set_enabled(false);
    Rskew_amplitude_.set_enabled(false);
    tail_amplitude_.set_enabled(false);
    step_amplitude_.set_enabled(false);
  }
}

void Hypermet::set_chi2(double c2)
{
  chi2_ = c2;
}

void Hypermet::set_center(const FitParam &ncenter)
{
  center_.set(ncenter);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_height(const FitParam &nheight)
{
  height_.set(nheight);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_width(const FitParam &nwidth)
{
  width_.set(nwidth);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_Lskew_amplitude(const FitParam &nLskew_amplitude)
{
  Lskew_amplitude_.set(nLskew_amplitude);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_Lskew_slope(const FitParam &nLskew_slope)
{
  Lskew_slope_.set(nLskew_slope);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_Rskew_amplitude(const FitParam &nRskew_amplitude)
{
  Rskew_amplitude_.set(nRskew_amplitude);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_Rskew_slope(const FitParam &nRskew_slope)
{
  Rskew_slope_.set(nRskew_slope);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_tail_amplitude(const FitParam &ntail_amplitude)
{
  tail_amplitude_.set(ntail_amplitude);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_tail_slope(const FitParam &ntail_slope)
{
  tail_slope_.set(ntail_slope);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_step_amplitude(const FitParam &nstep_amplitude)
{
  step_amplitude_.set(nstep_amplitude);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_center(const UncertainDouble &ncenter)
{
  center_.set_value(ncenter);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_height(const UncertainDouble &nheight)
{
  height_.set_value(nheight);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_width(const UncertainDouble &nwidth)
{
  width_.set_value(nwidth);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_Lskew_amplitude(const UncertainDouble &nLskew_amplitude)
{
  Lskew_amplitude_.set_value(nLskew_amplitude);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_Lskew_slope(const UncertainDouble &nLskew_slope)
{
  Lskew_slope_.set_value(nLskew_slope);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_Rskew_amplitude(const UncertainDouble &nRskew_amplitude)
{
  Rskew_amplitude_.set_value(nRskew_amplitude);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_Rskew_slope(const UncertainDouble &nRskew_slope)
{
  Rskew_slope_.set_value(nRskew_slope);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_tail_amplitude(const UncertainDouble &ntail_amplitude)
{
  tail_amplitude_.set_value(ntail_amplitude);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_tail_slope(const UncertainDouble &ntail_slope)
{
  tail_slope_.set_value(ntail_slope);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::set_step_amplitude(const UncertainDouble &nstep_amplitude)
{
  step_amplitude_.set_value(nstep_amplitude);
  user_modified_ = true;
  chi2_ = 0;
}

void Hypermet::constrain_center(double min, double max)
{
  center_.constrain(min, max);
  user_modified_ = true;
}

void Hypermet::constrain_height(double min, double max)
{
  height_.constrain(min, max);
  user_modified_ = true;
}

void Hypermet::constrain_width(double min, double max)
{
  width_.constrain(min, max);
  user_modified_ = true;
}



std::string Hypermet::to_string() const
{
  std::string ret = "Hypermet ";
  ret += "   area=" + area().to_string()
      + "   rsq=" + boost::lexical_cast<std::string>(chi2_) + "    where:\n";

  ret += "     " + center_.to_string() + "\n";
  ret += "     " + height_.to_string() + "\n";
  ret += "     " + width_.to_string() + "\n";
  ret += "     " + Lskew_amplitude_.to_string() + "\n";
  ret += "     " + Lskew_slope_.to_string() + "\n";
  ret += "     " + Rskew_amplitude_.to_string() + "\n";
  ret += "     " + Rskew_slope_.to_string() + "\n";
  ret += "     " + tail_amplitude_.to_string() + "\n";
  ret += "     " + tail_slope_.to_string() + "\n";
  ret += "     " + step_amplitude_.to_string();

  return ret;
}

Gaussian Hypermet::gaussian() const
{
  Gaussian ret;
  ret.set_height(height_);
  ret.set_center(center_);
  auto w = ret.hwhm();
  w.set(width_.lower() * sqrt(log(2)),
        width_.upper() * sqrt(log(2)),
        width_.value().value() * sqrt(log(2)));
  ret.set_hwhm(w);
  ret.set_chi2(chi2_);
  return ret;
}

double Hypermet::eval_peak(double x) const
{
  if (width_.value().value() == 0)
    return 0;

  double xc = x - center_.value().value();

  double gaussian = exp(- pow(xc/width_.value().value(), 2) );

  double left_short = 0;
  if (Lskew_amplitude_.enabled())
  {
    double lexp = exp(pow(0.5*width_.value().value()/Lskew_slope_.value().value(), 2) + xc/Lskew_slope_.value().value());
    if ((Lskew_slope_.value().value() != 0) && !std::isinf(lexp))
      left_short = Lskew_amplitude_.value().value() * lexp * erfc( 0.5*width_.value().value()/Lskew_slope_.value().value() + xc/width_.value().value());
  }

  double right_short = 0;
  if (Rskew_amplitude_.enabled())
  {
    double rexp = exp(pow(0.5*width_.value().value()/Rskew_slope_.value().value(), 2) - xc/Rskew_slope_.value().value());
    if ((Rskew_slope_.value().value() != 0) && !std::isinf(rexp))
      right_short = Rskew_amplitude_.value().value() * rexp * erfc( 0.5*width_.value().value()/Rskew_slope_.value().value()  - xc/width_.value().value());
  }

  double ret = height_.value().value() * (gaussian + 0.5 * (left_short + right_short) );

  return ret;
}

double Hypermet::eval_step_tail(double x) const
{
  if (width_.value().value() == 0)
    return 0;

  double xc = x - center_.value().value();

  double step = 0;
  if (step_amplitude_.enabled())
    step = step_amplitude_.value().value() * erfc( xc/width_.value().value() );

  double tail = 0;
  if (tail_amplitude_.enabled())
  {
    double lexp = exp(pow(0.5*width_.value().value()/tail_slope_.value().value(), 2) + xc/tail_slope_.value().value());
    if ((tail_slope_.value().value() != 0) && !std::isinf(lexp))
      tail = tail_amplitude_.value().value() * lexp * erfc( 0.5*width_.value().value()/tail_slope_.value().value() + xc/width_.value().value());
  }

  return height_.value().value() * 0.5 * (step + tail);
}

std::vector<double> Hypermet::peak(std::vector<double> x) const
{
  std::vector<double> y;
  for (auto &q : x)
    y.push_back(eval_peak(q));
  return y;
}

std::vector<double> Hypermet::step_tail(std::vector<double> x) const
{
  std::vector<double> y;
  for (auto &q : x)
    y.push_back(eval_step_tail(q));
  return y;
}

UncertainDouble Hypermet::area() const
{
  UncertainDouble ret;
  ret = height_.value() * width_.value() * UncertainDouble::from_double(sqrt(M_PI),0) *
      ( UncertainDouble::from_int(1,0) +
       (Lskew_amplitude_.enabled() ? Lskew_amplitude_.value() * width_.value() * Lskew_slope_.value() : UncertainDouble::from_int(0,0) ) +
       (Rskew_amplitude_.enabled() ? Rskew_amplitude_.value() * width_.value() * Rskew_slope_.value() : UncertainDouble::from_int(0,0) )
      );
  ret.setUncertainty(std::numeric_limits<double>::infinity());
  return ret;
}

bool Hypermet::gaussian_only() const
{
  return
      !(step_amplitude_.enabled()
        || tail_amplitude_.enabled()
        || Lskew_amplitude_.enabled()
        || Rskew_amplitude_.enabled());
}

void Hypermet::to_xml(pugi::xml_node &root) const
{
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_attribute("rsq").set_value(to_max_precision(chi2_).c_str());
  center_.to_xml(node);
  height_.to_xml(node);
  width_.to_xml(node);
  Lskew_amplitude_.to_xml(node);
  Lskew_slope_.to_xml(node);
  Rskew_amplitude_.to_xml(node);
  Rskew_slope_.to_xml(node);
  tail_amplitude_.to_xml(node);
  tail_slope_.to_xml(node);
  step_amplitude_.to_xml(node);
}


void Hypermet::from_xml(const pugi::xml_node &node)
{
  chi2_ = node.attribute("rsq").as_double(0);
  for (auto &q : node.children()) {
    FitParam param;
    if (std::string(q.name()) == param.xml_element_name()) {
      param.from_xml(q);
      if (param.name() == center_.name())
        center_ = param;
      else if (param.name() == height_.name())
        height_ = param;
      else if (param.name() == width_.name())
        width_ = param;
      else if (param.name() == step_amplitude_.name())
        step_amplitude_ = param;
      else if (param.name() == tail_amplitude_.name())
        tail_amplitude_ = param;
      else if (param.name() == tail_slope_.name())
        tail_slope_ = param;
      else if (param.name() == Lskew_amplitude_.name())
        Lskew_amplitude_ = param;
      else if (param.name() == Lskew_slope_.name())
        Lskew_slope_ = param;
      else if (param.name() == Rskew_amplitude_.name())
        Rskew_amplitude_ = param;
      else if (param.name() == Rskew_slope_.name())
        Rskew_slope_ = param;
    }
  }
}

