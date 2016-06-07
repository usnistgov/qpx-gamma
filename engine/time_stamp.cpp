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

#include "time_stamp.h"
#include <sstream>

namespace Qpx {

std::string TimeStamp::to_string() const {
  std::stringstream ss;
  ss << time_native_ << "x(" << timebase_multiplier_ << "/" << timebase_divider_ << ")";
  return ss.str();
}

void TimeStamp::from_xml(const pugi::xml_node &node)
{
  *this = TimeStamp();
  if (std::string(node.name()) != xml_element_name())
    return;
  if (node.attribute("timebase_mult"))
    timebase_multiplier_ = node.attribute("timebase_mult").as_uint(1);
  if (node.attribute("timebase_div"))
    timebase_divider_    = node.attribute("timebase_div").as_uint(1);
}

void TimeStamp::to_xml(pugi::xml_node &root) const
{
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());
  node.append_attribute("timebase_mult").set_value(std::to_string(timebase_multiplier_).c_str());
  node.append_attribute("timebase_div").set_value(std::to_string(timebase_divider_).c_str());
}

}
