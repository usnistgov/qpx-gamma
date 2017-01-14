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

#include "fityk_util.h"
#include "custom_logger.h"
#include "custom_timer.h"

#include <boost/lexical_cast.hpp>

#ifdef FITTER_ROOT_ENABLED
#include "TF1.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TH1.h"
#endif

Gaussian::Gaussian()
  : height_("height", 0)
  , center_("center", 0)
  , hwhm_("hwhm", 0)
  , rsq_(-1)
{}

void Gaussian::fit(const std::vector<double> &x, const std::vector<double> &y)
{
#ifdef FITTER_ROOT_ENABLED
  fit_root(x, y);
#else
  fit_fityk(x, y);
#endif
}

std::vector<Gaussian> Gaussian::fit_multi(const std::vector<double> &x,
                                          const std::vector<double> &y,
                                          std::vector<Gaussian> old,
                                          Polynomial &background,
                                          FitSettings settings)
{
#ifdef FITTER_ROOT_ENABLED
  bool use_w_common = (settings.width_common &&
                       settings.cali_fwhm_.valid() &&
                       settings.cali_nrg_.valid());

  if (use_w_common)
    return fit_multi_root_commonw(x,y, old, background, settings);
  else
    return fit_multi_root(x,y, old, background, settings);
#else
  return fit_multi_fityk(x,y, old, background, settings);
#endif
}

std::vector<Gaussian> Gaussian::fit_multi_fityk(const std::vector<double> &x,
                                                const std::vector<double> &y,
                                                std::vector<Gaussian> old,
                                                Polynomial &background,
                                                FitSettings settings)
{
  if (old.empty())
    return old;

  std::vector<double> sigma;
  for (auto &q : y)
    sigma.push_back(sqrt(q));

  bool success = true;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);


  try {
    f->execute("set fitting_method = nlopt_lbfgs");
  } catch ( std::string st ) {
    success = false;
    DBG << "Gaussian multifit failed to set fitter " << st;
  }

  try {
    f->execute(fityk_definition());
    f->execute(background.fityk_definition());
  } catch ( ... ) {
    success = false;
    DBG << "Gaussian multifit failed to define";
  }

  bool use_w_common = (settings.width_common && settings.cali_fwhm_.valid() && settings.cali_nrg_.valid());

  if (use_w_common)
  {
    double centers_avg {0};
    for (auto &p : old)
      centers_avg += p.center_.value.value();
    centers_avg /= old.size();

    double nrg = settings.bin_to_nrg(centers_avg);
    double fwhm_expected = settings.cali_fwhm_.transform(nrg);
    double L = settings.nrg_to_bin(nrg - fwhm_expected/2);
    double R = settings.nrg_to_bin(nrg + fwhm_expected/2);

    FitParam w_common = settings.width_common_bounds;
    w_common.value.setValue((R - L) / (2* sqrt(log(2))));
    w_common.lbound = w_common.value.value() * w_common.lbound;
    w_common.ubound = w_common.value.value() * w_common.ubound;

    try {
      f->execute(w_common.def_var());
    } catch ( ... ) {
      success = false;
      DBG << "Gaussian: multifit failed to define w_common";
    }
  }

  try {
    background.add_self(f);
  } catch ( ... ) {
    success = false;
    DBG << "Gaussian: multifit failed to set up common background";
  }

  bool can_uncert = true;

  int i=0;
  for (auto &o : old) {

    if (!use_w_common) {
      double width_expected = o.hwhm_.value.value();

      if (settings.cali_fwhm_.valid() && settings.cali_nrg_.valid()) {
        double fwhm_expected = settings.cali_fwhm_.transform(settings.bin_to_nrg(o.center_.value.value()));
        double L = settings.nrg_to_bin(settings.bin_to_nrg(o.center_.value.value()) - fwhm_expected/2);
        double R = settings.nrg_to_bin(settings.bin_to_nrg(o.center_.value.value()) + fwhm_expected/2);
        width_expected = (R - L) / (2* sqrt(log(2)));
      }

      o.hwhm_.lbound = width_expected * settings.width_common_bounds.lbound;
      o.hwhm_.ubound = width_expected * settings.width_common_bounds.ubound;

      if ((o.hwhm_.value.value() > o.hwhm_.lbound) && (o.hwhm_.value.value() < o.hwhm_.ubound))
        width_expected = o.hwhm_.value.value();
      o.hwhm_.value.setValue(width_expected);
    }

    o.height_.lbound = o.height_.value.value() * 1e-5;
    o.height_.ubound = o.height_.value.value() * 1e5;

    double lateral_slack = settings.lateral_slack * o.hwhm_.value.value() * 2 * sqrt(log(2));
    o.center_.lbound = o.center_.value.value() - lateral_slack;
    o.center_.ubound = o.center_.value.value() + lateral_slack;

    std::string initial = "F += Gauss2(" +
        o.center_.fityk_name(i) + "," +
        o.height_.fityk_name(i) + "," +
        o.hwhm_.fityk_name(use_w_common ? -1 : i)
        + ")";

    try {
      f->execute(o.center_.def_var(i));
      f->execute(o.height_.def_var(i));
      if (!use_w_common)
        f->execute(o.hwhm_.def_var(i));
      f->execute(initial);
    }
    catch ( ... ) {
      DBG << "Gaussian multifit failed to set up initial locals for peak " << i;
      success = false;
    }

    i++;
  }
  try {
    f->execute("fit " + boost::lexical_cast<std::string>(settings.fitter_max_iter));
  }
  catch ( ... ) {
    DBG << "Gaussian multifit failed to fit";
    success = false;
  }


  if (success)
  {
    std::vector<fityk::Func*> fns = f->all_functions();
    int i = 0;
    for (auto &q : fns) {
      if (q->get_template_name() == "Gauss2") {
        old[i].extract_params(f, q);
        old[i].rsq_ = f->get_rsquared(0);
        i++;
      } else if (q->get_template_name() == "Polynomial") {
        background.extract_params(f, q);
      }
    }
  }

  delete f;

  if (!success)
    old.clear();
  return old;

}

