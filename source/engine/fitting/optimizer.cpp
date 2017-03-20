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
 *      Optimizer
 *
 ******************************************************************************/

#include "optimizer.h"
#include "TH1.h"


namespace Qpx {

std::string Optimizer::def(uint16_t num, const FitParam& param)
{
  if (!param.fixed())
    return "[" + std::to_string(num) + "]";
  else if (param.value().finite())
    return std::to_string(param.value().value());
  else
    return "0";
}

void Optimizer::set(TF1* f, uint16_t num, const FitParam& param)
{
  f->SetParameter(num, param.value().value());
  f->SetParLimits(num, param.lower(), param.upper());
}

UncertainDouble Optimizer::get(TF1* f, uint16_t num)
{
  return UncertainDouble::from_double(f->GetParameter(num), f->GetParError(num));
}

uint16_t Optimizer::set_params(const CoefFunction &func, TF1* f, uint16_t start)
{
  for (const auto &p : func.get_coeffs())
    if (!p.second.fixed())
    {
      set(f, start, p.second);
      start++;
    }
  return start;
}

uint16_t Optimizer::get_params(CoefFunction &func, TF1* f, uint16_t start)
{
  for (auto p : func.get_coeffs())
    if (!p.second.fixed())
    {
      func.set_coeff(p.first, get(f, start));
      start++;
    }
  return start;
}

std::string Optimizer::CoefFunctionVisitor::define_chain(const CoefFunction& func,
                                                         const std::string &element) const
{
  std::string definition;
  int i = 0;
  for (auto &c : func.get_coeffs())
  {
    if (i > 0)
      definition += "+";
    if (!c.second.fixed())
    {
      definition += def(i, c.second);
      i++;
    }
    else
      definition += std::to_string(c.second.value().value());
    if (c.first > 0)
      definition += "*" + element;
    if (c.first > 1)
      definition += "^" + std::to_string(c.first);
  }
  return definition;
}

std::string Optimizer::def(const Gaussian& gaussian, uint16_t start)
{
  return def(gaussian, start, start+1, start+2);
}

std::string Optimizer::def(const Gaussian& gaussian, uint16_t a, uint16_t c, uint16_t w)
{
  return def(a, gaussian.height()) +
      "*TMath::Exp(-((x-" + def(c, gaussian.center()) + ")/" + def(w, gaussian.hwhm()) + ")^2)";
}

void Optimizer::set_params(const Gaussian& gaussian, TF1* f, uint16_t start)
{
  set_params(gaussian, f, start, start+1, start+2);
}

void Optimizer::set_params(const Gaussian& gaussian, TF1* f, uint16_t a, uint16_t c, uint16_t w)
{
  set(f, a, gaussian.height());
  set(f, c, gaussian.center());
  set(f, w, gaussian.hwhm());
}

void Optimizer::get_params(Gaussian& gaussian, TF1* f, uint16_t start)
{
  get_params(gaussian, f, start, start+1, start+2);
}

void Optimizer::get_params(Gaussian& gaussian, TF1* f, uint16_t a, uint16_t c, uint16_t w)
{
  gaussian.set_height(get(f, a));
  gaussian.set_center(get(f, c));
  gaussian.set_hwhm(get(f, w));
}

void Optimizer::initial_sanity(Gaussian& gaussian,
                               double xmin, double xmax,
                               double ymin, double ymax)
{
  gaussian.bound_center(xmin, xmax);
  gaussian.bound_height(0, ymax-ymin);
  gaussian.bound_hwhm(0, xmax-xmin);
}

void Optimizer::sanity_check(Gaussian& gaussian,
                             double xmin, double xmax,
                             double ymin, double ymax)
{
  gaussian.constrain_center(xmin, xmax);
  gaussian.constrain_height(0, ymax-ymin);
  gaussian.constrain_hwhm(0, xmax-xmin);
}

TH1D* Optimizer::fill_hist(const std::vector<double> &x,
                           const std::vector<double> &y)
{
  if ((x.size() < 1) || (x.size() != y.size()))
    return nullptr;

  TH1D* h1 = new TH1D("h1", "h1", x.size(), x.front(), x.back());
  int i=1;
  for (const auto &h : y)
  {
    h1->SetBinContent(i, h);
    h1->SetBinError(i, sqrt(h));
    i++;
  }
  return h1;
}


void Optimizer::fit(Gaussian& gaussian,
                    const std::vector<double> &x,
                    const std::vector<double> &y)
{
  TH1D* h1 = fill_hist(x, y);
  if (!h1)
    return;

  initial_sanity(gaussian, x.front(), x.back(),
                 h1->GetMinimum(), h1->GetMaximum());

  TF1* f1 = new TF1("f1", def(gaussian).c_str());

  set_params(gaussian, f1);

  h1->Fit("f1", "QEMN");

  get_params(gaussian, f1);
  gaussian.set_chi2(f1->GetChisquare());

  f1->Delete();
  h1->Delete();
}

std::vector<Gaussian> Optimizer::fit_multiplet(const std::vector<double> &x,
                                               const std::vector<double> &y,
                                               std::vector<Gaussian> old,
                                               Polynomial &background,
                                               FitSettings settings)
{
  bool use_w_common = (settings.width_common &&
                       settings.cali_fwhm_.valid() &&
                       settings.cali_nrg_.valid());

  if (use_w_common)
    return fit_multi_commonw(x,y, old, background, settings);
  else
    return fit_multi_variw(x,y, old, background, settings);
}

std::vector<Hypermet> Optimizer::fit_multiplet(const std::vector<double> &x,
                                               const std::vector<double> &y,
                                               std::vector<Hypermet> old,
                                               Polynomial &background,
                                               FitSettings settings)
{
  bool use_w_common = (settings.width_common &&
                       settings.cali_fwhm_.valid() &&
                       settings.cali_nrg_.valid());

  if (use_w_common)
    return fit_multi_commonw(x,y, old, background, settings);
  else
    return fit_multi_variw(x,y, old, background, settings);
}


void Optimizer::constrain_center(Gaussian &gaussian, double slack)
{
  double lateral_slack = slack * gaussian.hwhm().value().value() * 2 * sqrt(log(2));
  gaussian.constrain_center(gaussian.center().value().value() - lateral_slack,
                            gaussian.center().value().value() + lateral_slack);
}

std::vector<Gaussian> Optimizer::fit_multi_variw(const std::vector<double> &x,
                                                 const std::vector<double> &y,
                                                 std::vector<Gaussian> old,
                                                 Polynomial &background,
                                                 FitSettings settings)
{
  if (old.empty())
    return old;

  TH1D* h1 = fill_hist(x, y);
  if (!h1)
    return old;

  for (auto &gaussian : old)
  {
    sanity_check(gaussian, x.front(), x.back(), h1->GetMinimum(), h1->GetMaximum());
    constrain_center(gaussian, settings.lateral_slack);

    double width_expected = gaussian.hwhm().value().value();
    if (settings.cali_fwhm_.valid() && settings.cali_nrg_.valid())
      width_expected = settings.bin_to_width(gaussian.center().value().value()) / (2* sqrt(log(2)));

    gaussian.constrain_hwhm(width_expected * settings.width_common_bounds.lower(),
                            width_expected * settings.width_common_bounds.upper());
  }

  std::string definition = def_of(background);
  uint16_t backgroundparams = background.coeff_count(); //valid coef count?
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 3;
    definition += "+" + def(old.at(i), num);
  }

