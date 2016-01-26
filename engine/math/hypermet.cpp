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

#include "fityk.h"
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
    Llong_height_ = func->get_param_value("llong_h");
    Llong_slope_ = func->get_param_value("llong_s");
    step_height_ = func->get_param_value("step_h");
        PL_DBG << "Hypermet fit as  c=" << center_ << " h=" << height_ << " w=" << width_
               << " lsh=" << Lshort_height_ << " lss=" << Lshort_slope_
               << " rsh=" << Rshort_height_ << " rss=" << Rshort_slope_
               << " llh=" << Llong_height_ << " lls=" << Llong_slope_
               << " steph=" << step_height_;
  }
  catch ( ... ) {
    PL_DBG << "Hypermet could not extract parameters from Fityk";
    return false;
  }
  return true;
}

Hypermet::Hypermet(const std::vector<double> &x, const std::vector<double> &y, double h, double c, double w):
  Hypermet(h, c, w / sqrt(log(2.0))) {
  std::vector<double> sigma;
  //  for (auto &q : y) {
  //    sigma.push_back(1/sqrt(q));
  //  }
  sigma.resize(x.size(), 1);

  bool success = true;

  if ((x.size() < 1) || (x.size() != y.size()))
    return;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);

  try {
    //    f->execute("set fitting_method = mpfit");
//    f->execute("set fitting_method = nlopt_nm");
        f->execute("set fitting_method = nlopt_lbfgs");
  } catch ( ... ) {
    success = false;
    PL_DBG << "Fityk failed to set fitter";
  }

  try {
    f->execute(fityk_definition());
  } catch ( ... ) {
    success = false;
    PL_DBG << "Hypermet failed to define";
  }

  double lateral_slack = (x[x.size() -1]  - x[0]) / 10.0;

  std::string initial_c = FitykUtil::var_def("c", center_, center_ - lateral_slack, center_ + lateral_slack);

//  std::string initial_c = FitykUtil::var_def("c", center_, x[0], x[x.size()-1]);
  std::string initial_h = FitykUtil::var_def("h", height_, height_*0.90, height_*1.10);
  std::string initial_w = FitykUtil::var_def("w", width_, width_*0.7, width_*1.3);

  std::string initial_lsh = FitykUtil::var_def("lsh", 0.01, 0, 0.75);
  std::string initial_lss = FitykUtil::var_def("lss", 0.3, 0.3, 2);

  std::string initial_rsh = FitykUtil::var_def("rsh", 0.01, 0, 0.75);
  std::string initial_rss = FitykUtil::var_def("rss", 0.3, 0.3, 2);

  std::string initial_llh = FitykUtil::var_def("llh", 0.0001, 0, 0.015);
  std::string initial_lls = FitykUtil::var_def("lls", 2.5, 2.5, 50);

  std::string initial_step = FitykUtil::var_def("step", 0.00001, 0, 0.75);

  std::string initial = "F += Hypermet($c,$h,$w,$lsh,$lss,$llh,$lls,$step,$rsh,$rss)";

  //PL_DBG << "Hypermet initial c=" << center_ << " h=" << height_ << " w=" << width_;

  try {
    f->execute(initial_h);
    f->execute(initial_c);
    f->execute(initial_w);
    f->execute(initial_lsh);
    f->execute(initial_lss);
    f->execute(initial_rsh);
    f->execute(initial_rss);
    f->execute(initial_llh);
    f->execute(initial_lls);
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
//  for (auto &q : y) {
//    sigma.push_back(1/sqrt(q));
//  }
  sigma.resize(x.size(), 1);

  bool success = true;

  fityk::Fityk *f = new fityk::Fityk;
//  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);

  try {
    //    f->execute("set fitting_method = mpfit");
//    f->execute("set fitting_method = nlopt_nm");
        f->execute("set fitting_method = nlopt_lbfgs");
  } catch ( ... ) {
    success = false;
    PL_DBG << "Fityk failed to set fitter";
  }

  try {
    f->execute(fityk_definition());
  } catch ( ... ) {
    success = false;
    PL_DBG << "Hypermet failed to define";
  }

  std::string initial_lsh = FitykUtil::var_def("lsh", 0.001, 0, 0.75);
  std::string initial_lss = FitykUtil::var_def("lss", 0.3, 0.3, 2);

  std::string initial_rsh = FitykUtil::var_def("rsh", 0.001, 0, 0.75);
  std::string initial_rss = FitykUtil::var_def("rss", 0.3, 0.3, 2);

  std::string initial_llh = FitykUtil::var_def("llh", 0.00001, 0, 0.015);
  std::string initial_lls = FitykUtil::var_def("lls", 2.5, 2.5, 50);

  std::string initial_step = FitykUtil::var_def("step", 0.00001, 0, 0.75);


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

    double llh_upper = 2*f.Llong_height_;
    if (llh_upper > 0.015)
      llh_upper = 0.015;
    if (llh_upper < 0.001)
      llh_upper = 0.015;
    initial_llh = FitykUtil::var_def("llh", f.Llong_height_, 0, llh_upper);

    double lls_upper = 2.5 + 2*(f.Llong_slope_ - 2.5);
    if (lls_upper > 50)
      lls_upper = 50;
    if (lls_upper < 5)
      lls_upper = 50;
    initial_lls = FitykUtil::var_def("lls", f.Llong_slope_, 2.5, lss_upper);

//    std::string initial_w = FitykUtil::var_def("w", f.width_,  f.width_*0.7,   f.width_*1.3, i);

    PL_DBG << initial_lsh;
    PL_DBG << initial_lss;
    PL_DBG << initial_rsh;
    PL_DBG << initial_rss;
    PL_DBG << initial_llh;
    PL_DBG << initial_lls;
    PL_DBG << initial_step;
//    PL_DBG << initial_w;
  }

  try {
    f->execute(initial_lsh);
    f->execute(initial_lss);
    f->execute(initial_rsh);
    f->execute(initial_rss);
    f->execute(initial_llh);
    f->execute(initial_lls);
    f->execute(initial_step);


  } catch ( ... ) {
    success = false;
    PL_DBG << "Hypermet: multifit failed to set up initial globals";
  }

  double lateral_slack = (x[x.size() -1]  - x[0]) / old.size();

  for (int i=0; i < old.size(); ++i) {
    //std::string initial_c = FitykUtil::var_def("c", old[i].center_, x[0], x[x.size()-1], i);
    std::string initial_c = FitykUtil::var_def("c", old[i].center_, old[i].center_ - lateral_slack, old[i].center_ + lateral_slack, i);

    std::string initial_h = FitykUtil::var_def("h", old[i].height_, old[i].height_*0.70, old[i].height_*1.30, i);
    std::string initial_w = FitykUtil::var_def("w", old[i].width_,  old[i].width_*0.7,   old[i].width_*1.3, i);
    std::string initial = "F += Hypermet("
        "$c" + boost::lexical_cast<std::string>(i) + ","
        "$h" + boost::lexical_cast<std::string>(i) + ","
        "$w" + boost::lexical_cast<std::string>(i) + ","
        "$lsh,$lss,$llh,$lls,$step,$rsh,$rss)";

    PL_DBG << "Hypermet multifit setting up peak " << i;
    PL_DBG << initial_c;
    PL_DBG << initial_h;
    PL_DBG << initial_w;
    PL_DBG << initial;


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
      Hypermet one;
      one.extract_params(q);
      results.push_back(one);
    }
  }

  delete f;

  return results;
}