double Gaussian::evaluate(double x) {
  return height_.value.value() * exp(-log(2.0)*(pow(((x-center_.value.value())/hwhm_.value.value()),2)));
}

UncertainDouble Gaussian::area() const {
  UncertainDouble ret;
  ret = height_.value * hwhm_.value * UncertainDouble::from_double(sqrt(M_PI / log(2.0)), 0);
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

std::string Gaussian::fityk_definition()
{
  return "define Gauss2(center, height, hwhm) = "
         "height*("
         "   exp(-(xc/hwhm)^2)"
         ")"
         " where xc=(x-center)";
}

bool Gaussian::extract_params(fityk::Fityk* f, fityk::Func* func)
{
  try {
    if ((func->get_template_name() != "Gauss2")
        && (func->get_template_name() != "Gaussian"))
      return false;

    center_.extract(f, func);
    height_.extract(f, func);
    hwhm_.extract(f, func);

    if (func->get_template_name() == "Gauss2")
      hwhm_.value *= log(2);
  }
  catch ( ... ) {
    DBG << "Gaussian could not extract parameters from Fityk";
    return false;
  }
  return true;
}

void Gaussian::fit_fityk(const std::vector<double> &x, const std::vector<double> &y)
{
  std::vector<double> sigma;
  sigma.resize(x.size(), 1);

  bool success = true;

  if ((x.size() < 1) || (x.size() != y.size()))
    return;

  std::vector<fityk::Func*> fns;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);

  try {
    f->execute("guess Gaussian");
  }
  catch ( ... ) {
    DBG << "Gaussian could not guess";
    success = false;
  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    DBG << "Gaussian could not fit";
    success = false;
  }

  if (success) {
    std::vector<fityk::Func*> fns = f->all_functions();
    if (fns.size()) {
      fityk::Func* lastfn = fns.back();
      extract_params(f, lastfn);
    }
    rsq_ = f->get_rsquared(0);
  }

  delete f;
}

