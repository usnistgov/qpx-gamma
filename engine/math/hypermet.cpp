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


bool Hypermet::extract_params(fityk::Func* func) {
  try {
    if (func->get_template_name() != "Hypermet")
      return false;

    center_ = func->get_param_value("cc");
    height_ = func->get_param_value("hh");
    width_   = func->get_param_value("ww");
    Lshort_height_ = func->get_param_value("lshort_h");
    Lshort_slope_ = func->get_param_value("lshort_s");
    Rshort_height_ = func->get_param_value("rshort_h");
    Rshort_slope_ = func->get_param_value("rshort_s");
//    Llong_height_ = func->get_param_value("llong_h");
//    Llong_slope_ = func->get_param_value("llong_s");
    step_height_ = func->get_param_value("step_h");
//        PL_DBG << "Hypermet fit as  c=" << center_ << " h=" << height_ << " w=" << width_
//               << " lsh=" << Lshort_height_ << " lss=" << Lshort_slope_
//               << " rsh=" << Rshort_height_ << " rss=" << Rshort_slope_
//               << " llh=" << Llong_height_ << " lls=" << Llong_slope_
//               << " steph=" << step_height_;
  }
  catch ( ... ) {
    PL_DBG << "Hypermet could not extract parameters from Fityk";
    return false;
  }
  return true;
}

Hypermet::Hypermet(const std::vector<double> &x, const std::vector<double> &y, double h, double c, double w):
  Hypermet(h, c, w) {
  std::vector<double> sigma;
    for (auto &q : y) {
      sigma.push_back(1/sqrt(q));
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

  std::string initial_c = FitykUtil::var_def("c", center_, center_ - lateral_slack, center_ + lateral_slack);

//  std::string initial_c = FitykUtil::var_def("c", center_, x[0], x[x.size()-1]);
  std::string initial_h = FitykUtil::var_def("h", height_, height_*0.003, height_*3000);
  std::string initial_w = FitykUtil::var_def("w", width_, width_*0.7, width_*1.3);

  std::string initial_lsh = FitykUtil::var_def("lsh", 1.0e-10, 1.0e-10, 0.75);
  std::string initial_lss = FitykUtil::var_def("lss", 0.5, 0.3, 2);

  std::string initial_rsh = FitykUtil::var_def("rsh", 1.0e-10, 1.0e-10, 0.75);
  std::string initial_rss = FitykUtil::var_def("rss", 0.5, 0.3, 2);

//  std::string initial_llh = FitykUtil::var_def("llh", 0.0000, 0, 0.015);
//  std::string initial_lls = FitykUtil::var_def("lls", 2.5, 2.5, 50);

  std::string initial_step = FitykUtil::var_def("step", 1.0e-10, 1.0e-10, 0.75);

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
    std::vector<fityk::Func*> fns = f->all_functions();
    if (fns.size()) {
      fityk::Func* func = fns.back();
      extract_params(func); //discard return value
    }
    rsq_ = f->get_rsquared(0);
  }


  delete f;
}


