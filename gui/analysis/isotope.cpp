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

#include "isotope.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

namespace RadTypes {

void AbstractRadiation::to_xml(pugi::xml_node &node) const {
  pugi::xml_node child = node.append_child();
  child.set_name(this->xml_element_name().c_str());

  child.append_child("energy");
  child.last_child().append_child(pugi::node_pcdata).set_value(dbl2str(energy).c_str());
  child.append_child("abundance");
  child.last_child().append_child(pugi::node_pcdata).set_value(dbl2str(abundance).c_str());
}

void AbstractRadiation::from_xml(const pugi::xml_node &node) {
  std::string str;

  str = std::string(node.child_value("energy"));
  if (!str.empty())
    energy = boost::lexical_cast<double>(boost::algorithm::trim_copy(str));

  str = std::string(node.child_value("abundance"));
  if (!str.empty())
    abundance = boost::lexical_cast<double>(boost::algorithm::trim_copy(str));
}


void Isotope::to_xml(pugi::xml_node &node) const {
  pugi::xml_node child = node.append_child();
  child.set_name(this->xml_element_name().c_str());

  child.append_child("name").append_child(pugi::node_pcdata).set_value(name.c_str());

  if (gamma_constant.size()) {
    child.append_child("gammaConstant");
    child.last_child().append_child(pugi::node_pcdata).set_value(gamma_constant.c_str());
  }

  child.append_child("halfLife");
  child.last_child().append_child(pugi::node_pcdata).set_value(dbl2str(half_life).c_str());

  if (!gammas.empty())
    gammas.to_xml(child);

  if (beta.energy > 0)
    beta.to_xml(child);
}

void Isotope::from_xml(const pugi::xml_node &node) {
  name = std::string(node.child_value("name"));
  gamma_constant = std::string(node.child_value("gammaConstant"));
  std::string hl(node.child_value("halfLife"));
  if (!hl.empty())
    half_life = boost::lexical_cast<double>(hl);
  gammas.from_xml(node.child(gammas.xml_element_name().c_str()));
  beta.from_xml(node.child(beta.xml_element_name().c_str()));
}

}