std::string Hypermet::fityk_definition() {
  return "define Hypermet(cc, hh, ww, lshort_h, lshort_s, llong_h, llong_s, step_h, rshort_h, rshort_s) = "
         "hh*("
         "   exp(-(xc/ww)^2)"
         " + 0.5*lshort_h*exp( (xc/lshort_s)) * erfc((0.5*ww/lshort_s) + xc/ww)"
         " + 0.5*rshort_h*exp( - (xc/rshort_s)) * erfc((0.5*ww/rshort_s) - xc/ww)"
         " + 0.5*llong_h*exp( (xc/llong_s)) * erfc((0.5*ww/llong_s) + xc/ww)"
         " + 0.5*step_h*erfc(xc/ww)"
         ")"
         " where xc=(x-cc)";
}


double Hypermet::evaluate(double x) {
  double xc = x - center_;

  double gaussian = exp(- pow(xc/width_, 2) );

//  double left_short = 0;
  double left_short = Lshort_height_ * 0.5 *
      exp( /*pow(0.5*width_/Lshort_slope_, 2) +*/ xc/width_) *
      erfc( 0.5*width_/Lshort_slope_ +  xc/Lshort_slope_ );

  double right_short = Rshort_height_ * 0.5 *
      exp( /*pow(0.5*width_/Rshort_slope_, 2) +*/ xc/width_) *
      erfc( 0.5*width_/Rshort_slope_ +  xc/Rshort_slope_ );

  double left_long = Llong_height_ * 0.5 *
      exp( /*pow(0.5*width_/Llong_slope_, 2) +*/ xc/width_) *
      erfc( 0.5*width_/Llong_slope_ +  xc/width_ );

  double step = step_height_ * 0.5 *
      erfc( xc/width_ );

  return height_ * (gaussian + left_short + left_long + step + right_short);
//    return height_ * exp(-log(2.0)*(pow(((x-center_)/width_),2)));
}

std::vector<double> Hypermet::evaluate_array(std::vector<double> x) {
  std::vector<double> y;
  for (auto &q : x) {
    double val = evaluate(q);
    if (val < 0)
      y.push_back(0);
    else
      y.push_back(val);
  }
  return y;
}