#ifdef FITTER_ROOT_ENABLED

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

  center_.value.setValue((x.back() + x.front())/2);
  center_.lbound = x.front();
  center_.ubound = x.back();

  hwhm_.value.setValue((x.back() - x.front())/2);
  hwhm_.lbound = 0;
  hwhm_.ubound = (x.back() - x.front());

  height_.value.setValue((h1->GetMaximum() + h1->GetMinimum())/2);
  height_.lbound = 0;
  height_.ubound = (h1->GetMaximum() - h1->GetMinimum());

  set_params(f1);

  h1->Fit("f1", "QEMN");
  //  h1->Fit("f1", "EMN");

  get_params(f1);
  rsq_ = f1->GetChisquare();

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
  uint16_t backgroundparams = background.coeffs().size();
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 3;
    definition += "+" + Gaussian().root_definition(num);
  }

//  DBG << "Definition = " << definition;

  TF1* f1 = new TF1("f1", definition.c_str());

  for (auto &o : old)
  {
    double width_expected = o.hwhm_.value.value();
    if (settings.cali_fwhm_.valid() && settings.cali_nrg_.valid())
    {
      double fwhm_expected = settings.cali_fwhm_.transform(settings.bin_to_nrg(o.center_.value.value()));
      double L = settings.nrg_to_bin(settings.bin_to_nrg(o.center_.value.value()) - fwhm_expected/2);
      double R = settings.nrg_to_bin(settings.bin_to_nrg(o.center_.value.value()) + fwhm_expected/2);
      width_expected = (R - L) / (2* sqrt(log(2)));
    }

    o.hwhm_.lbound = width_expected * settings.width_common_bounds.lbound;
    o.hwhm_.ubound = width_expected * settings.width_common_bounds.ubound;

    if ((o.hwhm_.value.value() > o.hwhm_.lbound) && (o.hwhm_.value.value() < o.hwhm_.ubound))
      width_expected = o.hwhm_.value.value();
    o.hwhm_.value.setValue(width_expected);

    o.height_.lbound = o.height_.value.value() * 1e-5;
    o.height_.ubound = o.height_.value.value() * 1e5;

    double lateral_slack = settings.lateral_slack * o.hwhm_.value.value() * 2 * sqrt(log(2));
    o.center_.lbound = o.center_.value.value() - lateral_slack;
    o.center_.ubound = o.center_.value.value() + lateral_slack;
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
    old[i].rsq_ = f1->GetChisquare();
  }
  background.rsq_ = f1->GetChisquare();

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
    centers_avg += p.center_.value.value();
  centers_avg /= old.size();

  double nrg = settings.bin_to_nrg(centers_avg);
  double fwhm_expected = settings.cali_fwhm_.transform(nrg);
  double L = settings.nrg_to_bin(nrg - fwhm_expected/2);
  double R = settings.nrg_to_bin(nrg + fwhm_expected/2);

  FitParam w_common = settings.width_common_bounds;
  w_common.value.setValue((R - L) / (2* sqrt(log(2))));
  w_common.lbound = w_common.value.value() * w_common.lbound;
  w_common.ubound = w_common.value.value() * w_common.ubound;

  std::string definition = background.root_definition();
  uint16_t backgroundparams = background.coeffs().size();
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

    o.height_.lbound = o.height_.value.value() * 1e-5;
    o.height_.ubound = o.height_.value.value() * 1e5;

    double lateral_slack = settings.lateral_slack * o.hwhm_.value.value() * 2 * sqrt(log(2));
    o.center_.lbound = o.center_.value.value() - lateral_slack;
    o.center_.ubound = o.center_.value.value() + lateral_slack;
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
    old[i].rsq_ = f1->GetChisquare();
  }
  background.rsq_ = f1->GetChisquare();

  f1->Delete();
  h1->Delete();

  return old;
}

#endif
