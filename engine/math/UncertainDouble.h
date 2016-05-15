//Rework this according to http://arxiv.org/abs/physics/0306138v1 !!!

#ifndef UNCERTAINDOUBLE_H
#define UNCERTAINDOUBLE_H

#include <string>
#include <list>
#include "xmlable.h"

class UncertainDouble : public XMLable
{
public:

  UncertainDouble();
  UncertainDouble(double val, double sigma, uint16_t sigf);

  static UncertainDouble from_int(int64_t val, double sigma);
  static UncertainDouble from_uint(uint64_t val, double sigma);
  static UncertainDouble from_double(double val, double sigma,
                                     uint16_t sigs_below = 1);

  static UncertainDouble from_nsdf(std::string val, std::string uncert);

//  UncertainDouble & operator=(const UncertainDouble & other);

  double value() const;
  double uncertainty() const;
  double error() const;

  uint16_t sigfigs() const;
  uint16_t sigdec() const;

  void setValue(double val);
  void setUncertainty(double sigma);
  void setSigFigs(uint16_t sig);
  void autoSigs(uint16_t sigs_below = 0);
  void constrainSigs(uint16_t max_sigs = 6);

  bool finite() const;

  std::string to_string(bool ommit_tiny = true) const;
//  std::string to_markup() const; // outputs formatted text
  std::string error_percent() const;

  std::string debug() const;

  UncertainDouble & operator*=(const double &other);
  UncertainDouble operator*(const double &other) const;
  UncertainDouble & operator/=(const double &other);
  UncertainDouble operator/(const double &other) const;

  UncertainDouble & operator*=(const UncertainDouble &other);
  UncertainDouble operator*(const UncertainDouble &other) const;
  UncertainDouble & operator/=(const UncertainDouble &other);
  UncertainDouble operator/(const UncertainDouble &other) const;

  UncertainDouble & operator+=(const UncertainDouble &other);
  UncertainDouble operator+(const UncertainDouble &other) const;
  UncertainDouble & operator-=(const UncertainDouble &other);
  UncertainDouble operator-(const UncertainDouble &other) const;

//  operator double() const;

  bool almost (const UncertainDouble &other) const;
  bool operator == (const UncertainDouble &other) const {return value() == other.value();}
  bool operator < (const UncertainDouble &other) const {return value() < other.value();}
  bool operator > (const UncertainDouble &other) const {return value() > other.value();}

  static UncertainDouble average(const std::list<UncertainDouble> &list);

  void to_xml(pugi::xml_node &node) const override;
  void from_xml(const pugi::xml_node &node) override;
  std::string xml_element_name() const override {return "UncertainDouble";}

private:
  double value_;
  double sigma_;
  uint16_t sigfigs_;

  UncertainDouble& additive_uncert(const UncertainDouble &other);
  UncertainDouble& multipli_uncert(const UncertainDouble &other);
  int exponent() const;
};

#endif // UNCERTAINDOUBLE_H
