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

void AbstractRadiation::to_xml(tinyxml2::XMLPrinter& printer) const {

  std::string elname = this->xml_element_name();
  printer.OpenElement(elname.c_str());

  printer.OpenElement("energy");
  printer.PushText(dbl2str(energy).c_str());
  printer.CloseElement();

  printer.OpenElement("abundance");
  printer.PushText(dbl2str(abundance).c_str());
  printer.CloseElement();

  printer.CloseElement();
}

void AbstractRadiation::from_xml(tinyxml2::XMLElement* root) {
  tinyxml2::XMLElement* el;
  std::string str;
  
  el = root->FirstChildElement("energy");
  if ((el != nullptr) && (str = std::string(el->GetText())).size())
    energy = boost::lexical_cast<double>(boost::algorithm::trim_copy(str));

  el = root->FirstChildElement("abundance");
  if ((el != nullptr) && (str = std::string(el->GetText())).size())
    abundance = boost::lexical_cast<double>(boost::algorithm::trim_copy(str));
}


void Isotope::to_xml(tinyxml2::XMLPrinter& printer) const {

  printer.OpenElement("isotope");

  printer.OpenElement("name");
  printer.PushText(name.c_str());
  printer.CloseElement();
      
  printer.OpenElement("gammaConstant");
  printer.PushText(gamma_constant.c_str());
  printer.CloseElement();

  printer.OpenElement("halfLife");
  printer.PushText(dbl2str(half_life).c_str());
  printer.CloseElement();

  if (!gammas.empty())
    gammas.to_xml(printer);

  if (beta.energy > 0)
    beta.to_xml(printer);

  printer.CloseElement();
}

void Isotope::from_xml(tinyxml2::XMLElement* root) {
  tinyxml2::XMLElement* NameData = root->FirstChildElement("name");
  if (NameData == NULL) return;
  name = std::string(NameData->GetText());
  
  tinyxml2::XMLElement* gConstData = root->FirstChildElement("gammaConstant");
  if (gConstData == NULL) return;
  gamma_constant = std::string(gConstData->GetText());

  tinyxml2::XMLElement* hlData = root->FirstChildElement("halfLife");
  if (hlData == NULL) return;
  half_life = boost::lexical_cast<double>(hlData->GetText());

  tinyxml2::XMLElement* gammaData = root->FirstChildElement(gammas.xml_element_name().c_str());
  if (gammaData == NULL) return;
  gammas.from_xml(gammaData);

  tinyxml2::XMLElement* betaData = root->FirstChildElement(beta.xml_element_name().c_str());
  if (betaData == NULL) return;
  beta.from_xml(betaData);
}

}
