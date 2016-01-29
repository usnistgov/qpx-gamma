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

  std::string initial_lsh = FitykUtil::var_def("lsh", 0.00, 0, 0.75);
  std::string initial_lss = FitykUtil::var_def("lss", 0.3, 0.3, 2);

  std::string initial_rsh = FitykUtil::var_def("rsh", 0.00, 0, 0.75);
  std::string initial_rss = FitykUtil::var_def("rss", 0.3, 0.3, 2);

//  std::string initial_llh = FitykUtil::var_def("llh", 0.0000, 0, 0.015);
//  std::string initial_lls = FitykUtil::var_def("lls", 2.5, 2.5, 50);

  std::string initial_step = FitykUtil::var_def("step", 0.00000, 0, 0.75);

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
    rsq = f->get_rsquared(0);
  }


  delete f;
}


std::vector<Hypermet> Hypermet::fit_multi(const std::vector<double> &x,
                                          const std::vector<double> &y,
                                          const std::vector<Hypermet> &old,
                                          bool constrain_to_first) {

  std::vector<Hypermet> results;
  if (old.empty())
    return results;

  std::vector<double> sigma;
  for (auto &q : y) {
    sigma.push_back(1/sqrt(q));
  }
//  sigma.resize(x.size(), 1);

  bool success = true;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);

  try {
//        f->execute("set fitting_method = mpfit");
//    f->execute("set fitting_method = nlopt_nm");
        f->execute("set fitting_method = nlopt_lbfgs");
  } catch ( ... ) {
    success = false;
    PL_DBG << "Hypermet multifit failed to set fitter";
  }

  double x_offset = x[0];
  std::string background_def = "define Poly3(bc, bs, bl) = bc*xx*xx + bs*xx + bl"
                               " where xx = (x - " + std::to_string(x_offset) + ")";

  try {
    f->execute(fityk_definition());
    f->execute(background_def);
  } catch ( ... ) {
    success = false;
    PL_DBG << "Hypermet multifit failed to define";
  }

  std::string initial_lsh = FitykUtil::var_def("lsh", 0.000, 0, 0.75);
  std::string initial_lss = FitykUtil::var_def("lss", 0.3, 0.3, 2);

  std::string initial_rsh = FitykUtil::var_def("rsh", 0.000, 0, 0.75);
  std::string initial_rss = FitykUtil::var_def("rss", 0.3, 0.3, 2);

