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
 *      FitParam -
 *
 ******************************************************************************/

#ifndef FIT_PARAM_H
#define FIT_PARAM_H

#include <string>
#include <limits>
#include "xmlable.h"
#include "UncertainDouble.h"

#include "TF1.h"

class FitParam : public XMLable
{
public:
  FitParam();
  FitParam(std::string name, double v);
  FitParam(std::string name, double v, double lower, double upper);

  std::string name() const;
  UncertainDouble value() const;
  double lower() const;
  double upper() const;
  bool enabled() const;
  bool fixed() const;
  bool implicitly_fixed() const;

  void set_enabled(bool e);
  void set_name(std::string n);
  void set_value(UncertainDouble v);
  void set(const FitParam& other);
  void set(double min, double max, double val);
  void preset_bounds(double min, double max);
  void constrain(double min, double max);

  FitParam enforce_policy();

  bool same_bounds_and_policy(const FitParam &other) const;

  bool operator == (const FitParam &other) const {return value_ == other.value_;}
  bool operator < (const FitParam &other) const {return value_ < other.value_;}

  std::string to_string() const;

//XMLable
  void to_xml(pugi::xml_node &node) const override;
  void from_xml(const pugi::xml_node &node) override;
  std::string xml_element_name() const override {return "FitParam";}

//ROOT fitter
  void set(TF1* f, uint16_t num) const;
  void get(TF1* f, uint16_t num);

private:
  std::string name_;
  UncertainDouble value_;
  double lower_ {std::numeric_limits<double>::min()};
  double upper_ {std::numeric_limits<double>::max()};
  bool enabled_ {true};
  bool fixed_ {false};
};

#endif
