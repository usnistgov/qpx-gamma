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

#include "voigt_.h"

#include <gsl/gsl_multifit.h>
#include <sstream>
#include <iomanip>
#include <numeric>

#include "fityk.h"
#include "custom_logger.h"


SplitPseudoVoigt::SplitPseudoVoigt(const std::vector<double> &x, const std::vector<double> &y):
  SplitPseudoVoigt() {
  std::vector<double> sigma;
  sigma.resize(x.size(), 1);

  bool success = true;
  std::vector<fityk::Func*> fns;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);

  try {
    f->execute("guess SplitPseudoVoigt");
  }
  catch ( ... ) {
    //   PL_ERR << "Fytik threw exception while guessing";
    success = false;
  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    //    PL_ERR << "Fytik threw exception while fitting";
    success = false;
  }

  if (success) {
    std::vector<fityk::Func*> fns = f->all_functions();
    if (fns.size()) {
      fityk::Func* lastfn = fns.back();
      
      
      center_ = lastfn->get_param_value("center");
      height_ = lastfn->get_param_value("height");
      hwhm_l  = lastfn->get_param_value("hwhm1");
      hwhm_r  = lastfn->get_param_value("hwhm2");
      shape_l  = lastfn->get_param_value("shape1");
      shape_r  = lastfn->get_param_value("shape2");
    }
    rsq = f->get_rsquared(0);
  }

  delete f;
}

double SplitPseudoVoigt::evaluate(double x) {
  double hwhm = 0, shape = 0;
  if (x <= center_) {
    hwhm = hwhm_l;
    shape = shape_l;
  } else {
    hwhm = hwhm_r;
    shape = shape_r;
  }
  return height_ *
      (
          (1.0 - shape) * exp(-log(2.0)*(pow(((x-center_)/hwhm),2)))
          +
          (
              shape /
              (1.0 + pow((x - center_)/hwhm, 2))
           )
       );
}

std::vector<double> SplitPseudoVoigt::evaluate_array(std::vector<double> x) {
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
