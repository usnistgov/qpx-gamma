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

#ifndef UNIT_CONVERTER_H
#define UNIT_CONVERTER_H

#include <vector>
#include <map>
#include <string>
#include <boost/multiprecision/cpp_dec_float.hpp>

typedef boost::multiprecision::number<boost::multiprecision::cpp_dec_float<16> > PreciseFloat;

class UnitConverter {
public:
  UnitConverter();

  std::string strip_unit(std::string full_unit) const;
  std::string get_prefix(std::string full_unit) const;

  PreciseFloat convert_units(PreciseFloat value, std::string from, std::string to) const;
  PreciseFloat convert_prefix(PreciseFloat value, std::string from, std::string to) const;

  std::map<int32_t, std::string> make_ordered_map(std::string stripped_unit, PreciseFloat min_SI_value, PreciseFloat max_SI_value) const;

  void add_prefix(std::string prefix, PreciseFloat factor);

  std::map<std::string, PreciseFloat> prefix_values;
  std::vector<std::string> prefix_values_indexed;

};

#endif
