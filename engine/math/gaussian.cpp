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

#include "TF1.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TH1.h"

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


void Gaussian::set_chi2(double c2)
{
  chi2_ = c2;
}


void Gaussian::fit(const std::vector<double> &x, const std::vector<double> &y)
{
  fit_root(x, y);
}

std::vector<Gaussian> Gaussian::fit_multi(const std::vector<double> &x,
                                          const std::vector<double> &y,
                                          std::vector<Gaussian> old,
                                          Polynomial &background,
                                          FitSettings settings)
{
  bool use_w_common = (settings.width_common &&
                       settings.cali_fwhm_.valid() &&
                       settings.cali_nrg_.valid());

  if (use_w_common)
    return fit_multi_root_commonw(x,y, old, background, settings);
  else
    return fit_multi_root(x,y, old, background, settings);
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


std::string Gaussian::root_definition(uint16_t start)
{
  return root_definition(start, start+1, start+2);
}

std::string Gaussian::root_definition(uint16_t a, uint16_t c, uint16_t w)
{
  return "[" + std::to_string(a) + "]"
                                   "*TMath::Exp(-((x-[" + std::to_string(c) + "])/[" + std::to_string(w) + "])^2)";
}

void Gaussian::set_params(TF1* f, uint16_t start) const
{
  set_params(f, start, start+1, start+2);
}

void Gaussian::set_params(TF1* f, uint16_t a, uint16_t c, uint16_t w) const
{
  height_.set(f, a);
  center_.set(f, c);
  hwhm_.set(f, w);
}

void Gaussian::get_params(TF1* f, uint16_t start)
{
  get_params(f, start, start+1, start+2);
}

void Gaussian::get_params(TF1* f, uint16_t a, uint16_t c, uint16_t w)
{
  height_.get(f, a);
  center_.get(f, c);
  hwhm_.get(f, w);
}

void Gaussian::fit_root(const std::vector<double> &x, const std::vector<double> &y)
{
  if ((x.size() < 1) || (x.size() != y.size()))
    return;

  TH1D* h1 = new TH1D("h1", "h1", x.size(), x.front(), x.back());
  int i=1;
  for (const auto &h : y)
  {
    h1->SetBinContent(i, h);
    h1->SetBinError(i, sqrt(h));
    i++;
  }

  TF1* f1 = new TF1("f1", this->root_definition().c_str());

  center_.preset_bounds(x.front(), x.back());
  hwhm_.preset_bounds(0, x.back() - x.front());
  height_.preset_bounds(0, h1->GetMaximum() - h1->GetMinimum());

  set_params(f1);

  h1->Fit("f1", "QEMN");
  //  h1->Fit("f1", "EMN");

  get_params(f1);
  chi2_ = f1->GetChisquare();

  f1->Delete();
  h1->Delete();
}

std::vector<Gaussian> Gaussian::fit_multi_root(const std::vector<double> &x,
                                               const std::vector<double> &y,
                                               std::vector<Gaussian> old,
                                               Polynomial &background,
                                               FitSettings settings)
{
  if (old.empty())
    return old;

  if ((x.size() < 1) || (x.size() != y.size()))
    return old;

  TH1D* h1 = new TH1D("h1", "h1", x.size(), x.front(), x.back());
  int i=1;
  for (const auto &h : y)
  {
    h1->SetBinContent(i, h);
    h1->SetBinError(i, sqrt(h));
    i++;
  }

  std::string definition = background.root_definition();
  uint16_t backgroundparams = background.coeff_count();
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 3;
    definition += "+" + Gaussian().root_definition(num);
  }

//  DBG << "Definition = " << definition;

  TF1* f1 = new TF1("f1", definition.c_str());

  for (auto &o : old)
  {
    double width_expected = o.hwhm_.value().value();
    if (settings.cali_fwhm_.valid() && settings.cali_nrg_.valid())
      width_expected = settings.bin_to_width(o.center_.value().value()) / (2* sqrt(log(2)));

    o.hwhm_.set(width_expected * settings.width_common_bounds.lower(),
                width_expected * settings.width_common_bounds.upper(),
                o.hwhm_.value().value());

    o.height_.set(o.height_.value().value() * 1e-5,
                  o.height_.value().value() * 1e5,
                  o.height_.value().value());

    double lateral_slack = settings.lateral_slack * o.hwhm_.value().value() * 2 * sqrt(log(2));
    o.center_.set(o.center_.value().value() - lateral_slack,
                  o.center_.value().value() + lateral_slack,
                  o.center_.value().value());
  }


  background.set_params(f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 3;
    old[i].set_params(f1, num);
  }

  h1->Fit("f1", "QEMN");
  //  h1->Fit("f1", "EMN");

  background.get_params(f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 3;
    old[i].get_params(f1, num);
    old[i].chi2_ = f1->GetChisquare();
  }
  background.set_chi2(f1->GetChisquare());

  f1->Delete();
  h1->Delete();

  return old;
}

std::vector<Gaussian> Gaussian::fit_multi_root_commonw(const std::vector<double> &x,
                                                       const std::vector<double> &y,
                                                       std::vector<Gaussian> old,
                                                       Polynomial &background,
                                                       FitSettings settings)
{
  if (old.empty())
    return old;

  if ((x.size() < 1) || (x.size() != y.size()))
    return old;

  TH1D* h1 = new TH1D("h1", "h1", x.size(), x.front(), x.back());
  int i=1;
  for (const auto &h : y)
  {
    h1->SetBinContent(i, h);
    h1->SetBinError(i, sqrt(h));
    i++;
  }

  double centers_avg {0};
  for (auto &p : old)
    centers_avg += p.center_.value().value();
  centers_avg /= old.size();

  double width_expected = settings.bin_to_width(centers_avg) / (2 * sqrt(log(2)));
  FitParam w_common("hwhm", 0);
  w_common.preset_bounds(width_expected * settings.width_common_bounds.lower(),
                         width_expected * settings.width_common_bounds.upper());

  std::string definition = background.root_definition();
  uint16_t backgroundparams = background.coeff_count();
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + backgroundparams + i * 2;
    definition += "+" + Gaussian().root_definition(num, num+1, backgroundparams);
  }

//  DBG << "Definition = " << definition;

  TF1* f1 = new TF1("f1", definition.c_str());

  for (auto &o : old)
  {
    o.hwhm_ = w_common;

    o.height_.set(o.height_.value().value() * 1e-5,
                  o.height_.value().value() * 1e5,
                  o.height_.value().value());

    double lateral_slack = settings.lateral_slack * o.hwhm_.value().value() * 2 * sqrt(log(2));
    o.center_.set(o.center_.value().value() - lateral_slack,
                  o.center_.value().value() + lateral_slack,
                  o.center_.value().value());
  }

  background.set_params(f1, 0);
  w_common.set(f1, backgroundparams);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + backgroundparams + i * 2;
    old[i].set_params(f1, num, num+1, backgroundparams);
  }

  h1->Fit("f1", "QEMN");
  //  h1->Fit("f1", "EMN");

  background.get_params(f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + backgroundparams + i * 2;
    old[i].get_params(f1, num, num+1, backgroundparams);
    old[i].chi2_ = f1->GetChisquare();
  }
  background.set_chi2(f1->GetChisquare());

  f1->Delete();
  h1->Delete();

  return old;
}

