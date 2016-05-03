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


std::string Hypermet::fityk_definition() {

  return "define Hypermet(c, h, w,"
         "lskew_h, lskew_s,"
         "rskew_h, rskew_s,"
         "tail_h, tail_s,"
         "step_h) = "
         "h*("
         "   exp(-(xc/w)^2)"
         " + 0.5 * ("
         "   lskew_h*exp((0.5*w/lskew_s)^2 + (xc/lskew_s)) * erfc((0.5*w/lskew_s) + xc/w)"
         " + rskew_h*exp((0.5*w/rskew_s)^2 - (xc/rskew_s)) * erfc((0.5*w/rskew_s) - xc/w)"
         " + tail_h *exp((0.5*w/tail_s )^2 + (xc/tail_s )) * erfc((0.5*w/tail_s ) + xc/w)"
         " + step_h * erfc(xc/w)"
         " ) )"
         " where xc=(x-c)";
}

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
    tail_amplitude.extract(f, func);
    tail_slope.extract(f, func);
    step_amplitude.extract(f, func);
  }
  catch ( ... ) {
    DBG << "Hypermet could not extract parameters from Fityk";
    return false;
  }
  return true;
}

std::string Hypermet::to_string() const {
  std::string ret = "Hypermet ";
  ret += "   area=" + area().val_uncert(10)
       + "   rsq=" + boost::lexical_cast<std::string>(rsq_) + "    where:\n";

  ret += "     " + center_.to_string() + "\n";
  ret += "     " + height_.to_string() + "\n";
  ret += "     " + width_.to_string() + "\n";
  ret += "     " + Lskew_amplitude.to_string() + "\n";
  ret += "     " + Lskew_slope.to_string() + "\n";
  ret += "     " + Rskew_amplitude.to_string() + "\n";
  ret += "     " + Rskew_slope.to_string() + "\n";
  ret += "     " + tail_amplitude.to_string() + "\n";
  ret += "     " + tail_slope.to_string() + "\n";
  ret += "     " + step_amplitude.to_string();

  return ret;
}

Hypermet::Hypermet(Gaussian gauss, FitSettings settings) :
  height_("h", gauss.height_), center_("c", gauss.center_), width_("w", gauss.hwhm_ / sqrt(log(2))),
  Lskew_amplitude(settings.Lskew_amplitude), Lskew_slope(settings.Lskew_slope),
  Rskew_amplitude(settings.Rskew_amplitude), Rskew_slope(settings.Rskew_slope),
  tail_amplitude(settings.tail_amplitude), tail_slope(settings.tail_slope),
  step_amplitude(settings.step_amplitude),
  rsq_(0)
{

}

