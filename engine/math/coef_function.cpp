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
 *      CoefFunction -
 *
 ******************************************************************************/

#include "coef_function.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "fityk.h"
#include "custom_logger.h"
#include "qpx_util.h"

#ifdef FITTER_ROOT_ENABLED
#include "TF1.h"
#include "TGraph.h"
#include "TGraphErrors.h"
//#include "TMath.h"
#endif

CoefFunction::CoefFunction() :
  xoffset_("xoffset", 0),
  rsq_(0)
{}

CoefFunction::CoefFunction(std::vector<double> coeffs, double uncert, double rsq)
  : CoefFunction()
{
  for (size_t i=0; i < coeffs.size(); ++i)
    add_coeff(i, coeffs[i] - uncert, coeffs[i] + uncert, coeffs[i]);
  rsq_ = rsq;
}

void CoefFunction::add_coeff(int degree, double lbound, double ubound)
{
  double mid = (lbound + ubound) / 2;
  add_coeff(degree, lbound, ubound, mid);
}

void CoefFunction::add_coeff(int degree, double lbound, double ubound, double initial)
{
  if (lbound > ubound)
    return;
  coeffs_[degree] = FitParam("a" + boost::lexical_cast<std::string>(degree),
                             initial, lbound, ubound);
}

std::vector<double> CoefFunction::eval_array(const std::vector<double> &x)
{
  std::vector<double> y;
  for (auto &q : x)
    y.push_back(this->eval(q));
  return y;
}

double CoefFunction::eval_inverse(double y, double e)
{
  int i=0;
  double x0 = xoffset_.value.value();
  double x1 = x0 + (y - this->eval(x0)) / (this->derivative(x0));
  while( i<=100 && std::abs(x1-x0) > e)
  {
    x0 = x1;
    x1 = x0 + (y - this->eval(x0)) / (this->derivative(x0));
    i++;
  }

  double x_adjusted = x1 - xoffset_.value.value();

  if(std::abs(x1-x0) <= e)
    return x_adjusted;

  else
  {
    WARN <<"<" << this->type() << "> Maximum iteration reached in CoefFunction inverse evaluation";
    return nan("");
  }
}

std::vector<double> CoefFunction::coeffs()
{
  std::vector<double> ret;
  int top = 0;
  for (auto &c : coeffs_)
    if (c.first > top)
      top = c.first;
  ret.resize(top+1, 0);
  for (auto &c : coeffs_)
    ret[c.first] = c.second.value.value();
  return ret;
}


// Fityk implementation

bool CoefFunction::extract_params(fityk::Fityk* f, fityk::Func* func)
{
  try {
    if (func->get_template_name() != this->type())
      return false;

    for (auto &c : coeffs_) {
      coeffs_[c.first].extract(f, func);
    }
    //    rsq_ = fityk->get_rsquared(0);
  }
  catch ( ... ) {
    DBG << "<" << this->type() << "> could not extract parameters from Fityk";
    return false;
  }
  return true;
}

bool CoefFunction::add_self(fityk::Fityk *f, int function_num) const
{
  try {
    std::string add = " F += " + this->type() + "(";
    int i = 0;
    for (auto &c : coeffs_) {
      if (i > 0)
        add += ",";
      add += c.second.fityk_name(function_num);
      f->execute(c.second.def_var(function_num));
      i++;
    }
    add += ")";
    f->execute(add);
  }
  catch ( ... ) {
    DBG << "<" << this->type() << "> fit failed to add self";
    return false;
  }
  return true;
}

void CoefFunction::fit(const std::vector<double> &x,
                       const std::vector<double> &y,
                       const std::vector<double> &x_sigma,
                       const std::vector<double> &y_sigma)
{
  #ifdef FITTER_ROOT_ENABLED
    fit_root(x, y, x_sigma, y_sigma);
  #elif
    fit_fityk(x, y, y_sigma);
  #endif
}


void CoefFunction::fit_fityk(const std::vector<double> &x,
                             const std::vector<double> &y,
                             const std::vector<double> &y_sigma)
{
  if ((x.size() != y.size()) || (y.size() != y_sigma.size()))
    return;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, y_sigma);

  bool success = true;

  try {
    f->execute("set fitting_method = nlopt_lbfgs");
//    f->execute("set fitting_method = levenberg_marquardt");
  } catch ( ... ) {
    success = false;
    DBG << "<" << this->type() << "> fit failed to set fitter";
  }

  try {
    f->execute(fityk_definition());
  }
  catch ( ... ) {
    success = false;
    DBG << "<" << this->type() << "> fit failed to define self";
  }

  if (!add_self(f)) {
    success = false;
    DBG << "<" << this->type() << "> add self failed";
  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    DBG << "<" << this->type() << "> Fit failed";
    success = false;
  }

  if (success) {
    if (extract_params(f, f->all_functions().back()))
      rsq_ = f->get_rsquared(0);
    else
      DBG << "<" << this->type() << "> failed to extract fit parameters from Fityk";
  }

  delete f;

  DBG << "Solved (Fityk) as " << this->to_string();
}

#ifdef FITTER_ROOT_ENABLED
void CoefFunction::fit_root(const std::vector<double> &x,
                            const std::vector<double> &y,
                            const std::vector<double> &x_sigma,
                            const std::vector<double> &y_sigma)
{
  if (x.size() != y.size())
    return;

  TGraphErrors* g1;
  if ((x.size() == x_sigma.size()) && (x_sigma.size() == y_sigma.size()))
    g1 = new TGraphErrors(y.size(), x.data(), y.data(), x_sigma.data(), y_sigma.data());
  else
    g1 = new TGraphErrors(y.size(), x.data(), y.data(), nullptr, nullptr);

  TF1* f1 = new TF1("f1", this->root_definition().c_str());

  int i=0;
  for (const auto &p : coeffs_)
  {
    f1->SetParameter(i, p.second.value.value());
    f1->SetParLimits(i, p.second.lbound, p.second.ubound);
    i++;
  }

//  g1->Fit("f1", "QEMN");
  g1->Fit("f1", "EMN");

  i=0;
  for (auto &p : coeffs_)
  {
    p.second.value = UncertainDouble::from_double(f1->GetParameter(i), f1->GetParError(i));
    i++;
  }
  rsq_ = f1->GetChisquare();

  f1->Delete();
  g1->Delete();
}

#endif
