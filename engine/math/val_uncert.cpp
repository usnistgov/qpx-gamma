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
 *      FitykUtil -
 *
 ******************************************************************************/

#include "val_uncert.h"
#include <boost/lexical_cast.hpp>
#include <iomanip>

std::string ValUncert::to_string(bool errpct) const {
  std::string ret = boost::lexical_cast<std::string>(val) +
      "\u00B1" + boost::lexical_cast<std::string>(uncert);
  if (errpct)
    ret += " (" + err(2) + ")";
  return ret;
}

std::string ValUncert::val_uncert(uint16_t precision) const
{
  std::stringstream ss;
  ss << std::setprecision( precision ) << val << " \u00B1 " << uncert;
  return ss.str();
}

double ValUncert::err() const
{
  double ret = std::numeric_limits<double>::infinity();
  if (!std::isnan(val) && (val != 0))
    ret = uncert / val * 100.0;
  return ret;
}

std::string ValUncert::err(uint16_t precision) const
{
  std::stringstream ss;
  ss << std::setprecision( precision ) << err() << "%";
  return ss.str();
}


ValUncert ValUncert::merge(const std::list<ValUncert> &list)
{
  double sum = 0;
  for (auto& l : list)
    sum += l.val;
  double avg = (sum) / list.size();
  double min = avg;
  double max = avg;
  for (auto& l : list) {
    if (!std::isnan(l.uncert) && !std::isinf(l.uncert)) {
      min = std::min(min, l.val - 0.5 * l.uncert);
      max = std::max(max, l.val + 0.5 * l.uncert);
    }
  }

  ValUncert ret;
  ret.uncert = max - min;
  ret.val = (max + min) * 0.5;
  return ret;
}

bool ValUncert::almost(const ValUncert &other) const
{
  bool ret = false;
  if (!std::isnan(uncert) && (std::abs(val - other.val) <= uncert))
    ret = true;
  if (!std::isnan(other.uncert) && (std::abs(val - other.val) <= other.uncert))
    ret = true;
  return ret;
}




