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

#include "TF1.h"
#include "TH1.h"
#include "TGraphErrors.h"

namespace Qpx {

class OptimizerROOT : public Optimizer
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

  //General:
  static std::string def(uint16_t num, const FitParam&);
  static void set(TF1* f, uint16_t num, const FitParam &);
  static UncertainDouble get(TF1 *f, uint16_t num);

  static TH1D* fill_hist(const std::vector<double> &x,
                         const std::vector<double> &y);


  //CoefFunction:
  static uint16_t set_params(const CoefFunction&, TF1* f, uint16_t start);
  static uint16_t get_params(CoefFunction&, TF1* f, uint16_t start);
  static std::string def_of(CoefFunction* t);
  static std::string define_chain(const CoefFunction& func, const std::string &element);
  static std::string def_Polynomial(const CoefFunction& p);
  static std::string def_PolyLog(const CoefFunction& p);
  static std::string def_SqrtPoly(const CoefFunction& p);
  static std::string def_LogInverse(const CoefFunction& p);
  static std::string def_Effit(const CoefFunction& p);

  //Gaussian:
  static std::string def(const Gaussian& gaussian, uint16_t start = 0);
  static std::string def(const Gaussian& gaussian, uint16_t a, uint16_t c, uint16_t w);

  static void set_params(const Gaussian& gaussian, TF1* f, uint16_t start = 0);
  static void set_params(const Gaussian& gaussian, TF1* f, uint16_t a, uint16_t c, uint16_t w);
  static void get_params(Gaussian& gaussian, TF1* f, uint16_t start = 0);
  static void get_params(Gaussian& gaussian, TF1* f, uint16_t a, uint16_t c, uint16_t w);

  static void initial_sanity(Gaussian &gaussian,
                             double xmin, double xmax,
                             double ymin, double ymax);

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

  static void set_params(const Hypermet& hyp, TF1* f, uint16_t start = 0);
  static void set_params(const Hypermet& hyp, TF1* f, uint16_t w, uint16_t others_start);
  static void get_params(Hypermet& hyp, TF1* f, uint16_t start = 0);
  static void get_params(Hypermet& hyp, TF1* f, uint16_t w, uint16_t start);

  static void sanity_check(Hypermet &hyp,
                           double xmin, double xmax,
                           double ymin, double ymax);

  static void constrain_center(Hypermet &gaussian, double slack);

  static std::vector<Hypermet> fit_multi_variw(const std::vector<double> &x,
                                               const std::vector<double> &y,
                                               std::vector<Hypermet> old,
                                               Polynomial &background,
                                               FitSettings settings
                                              );

  static std::vector<Hypermet> fit_multi_commonw(const std::vector<double> &x,
                                                 const std::vector<double> &y,
                                                 std::vector<Hypermet> old,
                                                 Polynomial &background,
                                                 FitSettings settings
                                                );

};

}
