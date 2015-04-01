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
 *      Pixie::Spectrum::Template defines parameters for new spectrum
 *
 ******************************************************************************/

#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "spectrum_template.h"
#include "xmlable.h"

namespace Pixie {
namespace Spectrum {

void Template::to_xml(tinyxml2::XMLPrinter& printer) const {
  std::stringstream patterndata;
  
  printer.OpenElement("PixieSpectrumTemplate");
  printer.PushAttribute("type", type.c_str());

  printer.OpenElement("Name");
  printer.PushText(name_.c_str());
  printer.CloseElement();

  printer.OpenElement("Appearance");
  printer.PushText(std::to_string(appearance).c_str());
  printer.CloseElement();

  printer.OpenElement("Visible");
  printer.PushText(std::to_string(visible).c_str());
  printer.CloseElement();

  printer.OpenElement("Resolution");
  printer.PushText(std::to_string(bits).c_str());
  printer.CloseElement();

  printer.OpenElement("MatchPattern");
  for (int i = 0; i < max_chans; i++)
    patterndata << static_cast<short>(match_pattern[i]) << " ";
  printer.PushText(boost::algorithm::trim_copy(patterndata.str()).c_str());
  printer.CloseElement();

  patterndata.str(std::string()); //clear it
  printer.OpenElement("AddPattern");
  for (int i = 0; i < max_chans; i++)
    patterndata << static_cast<short>(add_pattern[i]) << " ";
  printer.PushText(boost::algorithm::trim_copy(patterndata.str()).c_str());
  printer.CloseElement();

  if (generic_attributes.size()) {
    printer.OpenElement("GenericAttributes");
    for (auto &q: generic_attributes)
      q.to_xml(printer);
    printer.CloseElement();
  }
  
  printer.CloseElement(); //PixieSpectrum
}

void Template::from_xml(tinyxml2::XMLElement* root) {
  tinyxml2::XMLElement* elem;
  std::string numero;

  if (root->Attribute("type") != nullptr)
    type = root->Attribute("type");
  if ((elem = root->FirstChildElement("Name")) != nullptr)
    name_ = elem->GetText();
  if ((elem = root->FirstChildElement("Resolution")) != nullptr)
    bits = boost::lexical_cast<short>(std::string(elem->GetText()));
  if ((elem = root->FirstChildElement("Appearance")) != nullptr)
    appearance = boost::lexical_cast<unsigned int>(std::string(elem->GetText()));
  if ((elem = root->FirstChildElement("Visible")) != nullptr)
    visible = boost::lexical_cast<bool>(std::string(elem->GetText()));
  if ((elem = root->FirstChildElement("MatchPattern")) != nullptr) {
    std::stringstream pattern_match(elem->GetText());
    for (int i = 0; i < max_chans; i++) {
      pattern_match >> numero;
      match_pattern[i] = boost::lexical_cast<short>(boost::algorithm::trim_copy(numero));
    }
  }
  if ((elem = root->FirstChildElement("AddPattern")) != nullptr) {
    std::stringstream pattern_add(elem->GetText());
    for (int i = 0; i < max_chans; i++) {
      pattern_add >> numero;
      add_pattern[i] = boost::lexical_cast<short>(boost::algorithm::trim_copy(numero));
    }
  }
  if ((elem = root->FirstChildElement("GenericAttributes")) != nullptr) {
    tinyxml2::XMLElement* one_setting = elem->FirstChildElement("Setting");
    while (one_setting != nullptr) {
      generic_attributes.push_back(Setting(one_setting));
      one_setting = dynamic_cast<tinyxml2::XMLElement*>(one_setting->NextSibling());
    }
  }
}          

}
}
