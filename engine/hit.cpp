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

void HitModel::from_xml(const pugi::xml_node &node)
{
  *this = HitModel();
  if (std::string(node.name()) != xml_element_name())
    return;
  if (node.attribute("trace_length"))
    tracelength = node.attribute("trace_length").as_uint(0);
  if (node.child(timebase.xml_element_name().c_str()))
    timebase.from_xml(node.child(timebase.xml_element_name().c_str()));
  if (node.child("Values"))
  {
    pugi::xml_node valsnode = node.child("Values");
    for (auto &v : valsnode.children())
    {
      std::string name;
      if (v.attribute("name"))
        name = std::string(v.attribute("name").value());
      if (name.empty())
        continue;
      size_t idx = 0;
      if (v.attribute("index"))
        idx = v.attribute("index").as_uint(0);
      uint8_t bits = 0;
      if (v.attribute("bits"))
        bits = v.attribute("bits").as_uint(0);

      if (idx >= values.size())
        values.resize(idx + 1);
      if (idx >= idx_to_name.size())
        idx_to_name.resize(idx + 1);

      values[idx] = DigitizedVal(0, bits);
      idx_to_name[idx] = name;
      name_to_idx[name] = idx;
    }
  }
}

void HitModel::to_xml(pugi::xml_node &root) const
{
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());
  if (tracelength)
    node.append_attribute("trace_length").set_value(std::to_string(tracelength).c_str());
  if (values.size())
  {
    pugi::xml_node valsnode = node.append_child("Values");
    for (auto &v : name_to_idx)
    {
      pugi::xml_node valnode = valsnode.append_child("Value");
      valnode.append_attribute("name").set_value(v.first.c_str());
      valnode.append_attribute("index").set_value(std::to_string(v.second).c_str());
      valnode.append_attribute("bits").set_value(std::to_string(int(values.at(v.second).bits())).c_str());
    }
  }
  timebase.to_xml(node);
}

std::string HitModel::to_string() const
{
  std::stringstream ss;
  ss << "[timebase=" << timebase.to_string() << " ";
  for (auto &n : name_to_idx)
    ss << n.first << "(" << int(values.at(n.second).bits()) << "b) ";
  if (tracelength)
    ss << "trace_length=" << tracelength;
  ss << "]";
  return ss.str();
}

std::string Hit::to_string() const
{
  std::stringstream ss;
  ss << "[ch" << source_channel_ << "|t" << timestamp_.to_string();
  for (auto &v : values_)
    ss << v.to_string();
  ss << "]";
  return ss.str();
}

}
