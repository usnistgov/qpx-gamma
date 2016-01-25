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
 *      Voigt type functions
 *
 ******************************************************************************/

#include "hypermet.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <boost/lexical_cast.hpp>

#include "fityk.h"
#include "custom_logger.h"


Hypermet::Hypermet(const std::vector<double> &x, const std::vector<double> &y, double h, double c, double w):
  Hypermet(h, c, w / sqrt(log(2.0))) {
  std::vector<double> sigma;
  for (auto &q : y) {
    sigma.push_back(1/sqrt(q));
  }

  bool success = true;


  if ((x.size() < 1) || (x.size() != y.size()))
    return;


  std::vector<fityk::Func*> fns;

  fityk::Fityk *f = new fityk::Fityk;
  //f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);


  try {
    f->execute("set fitting_method = nlopt_nm");
  } catch ( ... ) {
    success = false;
    PL_DBG << "failed to set fitter";
  }

    std::string definition = "define Hypermet(cc, hh, ww, lshort_h, lshort_s) = "
                              "hh*exp(-((x-cc)/ww)^2)"
                             " + 0.5*lshort_h*hh*exp((0.5*ww/lshort_s)^2 + ((x-cc)/lshort_s))*erfc((0.5*ww/lshort_s) + (x-cc)/lshort_s)";
//                             " where xc=(x-cc)";

    PL_DBG << "Definition: " << definition;

    try {
      f->execute(definition);
    } catch ( ... ) {
      success = false;
      PL_DBG << "Hypermet failed to define";
    }


std::string initial_h = "$h = ~" + boost::lexical_cast<std::string>(height_) +
                         " [" + boost::lexical_cast<std::string>(height_*0.001) +
                          ":" + boost::lexical_cast<std::string>(height_*1.5) + "]";

std::string initial_c = "$c = ~" + boost::lexical_cast<std::string>(center_) +
                         " [" + boost::lexical_cast<std::string>(x[0]) +
                          ":" + boost::lexical_cast<std::string>(x[x.size()-1]) + "]";

std::string initial_w = "$w = ~" + boost::lexical_cast<std::string>(width_) +
                            " [" + boost::lexical_cast<std::string>(width_*0.7) +
                          ":" + boost::lexical_cast<std::string>(width_*1.3) + "]";

std::string initial_lsh = "$lsh = ~" + boost::lexical_cast<std::string>(0.1) +
                            " [" + boost::lexical_cast<std::string>(0) +
                          ":" + boost::lexical_cast<std::string>(0.75) + "]";

std::string initial_lss = "$lss = ~" + boost::lexical_cast<std::string>(0.3) +
                            " [" + boost::lexical_cast<std::string>(0.3) +
                          ":" + boost::lexical_cast<std::string>(2.0) + "]";

std::string initial = "F += Hypermet($c,$h,$w,$lsh,$lss)";

PL_DBG << "Initial: " << initial_h;
PL_DBG << "Initial: " << initial_c;
PL_DBG << "Initial: " << initial_w;
PL_DBG << "Initial: " << initial_lsh;
PL_DBG << "Initial: " << initial_lss;
PL_DBG << "Initial: " << initial;

try {
  f->execute(initial_h);
  f->execute(initial_c);
  f->execute(initial_w);
  f->execute(initial_lsh);
  f->execute(initial_lss);
  f->execute(initial);

  PL_DBG << "debug df x";
  f->execute("debug df x");

  PL_DBG << "debug rd";
  f->execute("debug rd");

  PL_DBG << "info F[0]";
  f->execute("info F[0]");

} catch ( ... ) {
  success = false;
  PL_DBG << "Hypermet: failed to set up initial";
}

try {
  f->execute("fit");
}
catch ( ... ) {
  PL_ERR << "Hypermet could not fit";
//    success = false;
}

if (success) {
  std::vector<fityk::Func*> fns = f->all_functions();
  if (fns.size()) {
    fityk::Func* lastfn = fns.back();
    center_ = lastfn->get_param_value("cc");
    height_ = lastfn->get_param_value("hh");
    width_   = lastfn->get_param_value("ww");
    Lshort_height_ = lastfn->get_param_value("lshort_h");
    Lshort_slope_ = lastfn->get_param_value("lshort_s");
    PL_DBG << "Hypermet fit as c=" << center_ << " h=" << height_ << " w=" << width_
           << " lsh=" << Lshort_height_ << " lss=" << Lshort_slope_;
  }
  rsq = f->get_rsquared(0);
}


  delete f;
}


std::vector<Hypermet> Hypermet::fit_multi(const std::vector<double> &x, const std::vector<double> &y, const std::vector<Hypermet> &old) {

}



double Hypermet::evaluate(double x) {
  double xc = x - center_;

  double gaussian = height_ * exp(- pow(xc/width_, 2) );

//  double left_short = 0;
  double left_short = 0.5 * Lshort_height_ * height_ *
      exp( pow(0.5*width_/Lshort_slope_, 2) + xc/Lshort_slope_) *
      erfc( 0.5*width_/Lshort_slope_ +  xc/Lshort_slope_ );

  return gaussian + left_short;
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
