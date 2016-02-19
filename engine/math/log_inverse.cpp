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
 *      LogInverse -
 *
 ******************************************************************************/

#include "log_inverse.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "fityk.h"
#include "custom_logger.h"
#include "qpx_util.h"


std::string LogInverse::fityk_definition() {
  std::string declaration = "define " + type() + "(";
  std::string definition  = " = exp(";
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
  definition  += ") where xx = 1.0/(x - " + std::to_string(xoffset_.val) + ")";

  return declaration + definition;
}

std::string LogInverse::to_string() const {
  std::string ret = type() + " = exp(";
  std::string vars;
  int i = 0;
  for (auto &c : coeffs_) {
    if (i > 0)
      ret += " + ";
    ret += c.second.name();
    if (c.first > 0)
      ret += "/(x-xoffset)";
    if (c.first > 1)
      ret += "^" + std::to_string(c.first);
    i++;
    vars += "     " + c.second.to_string() + "\n";
  }
  vars += "     " + xoffset_.to_string();

  ret += ")   rsq=" + boost::lexical_cast<std::string>(rsq_) + "    where:\n" +  vars;

  return ret;
}

std::string LogInverse::to_UTF8(int precision, bool with_rsq) {

  std::string calib_eqn = "exp(";
  int i = 0;
  for (auto &c : coeffs_) {
    if (i > 0)
      calib_eqn += " + ";
    calib_eqn += to_str_precision(c.second.val, precision);
    if (c.first > 0) {
      if (xoffset_.val)
        calib_eqn += "/(x-" + to_str_precision(xoffset_.val, precision) + ")";
      else
        calib_eqn += "/x";
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
        + to_str_precision(rsq_, precision);

  return calib_eqn;
}

std::string LogInverse::to_markup(int precision, bool with_rsq) {
  std::string calib_eqn = "exp(";

  int i = 0;
  for (auto &c : coeffs_) {
    if (i > 0)
      calib_eqn += " + ";
    calib_eqn += to_str_precision(c.second.val, precision);
    if (c.first > 0) {
      if (xoffset_.val)
        calib_eqn += "1/(x-" + to_str_precision(xoffset_.val, precision) + ")";
      else
        calib_eqn += "1/x";
    }
    if (c.first > 1)
      calib_eqn +=  "<sup>" + UTF_superscript(c.first) + "</sup>";
    i++;
  }
  calib_eqn += ")";

  if (with_rsq)
  calib_eqn += "   r<sup>2</sup>"
        + std::string("=")
        + to_str_precision(rsq_, precision);

  return calib_eqn;
}

double LogInverse::eval(double x) {
  double x_adjusted = (x - xoffset_.val);
  if (x_adjusted != 0)
    x_adjusted = 1.0/x_adjusted;
  else
    x_adjusted = std::numeric_limits<double>::max();
  double result = 0.0;

  for (auto &c : coeffs_)
    result += c.second.val * pow(x_adjusted, c.first);

  return exp(result);
}

double LogInverse::derivative(double x) {
  return x;
}
