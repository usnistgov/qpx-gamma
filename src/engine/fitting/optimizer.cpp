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

#include "optimizer.h"
#include "custom_logger.h"

namespace Qpx {

OptimizerPtr OptimizerFactory::create_any() const
{
  if (!constructors.empty())
    return create_type(constructors.begin()->first);
  return OptimizerPtr();
}


OptimizerPtr OptimizerFactory::create_type(std::string type) const
{
  OptimizerPtr instance;
  auto it = constructors.find(type);
  if (it != constructors.end())
    instance = OptimizerPtr(it->second());
  if (instance.operator bool())
    return instance;
  return OptimizerPtr();
}

void OptimizerFactory::register_type(std::string name, std::function<Optimizer*(void)> typeConstructor)
{
  LINFO << "<OptimizerFactory> registering Optimizer '" << name << "'";
  constructors[name] = typeConstructor;
}

std::vector<std::__cxx11::string> OptimizerFactory::types() const {
  std::vector<std::string> all_types;
  for (auto &q : constructors)
    all_types.push_back(q.first);
  return all_types;
}

}