  TF1* f1 = new TF1("f1", definition.c_str());

  set_params(background, f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 3;
    set_params(old[i], f1, num);
  }

  h1->Fit("f1", "QEMN");

  get_params(background, f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 3;
    get_params(old[i], f1, num);
    old[i].set_chi2(f1->GetChisquare());
  }
  background.set_chi2(f1->GetChisquare());

  f1->Delete();
  h1->Delete();

  return old;
}

std::vector<Gaussian> Optimizer::fit_multi_commonw(const std::vector<double> &x,
                                                   const std::vector<double> &y,
                                                   std::vector<Gaussian> old,
                                                   Polynomial &background,
                                                   FitSettings settings)
{
  if (old.empty())
    return old;

  TH1D* h1 = fill_hist(x, y);
  if (!h1)
    return old;

  FitParam w_common("hwhm", 0);
  double width_expected = settings.bin_to_width((x.front() + x.back())/2) / (2 * sqrt(log(2)));
  w_common.preset_bounds(width_expected * settings.width_common_bounds.lower(),
                         width_expected * settings.width_common_bounds.upper());

  for (auto &gaussian : old)
  {
    gaussian.set_hwhm(w_common);
    sanity_check(gaussian, x.front(), x.back(), h1->GetMinimum(), h1->GetMaximum());
    constrain_center(gaussian, settings.lateral_slack);
  }

  std::string definition = def_of(background);
  uint16_t w_index = background.coeff_count(); //valid coef count?
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + w_index + i * 2;
    definition += "+" + def(old.at(i), num, num+1, w_index);
  }

  TF1* f1 = new TF1("f1", definition.c_str());

  set_params(background, f1, 0);
  set(f1, w_index, w_common);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + w_index + i * 2;
    set_params(old[i], f1, num, num+1, w_index);
  }

  h1->Fit("f1", "QEMN");

  get_params(background, f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + w_index + i * 2;
    get_params(old[i], f1, num, num+1, w_index);
    old[i].set_chi2(f1->GetChisquare());
  }
  background.set_chi2(f1->GetChisquare());

  f1->Delete();
  h1->Delete();

  return old;
}

