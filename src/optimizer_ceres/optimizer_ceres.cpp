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

namespace Qpx {

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
