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
#include "fityk.h"
#include "custom_logger.h"
#include "qpx_util.h"

Effit::Effit(std::vector<double> coeffs, double center)
  :Effit()
{
  xoffset_ = center;
  if (coeffs.size() > 6) {
    A = coeffs[0];
    B = coeffs[1];
    C = coeffs[2];
    D = coeffs[3];
    E = coeffs[4];
    F = coeffs[5];
    G = coeffs[6];
  }
  if (G==0)
    G = 20;
}

Effit::Effit(std::vector<double> &x, std::vector<double> &y, double center)
  :Effit()
{
  fit(x, y, center);
}

void Effit::fit(std::vector<double> &x, std::vector<double> &y, double center) {

  xoffset_ = center;
  if (x.size() != y.size())
    return;

  std::vector<double> x_c, yy, sigma;
  sigma.resize(x.size(), 1);
  
  for (auto &q : x)
    x_c.push_back(q - center);

  for (auto &q : y)
    yy.push_back(log(q));


  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);

  f->load_data(0, x_c, yy, sigma);

  bool success = true;

//  std::string definition = "define Effit(a=~0,b=~1,c=~0,d=~0,e=~0,f=~0,g=~20) = ((a + b*xa + c*(xa^2))^(-g) + (d + e*xb + f*(xb^2))^(-g))^(1-(1/g)) where xa=ln(x/100), xb=ln(x/1000)";
//  std::string definition = "define Effit(a=~0,d=~0,e=~0,f=~0) = ((a + xa)^(-20) + (d + e*xb + f*(xb^2))^(-20))^(-0.05) where xa=ln(x/100), xb=ln(x/1000)";
  std::string definition = "define Effit(a=~0,d=~0,e=~0,f=~0) = ((d + e*xb + f*(xb^2))^(-20))^(-0.05) where xb=ln(x/1000)";

  DBG << "Definition: " << definition;

  try {
    f->execute(definition);
  } catch ( ... ) {
    success = false;
    DBG << "failed to define Effit";
  }

  try {
    f->execute("guess Effit");
  }
  catch ( ... ) {
    success = false;
    DBG << "cannot guess Effit";
  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    success = false;
    DBG << "cannot fit Effit";
  }

  if (success) {
    fityk::Func* lastfn = f->all_functions().back();
//    A = lastfn->get_param_value("a");
  //  B = lastfn->get_param_value("b");
//    C = lastfn->get_param_value("c");
    D = lastfn->get_param_value("d");
    E = lastfn->get_param_value("e");
    F = lastfn->get_param_value("f");
//    G = lastfn->get_param_value("g");
    rsq = f->get_rsquared(0);

    if (G==0)
      G = 20;

  }

  delete f;
}


double Effit::evaluate(double x) {
  double xa = log((x - xoffset_)/100);
  double xb = log((x - xoffset_)/1000);
//   x= ((A + B*ln(x/100.0) + C*(ln(x/100.0)^2))^(-G) + (D + E*ln(x/1000.0) + F*(ln(x/1000.0)^2))^(1-G))^(1-(1/G))
  double result = pow(pow(A + B*xa + C*xa*xa,-G) + pow(D + E*xb + F*xb*xb,-G), -1.0/G);
//  DBG << "effit eval =" << exp(result);
  return exp(result);
}

/*
Effit Effit::derivative() {
  Effit new_poly;
  new_poly.xoffset_ = xoffset_;
  if (degree_ > 0)
    new_poly.coeffs_.resize(coeffs_.size() - 1, 0);
  for (int i=1; i < coeffs_.size(); ++ i)
    new_poly.coeffs_[i-1] = i * coeffs_[i];
  new_poly.degree_ = degree_ - 1;
  return new_poly;
}

double Effit::inverse_evaluate(double y, double e) {
  int i=0;
  double x0=1;
  Effit deriv = derivative();
  double x1 = x0 + (y - evaluate(x0)) / (deriv.evaluate(x0));
  while( i<=100 && std::abs(x1-x0) > e)
  {
    x0 = x1;
    x1 = x0 + (y - evaluate(x0)) / (deriv.evaluate(x0));
    i++;
  }

  double x_adjusted = x1 - xoffset_;

  if(std::abs(x1-x0) <= e)
    return x_adjusted;

  else
  {
    WARN <<"Maximum iteration reached in Effit inverse evaluation";
    return nan("");
  }
}
*/


std::vector<double> Effit::evaluate_array(std::vector<double> x) {
  std::vector<double> y;
  for (auto &q : x)
    y.push_back(evaluate(q));
  return y;
}

std::string Effit::to_string() {
  std::stringstream ss;
  //ss << "exp( ((A + B*x + C*x^2)^-G + (D + E*y + F*y^2)^-G)^(-1/G) )";
  ss << "A=" << A;
  ss << " B=" << B;
  ss << " C=" << C;
  ss << " D=" << D;
  ss << " E=" << E;
  ss << " F=" << F;
  ss << " G=" << G;
  return ss.str();
}

std::string Effit::to_UTF8(int precision, bool with_rsq) {
  std::stringstream ss;
  //ss << "exp( ((A + B*x + C*x^2)^-G + (D + E*y + F*y^2)^-G)^(-1/G) )";
  ss << "A=" << A;
  ss << " B=" << B;
  ss << " C=" << C;
  ss << " D=" << D;
  ss << " E=" << E;
  ss << " F=" << F;
  ss << " G=" << G;
  return ss.str();
}

std::string Effit::to_markup() {
  std::stringstream ss;
  //ss << "exp( ((A + B*x + C*x^2)^-G + (D + E*y + F*y^2)^-G)^(-1/G) )";
  ss << "A=" << A;
  ss << " B=" << B;
  ss << " C=" << C;
  ss << " D=" << D;
  ss << " E=" << E;
  ss << " F=" << F;
  ss << " G=" << G;
  return ss.str();
}

std::string Effit::coef_to_string() const{
  std::stringstream ss;
  //ss << "exp( ((A + B*x + C*x^2)^-G + (D + E*y + F*y^2)^-G)^(-1/G) )";
  ss << A;
  ss << " " << B;
  ss << " " << C;
  ss << " " << D;
  ss << " " << E;
  ss << " " << F;
  ss << " " << G;

  return boost::algorithm::trim_copy(ss.str());
}

std::vector<double> Effit::coeffs() {
  std::vector<double> coeffs(7);
  coeffs[0] = A;
  coeffs[1] = B;
  coeffs[2] = C;
  coeffs[3] = D;
  coeffs[4] = E;
  coeffs[5] = F;
  coeffs[6] = G;
  return coeffs;
}

void Effit::coef_from_string(std::string coefs) {
  std::stringstream dss(boost::algorithm::trim_copy(coefs));

  std::list<double> templist; double coef;
  while (dss.rdbuf()->in_avail()) {
    dss >> coef;
    templist.push_back(coef);
  }

  std::vector<double> coeffs(templist.size());

  int i=0;
  for (auto &q: templist) {
    coeffs[i] = q;
    i++;
  }

  if (coeffs.size() > 6) {
    A = coeffs[0];
    B = coeffs[1];
    C = coeffs[2];
    D = coeffs[3];
    E = coeffs[4];
    F = coeffs[5];
    G = coeffs[6];
  }
  if (G==0)
    G = 20;

}
