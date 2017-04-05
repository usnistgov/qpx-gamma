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


void Optimizer::initial_sanity(Gaussian& gaussian,
                               double xmin, double xmax,
                               double ymin, double ymax)
{
  gaussian.bound_center(xmin, xmax);
  gaussian.bound_height(0, ymax-ymin);
  gaussian.bound_hwhm(0, xmax-xmin);
}

void Optimizer::sanity_check(Gaussian& gaussian,
                             double xmin, double xmax,
                             double ymin, double ymax)
{
  gaussian.constrain_center(xmin, xmax);
  gaussian.constrain_height(0, ymax-ymin);
  gaussian.constrain_hwhm(0, xmax-xmin);
}

void Optimizer::constrain_center(Gaussian &gaussian, double slack)
{
  double lateral_slack = slack * gaussian.hwhm().value().value() * 2 * sqrt(log(2));
  gaussian.constrain_center(gaussian.center().value().value() - lateral_slack,
                            gaussian.center().value().value() + lateral_slack);
}

void Optimizer::sanity_check(Hypermet& hyp,
                             double xmin, double xmax,
                             double ymin, double ymax)
{
  hyp.constrain_height(0, ymax-ymin);
  hyp.constrain_center(xmin, xmax);
  hyp.constrain_width(0, xmax-xmin);
}

void Optimizer::constrain_center(Hypermet &hyp, double slack)
{
  double lateral_slack = slack * hyp.width().value().value() * 2 * sqrt(log(2));
  hyp.constrain_center(hyp.center().value().value() - lateral_slack,
                       hyp.center().value().value() + lateral_slack);
}


}
