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
#include "custom_timer.h"

#include <boost/lexical_cast.hpp>

Gaussian::Gaussian()
  : height_("height", 0)
  , center_("center", 0)
  , hwhm_("hwhm", 0)
  , rsq_(-1)
{}

std::string Gaussian::fityk_definition() {
  return "define Gauss2(center, height, hwhm) = "
         "height*("
         "   exp(-(xc/hwhm)^2)"
         ")"
         " where xc=(x-center)";
}

bool Gaussian::extract_params(fityk::Fityk* f, fityk::Func* func) {
  try {
    if ((func->get_template_name() != "Gauss2")
      && (func->get_template_name() != "Gaussian"))
      return false;

    center_.extract(f, func);
    height_.extract(f, func);
    hwhm_.extract(f, func);

    if (func->get_template_name() == "Gauss2") {
      hwhm_.val *= log(2);
      hwhm_.uncert *= log(2);
    }

//        DBG << "Gaussian fit as  c=" << center_ << " h=" << height_ << " w=" << width_;
  }
  catch ( ... ) {
    DBG << "Gaussian could not extract parameters from Fityk";
    return false;
  }
  return true;
}

Gaussian::Gaussian(const std::vector<double> &x, const std::vector<double> &y):
  Gaussian() {
  std::vector<double> sigma;
//  for (auto &q : y) {
//    sigma.push_back(sqrt(q));
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
//      DBG << "failed to set fitter";
//    }

  try {
    f->execute("guess Gaussian");
  }
  catch ( ... ) {
    DBG << "Gaussian could not guess";
    success = false;
  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    DBG << "Gaussian could not fit";
    success = false;
  }

  if (success) {
    std::vector<fityk::Func*> fns = f->all_functions();
    if (fns.size()) {
      fityk::Func* lastfn = fns.back();
      extract_params(f, lastfn);
//      center_ = lastfn->get_param_value("center");
//      height_ = lastfn->get_param_value("height");
//      hwhm_   = lastfn->get_param_value("hwhm");

//      CustomTimer timer(true);
//      DBG << "c uncert =" << FitykUtil::get_err(f, lastfn->name, "center");
//      DBG << "h uncert =" << FitykUtil::get_err(f, lastfn->name, "height");
//      DBG << "w uncert =" << FitykUtil::get_err(f, lastfn->name, "hwhm");
//      DBG << "Time to retrieve uncerts " << timer.ms() << " ms";
//      DBG << "Gaussian fit as c=" << center_ << " h=" << height_ << " w=" << hwhm_;
    }
    rsq_ = f->get_rsquared(0);
  }

  delete f;
}

