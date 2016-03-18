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
 *      Qpx::Spectrum::Template defines parameters for new spectrum
 *
 ******************************************************************************/

#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "spectrum_template.h"
#include "qpx_util.h"

namespace Qpx {
namespace Spectrum {

void Template::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_attribute("Type").set_value(type.c_str());
  node.append_child("Name").append_child(pugi::node_pcdata).set_value(name_.c_str());
  node.append_child("Resolution").append_child(pugi::node_pcdata).set_value(std::to_string(bits).c_str());

  if (attributes.branches.size())
    attributes.to_xml(node);
}

void Template::from_xml(const pugi::xml_node &node) {
  if (std::string(node.name()) != xml_element_name())
    return;

  type = std::string(node.attribute("Type").value());
  name_ = std::string(node.child_value("Name"));
  bits = boost::lexical_cast<short>(node.child_value("Resolution"));

  if (node.child(attributes.xml_element_name().c_str()))
    attributes.from_xml(node.child(attributes.xml_element_name().c_str()));

}

}
}
