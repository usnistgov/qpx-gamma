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
 *      Types for organizing data aquired from Device
 *        Qpx::Hit        single energy event with coincidence flags
 *
 ******************************************************************************/

#include "hit.h"
#include <sstream>

namespace Qpx {

void Hit::from_xml(const pugi::xml_node &node)
{
  *this = Hit();
  if (std::string(node.name()) != xml_element_name())
    return;
  source_channel = node.attribute("channel").as_int();
  energy = DigitizedVal(0, node.attribute("energy_bits").as_int());
  if (node.child(timestamp.xml_element_name().c_str()))
    timestamp.from_xml(node.child(timestamp.xml_element_name().c_str()));
  if (node.attribute("trace_length"))
    trace.resize(node.attribute("trace_length").as_uint(0), 0);
}

void Hit::to_xml(pugi::xml_node &root) const
{
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());
  node.append_attribute("channel").set_value(std::to_string(source_channel).c_str());
  node.append_attribute("energy_bits").set_value(std::to_string(energy.bits()).c_str());
  if (trace.size())
    node.append_attribute("trace_length").set_value(std::to_string(trace.size()).c_str());
  timestamp.to_xml(node);
}

std::string Hit::to_string() const
{
  std::stringstream ss;
  ss << "[ch" << source_channel << "|t" << timestamp.to_string() << "|e" << energy.to_string() << "]";
  return ss.str();
}

}