std::vector<Gaussian> Gaussian::fit_multi(const std::vector<double> &x,
                                          const std::vector<double> &y,
                                          std::vector<Gaussian> old,
                                          PolyBounded &background,
                                          FitSettings settings)
{
  if (old.empty())
    return old;

  std::vector<double> sigma;
  for (auto &q : y)
    sigma.push_back(sqrt(q));
//  sigma.resize(x.size(), 1);

  bool success = true;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);


      try {
//        f->execute("set fitting_method = mpfit");
//        f->execute("set fitting_method = nlopt_nm");
        f->execute("set fitting_method = nlopt_lbfgs");
      } catch ( std::string st ) {
        success = false;
        DBG << "Gaussian multifit failed to set fitter " << st;
      }

  //DBG << "<Gaussian> fitting multiplet [" << x[0] << "-" << x[x.size() - 1] << "]";

  try {
    f->execute(fityk_definition());
    f->execute(background.fityk_definition());
  } catch ( ... ) {
    success = false;
    DBG << "Gaussian multifit failed to define";
  }

  bool use_w_common = (settings.width_common && settings.cali_fwhm_.valid() && settings.cali_nrg_.valid());

  if (use_w_common) {
    FitParam w_common = settings.width_common_bounds;
    double centers_avg;
    for (auto &p : old)
      centers_avg += p.center_.val;
    centers_avg /= old.size();

    double nrg = settings.cali_nrg_.transform(centers_avg);
    double fwhm_expected = settings.cali_fwhm_.transform(nrg);
    double L = settings.cali_nrg_.inverse_transform(nrg - fwhm_expected/2);
    double R = settings.cali_nrg_.inverse_transform(nrg + fwhm_expected/2);
    w_common.val = (R - L) / (2* sqrt(log(2)));

    w_common.lbound = w_common.val * w_common.lbound;
    w_common.ubound = w_common.val * w_common.ubound;

    try {
      f->execute(w_common.def_var());
    } catch ( ... ) {
      success = false;
      DBG << "Gaussian: multifit failed to define w_common";
    }
  }

  try {
    background.add_self(f);
  } catch ( ... ) {
    success = false;
    DBG << "Gaussian: multifit failed to set up common background";
  }

  bool can_uncert = true;

  int i=0;
  for (auto &o : old) {

    if (!use_w_common) {
      double width_expected = o.hwhm_.val;

      if (settings.cali_fwhm_.valid() && settings.cali_nrg_.valid()) {
        double fwhm_expected = settings.cali_fwhm_.transform(settings.cali_nrg_.transform(o.center_.val));
        double L = settings.cali_nrg_.inverse_transform(settings.cali_nrg_.transform(o.center_.val) - fwhm_expected/2);
        double R = settings.cali_nrg_.inverse_transform(settings.cali_nrg_.transform(o.center_.val) + fwhm_expected/2);
        width_expected = (R - L) / (2* sqrt(log(2)));
      }

      o.hwhm_.lbound = width_expected * settings.width_common_bounds.lbound;
      o.hwhm_.ubound = width_expected * settings.width_common_bounds.ubound;

      if ((o.hwhm_.val > o.hwhm_.lbound) && (o.hwhm_.val < o.hwhm_.ubound))
        width_expected = o.hwhm_.val;
      o.hwhm_.val = width_expected;
    }

    o.height_.lbound = o.height_.val * 1e-5;
    o.height_.ubound = o.height_.val * 1e5;

    double lateral_slack = settings.lateral_slack * o.hwhm_.val * 2 * sqrt(log(2));
    o.center_.lbound = o.center_.val - lateral_slack;
    o.center_.ubound = o.center_.val + lateral_slack;

    std::string initial = "F += Gauss2(" +
        o.center_.fityk_name(i) + "," +
        o.height_.fityk_name(i) + "," +
        o.hwhm_.fityk_name(use_w_common ? -1 : i)
        + ")";

    try {
      f->execute(o.center_.def_var(i));
      f->execute(o.height_.def_var(i));
      if (!use_w_common)
        f->execute(o.hwhm_.def_var(i));
      f->execute(initial);
    }
    catch ( ... ) {
      DBG << "Gaussian multifit failed to set up initial locals for peak " << i;
      success = false;
    }

    i++;
  }
  try {
    f->execute("fit " + boost::lexical_cast<std::string>(settings.fitter_max_iter));
//    if (can_uncert) {
//      f->execute("set fitting_method = mpfit");
////    f->execute("set fitting_method = levenberg_marquardt");
//      f->execute("fit");
////    f->execute("fit " + boost::lexical_cast<std::string>(settings.fitter_max_iter));
////    DBG << "refit with mp done";
//    }
  }
  catch ( ... ) {
    DBG << "Gaussian multifit failed to fit";
    success = false;
  }


  if (success) {
//      try {
//    f->execute("info errors");
//    f->execute("info state > nl.fit");

//    } catch ( ... ) {
//      DBG << "Gaussian multifit failed to do error thing";
//      success = false;
//    }

    std::vector<fityk::Func*> fns = f->all_functions();
    int i = 0;
    for (auto &q : fns) {
      if (q->get_template_name() == "Gauss2") {
        old[i].extract_params(f, q);
        old[i].rsq_ = f->get_rsquared(0);
        i++;
      } else if (q->get_template_name() == "PolyBounded") {
        background.extract_params(f, q);
      }
    }


//        f->execute("info errors");

//    f->execute("info state > lm.fit");
  }

  delete f;

  if (!success)
    old.clear();
  return old;

}

double Gaussian::evaluate(double x) {
  return height_.val * exp(-log(2.0)*(pow(((x-center_.val)/hwhm_.val),2)));
}

ValUncert Gaussian::area() const {
  ValUncert ret;
  ret.val = height_.val * hwhm_.val * sqrt(M_PI / log(2.0));
  return ret;
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
