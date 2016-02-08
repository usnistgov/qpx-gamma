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
 *      Gaussian
 *
 ******************************************************************************/

#include "gaussian.h"

#include <sstream>
#include <iomanip>
#include <numeric>

#include "fityk_util.h"
#include "custom_logger.h"

#include <boost/lexical_cast.hpp>


bool Gaussian::extract_params(fityk::Func* func) {
  try {
    if (func->get_template_name() != "Gauss2")
      return false;

    center_ = func->get_param_value("cc");
    height_ = func->get_param_value("hh");
    hwhm_   = log(2) * func->get_param_value("ww");
//        PL_DBG << "Gaussian fit as  c=" << center_ << " h=" << height_ << " w=" << width_;
  }
  catch ( ... ) {
    PL_DBG << "Gaussian could not extract parameters from Fityk";
    return false;
  }
  return true;
}

Gaussian::Gaussian(const std::vector<double> &x, const std::vector<double> &y):
  Gaussian() {
  std::vector<double> sigma;
//  for (auto &q : y) {
//    sigma.push_back(1/sqrt(q));
//  }
  sigma.resize(x.size(), 1);

  bool success = true;

  if ((x.size() < 1) || (x.size() != y.size()))
    return;

  std::vector<fityk::Func*> fns;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);

//    try {
//      f->execute("set fitting_method = nlopt_nm");
//      f->execute("set fitting_method = nlopt_lbfgs");
//    } catch ( ... ) {
//      success = false;
//      PL_DBG << "failed to set fitter";
//    }

  try {
    f->execute("guess Gaussian");
  }
  catch ( ... ) {
    PL_DBG << "Gaussian could not guess";
    success = false;
  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    PL_DBG << "Gaussian could not fit";
    success = false;
  }

  if (success) {
    std::vector<fityk::Func*> fns = f->all_functions();
    if (fns.size()) {
      fityk::Func* lastfn = fns.back();
      center_ = lastfn->get_param_value("center");
      height_ = lastfn->get_param_value("height");
      hwhm_   = lastfn->get_param_value("hwhm");
//      PL_DBG << "Gaussian fit as c=" << center_ << " h=" << height_ << " w=" << hwhm_;
    }
    rsq = f->get_rsquared(0);
  }

  delete f;
}

std::vector<Gaussian> Gaussian::fit_multi(const std::vector<double> &x, const std::vector<double> &y, const std::vector<Gaussian> &old) {
  std::vector<double> sigma;
//  for (auto &q : y)
//    sigma.push_back(1/sqrt(q));
  sigma.resize(x.size(), 1);

  bool success = true;
  std::vector<fityk::Func*> fns;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);


      try {
//        f->execute("set fitting_method = mpfit");
//        f->execute("set fitting_method = nlopt_nm");
        f->execute("set fitting_method = nlopt_lbfgs");
      } catch ( std::string st ) {
        success = false;
        PL_DBG << "Gaussian multifit failed to set fitter " << st;
      }

  //PL_DBG << "<Gaussian> fitting multiplet [" << x[0] << "-" << x[x.size() - 1] << "]";

  try {
    f->execute(fityk_definition());
  } catch ( ... ) {
    success = false;
    PL_DBG << "Gaussian multifit failed to define";
  }


  double lateral_slack = (x[x.size() -1]  - x[0]) / old.size();

  for (int i=0; i < old.size(); ++i) {
    //std::string initial_c = FitykUtil::var_def("c", old[i].center_, x[0], x[x.size()-1], i);
    std::string initial_c = FitykUtil::var_def("c", old[i].center_, old[i].center_ - lateral_slack, old[i].center_ + lateral_slack, i);

    std::string initial_h = FitykUtil::var_def("h", old[i].height_, old[i].height_*0.70, old[i].height_*1.30, i);
    std::string initial_w = FitykUtil::var_def("w", old[i].hwhm_,  old[i].hwhm_*0.7,   old[i].hwhm_*1.3, i);
    std::string initial = "F += Gauss2("
        "$c" + boost::lexical_cast<std::string>(i) + ","
        "$h" + boost::lexical_cast<std::string>(i) + ","
        "$w" + boost::lexical_cast<std::string>(i) + ")";

//    PL_DBG << "Gaussian multifit setting up peak " << i;
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
      PL_DBG << "Gaussian multifit failed to set up initial locals for peak " << i;
      success = false;
    }
  }

//  for (int i=0; i < old.size(); ++i) {
//    std::string fn = "F += Gaussian(height=~" + std::to_string(old[i].height_)
//        + ", hwhm=~" + std::to_string(old[i].hwhm_)
//        + ", center=~" + std::to_string(old[i].center_) + ")";
//    try {
//      f->execute(fn.c_str());
//    }
//    catch ( ... ) {
//      //PL_DBG << "Fytik threw exception a";
//      success = false;
//    }
//  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    PL_DBG << "Gaussian multifit failed to fit";
    success = false;
  }

  std::vector<Gaussian> results;

  if (success) {
    std::vector<fityk::Func*> fns = f->all_functions();
    for (auto &q : fns) {
      Gaussian one;
      one.extract_params(q);
      results.push_back(one);
    }
//    for (auto &q : fns) {
//      Gaussian one;
//      one.center_ = q->get_param_value("center");
//      one.height_ = q->get_param_value("height");
//      one.hwhm_   = q->get_param_value("hwhm");
//      one.rsq = f->get_rsquared(0);
//      results.push_back(one);
//    }
  } else {
    results = old;
  }

  delete f;

  return results;
}


std::string Gaussian::fityk_definition() {
  return "define Gauss2(cc, hh, ww) = "
         "hh*("
         "   exp(-(xc/ww)^2)"
         ")"
         " where xc=(x-cc)";
}

double Gaussian::evaluate(double x) {
  return height_ * exp(-log(2.0)*(pow(((x-center_)/hwhm_),2)));
}

double Gaussian::area() {
  return height_ * hwhm_ * sqrt(M_PI / log(2.0));
}

std::vector<double> Gaussian::evaluate_array(std::vector<double> x) {
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
