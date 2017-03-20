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

#pragma once

#include "polynomial.h"
#include "polylog.h"
#include "sqrt_poly.h"
#include "log_inverse.h"
#include "effit.h"

#include "gaussian.h"
#include "hypermet.h"

#include "TF1.h"
#include "TH1.h"


#include "TGraphErrors.h"
#include "custom_logger.h"

namespace Qpx {

class Optimizer
{
private:

  struct CoefFunctionVisitor
  {
    inline std::string operator() (const Polynomial& p) const
    {
      std::string xc = "x";
      if (p.xoffset().value().finite() && p.xoffset().value().value())
        xc = "(x-" + std::to_string(p.xoffset().value().value()) + ")";

      return define_chain(p, xc);
    }

    inline std::string operator() (const PolyLog& p) const
    {
      std::string xc = "TMath::Log(x)";
      if (p.xoffset().value().finite() && p.xoffset().value().value())
        xc = "TMath::Log(x-" + std::to_string(p.xoffset().value().value()) + ")";

      return "TMath::Exp(" + define_chain(p, xc) + ")";
    }

    inline std::string operator() (const SqrtPoly& p) const
    {
      std::string xc = "x";
      if (p.xoffset().value().finite() && p.xoffset().value().value())
        xc = "(x-" + std::to_string(p.xoffset().value().value()) + ")";

      return "TMath::Sqrt(" + define_chain(p, xc) + ")";
    }

    inline std::string operator() (const LogInverse& p) const
    {
      std::string xc = "(1.0/x)";
      if (p.xoffset().value().finite() && p.xoffset().value().value())
        xc = "1.0/(x-" + std::to_string(p.xoffset().value().value()) + ")";

      return "TMath::Exp(" + define_chain(p, xc) + ")";
    }

    inline std::string operator() (const Effit& p) const
    {
      std::string definition = "((d + e*xb + f*(xb^2))^(-20))^(-0.05) where xb=ln(x/1000)";
      //   x= ((A + B*ln(x/100.0) + C*(ln(x/100.0)^2))^(-G) + (D + E*ln(x/1000.0) + F*(ln(x/1000.0)^2))^(1-G))^(1-(1/G))
      //pow(pow(A + B*xa + C*xa*xa,-G) + pow(D + E*xb + F*xb*xb,-G), -1.0/G);
    //  if (G==0)
    //      G = 20;
      return definition;
    }

    std::string define_chain(const CoefFunction& func,
                             const std::string &element) const;
  };

  template <typename T>
  inline static std::string def_of(const T& t)
  {
    CoefFunctionVisitor v;
    return v(t);
  }

public:

  template <typename T>
  static void fit(T &func,
                  const std::vector<double> &x,
                  const std::vector<double> &y,
                  const std::vector<double> &x_sigma,
                  const std::vector<double> &y_sigma)
  {
    if (x.size() != y.size())
      return;

    TGraphErrors* g1;
    if ((x.size() == x_sigma.size()) && (x_sigma.size() == y_sigma.size()))
      g1 = new TGraphErrors(y.size(),
                            x.data(), y.data(),
                            x_sigma.data(), y_sigma.data());
    else
      g1 = new TGraphErrors(y.size(),
                            x.data(), y.data(),
                            nullptr, nullptr);

    std::string def = def_of(func);

    DBG << "Def = " << def;

    TF1* f1 = new TF1("f1", def.c_str());
    set_params(func, f1, 0);

    g1->Fit("f1", "QEMN");

    get_params(func, f1, 0);
    func.set_chi2(f1->GetChisquare());

    f1->Delete();
    g1->Delete();
  }

  static void fit(Gaussian& gaussian, const std::vector<double> &x, const std::vector<double> &y);

  static std::vector<Gaussian> fit_multiplet(const std::vector<double> &x,
                                             const std::vector<double> &y,
                                             std::vector<Gaussian> old,
                                             Polynomial &background,
                                             FitSettings settings
                                            );

  static std::vector<Hypermet> fit_multiplet(const std::vector<double> &x,
                                             const std::vector<double> &y,
                                             std::vector<Hypermet> old,
                                             Polynomial &background,
                                             FitSettings settings
                                            );

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
