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
 *      SqrtPoly -
 *
 ******************************************************************************/

#include "sqrt_poly.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "custom_logger.h"
#include "qpx_util.h"

std::string SqrtPoly::to_string() const
{
  std::string ret = type() + " = sqrt(";
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

  ret += ")   rsq=" + boost::lexical_cast<std::string>(chi2_) + "    where:\n" +  vars;

  return ret;
}

std::string SqrtPoly::to_UTF8(int precision, bool with_rsq) const
{
  std::string calib_eqn = "\u221A(";
  int i = 0;
  for (auto &c : coeffs_) {
    if (i > 0)
      calib_eqn += " + ";
    calib_eqn += to_str_precision(c.second.value().value(), precision);
    if (c.first > 0) {
      if (xoffset_.value().value())
        calib_eqn += "(x-" + to_str_precision(xoffset_.value().value(), precision) + ")";
      else
        calib_eqn += "x";
    }
    if (c.first > 1)
      calib_eqn += UTF_superscript(c.first);
    i++;
  }
  calib_eqn += ")";

  if (with_rsq)
    calib_eqn += std::string("   r")
        + UTF_superscript(2)
        + std::string("=")
        + to_str_precision(chi2_, precision);

  return calib_eqn;
}

std::string SqrtPoly::to_markup(int precision, bool with_rsq) const
{
  std::string calib_eqn = "&radic;<span style=\"text-decoration:overline;\">";

  int i = 0;
  for (auto &c : coeffs_) {
    if (i > 0)
      calib_eqn += " + ";
    calib_eqn += to_str_precision(c.second.value().value(), precision);
    if (c.first > 0) {
      if (xoffset_.value().value())
        calib_eqn += "(x-" + to_str_precision(xoffset_.value().value(), precision) + ")";
      else
        calib_eqn += "x";
    }
    if (c.first > 1)
      calib_eqn +=  "<sup>" + UTF_superscript(c.first) + "</sup>";
    i++;
  }
  calib_eqn += "</span>";

  if (with_rsq)
  calib_eqn += "   r<sup>2</sup>"
        + std::string("=")
        + to_str_precision(chi2_, precision);

  return calib_eqn;
}

double SqrtPoly::eval(double x) const
{
  double x_adjusted = x - xoffset_.value().value();
  double result = 0.0;
  for (auto &c : coeffs_)
    result += c.second.value().value() * pow(x_adjusted, c.first);
  return sqrt(result);
}

double SqrtPoly::derivative(double x) const
{
  return x;
}

std::string SqrtPoly::root_definition()
{
  std::string definition = "TMath::Sqrt(";
  int i = 0;
  for (auto &c : coeffs_)
  {
    if (i > 0)
      definition += "+";
    definition += "[" + std::to_string(i) + "]";
    if (c.first > 0)
    {
      definition += "*";
      if (xoffset_.value().value())
        definition += "(x-" + std::to_string(xoffset_.value().value()) + ")";
      else
        definition += "x";
    }
    if (c.first > 1)
      definition += "^" + std::to_string(c.first);
    i++;
  }
  definition  += ")";
  return definition;
}
