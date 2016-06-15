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
 *      Polynomial - 
 *
 ******************************************************************************/

#include "polynomial.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "fityk.h"
//#include "custom_logger.h"
#include "qpx_util.h"


//PolyBounded::PolyBounded(Polynomial p)
//{
//  xoffset_ = FitParam("xoffset", p.xoffset_);
//  rsq_ = p.rsq_;

//  for (int i=0; i < p.coeffs_.size(); ++i)
//    coeffs_[i] = FitParam("a" + boost::lexical_cast<std::string>(i), p.coeffs_[i]);
//}

//Polynomial PolyBounded::to_simple() const {
//  int max = 0;
//  for (auto &c : coeffs_)
//    if (c.first > max)
//      max = c.first;
//  std::vector<double> coeffs(max + 1);
//  for (auto &c : coeffs_)
//    coeffs[c.first] =  c.second.val;
//  return Polynomial(coeffs, xoffset_.val, rsq_);
//}

std::string PolyBounded::to_string() const {
  std::string ret = type() + " = ";
  std::string vars;
  int i = 0;
  for (auto &c : coeffs_) {
    if (i > 0)
      ret += " + ";
    ret += c.second.name();
    if (c.first > 0)
      ret += "*(x-xoffset)";
    if (c.first > 1)
      ret += "^" + std::to_string(c.first);
    i++;
    vars += "     " + c.second.to_string() + "\n";
  }
  vars += "     " + xoffset_.to_string();

  ret += "   rsq=" + boost::lexical_cast<std::string>(rsq_) + "    where:\n" +  vars;

  return ret;
}

std::string PolyBounded::to_UTF8(int precision, bool with_rsq) {

  std::string calib_eqn;
  int i = 0;
  for (auto &c : coeffs_) {
    if (i > 0)
      calib_eqn += " + ";
    calib_eqn += to_str_precision(c.second.value.value(), precision);
    if (c.first > 0) {
      if (xoffset_.value.value())
        calib_eqn += "(x-" + to_str_precision(xoffset_.value.value(), precision) + ")";
      else
        calib_eqn += "x";
    }
    if (c.first > 1)
      calib_eqn += UTF_superscript(c.first);
    i++;
  }

  if (with_rsq)
    calib_eqn += std::string("   r")
        + UTF_superscript(2)
        + std::string("=")
        + to_str_precision(rsq_, precision);

  return calib_eqn;
}

std::string PolyBounded::to_markup(int precision, bool with_rsq) {
  std::string calib_eqn;

  int i = 0;
  for (auto &c : coeffs_) {
    if (i > 0)
      calib_eqn += " + ";
    calib_eqn += to_str_precision(c.second.value.value(), precision);
    if (c.first > 0) {
      if (xoffset_.value.value())
        calib_eqn += "(x-" + to_str_precision(xoffset_.value.value(), precision) + ")";
      else
        calib_eqn += "x";
    }
    if (c.first > 1)
      calib_eqn +=  "<sup>" + UTF_superscript(c.first) + "</sup>";
    i++;
  }

    if (with_rsq)
    calib_eqn += "   r<sup>2</sup>"
        + std::string("=")
        + to_str_precision(rsq_, precision);

  return calib_eqn;
}


double PolyBounded::eval(double x) {
  double x_adjusted = x - xoffset_.value.value();
  double result = 0.0;
  for (auto &c : coeffs_)
    result += c.second.value.value() * pow(x_adjusted, c.first);
  return result;
}


double PolyBounded::derivative(double x) {
  PolyBounded new_poly;  // derivative not true if offset != 0
  new_poly.xoffset_ = xoffset_;

  for (auto &c : coeffs_) {
    if (c.first != 0) {
      new_poly.add_coeff(c.first - 1, c.second.lbound * c.first, c.second.ubound * c.first, c.second.value.value() * c.first);
    }
  }

  return new_poly.eval(x);
}

void PolyBounded::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_attribute("rsq").set_value(to_max_precision(rsq_).c_str());
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


