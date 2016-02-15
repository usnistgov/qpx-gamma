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
 *      Hypermet function ensemble
 *
 ******************************************************************************/

#include "hypermet.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <boost/lexical_cast.hpp>

#include "fityk_util.h"
#include "custom_logger.h"


bool Hypermet::extract_params(fityk::Fityk* f, fityk::Func* func) {
  try {
    if (func->get_template_name() != "Hypermet")
      return false;

    center_.extract(f, func);
    height_.extract(f, func);
    width_.extract(f, func);
    Lskew_amplitude.extract(f, func);
    Lskew_slope.extract(f, func);
    Rskew_amplitude.extract(f, func);
    Rskew_slope.extract(f, func);
//    tail_amplitude = func->get_param_value("tail_h");
//    tail_slope = func->get_param_value("tail_s");
    step_amplitude.extract(f, func);
//        PL_DBG << "Hypermet fit as  c=" << center_ << " h=" << height_ << " w=" << width_
//               << " lsh=" << Lskew_amplitude << " lss=" << Lskew_slope
//               << " rsh=" << Rskew_amplitude << " rss=" << Rskew_slope
//               << " llh=" << tail_amplitude << " lls=" << tail_slope
//               << " steph=" << step_amplitude;
  }
  catch ( ... ) {
    PL_DBG << "Hypermet could not extract parameters from Fityk";
    return false;
  }
  return true;
}

Hypermet::Hypermet(const std::vector<double> &x, const std::vector<double> &y,
                   double h, double c, double w,
                    FitSettings settings):
  Hypermet(h, c, w, settings) {
  std::vector<double> sigma;
    for (auto &q : y) {
      sigma.push_back(sqrt(q));
    }
//  sigma.resize(x.size(), 1);

  bool success = true;

  if ((x.size() < 1) || (x.size() != y.size()))
    return;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);

  try {
//        f->execute("set fitting_method = mpfit");
//    f->execute("set fitting_method = nlopt_nm");
//    f->execute("set fitting_method = nlopt_bobyqa");
        f->execute("set fitting_method = nlopt_lbfgs");
  } catch ( ... ) {
    success = false;
    PL_DBG << "Hypermet failed to set fitter";
  }

  try {
    f->execute(fityk_definition());
  } catch ( ... ) {
    success = false;
    PL_DBG << "Hypermet failed to define";
  }

  double lateral_slack = (x[x.size() -1]  - x[0]) / 5.0;

  center_.lbound = center_.val - lateral_slack;
  center_.ubound = center_.val + lateral_slack;

  height_.lbound = height_.val * 0.003;
  height_.ubound = height_.val * 3000;

  width_.lbound = width_.val * 0.7;
  width_.ubound = width_.val * 1.3;

  std::string initial_c = "$c = " + center_.def_bounds();
  std::string initial_h = "$h = " + height_.def_bounds();
  std::string initial_w = "$w = " + width_.def_bounds();

  std::string initial_lsh = "$lsh = " + Lskew_amplitude.def_bounds();
  std::string initial_lss = "$lss = " + Lskew_slope.def_bounds();

  std::string initial_rsh = "$rsh = " + Rskew_amplitude.def_bounds();
  std::string initial_rss = "$rss = " + Rskew_slope.def_bounds();

//  std::string initial_llh = FitykUtil::var_def("llh", 0.0000, 0, 0.015);
//  std::string initial_lls = FitykUtil::var_def("lls", 2.5, 2.5, 50);

  std::string initial_step = "$step = " + step_amplitude.def_bounds();

  std::string initial = "F += Hypermet($c,$h,$w,$lsh,$lss,$rsh,$rss,$step)";

  //PL_DBG << "Hypermet initial c=" << center_ << " h=" << height_ << " w=" << width_;

  try {
    f->execute(initial_h);
    f->execute(initial_c);
    f->execute(initial_w);
    f->execute(initial_lsh);
    f->execute(initial_lss);
    f->execute(initial_rsh);
    f->execute(initial_rss);
//    f->execute(initial_llh);
//    f->execute(initial_lls);
    f->execute(initial_step);
    f->execute(initial);

  } catch ( ... ) {
    success = false;
    PL_DBG << "Hypermet: failed to set up initial";
  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    PL_DBG << "Hypermet could not fit";
    success = false;
  }

  if (success) {
    f->execute("set fitting_method = levenberg_marquardt");
    std::vector<fityk::Func*> fns = f->all_functions();
    if (fns.size()) {
      fityk::Func* func = fns.back();
      extract_params(f, func); //discard return value
    }
    rsq_ = f->get_rsquared(0);
  }


  delete f;
}


