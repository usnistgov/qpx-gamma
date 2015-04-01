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
 *      Isotope
 *
 ******************************************************************************/


#ifndef ISOTOPE_H_
#define ISOTOPE_H_

#include <list>
#include <string>
#include <iomanip>
#include "xmlable.h"

namespace RadTypes {


static std::string dbl2str(double d)
{
  std::stringstream ss;
  ss << std::fixed << std::setprecision(10) << d;
  std::string s = ss.str();
  s.erase(s.find_last_not_of('0') + 1, std::string::npos);
  return (s[s.size()-1] == '.') ? s.substr(0, s.size()-1) : s;
}

  
class AbstractRadiation : public XMLable {
 public:
  AbstractRadiation() : energy(0), abundance(0) {}
  AbstractRadiation(double en, double ab) : energy(en), abundance(ab) {}
  
  void to_xml(tinyxml2::XMLPrinter&) const;
  void from_xml(tinyxml2::XMLElement*) override;
  std::string xml_element_name() const {return "radiation_type";}

  bool shallow_equals(const AbstractRadiation& other) const {return (energy == other.energy);}
  bool operator!= (const AbstractRadiation& other) const {return !(this->operator==(other));}
  bool operator== (const AbstractRadiation& other) const {
    if (energy != other.energy) return false;
    if (abundance != other.abundance) return false;
    return true;
  }
  
  double energy;
  double abundance;
};

class Beta : public AbstractRadiation {
public:
  Beta() : AbstractRadiation() {}
  Beta(double en, double ab) : AbstractRadiation(en, ab) {}
  std::string xml_element_name() const override {return "beta";}
};

class Gamma : public AbstractRadiation {
public:
  Gamma() : AbstractRadiation() {}
  Gamma(double en, double ab) : AbstractRadiation(en, ab) {}
  std::string xml_element_name() const override {return "gamma";}
};
  
class Isotope : public XMLable {
 public:
  Isotope () : gammas("gammas"), half_life(0.0) {}
  Isotope (std::string nm) : Isotope() {name = nm;}

  std::string xml_element_name() const {return "isotope";}
  
  void to_xml(tinyxml2::XMLPrinter&) const;
  void from_xml(tinyxml2::XMLElement*) override;

  bool shallow_equals(const Isotope& other) const {return (name == other.name);}
  bool operator!= (const Isotope& other) const {return !(this->operator==(other));}
  bool operator== (const Isotope& other) const {
    if (name != other.name) return false;
    if (half_life != other.half_life) return false;
    if (gamma_constant != other.gamma_constant) return false;
    if (beta != other.beta) return false;
    if (gammas != other.gammas) return false;
    return true;
  }
  
  std::string name;
  double half_life;
  std::string gamma_constant;
  Beta beta;
  XMLableDB<Gamma> gammas;
};

}

#endif
