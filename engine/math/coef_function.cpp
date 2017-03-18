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
#include "custom_logger.h"
#include "qpx_util.h"

CoefFunction::CoefFunction()
{}

CoefFunction::CoefFunction(std::vector<double> coeffs, double uncert, double rsq)
  : CoefFunction()
{
  for (size_t i=0; i < coeffs.size(); ++i)
    add_coeff(i, coeffs[i] - uncert, coeffs[i] + uncert, coeffs[i]);
  chi2_ = rsq;
}

FitParam CoefFunction::xoffset() const
{
  return xoffset_;
}

double CoefFunction::chi2() const
{
  return chi2_;
}

size_t CoefFunction::coeff_count() const
{
  return coeffs_.size();
}

std::vector<int> CoefFunction::powers() const
{
  std::vector<int> ret;
  for (auto c : coeffs_)
    ret.push_back(c.first);
  return ret;
}

FitParam CoefFunction::get_coeff(int degree) const
{
  if (coeffs_.count(degree))
    return coeffs_.at(degree);
  else
    return FitParam();
}

std::map<int, FitParam> CoefFunction::get_coeffs() const
{
  return coeffs_;
}

void CoefFunction::set_xoffset(const FitParam& o)
{
  xoffset_ = o;
}

void CoefFunction::set_chi2(double c2)
{
  chi2_ = c2;
}

void CoefFunction::set_coeff(int degree, const FitParam& p)
{
  coeffs_[degree] = p;
}

void CoefFunction::set_coeff(int degree, const UncertainDouble& p)
{
  coeffs_[degree].set_value(p);
  //bounds?
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

std::vector<double> CoefFunction::eval_array(const std::vector<double> &x) const
{
  std::vector<double> y;
  for (auto &q : x)
    y.push_back(this->eval(q));
  return y;
}

double CoefFunction::eval_inverse(double y, double e) const
{
  int i=0;
  double x0 = xoffset_.value().value();
  double x1 = x0 + (y - this->eval(x0)) / (this->derivative(x0));
  while( i<=100 && std::abs(x1-x0) > e)
  {
    x0 = x1;
    x1 = x0 + (y - this->eval(x0)) / (this->derivative(x0));
    i++;
  }

  double x_adjusted = x1 - xoffset_.value().value();

  if(std::abs(x1-x0) <= e)
    return x_adjusted;

  else
  {
//    WARN <<"<" << this->type() << "> Maximum iteration reached in CoefFunction inverse evaluation";
    return nan("");
  }
}

std::vector<double> CoefFunction::coeffs_consecutive() const
{
  std::vector<double> ret;
  int top = 0;
  for (auto &c : coeffs_)
    if (c.first > top)
      top = c.first;
  ret.resize(top+1, 0);
  for (auto &c : coeffs_)
    ret[c.first] = c.second.value().value();
  return ret;
}

void CoefFunction::to_xml(pugi::xml_node &root) const
{
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_attribute("rsq").set_value(to_max_precision(chi2_).c_str());
  xoffset_.to_xml(node);

  if (coeffs_.size()) {
    pugi::xml_node terms = node.append_child("Terms");
    for (auto t : coeffs_)
    {
      pugi::xml_node term = terms.append_child("Term");
      term.append_attribute("order").set_value(t.first);
      t.second.to_xml(term);
    }
  }
}


void CoefFunction::from_xml(const pugi::xml_node &node)
{
  coeffs_.clear();

  chi2_ = node.attribute("rsq").as_double(0);
  if (node.child(xoffset_.xml_element_name().c_str()))
    xoffset_.from_xml(node.child(xoffset_.xml_element_name().c_str()));

  if (node.child("Terms")) {
    for (auto &t : node.child("Terms").children()) {
      if (std::string(t.name()) == "Term")
      {
        int order = t.attribute("order").as_int();
        FitParam coef;
        coef.from_xml(t.child(xoffset_.xml_element_name().c_str()));
        coeffs_[order] = coef;
      }
    }
  }
}

void to_json(json& j, const CoefFunction& s)
{
  for (auto c : s.get_coeffs())
  {
    json cc;
    cc["degree"] = c.first;
    cc["coefficient"] = c.second;
    j["coefficients"].push_back(cc);
  }
  j["xoffset"] = s.xoffset();
  j["chi2"] = s.chi2();
}

void from_json(const json& j, CoefFunction& s)
{
  if (j.count("coefficients"))
  {
    auto o = j["coefficients"];
    for (json::iterator it = o.begin(); it != o.end(); ++it)
    {
      int d = it.value()["degree"];
      FitParam p = it.value()["coefficient"];
      s.set_coeff(d, p);
    }
  }
  s.set_xoffset(j["xoffset"]);
  s.set_chi2(j["chi2"]);
}

