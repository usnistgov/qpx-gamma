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
 *      Gaussian
 *
 ******************************************************************************/

#include "gaussian.h"

#include <sstream>
#include <iomanip>
#include <numeric>

#include "custom_logger.h"
#include "custom_timer.h"

#include <boost/lexical_cast.hpp>

Gaussian::Gaussian()
{}


void Gaussian::set_center(const FitParam &ncenter)
{
  center_.set(ncenter);
//  chi2_ = 0;
}

void Gaussian::set_height(const FitParam &nheight)
{
  height_.set(nheight);
//  chi2_ = 0;
}

void Gaussian::set_hwhm(const FitParam &nwidth)
{
  hwhm_.set(nwidth);
//  chi2_ = 0;
}

void Gaussian::set_center(const UncertainDouble &ncenter)
{
  center_.set_value(ncenter);
//  chi2_ = 0;
}

void Gaussian::set_height(const UncertainDouble &nheight)
{
  height_.set_value(nheight);
//  chi2_ = 0;
}

void Gaussian::set_hwhm(const UncertainDouble &nwidth)
{
  hwhm_.set_value(nwidth);
//  chi2_ = 0;
}

void Gaussian::constrain_center(double min, double max)
{
  center_.constrain(min, max);
}

void Gaussian::constrain_height(double min, double max)
{
  height_.constrain(min, max);
}

void Gaussian::constrain_hwhm(double min, double max)
{
  hwhm_.constrain(min, max);
}

void Gaussian::bound_center(double min, double max)
{
  center_.preset_bounds(min, max);
}

void Gaussian::bound_height(double min, double max)
{
  height_.preset_bounds(min, max);
}

void Gaussian::bound_hwhm(double min, double max)
{
  hwhm_.preset_bounds(min, max);
}


void Gaussian::set_chi2(double c2)
{
  chi2_ = c2;
}

std::string Gaussian::to_string() const
{
  std::string ret = "gaussian ";
  ret += "   area=" + area().to_string()
      + "   chi2=" + boost::lexical_cast<std::string>(chi2_) + "    where:\n";

  ret += "     " + center_.to_string() + "\n";
  ret += "     " + height_.to_string() + "\n";
  ret += "     " + hwhm_.to_string() + "\n";

  return ret;
}

double Gaussian::evaluate(double x) {
  return height_.value().value() *
      exp(-log(2.0)*(pow(((x-center_.value().value())/hwhm_.value().value()),2)));
}

UncertainDouble Gaussian::area() const
{
  UncertainDouble ret;
  ret = height_.value() * hwhm_.value() *
      UncertainDouble::from_double(sqrt(M_PI / log(2.0)), 0);
  return ret;
}

std::vector<double> Gaussian::evaluate_array(std::vector<double> x) {
  std::vector<double> y;
  for (auto &q : x) {
    double val = evaluate(q);
    if (val < 0)
      y.push_back(0);
    else
      y.push_back(val);
  }
  return y;
}
