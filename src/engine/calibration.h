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
 ******************************************************************************/

#pragma once

#include <boost/date_time.hpp>
#include "xmlable.h"
#include "coef_function.h"

#include "json.hpp"
using namespace nlohmann;

namespace Qpx {

class Calibration : public XMLable
{
private:
  boost::posix_time::ptime calib_date_
    {boost::posix_time::microsec_clock::universal_time()};
  std::string units_;
  std::string to_;
  uint16_t bits_ {0};
  std::shared_ptr<CoefFunction> function_;

public:
  Calibration() {}
  Calibration(uint16_t bbits);

  bool valid() const;
  double transform(double) const;
  double transform(double, uint16_t) const;
  std::vector<double> transform(const std::vector<double>&, uint16_t) const;
  double inverse_transform(double) const;
  double inverse_transform(double, uint16_t) const;

  uint16_t bits() const;
  std::string units() const;
  std::string to() const;
  std::string model() const;
  boost::posix_time::ptime calib_date() const;
  std::string debug() const;
  std::string fancy_equation(int precision = -1, bool with_rsq=false) const;

  void set_units(const std::string& u);
  void set_to(const std::string& t);
  void set_function(std::shared_ptr<CoefFunction> f);
  void set_function(const std::string& type, const std::vector<double>& coefs);

  bool shallow_equals(const Calibration& other) const
  {return ((bits_ == other.bits_) && (to_ == other.to_));}
  bool operator!= (const Calibration& other) const;
  bool operator== (const Calibration& other) const;

  //XMLable
  void to_xml(pugi::xml_node &node) const override;
  void from_xml(const pugi::xml_node &node) override;
  std::string xml_element_name() const override {return "Calibration";}

  friend void to_json(json& j, const Calibration &s);
  friend void from_json(const json& j, Calibration &s);

  std::string coefs_to_string() const;
  static std::vector<double> coefs_from_string(const std::string&);
};

}
