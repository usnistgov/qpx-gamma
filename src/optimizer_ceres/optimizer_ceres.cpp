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

#include "optimizer_ceres.h"
#include "custom_logger.h"

#include "ceres/ceres.h"
#include "glog/logging.h"
using ceres::AutoDiffCostFunction;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solver;
using ceres::Solve;

namespace Qpx {

inline int init_logger()
{
  google::InitGoogleLogging("Qpx::OptimizerCeres");
  return 1;
}

static const int initializer = init_logger();

struct GaussianResidual
{
  GaussianResidual(double x, double y)
      : x_(x)
      , y_(y)
  {}

  template <typename T>
  bool operator()(const T* const a,
                  const T* const c,
                  const T* const w,
                  T* residual) const
  {
    residual[0] = T(y_) - a[0] * exp(- (T(x_) - c[0]) / (w[0] * w[0]));
    return true;
  }

 private:
  // Observations for a sample.
  const double x_;
  const double y_;
};


struct PolyResidual
{
  PolyResidual(double x, double y)
      : x_(x)
      , y_(y)
  {}

  template <typename T>
  bool operator()(const T* const params,
                  T* residual) const
  {
    residual[0] = params[0];
    return true;
  }

 private:
  // Observations for a sample.
  const double x_;
  const double y_;
};



static OptimizerRegistrar<OptimizerCeres> registrar(std::string("Ceres"));

void OptimizerCeres::fit(std::shared_ptr<CoefFunction> func,
                         const std::vector<double> &x,
                         const std::vector<double> &y,
                         const std::vector<double> &x_sigma,
                         const std::vector<double> &y_sigma)
{
  if (!func || (x.size() != y.size()))
    return;

  if ((x.size() == x_sigma.size()) && (x_sigma.size() == y_sigma.size()))
  {

  }
  else
  {

  }
}


void OptimizerCeres::fit(Gaussian& gaussian,
                         const std::vector<double> &x,
                         const std::vector<double> &y)
{
  if (x.size() != y.size())
    return;

  auto a = gaussian.height().value().value();
  auto c = gaussian.center().value().value();
  auto w = gaussian.hwhm().value().value();
  Problem problem;
  for (int i = 0; i < x.size(); ++i)
  {
    problem.AddResidualBlock(
          new AutoDiffCostFunction<GaussianResidual, 1, 1, 1, 1>(
            new GaussianResidual(x[i], y[i])),
          NULL,
          &a, &c, &w);
  }
  Solver::Options options;
  options.max_num_iterations = 25;
  options.linear_solver_type = ceres::DENSE_QR;
  options.minimizer_progress_to_stdout = true;
  Solver::Summary summary;
  Solve(options, &problem, &summary);
  DBG << summary.BriefReport();
  gaussian.set_height(UncertainDouble(a, 0, 4));
  gaussian.set_center(UncertainDouble(c, 0, 4));
  gaussian.set_hwhm(UncertainDouble(w, 0, 4));
  DBG << "<OptimizerCeres::fit> Result: " << gaussian.to_string();
}

std::vector<Gaussian> OptimizerCeres::fit_multiplet(const std::vector<double> &x,
                                                    const std::vector<double> &y,
                                                    std::vector<Gaussian> old,
                                                    Polynomial &background,
                                                    FitSettings settings)
{
  bool use_w_common = (settings.width_common &&
                       settings.cali_fwhm_.valid() &&
                       settings.cali_nrg_.valid());

//  if (use_w_common)
//    return fit_multi_commonw(x,y, old, background, settings);
//  else
//    return fit_multi_variw(x,y, old, background, settings);
  return std::vector<Gaussian>();
}

std::vector<Hypermet> OptimizerCeres::fit_multiplet(const std::vector<double> &x,
                                                    const std::vector<double> &y,
                                                    std::vector<Hypermet> old,
                                                    Polynomial &background,
                                                    FitSettings settings)
{
  bool use_w_common = (settings.width_common &&
                       settings.cali_fwhm_.valid() &&
                       settings.cali_nrg_.valid());

//  if (use_w_common)
//    return fit_multi_commonw(x,y, old, background, settings);
//  else
//    return fit_multi_variw(x,y, old, background, settings);
  return std::vector<Hypermet>();
}

}
