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

#include "optimizer.h"

namespace Qpx {

class OptimizerCeres : public Optimizer
{
public:

  void fit(std::shared_ptr<CoefFunction> func,
          const std::vector<double> &x,
          const std::vector<double> &y,
          const std::vector<double> &x_sigma,
          const std::vector<double> &y_sigma) override;

  void fit(Gaussian& gaussian,
           const std::vector<double> &x,
           const std::vector<double> &y) override;

  std::vector<Gaussian> fit_multiplet(const std::vector<double> &x,
                                      const std::vector<double> &y,
                                      std::vector<Gaussian> old,
                                      Polynomial &background,
                                      FitSettings settings) override;

  std::vector<Hypermet> fit_multiplet(const std::vector<double> &x,
                                      const std::vector<double> &y,
                                      std::vector<Hypermet> old,
                                      Polynomial &background,
                                      FitSettings settings) override;

private:

  struct GaussianResidual {
    GaussianResidual(double x, double y)
        : x_(x)
        , y_(y)
    {}

    template <typename T>
    bool operator()(const T* const a, const T* const c, const T* const w, T* residual) const {
      residual[0] = T(y_) - a[0] * exp(- (T(x_) - c[0]) / (w[0] * w[0]));
      return true;
    }

   private:
    // Observations for a sample.
    const double x_;
    const double y_;
  };

};

}