std::vector<Hypermet> Hypermet::fit_multi(const std::vector<double> &x,
                                          const std::vector<double> &y,
                                          std::vector<Hypermet> old,
                                          Polynomial &background,
                                          FitSettings settings)
{
  std::vector<Hypermet> results;
  if (old.empty())
    return results;

  std::vector<double> sigma;
  for (auto &q : y) {
    sigma.push_back(sqrt(q));
  }

  bool success = true;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);

  try {
    f->execute("set fitting_method = nlopt_lbfgs");
//    f->execute("set fitting_method = levenberg_marquardt");
  } catch ( ... ) {
    success = false;
    PL_DBG << "Hypermet multifit failed to set fitter";
  }

  try {
    f->execute(fityk_definition());
    f->execute(background.fityk_definition({0,1,2}, background.xoffset_));
  } catch ( ... ) {
    success = false;
    PL_DBG << "Hypermet multifit failed to define";
  }

  double xl = 0, xs = 0, xc = 0;
  if (background.coeffs_.size() > 0)
    xl = background.coeffs_[0];
  if (background.coeffs_.size() > 1)
    xs = background.coeffs_[1];
  if (background.coeffs_.size() > 2)
    xc = background.coeffs_[2];

  double rise = (x[x.size()-1] - x[0]) * xs;
  std::string initial_xl = FitykUtil::var_def("sxl", xl, xl-abs(rise), xl + 0.05 * abs(rise));
  std::string initial_xs = FitykUtil::var_def("sxs", xs, -1.5*abs(xs), 0);
  std::string initial_xc = FitykUtil::var_def("sxc", xc, 0, 0);


  std::string initial_lsh = "$lsh = " + old[0].Lskew_amplitude.def_bounds();
  std::string initial_lss = "$lss = " + old[0].Lskew_slope.def_bounds();

  std::string initial_rsh = "$rsh = " + old[0].Rskew_amplitude.def_bounds();
  std::string initial_rss = "$rss = " + old[0].Rskew_slope.def_bounds();

//  std::string initial_llh = FitykUtil::var_def("llh", 0.0000, 0, 0.015);
//  std::string initial_lls = FitykUtil::var_def("lls", 2.5, 2.5, 50);

  std::string initial_step = "$step = " + old[0].step_amplitude.def_bounds();

  //  PL_DBG << initial_lsh;
  //  PL_DBG << initial_lss;
  //  PL_DBG << initial_rsh;
  //  PL_DBG << initial_rss;
  //  PL_DBG << initial_step;

  try {
    f->execute(initial_lsh);
    f->execute(initial_lss);
    f->execute(initial_rsh);
    f->execute(initial_rss);
//    f->execute(initial_llh);
//    f->execute(initial_lls);
    f->execute(initial_step);

    f->execute(initial_xc);
    f->execute(initial_xs);
    f->execute(initial_xl);
    f->execute("F += MyPoly($sxl, $sxs, $sxc)");

  } catch ( ... ) {
    success = false;
    PL_DBG << "Hypermet: multifit failed to set up initial globals";
  }

  double lateral_slack = (x[x.size() -1]  - x[0]) / (old.size() * 12);

  int i=0;
  for (auto &o : old) {
    double width_expected = o.width_.val;

    if (settings.cali_fwhm_.valid() && settings.cali_nrg_.valid()) {
      double fwhm_expected = settings.cali_fwhm_.transform(settings.cali_nrg_.transform(o.center_.val));
      double L = settings.cali_nrg_.inverse_transform(settings.cali_nrg_.transform(o.center_.val) - fwhm_expected/2);
      double R = settings.cali_nrg_.inverse_transform(settings.cali_nrg_.transform(o.center_.val) + fwhm_expected/2);
      width_expected = (R - L) / (2* sqrt(log(2)));
    }

    double width_lower = width_expected * 0.7;
    double width_upper = width_expected * 1.3;

    if ((o.width_.val > width_lower) && (o.width_.val < width_upper)) {
//      PL_DBG << "    Hypermet peak c=" << o.center_
//             << "    w=" << o.width_ << "  on "
//             << " (" << width_lower << "-" << width_upper << ")";
      width_expected = o.width_.val;
    } else {
//      PL_DBG << "    Hypermet peak c=" << o.center_
//             << "    w=" << o.width_ << "  on "
//             << " (" << width_lower << "-" << width_upper << ")"
//             << "      Not in range!";
    }

    o.center_.lbound = o.center_.val - lateral_slack;
    o.center_.ubound = o.center_.val + lateral_slack;

    o.height_.lbound = o.height_.val * 1e-5;
    o.height_.ubound = o.height_.val * 1e5;

    o.width_.val = width_expected;
    o.width_.lbound = width_lower;
    o.width_.ubound = width_upper;



    std::string initial_c = "$c" + boost::lexical_cast<std::string>(i) + " = " +
        o.center_.def_bounds();
    std::string initial_h = "$h" + boost::lexical_cast<std::string>(i) + " = " +
        o.height_.def_bounds();
    std::string initial_w = "$w" + boost::lexical_cast<std::string>(i) + " = " +
        o.width_.def_bounds();

    std::string initial = "F += Hypermet("
        "$c" + boost::lexical_cast<std::string>(i) + ","
        "$h" + boost::lexical_cast<std::string>(i) + ","
        "$w" + boost::lexical_cast<std::string>(i) + ","
        "$lsh,$lss,"
                                                     "$rsh,$rss,$step)";
    try {
      f->execute(initial_h);
      f->execute(initial_c);
      f->execute(initial_w);
      f->execute(initial);
    }
    catch ( ... ) {
      PL_DBG << "Hypermet multifit failed to set up initial locals for peak " << i;
      success = false;
    }
    i++;
  }
  try {
    f->execute("fit");
  }
  catch ( ... ) {
    PL_DBG << "Hypermet multifit failed to fit";
    success = false;
  }

  if (success) {
//      try {
//    f->execute("info errors");
//    f->execute("info state > nl.fit");

//    f->execute("set fitting_method = levenberg_marquardt");
//    f->execute("fit 1");
//    f->execute("info errors");
//    } catch ( ... ) {
//      PL_DBG << "Hypermet multifit failed to do error thing";
//      success = false;
//    }

    std::vector<fityk::Func*> fns = f->all_functions();
    for (auto &q : fns) {
      if (q->get_template_name() == "Hypermet") {
        Hypermet one;
        one.extract_params(f, q);
        one.rsq_ = f->get_rsquared(0);
        if (one.width_.val > 0)
          results.push_back(one);
      } else if (q->get_template_name() == "MyPoly") {
        background.extract_params(q, {0,1,2});
      }
    }

//    f->execute("info state > lm.fit");
  }

  delete f;

  return results;
}