//  std::string initial_llh = FitykUtil::var_def("llh", 0.00000, 0, 0.015);
//  std::string initial_lls = FitykUtil::var_def("lls", 2.5, 2.5, 50);

  std::string initial_step = FitykUtil::var_def("step", 0.00000, 0, 0.75);

  uint16_t samples = 5;
  if (x.size() < 5)
    samples = x.size();

  double min_L = y[0];
  for (int i=0; i < samples; ++i)
    min_L = std::min(min_L, y[i]);

  double min_R = y[y.size()-1];
  for (int i=y.size()-1; i >= y.size()-samples; --i)
    min_R = std::min(min_R, y[i]);


  double rise = min_R - min_L;
  double run = x[x.size()-1] - x[0];
  double slope = rise / run;
  double offset = min_L;

  double xc = 0;
  double xs = slope;
  double xl = offset;

  std::string initial_xc = FitykUtil::var_def("sxc", xc, 0, 0);
  std::string initial_xs = FitykUtil::var_def("sxs", xs, -1.5*abs(slope), 0);
  std::string initial_xl = FitykUtil::var_def("sxl", xl, xl-abs(rise), xl + 0.05 * abs(rise));


  if (false) {
    PL_DBG << "Using frist peak as template";

    Hypermet f = old[0];

    double lsh_upper = 2*f.Lshort_height_;
    if (lsh_upper > 0.75)
      lsh_upper = 0.75;
    if (lsh_upper < 0.1)
      lsh_upper = 0.57;
    initial_lsh = FitykUtil::var_def("lsh", f.Lshort_height_, 0, lsh_upper);

    double lss_upper = 0.3 + 2*(f.Lshort_slope_ - 0.3);
    if (lss_upper > 2)
      lss_upper = 2;
    if (lss_upper < 0.4)
      lss_upper = 2;
    initial_lss = FitykUtil::var_def("lss", f.Lshort_slope_, 0.3, lss_upper);

    double rsh_upper = 2*f.Rshort_height_;
    if (rsh_upper > 0.75)
      rsh_upper = 0.75;
    if (rsh_upper > 0.1)
      rsh_upper = 0.75;
    initial_rsh = FitykUtil::var_def("rsh", f.Rshort_height_, 0, rsh_upper);

    double rss_upper = 0.3 + 2*(f.Rshort_slope_ - 0.3);
    if (rss_upper > 2)
      rss_upper = 2;
    if (rss_upper < 0.4)
      rss_upper = 2;
    initial_rss = FitykUtil::var_def("rss", f.Rshort_slope_, 0.3, rss_upper);

//    double llh_upper = 2*f.Llong_height_;
//    if (llh_upper > 0.015)
//      llh_upper = 0.015;
//    if (llh_upper < 0.001)
//      llh_upper = 0.015;
//    initial_llh = FitykUtil::var_def("llh", f.Llong_height_, 0, llh_upper);

//    double lls_upper = 2.5 + 2*(f.Llong_slope_ - 2.5);
//    if (lls_upper > 50)
//      lls_upper = 50;
//    if (lls_upper < 5)
//      lls_upper = 50;
//    initial_lls = FitykUtil::var_def("lls", f.Llong_slope_, 2.5, lss_upper);

//    std::string initial_w = FitykUtil::var_def("w", f.width_,  f.width_*0.7,   f.width_*1.3, i);

    PL_DBG << initial_lsh;
    PL_DBG << initial_lss;
    PL_DBG << initial_rsh;
    PL_DBG << initial_rss;
//    PL_DBG << initial_llh;
//    PL_DBG << initial_lls;
    PL_DBG << initial_step;
//    PL_DBG << initial_w;
  }

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
    f->execute("F += Poly3($sxc, $sxs, $sxl)");

  } catch ( ... ) {
    success = false;
    PL_DBG << "Hypermet: multifit failed to set up initial globals";
  }

  double lateral_slack = (x[x.size() -1]  - x[0]) / (old.size() * 5);

  for (int i=0; i < old.size(); ++i) {
    //std::string initial_c = FitykUtil::var_def("c", old[i].center_, x[0], x[x.size()-1], i);
    std::string initial_c = FitykUtil::var_def("c", old[i].center_, old[i].center_ - lateral_slack, old[i].center_ + lateral_slack, i);

    std::string initial_h = FitykUtil::var_def("h", old[i].height_, old[i].height_*0.003, old[i].height_*3000, i);
    std::string initial_w = FitykUtil::var_def("w", old[i].width_,  old[i].width_*0.7,   old[i].width_*1.3, i);
    std::string initial = "F += Hypermet("
        "$c" + boost::lexical_cast<std::string>(i) + ","
        "$h" + boost::lexical_cast<std::string>(i) + ","
        "$w" + boost::lexical_cast<std::string>(i) + ","
        "$lsh,$lss,"
                                                     "$rsh,$rss,$step)";

//    PL_DBG << "Hypermet multifit setting up peak " << i;
//    PL_DBG << initial_c;
//    PL_DBG << initial_h;
//    PL_DBG << initial_w;
//    PL_DBG << initial;


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
        if (one.width_ > 0)
          results.push_back(one);
      } else if (q->get_template_name() == "Poly3") {
        xc = q->get_param_value("bc");
        xs = q->get_param_value("bs");
        xl = q->get_param_value("bl");
      }
    }
  }

  delete f;

  for (auto &q : results) {
//    PL_DBG << "Hypermet propagate background to results " << xc << " " << xs << " " << xl << " offset " << x_offset;
    q.x_offset_ = x_offset;
    q.xc_ = xc;
    q.xs_ = xs;
    q.xl_ = xl;
  }

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

double Hypermet::eval_poly(double x) {
  double xc = x - x_offset_;

  return xc_*xc*xc + xs_*xc + xl_;
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

std::vector<double> Hypermet::poly(std::vector<double> x) {
  std::vector<double> y;
  for (auto &q : x)
      y.push_back(eval_poly(q));
  return y;
}

double Hypermet::area() {
  return height_ * width_ * sqrt(M_PI) *
      (1 +
       Lshort_height_ * width_ * Lshort_slope_ +
       Rshort_height_ * width_ * Rshort_slope_);
}