std::vector<Hypermet> Hypermet::fit_multi(const std::vector<double> &x,
                                          const std::vector<double> &y,
                                          const std::vector<Hypermet> &old,
                                          Polynomial &background,
                                          Gamma::Calibration cali_nrg,
                                          Gamma::Calibration cali_fwhm
                                          ) {

  std::vector<Hypermet> results;
  if (old.empty())
    return results;

  std::vector<double> sigma;
  for (auto &q : y) {
    sigma.push_back(1/sqrt(q));
  }

  bool success = true;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);

  try {
    f->execute("set fitting_method = nlopt_lbfgs");
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



  std::string initial_lsh = FitykUtil::var_def("lsh", 1.0e-10, 1.0e-10, 0.75);
  std::string initial_lss = FitykUtil::var_def("lss", 0.5, 0.3, 2);

  std::string initial_rsh = FitykUtil::var_def("rsh", 1.0e-10, 1.0e-10, 0.75);
  std::string initial_rss = FitykUtil::var_def("rss", 0.5, 0.3, 2);

//  std::string initial_llh = FitykUtil::var_def("llh", 0.00000, 0, 0.015);
//  std::string initial_lls = FitykUtil::var_def("lls", 2.5, 2.5, 50);

  std::string initial_step = FitykUtil::var_def("step", 1.0e-10, 1.0e-10, 0.75);

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

  for (int i=0; i < old.size(); ++i) {
    double width_expected = old[i].width_;

    if (cali_fwhm.valid() && cali_nrg.valid()) {
      double fwhm_expected = cali_fwhm.transform(cali_nrg.transform(old[i].center_));
      double L = cali_nrg.inverse_transform(cali_nrg.transform(old[i].center_) - fwhm_expected/2);
      double R = cali_nrg.inverse_transform(cali_nrg.transform(old[i].center_) + fwhm_expected/2);
      width_expected = (R - L) / (2* sqrt(log(2)));
    }

    double width_lower = width_expected * 0.7;
    double width_upper = width_expected * 1.3;

    if ((old[i].width_ > width_lower) && (old[i].width_ < width_upper)) {
//      PL_DBG << "    Hypermet peak c=" << old[i].center_
//             << "    w=" << old[i].width_ << "  on "
//             << " (" << width_lower << "-" << width_upper << ")";
      width_expected = old[i].width_;
    } else {
//      PL_DBG << "    Hypermet peak c=" << old[i].center_
//             << "    w=" << old[i].width_ << "  on "
//             << " (" << width_lower << "-" << width_upper << ")"
//             << "      Not in range!";
    }

    std::string initial_c = FitykUtil::var_def("c", old[i].center_, old[i].center_ - lateral_slack, old[i].center_ + lateral_slack, i);
    std::string initial_h = FitykUtil::var_def("h", old[i].height_, old[i].height_*1.0e-10, old[i].height_*1.0e10, i);
    std::string initial_w = FitykUtil::var_def("w", width_expected,  width_lower, width_upper, i);
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
  }
  try {
    f->execute("fit");
  }
  catch ( ... ) {
    PL_DBG << "Hypermet multifit failed to fit";
    success = false;
  }

  if (success) {
    std::vector<fityk::Func*> fns = f->all_functions();
    for (auto &q : fns) {
      if (q->get_template_name() == "Hypermet") {
        Hypermet one;
        one.extract_params(q);
        one.rsq_ = f->get_rsquared(0);
        if (one.width_ > 0)
          results.push_back(one);
      } else if (q->get_template_name() == "MyPoly") {
        background.extract_params(q, {0,1,2});
      }
    }
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
  if (width_ == 0)
    return 0;

  double xc = x - center_;

  double gaussian = exp(- pow(xc/width_, 2) );

  double left_short = 0;
  double lexp = exp(pow(0.5*width_/Lshort_slope_, 2) + xc/Lshort_slope_);
  if ((Lshort_slope_ != 0) && !isinf(lexp))
    left_short = Lshort_height_ * lexp *
        erfc( 0.5*width_/Lshort_slope_ + xc/width_);

  double right_short = 0;
  double rexp = exp(pow(0.5*width_/Rshort_slope_, 2) - xc/Rshort_slope_);
  if ((Rshort_slope_ != 0) && !isinf(rexp))
    right_short = Rshort_height_ * rexp *
      erfc( 0.5*width_/Rshort_slope_  - xc/width_);

  double ret = height_ * (gaussian + 0.5 * (left_short + right_short) );

  return ret;
//    return height_ * exp(-log(2.0)*(pow(((x-center_)/width_),2)));
}

double Hypermet::eval_step_tail(double x) {
  if (width_ == 0)
    return 0;

  double xc = x - center_;

  double step = step_height_ * erfc( xc/width_ );

  double left_long = 0;
//  double left_long = Llong_height_ * 0.5 *
//      exp( /*pow(0.5*width_/Llong_slope_, 2) +*/ xc/width_) *
//      erfc( 0.5*width_/Llong_slope_ +  xc/width_ );

  return height_ * 0.5 * (step + left_long);
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
  return height_ * width_ * sqrt(M_PI) *
      (1 +
       Lshort_height_ * width_ * Lshort_slope_ +
       Rshort_height_ * width_ * Rshort_slope_);
}
