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

#ifndef SQRT_POLY_H
#define SQRT_POLY_H

#include "coef_function.h"

class SqrtPoly : public CoefFunction
{
public:
  SqrtPoly() {}
  SqrtPoly(std::vector<double> coeffs, double uncert, double rsq) :
    CoefFunction(coeffs, uncert, rsq) {}

  std::string type() const override {return "SqrtPoly";}
  std::string to_string() const override;
  std::string to_UTF8(int precision = -1, bool with_rsq = false) const override;
  std::string to_markup(int precision = -1, bool with_rsq = false) const override;
  double eval(double x) const override;
  double derivative(double x) const override;
};

#endif