std::string Hypermet::fityk_definition() {
  return "define Hypermet(cc, hh, ww, lshort_h, lshort_s, rshort_h, rshort_s, step_h) = "
         "hh*("
         "   exp(-(xc/ww)^2)"
         " + 0.5 * ("
         "   lshort_h*exp((0.5*ww/lshort_s)^2 + (xc/lshort_s)) * erfc((0.5*ww/lshort_s) + xc/ww)"
         " + rshort_h*exp((0.5*ww/rshort_s)^2 - (xc/rshort_s)) * erfc((0.5*ww/rshort_s) - xc/ww)"
//         " + 0.5*llong_h*exp( (xc/llong_s)) * erfc((0.5*ww/llong_s) + xc/ww)"
         " + step_h * erfc(xc/ww)"
         " ) )"
         " where xc=(x-cc)";
}


double Hypermet::eval_peak(double x) {
  if (width_.val == 0)
    return 0;

  double xc = x - center_.val;

  double gaussian = exp(- pow(xc/width_.val, 2) );

  double left_short = 0;
  double lexp = exp(pow(0.5*width_.val/Lskew_slope.val, 2) + xc/Lskew_slope.val);
  if ((Lskew_slope.val != 0) && !isinf(lexp))
    left_short = Lskew_amplitude.val * lexp *
        erfc( 0.5*width_.val/Lskew_slope.val + xc/width_.val);

  double right_short = 0;
  double rexp = exp(pow(0.5*width_.val/Rskew_slope.val, 2) - xc/Rskew_slope.val);
  if ((Rskew_slope.val != 0) && !isinf(rexp))
    right_short = Rskew_amplitude.val * rexp *
      erfc( 0.5*width_.val/Rskew_slope.val  - xc/width_.val);

  double ret = height_.val * (gaussian + 0.5 * (left_short + right_short) );

  return ret;
//    return height_ * exp(-log(2.0)*(pow(((x-center_)/width_),2)));
}

double Hypermet::eval_step_tail(double x) {
  if (width_.val == 0)
    return 0;

  double xc = x - center_.val;

  double step = step_amplitude.val * erfc( xc/width_.val );

  double left_long = 0;
//  double left_long = tail_amplitude * 0.5 *
//      exp( /*pow(0.5*width_/tail_slope, 2) +*/ xc/width_) *
//      erfc( 0.5*width_/tail_slope +  xc/width_ );

  return height_.val * 0.5 * (step + left_long);
}

std::vector<double> Hypermet::peak(std::vector<double> x) {
  std::vector<double> y;
  for (auto &q : x)
      y.push_back(eval_peak(q));
  return y;
}

std::vector<double> Hypermet::step_tail(std::vector<double> x) {
  std::vector<double> y;
  for (auto &q : x)
      y.push_back(eval_step_tail(q));
  return y;
}

double Hypermet::area() {
  return height_.val * width_.val * sqrt(M_PI) *
      (1 +
       Lskew_amplitude.val * width_.val * Lskew_slope.val +
       Rskew_amplitude.val * width_.val * Rskew_slope.val);
}
