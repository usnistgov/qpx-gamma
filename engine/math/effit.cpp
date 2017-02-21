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
 *      Effit -
 *
 ******************************************************************************/

#include "effit.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "custom_logger.h"
#include "qpx_util.h"


double Effit::eval(double x) const
{
  double xa = log((x - xoffset_.value().value())/100);
  double xb = log((x - xoffset_.value().value())/1000);
//   x= ((A + B*ln(x/100.0) + C*(ln(x/100.0)^2))^(-G) + (D + E*ln(x/1000.0) + F*(ln(x/1000.0)^2))^(1-G))^(1-(1/G))
  double result = 0;//pow(pow(A + B*xa + C*xa*xa,-G) + pow(D + E*xb + F*xb*xb,-G), -1.0/G);
//  DBG << "effit eval =" << exp(result);
  return exp(result);
}

double Effit::derivative(double x) const
{
  return x;
}

std::string Effit::to_string() const
{
  std::stringstream ss;
  //ss << "exp( ((A + B*x + C*x^2)^-G + (D + E*y + F*y^2)^-G)^(-1/G) )";
//  ss << "A=" << A;
//  ss << " B=" << B;
//  ss << " C=" << C;
//  ss << " D=" << D;
//  ss << " E=" << E;
//  ss << " F=" << F;
//  ss << " G=" << G;
  return ss.str();
}

std::string Effit::to_UTF8(int precision, bool with_rsq) const
{
  std::stringstream ss;
  //ss << "exp( ((A + B*x + C*x^2)^-G + (D + E*y + F*y^2)^-G)^(-1/G) )";
  return ss.str();
}

std::string Effit::to_markup(int precision, bool with_rsq) const
{
  std::stringstream ss;
  //ss << "exp( ((A + B*x + C*x^2)^-G + (D + E*y + F*y^2)^-G)^(-1/G) )";
  return ss.str();
}