std::string Optimizer::def(const Hypermet& h, uint16_t start)
{
  return def(h, start, start+1);
}

std::string Optimizer::def_skew(const FitParam& ampl, const FitParam& slope,
                                const std::string &w, const std::string &xc,
                                const std::string &xcw, uint16_t idx)
{
  if (!ampl.enabled())
    return "";
  std::string skewh = def(idx, ampl);
  std::string skews = "/" + def(idx+1, slope);
  return skewh + "*TMath::Exp((0.5*"+ w+skews +")^2 + ("+ xc+skews +"))*TMath::Erfc((0.5*"+ w+skews +") + "+ xcw +")";
}


std::string Optimizer::def(const Hypermet& hyp, uint16_t width, uint16_t i)
{
  std::string h = def(i, hyp.height());
  std::string w = def(width, hyp.width());
  std::string xc = "(x-" + def(i+1, hyp.center()) + ")";
  std::string xcw = xc + "/" + w;
  std::string lskew = "0";
  std::string rskew = "0";
  std::string tail = "0";
  std::string step = "0";

  std::string lskewh = def(i+2, hyp.Lskew_amplitude());
  std::string lskews = "/" + def(i+3, hyp.Lskew_slope());
  lskew = lskewh + "*TMath::Exp((0.5*"+ w+lskews +")^2 + ("+ xc+lskews +"))*TMath::Erfc((0.5*"+ w+lskews +") + "+ xcw +")";

  std::string rskewh = def(i+4, hyp.Rskew_amplitude());
  std::string rskews = "/" + def(i+5, hyp.Rskew_slope());
  rskew = rskewh + "*TMath::Exp((0.5*"+ w+rskews +")^2 - ("+ xc+rskews +"))*TMath::Erfc((0.5*"+ w+rskews +") - "+ xcw +")";

  std::string tailh = def(i+6, hyp.tail_amplitude());
  std::string tails = "/" + def(i+7, hyp.tail_slope());
  tail = tailh  + "*TMath::Exp((0.5*"+ w+tails  +")^2 + ("+ xc+tails  +"))*TMath::Erfc((0.5*"+ w+tails  +") + "+ xcw +")";

  std::string steph = def(i+8, hyp.step_amplitude());
  step = steph  + "*TMath::Erfc(" + xc + "/" + w + ")";

  return h + "*( TMath::Exp(-(" + xcw + ")^2) + 0.5 * (" + lskew +  " + " + rskew + " + " + tail  + " + " + step  + " ) )";
}

void Optimizer::set_params(const Hypermet& hyp, TF1* f, uint16_t start)
{
  set_params(hyp, f, start, start+1);
}

void Optimizer::set_params(const Hypermet& hyp, TF1* f, uint16_t width, uint16_t others_start)
{
  set(f, width, hyp.width());
  set(f, others_start, hyp.height());
  set(f, others_start+1, hyp.center());
  set(f, others_start+2, hyp.Lskew_amplitude());
  set(f, others_start+3, hyp.Lskew_slope());
  set(f, others_start+4, hyp.Rskew_amplitude());
  set(f, others_start+5, hyp.Rskew_slope());
  set(f, others_start+6, hyp.tail_amplitude());
  set(f, others_start+7, hyp.tail_slope());
  set(f, others_start+8, hyp.step_amplitude());
}

void Optimizer::get_params(Hypermet& hyp, TF1* f, uint16_t start)
{
  get_params(hyp, f, start, start+1);
}

