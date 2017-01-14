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

#include "TF1.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TH1.h"

Hypermet::Hypermet(Gaussian gauss, FitSettings settings)
  : height_("h", gauss.height_.value().value())
  , center_("c", gauss.center_.value().value())
  , width_("w", gauss.hwhm_.value().value() / sqrt(log(2)))
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

void Hypermet::set_center(const FitParam &ncenter)
{
  center_.set(ncenter);
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_height(const FitParam &nheight)
{
  height_.set(nheight);
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_width(const FitParam &nwidth)
{
  width_.set(nwidth);
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_Lskew_amplitude(const FitParam &nLskew_amplitude)
{
  Lskew_amplitude_.set(nLskew_amplitude);
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_Lskew_slope(const FitParam &nLskew_slope)
{
  Lskew_slope_.set(nLskew_slope);
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_Rskew_amplitude(const FitParam &nRskew_amplitude)
{
  Rskew_amplitude_.set(nRskew_amplitude);
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_Rskew_slope(const FitParam &nRskew_slope)
{
  Rskew_slope_.set(nRskew_slope);
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_tail_amplitude(const FitParam &ntail_amplitude)
{
  tail_amplitude_.set(ntail_amplitude);
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_tail_slope(const FitParam &ntail_slope)
{
  tail_slope_.set(ntail_slope);
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_step_amplitude(const FitParam &nstep_amplitude)
{
  step_amplitude_.set(nstep_amplitude);
  user_modified_ = true;
  rsq_ = 0;
}


std::string Hypermet::to_string() const
{
  std::string ret = "Hypermet ";
  ret += "   area=" + area().to_string()
      + "   rsq=" + boost::lexical_cast<std::string>(rsq_) + "    where:\n";

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
  ret.height_.set(height_);
  ret.center_.set(center_);
  ret.hwhm_.set(width_.lower() * sqrt(log(2)),
                width_.upper() * sqrt(log(2)),
                width_.value().value() * sqrt(log(2)));
  ret.rsq_ = rsq_;
  return ret;
}

void Hypermet::fit(const std::vector<double> &x, const std::vector<double> &y,
              Gaussian gauss, FitSettings settings)
{
  *this = Hypermet(gauss, settings);
  fit_root(x, y);
}


std::vector<Hypermet> Hypermet::fit_multi(const std::vector<double> &x,
                                          const std::vector<double> &y,
                                          std::vector<Hypermet> old,
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

  node.append_attribute("rsq").set_value(to_max_precision(rsq_).c_str());
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
  rsq_ = node.attribute("rsq").as_double(0);
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

std::string Hypermet::root_definition(uint16_t start)
{
  return root_definition(start, start+1);
}

std::string Hypermet::root_definition(uint16_t width, uint16_t i)
{
  std::string h = "[" + std::to_string(i) + "]";
  std::string w = "[" + std::to_string(width) + "]";
  std::string xc = "(x-[" + std::to_string(i+1) + "])";
  std::string xcw = xc + "/" + w;
  std::string lskewh = "[" + std::to_string(i+2) + "]";
  std::string lskews = "/[" + std::to_string(i+3) + "]";
  std::string rskewh = "[" + std::to_string(i+4) + "]";
  std::string rskews = "/[" + std::to_string(i+5) + "]";
  std::string tailh = "[" + std::to_string(i+6) + "]";
  std::string tails = "/[" + std::to_string(i+7) + "]";
  std::string steph = "[" + std::to_string(i+8) + "]";

  return h + "*("
         "   TMath::Exp(-(" + xcw + ")^2)"
         " + 0.5 * ("
         "   " + lskewh + "*TMath::Exp((0.5*"+ w+lskews +")^2 + ("+ xc+lskews +"))*TMath::Erfc((0.5*"+ w+lskews +") + "+ xcw +")"
         " + " + rskewh + "*TMath::Exp((0.5*"+ w+rskews +")^2 - ("+ xc+rskews +"))*TMath::Erfc((0.5*"+ w+rskews +") - "+ xcw +")"
         " + " + tailh  + "*TMath::Exp((0.5*"+ w+tails  +")^2 + ("+ xc+tails  +"))*TMath::Erfc((0.5*"+ w+tails  +") + "+ xcw +")"
         " + " + steph  + "*TMath::Erfc(" + xc +"/" + w + ")"
         " ) )";
}

void Hypermet::set_params(TF1* f, uint16_t start) const
{
  set_params(f, start, start+1);
}

void Hypermet::set_params(TF1* f, uint16_t width, uint16_t others_start) const
{
  width_.set(f, width);
  height_.set(f, others_start);
  center_.set(f, others_start+1);
  Lskew_amplitude_.set(f, others_start+2);
  Lskew_slope_.set(f, others_start+3);
  Rskew_amplitude_.set(f, others_start+4);
  Rskew_slope_.set(f, others_start+5);
  tail_amplitude_.set(f, others_start+6);
  tail_slope_.set(f, others_start+7);
  step_amplitude_.set(f, others_start+8);
}

void Hypermet::get_params(TF1* f, uint16_t start)
{
  get_params(f, start, start+1);
}

void Hypermet::get_params(TF1* f, uint16_t width, uint16_t others_start)
{
  width_.get(f, width);
  height_.get(f, others_start);
  center_.get(f, others_start+1);
  Lskew_amplitude_.get(f, others_start+2);
  Lskew_slope_.get(f, others_start+3);
  Rskew_amplitude_.get(f, others_start+4);
  Rskew_slope_.get(f, others_start+5);
  tail_amplitude_.get(f, others_start+6);
  tail_slope_.get(f, others_start+7);
  step_amplitude_.get(f, others_start+8);
}

void Hypermet::fit_root(const std::vector<double> &x, const std::vector<double> &y)
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

  width_.set(width_.value().value() * 0.7,
             std::min(width_.value().value() * 1.3, x.back() - x.front()),
             width_.value().value());

  height_.set(height_.value().value() * 0.003,
              std::min(height_.value().value() * 3000.0, h1->GetMaximum() - h1->GetMinimum()),
              height_.value().value());

  double lateral_slack = (x.back() - x.front()) / 5.0;
  center_.set(std::max(center_.value().value() - lateral_slack, x.front()),
              std::min(center_.value().value() + lateral_slack, x.back()),
              center_.value().value());

  TF1* f1 = new TF1("f1", this->root_definition().c_str());

  set_params(f1);

  h1->Fit("f1", "QN");
  h1->Fit("f1", "QEMN");

//  h1->Fit("f1", "QN");
  //  h1->Fit("f1", "EMN");

  get_params(f1);
  rsq_ = f1->GetChisquare();

  f1->Delete();
  h1->Delete();
}

std::vector<Hypermet> Hypermet::fit_multi_root(const std::vector<double> &x,
                                               const std::vector<double> &y,
                                               std::vector<Hypermet> old,
                                               Polynomial &background,
                                               FitSettings settings)
{
  DBG << "Multifit with variable width";

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
    uint16_t num = backgroundparams + i * 10;
    definition += "+" + Hypermet().root_definition(num);
  }

//  DBG << "Definition = " << definition;

  TF1* f1 = new TF1("f1", definition.c_str());

  for (auto &o : old)
  {
    double width_expected = o.width_.value().value();
    if (settings.cali_fwhm_.valid() && settings.cali_nrg_.valid())
      width_expected = settings.bin_to_width(o.center_.value().value()) / (2*sqrt(log(2)));

    o.width_.set(width_expected * settings.width_common_bounds.lower(),
                 std::min(width_expected * settings.width_common_bounds.upper(),
                          x.back() - x.front()),
                 o.width_.value().value());

    o.height_.set(o.height_.value().value() * 1e-5,
                  std::min(o.height_.value().value() * 1e5,
                           h1->GetMaximum() - h1->GetMinimum()),
                  o.height_.value().value());

    double lateral_slack = settings.lateral_slack * o.width_.value().value() * 2 * sqrt(log(2));
    o.center_.set(std::max(o.center_.value().value() - lateral_slack, x.front()),
                  std::min(o.center_.value().value() + lateral_slack, x.back()),
                  o.center_.value().value());
  }

  background.set_params(f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 10;
    old[i].set_params(f1, num);
  }

//  h1->Fit("f1", "QN");
  h1->Fit("f1", "QEMN");

//  h1->Fit("f1", "QN");
  //  h1->Fit("f1", "EMN");

  background.get_params(f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 10;
    old[i].get_params(f1, num);
    old[i].rsq_ = f1->GetChisquare();
  }
  background.rsq_ = f1->GetChisquare();

  f1->Delete();
  h1->Delete();

  return old;
}

std::vector<Hypermet> Hypermet::fit_multi_root_commonw(const std::vector<double> &x,
                                                       const std::vector<double> &y,
                                                       std::vector<Hypermet> old,
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
  FitParam w_common("w", 0);
  w_common.preset_bounds(width_expected * settings.width_common_bounds.lower(),
                         std::min(width_expected * settings.width_common_bounds.upper(),
                                  x.back() - x.front()));

//  DBG << "Calculated common width param " << w_common.to_string();

  std::string definition = background.root_definition();
  uint16_t backgroundparams = background.coeffs().size();
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + backgroundparams + i * 9;
    definition += "+" + Hypermet().root_definition(backgroundparams, num);
  }

  TF1* f1 = new TF1("f1", definition.c_str());

  for (auto &o : old)
  {
    o.width_ = w_common;

    o.height_.set(o.height_.value().value() * 0.1,
                  o.height_.value().value() * 10.0,
                  o.height_.value().value());

    o.height_.set(o.height_.value().value() * 1e-3,
                  std::min(o.height_.value().value() * 1e3,
                           h1->GetMaximum() - h1->GetMinimum()),
                  o.height_.value().value());

    double lateral_slack = settings.lateral_slack * o.width_.value().value() * 2 * sqrt(log(2));
    o.center_.set(std::max(o.center_.value().value() - lateral_slack, x.front()),
                  std::min(o.center_.value().value() + lateral_slack, x.back()),
                  o.center_.value().value());
  }

  background.set_params(f1, 0);
  w_common.set(f1, backgroundparams);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + backgroundparams + i * 9;
    old[i].set_params(f1, backgroundparams, num);
  }

//  h1->Fit("f1", "QN");
  h1->Fit("f1", "QEMN");

  //  h1->Fit("f1", "EMN");

  background.get_params(f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + backgroundparams + i * 9;
    old[i].get_params(f1, backgroundparams, num);
    old[i].rsq_ = f1->GetChisquare();
  }

  background.rsq_ = f1->GetChisquare();

  f1->Delete();
  h1->Delete();

  return old;
}
