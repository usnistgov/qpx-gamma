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
  PolyResidual(double x, double y, std::vector<double> p)
      : x_(x), y_(y), p_(p) {}

  template <typename T> bool operator()(const T* const c,
                                        T* residual) const {
    T val(0);
    for (int i=0; i < p_.size(); ++i)
    {
      val += T(y_) - P_ * pow(T(x_), T(c));
    }
    residual[0] = val;

    return true;
  }

 private:
  const double x_;
  const double y_;
  const std::vector<double> p_;
};


void PolyBounded::add_residual_blocks(ceres::Problem &problem,
                                      const std::vector<double> &x,
                                      const std::vector<double> &y,
                                      std::vector<double> &c
                                      )
{
  if (x.size() != y.size())
    return;

  for (int i=0; i < x.size(); ++i) {
    double xx = x.at(i) - xoffset_.value.value();

    problem.AddResidualBlock(
        new ceres::AutoDiffCostFunction<PolyResidual, 1, 1>(
            new PolyResidual(xx, y.at(i), c)),
        NULL,
        c.data());
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