void Optimizer::get_params(Hypermet& hyp, TF1* f, uint16_t w, uint16_t others_start)
{
  hyp.set_width(get(f, w));
  hyp.set_height(get(f, others_start));
  hyp.set_center(get(f, others_start+1));
  hyp.set_Lskew_amplitude(get(f, others_start+2));
  hyp.set_Lskew_slope(get(f, others_start+3));
  hyp.set_Rskew_amplitude(get(f, others_start+4));
  hyp.set_Rskew_slope(get(f, others_start+5));
  hyp.set_tail_amplitude(get(f, others_start+6));
  hyp.set_tail_slope(get(f, others_start+7));
  hyp.set_step_amplitude(get(f, others_start+8));
}

void Optimizer::sanity_check(Hypermet& hyp,
                             double xmin, double xmax,
                             double ymin, double ymax)
{
  hyp.constrain_height(0, ymax-ymin);
  hyp.constrain_center(xmin, xmax);
  hyp.constrain_width(0, xmax-xmin);
}

void Optimizer::constrain_center(Hypermet &hyp, double slack)
{
  double lateral_slack = slack * hyp.width().value().value() * 2 * sqrt(log(2));
  hyp.constrain_center(hyp.center().value().value() - lateral_slack,
                       hyp.center().value().value() + lateral_slack);
}

std::vector<Hypermet> Optimizer::fit_multi_variw(const std::vector<double> &x,
                                                 const std::vector<double> &y,
                                                 std::vector<Hypermet> old,
                                                 Polynomial &background,
                                                 FitSettings settings)
{
  if (old.empty())
    return old;

  TH1D* h1 = fill_hist(x, y);
  if (!h1)
    return old;

  for (auto &hyp : old)
  {
    sanity_check(hyp, x.front(), x.back(), h1->GetMinimum(), h1->GetMaximum());
    constrain_center(hyp, settings.lateral_slack);

    double width_expected = hyp.width().value().value();
    if (settings.cali_fwhm_.valid() && settings.cali_nrg_.valid())
      width_expected = settings.bin_to_width(hyp.center().value().value()) / (2* sqrt(log(2)));

    hyp.constrain_width(width_expected * settings.width_common_bounds.lower(),
                        width_expected * settings.width_common_bounds.upper());
  }

  std::string definition = def_of(background);
  uint16_t backgroundparams = background.coeff_count(); //valid coef count?
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 10;
    definition += "+" + def(old.at(i), num);
  }

  TF1* f1 = new TF1("f1", definition.c_str());

  set_params(background, f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 10;
    set_params(old[i], f1, num);
  }

  h1->Fit("f1", "QEMN");

  get_params(background, f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 10;
    get_params(old[i], f1, num);
    old[i].set_chi2(f1->GetChisquare());
  }
  background.set_chi2(f1->GetChisquare());

  f1->Delete();
  h1->Delete();

  return old;
}

std::vector<Hypermet> Optimizer::fit_multi_commonw(const std::vector<double> &x,
                                                   const std::vector<double> &y,
                                                   std::vector<Hypermet> old,
                                                   Polynomial &background,
                                                   FitSettings settings)
{
  if (old.empty())
    return old;

  TH1D* h1 = fill_hist(x, y);
  if (!h1)
    return old;

  FitParam w_common("w", 0);
  double width_expected = settings.bin_to_width((x.front() + x.back())/2) / (2 * sqrt(log(2)));
  w_common.preset_bounds(width_expected * settings.width_common_bounds.lower(),
                         width_expected * settings.width_common_bounds.upper());

  for (auto &hyp : old)
  {
    hyp.set_width(w_common);
    sanity_check(hyp, x.front(), x.back(), h1->GetMinimum(), h1->GetMaximum());
    constrain_center(hyp, settings.lateral_slack);
  }

  std::string definition = def_of(background);
  uint16_t w_index = background.coeff_count(); //valid coef count?
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + w_index + i * 9;
    definition += "+" + def(old.at(i), w_index, num);
  }

  TF1* f1 = new TF1("f1", definition.c_str());

  set_params(background, f1, 0);
  set(f1, w_index, w_common);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + w_index + i * 9;
    set_params(old[i], f1, w_index, num);
  }

  h1->Fit("f1", "QEMN");

  get_params(background, f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + w_index + i * 9;
    get_params(old[i], f1, w_index, num);
    old[i].set_chi2(f1->GetChisquare());
  }
  background.set_chi2(f1->GetChisquare());

  f1->Delete();
  h1->Delete();

  return old;
}

}
