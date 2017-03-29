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

#pragma once

#include "coef_function.h"
#include "gaussian.h"
#include "hypermet.h"
#include <memory>

namespace Qpx {

class Optimizer
{
public:

  virtual void fit(std::shared_ptr<CoefFunction> func,
                   const std::vector<double> &x,
                   const std::vector<double> &y,
                   const std::vector<double> &x_sigma,
                   const std::vector<double> &y_sigma) = 0;

  virtual void fit(Gaussian& gaussian,
                  const std::vector<double> &x,
                  const std::vector<double> &y) = 0;

  virtual std::vector<Gaussian> fit_multiplet(const std::vector<double> &x,
                                             const std::vector<double> &y,
                                             std::vector<Gaussian> old,
                                             Polynomial &background,
                                             FitSettings settings) = 0;

  virtual std::vector<Hypermet> fit_multiplet(const std::vector<double> &x,
                                             const std::vector<double> &y,
                                             std::vector<Hypermet> old,
                                             Polynomial &background,
                                             FitSettings settings) = 0;
};

using OptimizerPtr = std::shared_ptr<Optimizer>;


class OptimizerFactory {
public:
  static OptimizerFactory& getInstance()
  {
    static OptimizerFactory singleton_instance;
    return singleton_instance;
  }

  OptimizerPtr create_any() const;
  OptimizerPtr create_type(std::string type) const;
  void register_type(std::string name, std::function<Optimizer*(void)> typeConstructor);
  std::vector<std::string> types() const;

private:
  std::map<std::string, std::function<Optimizer*(void)>> constructors;

  //singleton assurance
  OptimizerFactory() {}
  OptimizerFactory(OptimizerFactory const&);
  void operator=(OptimizerFactory const&);
};

template<class T>
class OptimizerRegistrar {
public:
  OptimizerRegistrar(std::string name)
  {
    OptimizerFactory::getInstance().register_type(name,
                                               [](void) -> Optimizer * { return new T();});
  }
};

}