Hypermet::Hypermet(const std::vector<double> &x, const std::vector<double> &y,
                   Gaussian gauss,
                    FitSettings settings):
  Hypermet(gauss, settings) {
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
    DBG << "Hypermet failed to set fitter";
  }

  try {
    f->execute(fityk_definition());
  } catch ( ... ) {
    success = false;
    DBG << "Hypermet failed to define";
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

  //DBG << "Hypermet initial c=" << center_ << " h=" << height_ << " w=" << width_;

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
    DBG << "Hypermet: failed to set up initial";
  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    DBG << "Hypermet could not fit";
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
                                          PolyBounded &background,
                                          FitSettings settings)
{
  if (old.empty())
    return old;

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
//    f->execute("set fitting_method = mpfit");
//    f->execute("set fitting_method = levenberg_marquardt");
  } catch ( ... ) {
    success = false;
    DBG << "Hypermet multifit failed to set fitter";
  }

  try {
    f->execute(fityk_definition());
    f->execute(background.fityk_definition());
  } catch ( ... ) {
    success = false;
    DBG << "Hypermet multifit failed to define";
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
      DBG << "Hypermet: multifit failed to define w_common";
    }
  }


  try {
    background.add_self(f);
  } catch ( ... ) {
    success = false;
    DBG << "Hypermet: multifit failed to set up initial globals";
  }

  bool can_uncert = true;

  int i=0;
  for (auto &o : old) {

    if (!use_w_common) {
      double width_expected = o.width_.val;

      if (settings.cali_fwhm_.valid() && settings.cali_nrg_.valid()) {
        double fwhm_expected = settings.cali_fwhm_.transform(settings.cali_nrg_.transform(o.center_.val));
        double L = settings.cali_nrg_.inverse_transform(settings.cali_nrg_.transform(o.center_.val) - fwhm_expected/2);
        double R = settings.cali_nrg_.inverse_transform(settings.cali_nrg_.transform(o.center_.val) + fwhm_expected/2);
        width_expected = (R - L) / (2* sqrt(log(2)));
      }

      o.width_.lbound = width_expected * settings.width_common_bounds.lbound;
      o.width_.ubound = width_expected * settings.width_common_bounds.ubound;

      if ((o.width_.val > o.width_.lbound) && (o.width_.val < o.width_.ubound))
        width_expected = o.width_.val;
      o.width_.val = width_expected;
    }

    o.height_.lbound = o.height_.val * 1e-5;
    o.height_.ubound = o.height_.val * 1e5;

    double lateral_slack = settings.lateral_slack * o.width_.val * 2 * sqrt(log(2));
    o.center_.lbound = o.center_.val - lateral_slack;
    o.center_.ubound = o.center_.val + lateral_slack;

    std::string initial = "F += Hypermet(" +
        o.center_.fityk_name(i) + "," +
        o.height_.fityk_name(i) + "," +
        o.width_.fityk_name(use_w_common ? -1 : i)  + "," +
        o.Lskew_amplitude.fityk_name(i)  + "," +
        o.Lskew_slope.fityk_name(i)  + "," +
        o.Rskew_amplitude.fityk_name(i)  + "," +
        o.Rskew_slope.fityk_name(i)  + "," +
        o.tail_amplitude.fityk_name(i)  + "," +
        o.tail_slope.fityk_name(i)  + "," +
        o.step_amplitude.fityk_name(i)
        + ")";

    try {
      f->execute(o.center_.def_var(i));
      f->execute(o.height_.def_var(i));
      if (!use_w_common)
        f->execute(o.width_.def_var(i));
      f->execute(o.Lskew_amplitude.enforce_policy().def_var(i));
      f->execute(o.Lskew_slope.def_var(i));
      f->execute(o.Rskew_amplitude.enforce_policy().def_var(i));
      f->execute(o.Rskew_slope.def_var(i));
      f->execute(o.tail_amplitude.enforce_policy().def_var(i));
      f->execute(o.tail_slope.def_var(i));
      f->execute(o.step_amplitude.enforce_policy().def_var(i));

      f->execute(initial);
    }
    catch ( ... ) {
      DBG << "Hypermet multifit failed to set up initial locals for peak " << i;
      success = false;
    }

    if (!o.Lskew_amplitude.enabled || !o.Rskew_amplitude.enabled ||
        !o.tail_amplitude.enabled || !o.step_amplitude.enabled)
      can_uncert = false;

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
    DBG << "Hypermet multifit failed to fit";
    success = false;
  }

  if (success) {
//      try {
//    f->execute("info errors");
//    f->execute("info state > nl.fit");

//    } catch ( ... ) {
//      DBG << "Hypermet multifit failed to do error thing";
//      success = false;
//    }

    std::vector<fityk::Func*> fns = f->all_functions();
    int i = 0;
    for (auto &q : fns) {
      if (q->get_template_name() == "Hypermet") {
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

double Hypermet::eval_peak(double x) {
  if (width_.val == 0)
    return 0;

  double xc = x - center_.val;

  double gaussian = exp(- pow(xc/width_.val, 2) );

  double left_short = 0;
  double lexp = exp(pow(0.5*width_.val/Lskew_slope.val, 2) + xc/Lskew_slope.val);
  if ((Lskew_slope.val != 0) && !isinf(lexp))
    left_short = Lskew_amplitude.val * lexp * erfc( 0.5*width_.val/Lskew_slope.val + xc/width_.val);

  double right_short = 0;
  double rexp = exp(pow(0.5*width_.val/Rskew_slope.val, 2) - xc/Rskew_slope.val);
  if ((Rskew_slope.val != 0) && !isinf(rexp))
    right_short = Rskew_amplitude.val * rexp * erfc( 0.5*width_.val/Rskew_slope.val  - xc/width_.val);

  double ret = height_.val * (gaussian + 0.5 * (left_short + right_short) );

  return ret;
}

double Hypermet::eval_step_tail(double x) {
  if (width_.val == 0)
    return 0;

  double xc = x - center_.val;

  double step = step_amplitude.val * erfc( xc/width_.val );

  double tail = 0;
  double lexp = exp(pow(0.5*width_.val/tail_slope.val, 2) + xc/tail_slope.val);
  if ((tail_slope.val != 0) && !isinf(lexp))
    tail = tail_amplitude.val * lexp * erfc( 0.5*width_.val/tail_slope.val + xc/width_.val);

  return height_.val * 0.5 * (step + tail);
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

ValUncert Hypermet::area() const {
  ValUncert ret;
  ret.val = height_.val * width_.val * sqrt(M_PI) *
      (1 +
       Lskew_amplitude.val * width_.val * Lskew_slope.val +
       Rskew_amplitude.val * width_.val * Rskew_slope.val);
  return ret;
}