void PolyBounded::from_xml(const pugi::xml_node &node) {
  coeffs_.clear();

  rsq_ = node.attribute("rsq").as_double(0);
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


//Fityk
std::string PolyBounded::fityk_definition() {
  std::string declaration = "define " + type() + "(";
  std::string definition  = " = ";
  int i = 0;
  for (auto &c : coeffs_) {
    if (i > 0) {
      declaration += ", ";
      definition += " + ";
    }
    declaration += c.second.name();
    definition  += c.second.name();
    if (c.first > 0)
      definition += "*xx";
    if (c.first > 1)
      definition += "^" + std::to_string(c.first);
    i++;
  }
  declaration += ")";
  definition  += " where xx = (x - " + std::to_string(xoffset_.value.value()) + ")";

  return declaration + definition;
}

#ifdef FITTER_CERES_ENABLED

//Ceres


struct PolyResidual {
  PolyResidual(double x, double y, std::map<int, double> p)
      : x_(x), y_(y), p_(p) {}

  template <typename T> bool operator()(const T* const c,
                                        T* residual) const {
    T val(0);
    int i=0;
    for (auto &c : cc) {
      val +=
      residual[0] = T(y_) - c[0] * pow(T(x_), T(p_));
    }
    return true;
  }

 private:
  const double x_;
  const double y_;
  const std::map<int, double> p_;
};


void PolyBounded::add_residual_blocks(ceres::Problem &problem,
                                      const std::vector<double> &x,
                                      const std::vector<double> &y,
                                      std::map<int, double> &cc)
{
  if (x.size() != y.size())
    return;

  for (int i=0; i < x.size(); ++i) {
    double xx = x.at(i) - xoffset_.value.value();

    problem.AddResidualBlock(
        new ceres::AutoDiffCostFunction<PolyResidual, 1, 1>(
            new PolyResidual(xx, y.at(i), cc)),
        NULL,
        &c.second);
//    for (auto &c : cc) {
//      problem.AddResidualBlock(
//          new ceres::AutoDiffCostFunction<PolyResidual, 1, 1>(
//              new PolyResidual(xx, y.at(i), c.first)),
//          NULL,
//          &c.second);
//    }
  }
}
#endif






//Polynomial::Polynomial(std::vector<double> coeffs, double xoffset, double rsq)
//  :Polynomial()
//{
//  xoffset_ = xoffset;
//  rsq_ = rsq;
//  int highest = -1;
//  for (int i=0; i < coeffs.size(); ++i)
//    if (coeffs[i] != 0)
//      highest = i;
//  for (int i=0; i <= highest; ++i)
//    coeffs_.push_back(coeffs[i]);
//}

//Polynomial::Polynomial(std::vector<double> &x, std::vector<double> &y, uint16_t degree, double xoffset)
//  :Polynomial()
//{
//  std::vector<uint16_t> degrees;
//  for (int i=0; i <= degree; ++i) {
//    degrees.push_back(i);
//  }

//  fit(x, y, degrees, xoffset);
//}

//Polynomial::Polynomial(std::vector<double> &x, std::vector<double> &y,
//                       std::vector<uint16_t> &degrees, double xoffset) {
//  fit(x, y, degrees, xoffset);
//}


//void Polynomial::fit(std::vector<double> &x, std::vector<double> &y,
//                       std::vector<uint16_t> &degrees, double xoffset) {
//  xoffset_ = xoffset;
//  coeffs_.clear();

//  if (x.size() != y.size())
//    return;

//  std::vector<double> sigma;
//  sigma.resize(x.size(), 1);
  
//  fityk::Fityk *f = new fityk::Fityk;
//  f->redir_messages(NULL);

//  f->load_data(0, x, y, sigma);

//  bool success = true;

//  try {
//    f->execute(fityk_definition(degrees, xoffset));
//    f->execute("guess MyPoly");
//  }
//  catch ( ... ) {
//    success = false;
//  }

//  try {
//    f->execute("fit");
//  }
//  catch ( ... ) {
//    success = false;
//  }

//  if (success) {
//    if (extract_params(f->all_functions().back(), degrees))
//      rsq_ = f->get_rsquared(0);
//    else
//      DBG << "Polynomial failed to extract fit parameters from Fityk";
//  }

//  delete f;
//}


//double Polynomial::eval(double x) {
//  double x_adjusted = x - xoffset_;
//  double result = 0.0;
//  for (int i=0; i < coeffs_.size(); i++)
//    result += coeffs_[i] * pow(x_adjusted, i);
//  return result;
//}

//Polynomial Polynomial::derivative() {
//  Polynomial new_poly;  // derivative not true if offset != 0
//  new_poly.xoffset_ = xoffset_;

//  int highest = 0;
//  for (int i=0; i < coeffs_.size(); ++i)
//    if (coeffs_[i] != 0)
//      highest = i;

//  if (highest == 0)
//    return new_poly;

//  new_poly.coeffs_.resize(highest, 0);
//  for (int i=1; i <= highest; ++ i)
//    new_poly.coeffs_[i-1] = i * coeffs_[i];

//  return new_poly;
//}

//double Polynomial::eval_inverse(double y, double e) {
//  int i=0;
//  double x0=1;
//  Polynomial deriv = derivative();
//  double x1 = x0 + (y - eval(x0)) / (deriv.eval(x0));
//  while( i<=100 && std::abs(x1-x0) > e)
//  {
//    x0 = x1;
//    x1 = x0 + (y - eval(x0)) / (deriv.eval(x0));
//    i++;
//  }

//  double x_adjusted = x1 - xoffset_;

//  if(std::abs(x1-x0) <= e)
//    return x_adjusted;

//  else
//  {
//    WARN <<"Maximum iteration reached in polynomial inverse evaluation";
//    return nan("");
//  }
//}


//std::vector<double> Polynomial::eval(std::vector<double> x) {
//  std::vector<double> y;
//  for (auto &q : x)
//    y.push_back(eval(q));
//  return y;
//}

//std::string Polynomial::to_string(bool with_rsq) {
//  std::stringstream ss;
//  for (int i=0; i < coeffs_.size(); i++) {
//    if (i > 0)
//      ss << " + ";
//    ss << coeffs_[i];
//    if (i > 0)
//      ss << "x";
//    if (i > 1)
//      ss << "^" << i;
//  }

//  if (with_rsq)
//    ss << "   rsq=" << rsq_;

//  return ss.str();
//}

//std::string Polynomial::to_UTF8(int precision, bool with_rsq) {

//  std::string calib_eqn;
//  for (int i=0; i < coeffs_.size(); i++) {
//    if (i > 0)
//      calib_eqn += " + ";
//    calib_eqn += to_str_precision(coeffs_[i], precision);
//    if (i > 0)
//      calib_eqn += "x";
//    if (i > 1)
//      calib_eqn += UTF_superscript(i);
//  }

//  if (with_rsq)
//    calib_eqn += std::string("   r")
//        + UTF_superscript(2)
//        + std::string("=")
//        + to_str_precision(rsq_, precision);
  
//  return calib_eqn;
//}

//std::string Polynomial::to_markup(int precision, bool with_rsq) {
//  std::string calib_eqn;
//  for (int i=0; i < coeffs_.size(); i++) {
//    if (i > 0)
//      calib_eqn += " + ";
//    calib_eqn += to_str_precision(coeffs_[i], precision);
//    if (i > 0)
//      calib_eqn += "x";
//    if (i > 1)
//      calib_eqn += "<sup>" + UTF_superscript(i) + "</sup>";
//  }

//  if (with_rsq)
//    calib_eqn += "   r<sup>2</sup>"
//        + std::string("=")
//        + to_str_precision(rsq_, precision);

//  return calib_eqn;
//}

//std::string Polynomial::coef_to_string() const{
//  std::stringstream dss;
//  dss.str(std::string());
//  for (auto &q : coeffs_) {
//    dss << q << " ";
//  }
//  return boost::algorithm::trim_copy(dss.str());
//}

//void Polynomial::coef_from_string(std::string coefs) {
//  std::stringstream dss(boost::algorithm::trim_copy(coefs));

//  std::list<double> templist; double coef;
//  while (dss.rdbuf()->in_avail()) {
//    dss >> coef;
//    templist.push_back(coef);
//  }

//  coeffs_.resize(templist.size());
//  int i=0;
//  for (auto &q: templist) {
//    coeffs_[i] = q;
//    i++;
//  }
//}

//std::string Polynomial::fityk_definition(const std::vector<uint16_t> &degrees, double xoffset) {
//  std::string definition = "define MyPoly(";
//  std::vector<std::string> varnames;
//  for (int i=0; i < degrees.size(); ++i) {
//    std::stringstream ss;
//    ss << degrees[i];
//    varnames.push_back(ss.str());
//    definition += (i==0)?"":",";
//    definition += "a" + ss.str() + "=~0";
//  }
//  definition += ") = ";
//  for (int i=0; i<varnames.size(); ++i) {
//    definition += (i==0)?"":"+";
//    definition += "a" + varnames[i] + "*xx^" + varnames[i];
//  }
//  definition += " where xx = (x - " + std::to_string(xoffset) + ")";
//  return definition;
//}

//bool Polynomial::extract_params(fityk::Func* func, const std::vector<uint16_t> &degrees) {
//  try {
//    if (func->get_template_name() != "MyPoly")
//      return false;

//    int highest = 0;
//    for (auto &q : degrees)
//      if (q > highest)
//        highest = q;

//    coeffs_.resize(highest + 1, 0);
//    for (auto &q : degrees) {
//      std::stringstream ss;
//      ss << q;
//      coeffs_[q] = func->get_param_value("a" + ss.str());
//    }
//    //    rsq_ = fityk->get_rsquared(0);
//  }
//  catch ( ... ) {
//    DBG << "Polynomial could not extract parameters from Fityk";
//    return false;
//  }
//  return true;
//}
