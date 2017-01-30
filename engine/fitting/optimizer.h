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
 *      Optimizer
 *
 ******************************************************************************/

#ifndef QPX_OPTIMIZER_H
#define QPX_OPTIMIZER_H

#include "polynomial.h"
#include "polylog.h"
#include "sqrt_poly.h"
#include "log_inverse.h"
#include "effit.h"

#include "gaussian.h"
#include "hypermet.h"

#include "TF1.h"
#include "TH1.h"

namespace Qpx {

class Optimizer
{
public:
  static void fit(CoefFunction &func,
                  const std::vector<double> &x,
                  const std::vector<double> &y,
                  const std::vector<double> &x_sigma,
                  const std::vector<double> &y_sigma);

  void fit(Gaussian& gaussian, const std::vector<double> &x, const std::vector<double> &y);

  static std::vector<Gaussian> fit_multiplet(const std::vector<double> &x,
                                             const std::vector<double> &y,
                                             std::vector<Gaussian> old,
                                             Polynomial &background,
                                             FitSettings settings
                                            );

private:

  //General:
  static std::string def(uint16_t num, const FitParam&);
  static void set(TF1* f, uint16_t num, const FitParam&);
  static void get(TF1* f, uint16_t num, FitParam&);

  static TH1D* fill_hist(const std::vector<double> &x,
                         const std::vector<double> &y);


  //CoefFunction:
  static uint16_t set_params(const CoefFunction&, TF1* f, uint16_t start);
  static uint16_t get_params(CoefFunction&, TF1* f, uint16_t start);

  struct CoefFunctionVisitor
  {
    std::string definition(const Polynomial&);
    std::string definition(const PolyLog&);
    std::string definition(const SqrtPoly&);
    std::string definition(const LogInverse&);
    std::string definition(const Effit&);

    std::string definition(const CoefFunction&)
    { return ""; }

    std::string define_chain(const CoefFunction& func,
                             const std::string &element);
  };

  //Gaussian:
  static std::string def(const Gaussian& gaussian, uint16_t start = 0);
  static std::string def(const Gaussian& gaussian, uint16_t a, uint16_t c, uint16_t w);

  static void set_params(const Gaussian& gaussian, TF1* f, uint16_t start = 0);
  static void set_params(const Gaussian& gaussian, TF1* f, uint16_t a, uint16_t c, uint16_t w);
  static void get_params(Gaussian& gaussian, TF1* f, uint16_t start = 0);
  static void get_params(Gaussian& gaussian, TF1* f, uint16_t a, uint16_t c, uint16_t w);

  static void sanity_check(Gaussian &gaussian,
                           double xmin, double xmax,
                           double ymin, double ymax);

  static void constrain_center(Gaussian &gaussian, double slack);

  static std::vector<Gaussian> fit_multi_variw(const std::vector<double> &x,
                                               const std::vector<double> &y,
                                               std::vector<Gaussian> old,
                                               Polynomial &background,
                                               FitSettings settings
                                              );

  static std::vector<Gaussian> fit_multi_commonw(const std::vector<double> &x,
                                                 const std::vector<double> &y,
                                                 std::vector<Gaussian> old,
                                                 Polynomial &background,
                                                 FitSettings settings
                                                );


  //Hypermet:
  static std::string def(const Hypermet& h, uint16_t start = 0);
  static std::string def(const Hypermet& h, uint16_t width, uint16_t others_start);
  static std::string def_skew(const FitParam& ampl, const FitParam& slope,
                              const std::string& w, const std::string& xc,
                              const std::string& xcw, uint16_t idx);


};

}

#endif
