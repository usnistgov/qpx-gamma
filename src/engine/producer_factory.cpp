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
 ******************************************************************************/

#include "producer_factory.h"

namespace Qpx {

ProducerPtr ProducerFactory::create_type(std::string type, std::string file)
{
  ProducerPtr instance;
  auto it = constructors.find(type);
  if (it != constructors.end())
    instance = ProducerPtr(it->second());
  if (instance.operator bool() && instance->load_setting_definitions(file))
    return instance;
  return ProducerPtr();
}

ProducerPtr ProducerFactory::create_json(std::string type, std::string file)
{
  ProducerPtr instance;
  auto it = constructors.find(type);
  if (it != constructors.end())
    instance = ProducerPtr(it->second());
  if (instance.operator bool() && instance->load_setting_json(file))
    return instance;
  return ProducerPtr();
}

void ProducerFactory::register_type(std::string name, std::function<Producer*(void)> typeConstructor)
{
  LINFO << "<ProducerFactory> registering source '" << name << "'";
  constructors[name] = typeConstructor;
}

const std::vector<std::string> ProducerFactory::types() {
  std::vector<std::string> all_types;
  for (auto &q : constructors)
    all_types.push_back(q.first);
  return all_types;
}

}

