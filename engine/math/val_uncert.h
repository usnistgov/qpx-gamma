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
 *      ValUncert -
 *
 ******************************************************************************/

#ifndef FIT_VAL_UNCERT_H
#define FIT_VAL_UNCERT_H

#include <limits>
#include <string>
#include <numeric>
#include <cmath>
#include <list>

class ValUncert {

public:
  ValUncert() :
    ValUncert( nan("") )
  {}

  ValUncert(double v, double unc = std::numeric_limits<double>::infinity()) :
    val(v),
    uncert(unc)
  {}

  inline ValUncert operator+(const ValUncert& other) const
    { return ValUncert (val + other.val, sqrt(uncert*uncert + other.uncert*other.uncert)); }
  inline ValUncert operator+(const double& other) const
    { return ValUncert (val + other, uncert); }

  inline ValUncert operator-(const ValUncert& other) const
    { return ValUncert (val - other.val, sqrt(uncert*uncert + other.uncert*other.uncert)); }
  inline ValUncert operator-(const double& other) const
    { return ValUncert (val - other, uncert); }

  inline ValUncert operator*(const ValUncert& other) const
    { return ValUncert (val * other.val,
                        sqrt(val*val*other.uncert*other.uncert
                             + other.val*other.val*uncert*uncert)); }
  inline ValUncert operator*(const double& other) const
    { return ValUncert (val * other, uncert * abs(val)); }

  inline ValUncert operator/(const ValUncert& other) const
    { return ValUncert (val / other.val,
                        sqrt(val*val*other.uncert*other.uncert
                             + other.val*other.val*uncert*uncert)); }
  inline ValUncert operator/(const double& other) const
    { return ValUncert (val / other, uncert * abs(val)); }

  static ValUncert merge (const std::list<ValUncert> &list);
  bool almost (const ValUncert &other) const;
  bool operator == (const ValUncert &other) const {return val == other.val;}
  bool operator < (const ValUncert &other) const {return val < other.val;}
  bool operator > (const ValUncert &other) const {return val > other.val;}


  std::string to_string(bool errpct = true) const;
  std::string val_uncert(uint16_t precision) const;
  std::string err(uint16_t precision) const;
  double err() const;

  double val, uncert;

};

#endif
