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
 *      Units -
 *
 ******************************************************************************/

#include "units.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include "custom_logger.h"

UnitConverter::UnitConverter() {
//  add_prefix("P", 1E12f);
//  add_prefix("T", 1E9f);
  add_prefix("M", 1E6f);
  add_prefix("k", 1E3f);
  prefix_values_indexed.push_back("");
//  add_prefix("c", 1E-2f);
  add_prefix("m", 1E-3f);
  add_prefix("\u03BC", 1E-6f);
  prefix_values["u"] = 1E-6f;
  add_prefix("n", 1E-9f);
  add_prefix("p", 1E-12f);
//  add_prefix("f", 1E-15f);
}

void UnitConverter::add_prefix(std::string prefix, PreciseFloat factor) {
  if (!prefix_values.count(prefix)) {
    prefix_values[prefix] = factor;
    prefix_values_indexed.push_back(prefix);
  }
}

std::map<int32_t, std::string> UnitConverter::make_ordered_map(std::string default_unit,
                                                               PreciseFloat min_SI_value,
                                                               PreciseFloat max_SI_value) const {

  std::string stripped_unit = strip_unit(default_unit);
  std::string default_prefix = get_prefix(default_unit);

  if (min_SI_value < 0)
    min_SI_value = -min_SI_value;

  if (max_SI_value < 0)
    max_SI_value = -max_SI_value;

  std::map<int32_t, std::string> newmap;
  for (std::size_t i=0; i < prefix_values_indexed.size(); ++i) {
    bool min_criterion = (convert_prefix(min_SI_value, default_prefix, prefix_values_indexed[i]) >= 0.00001);
    bool max_criterion = (convert_prefix(max_SI_value, default_prefix, prefix_values_indexed[i]) <= 999999);
    if ((min_SI_value && min_criterion && max_SI_value && max_criterion)
        || (!min_SI_value && max_SI_value && max_criterion)
        || (!max_SI_value && min_SI_value && min_criterion)) {
    std::string full_unit = prefix_values_indexed[i] + stripped_unit;
    newmap[i] = full_unit;
    }
  }

  return newmap;
}



PreciseFloat UnitConverter::convert_units(PreciseFloat value, std::string from, std::string to) const {
  boost::algorithm::trim(from);
  boost::algorithm::trim(to);
  if (from == to)
    return value;

  if (strip_unit(from) != strip_unit(to))
    return value;

  return convert_prefix(value, get_prefix(from), get_prefix(to));
}

std::string UnitConverter::get_prefix(std::string full_unit) const {
  std::u32string ucs4 = boost::locale::conv::utf_to_utf<char32_t>(full_unit);

  std::u32string prefix = ucs4;
  if (ucs4.size() > 1)
    prefix = ucs4.substr(0, 1);
  return boost::locale::conv::utf_to_utf<char>(prefix);
}

std::string UnitConverter::strip_unit(std::string full_unit) const {
  std::u32string ucs4 = boost::locale::conv::utf_to_utf<char32_t>(full_unit);

  std::u32string stripped_unit = ucs4;
  if (ucs4.size() > 1)
    stripped_unit = ucs4.substr(1, ucs4.size()-1);
  return boost::locale::conv::utf_to_utf<char>(stripped_unit);
}

PreciseFloat UnitConverter::convert_prefix(PreciseFloat value, std::string from, std::string to) const {
  if (!from.empty() && prefix_values.count(from))
    value *= prefix_values.at(from);

  if (!to.empty() && prefix_values.count(to))
    value /= prefix_values.at(to);

  return value;
}
