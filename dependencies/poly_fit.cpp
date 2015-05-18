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
 *      Polynomial fit
 *
 ******************************************************************************/

#include "poly_fit.h"

#include <gsl/gsl_multifit.h>
#include "fityk.h"
#include "custom_logger.h"

Polynomial::Polynomial(std::vector<double> coeffs, double xoff) {
  xoffset_ = xoff;
  coeffs_ = coeffs;
  int deg = -1;
  for (int i = 0; i < coeffs.size(); i++)
    if (coeffs[i])
      deg = i;
  degree_ = deg;
}

double Polynomial::evaluate(double x) {
  double x_adjusted = x - xoffset_;
  double result = 0.0;
  for (int i=0; i < degree_; i++)
    result += coeffs_[i] * pow(x_adjusted, i);
  return result;
}

std::vector<double> Polynomial::evaluate_array(std::vector<double> x) {
  std::vector<double> y;
  for (auto &q : x)
    y.push_back(evaluate(q));
  return y;
}

std::ostream &operator<<(std::ostream &out, Polynomial d) {
  if (d.degree_ >=0 )
    out << d.coeffs_[0];
  for (int i=1; i < d.degree_; i++)
    out << " + " << d.coeffs_[i] << "x^" << i;
}




double Gaussian::evaluate(double x) {
  return height_ * exp(-log(2.0)*(pow(((x-center_)/hwhm_),2)));
}

std::vector<double> Gaussian::evaluate_array(std::vector<double> x) {
  std::vector<double> y;
  for (auto &q : x)
    y.push_back(evaluate(q));
  return y;
}

std::ostream &operator<<(std::ostream &out, Gaussian d) {
  out << "Gaussian center=" << d.center_ << " height="  << d.height_ << " hwhm=" << d.hwhm_;
}


Peak::Peak(std::vector<double> &x, std::vector<double> &y, int min, int max) {
  if (
          (x.size() == y.size())
          &&
          (min > -1) && (min < x.size())
          &&
          (min > -1) && (min < x.size())
      )
  {
    for (int i = min; i < (max+1); ++i) {
      x_.push_back(x[i]);
      y_.push_back(y[i]);
    }
    rough_ = find_g(x_, y_);

    for (int i = 0; i < y_.size(); ++i)
      diff_y_.push_back(y_[i] - rough_.evaluate(x_[i]));

    fill0_linear(3, 5);

    baseline_ = find_p(x_, filled_y_, rough_.center_);

    for (int i = 0; i < y_.size(); ++i)
      y_nobase_.push_back(y_[i] - baseline_.evaluate(x_[i]));

    refined_ = find_g(x_, y_nobase_);

    for (int i = 0; i < y_.size(); ++i)
      y_fullfit_.push_back(baseline_.evaluate(x_[i]) + refined_.evaluate(x_[i]));

  }
}

Gaussian Peak::find_g(std::vector<double> &x, std::vector<double> &y) {
  std::vector<double> sigma(x.size(), 1);
  
  fityk::Fityk *f = new fityk::Fityk;
  f->load_data(0, x, y, sigma);
  f->execute("guess Gaussian");
  f->execute("fit");
  fityk::Func* lastfn = f->all_functions().back();
  Gaussian g = Gaussian(lastfn->get_param_value("center"),
                        lastfn->get_param_value("height"),
                        lastfn->get_param_value("hwhm"));
  delete f;
  return g;
}

Polynomial Peak::find_p(std::vector<double> &x, std::vector<double> &y, double center) {
  std::vector<double> x_c, coefs(6), sigma(x.size(), 1);
  for (int i = 0; i < x.size(); i++)
    x_c.push_back(x[i] - center);

  fityk::Fityk *f = new fityk::Fityk;
  f->load_data(0, x_c, y, sigma);
  //f->execute("define MyCubic(a0=height,a1=0, a2=0, a3=0) = a0 + a1*x + a2*x^2 + a3*x^3");
  f->execute("guess Polynomial5");
  f->execute("fit");
  fityk::Func* lastfn = f->all_functions().back();
  coefs[0] = lastfn->get_param_value("a0");
  coefs[1] = lastfn->get_param_value("a1");
  coefs[2] = lastfn->get_param_value("a2");
  coefs[3] = lastfn->get_param_value("a3");
  coefs[4] = lastfn->get_param_value("a4");
  coefs[5] = lastfn->get_param_value("a5");
  Polynomial p = Polynomial(coefs, center);
  delete f;
  return p;
}

void Peak::fill0_linear(int buffer, int sample) {
  filled_y_ = diff_y_;

  int first0 = diff_y_.size(), last0 = 0;
    for (int i=0; i < diff_y_.size(); i++) {
      if ((diff_y_[i] <= 0) && (i < first0))
        first0 = i;
      if ((diff_y_[i] <= 0) && (i > last0))
        last0 = i;
    }

    PL_INFO << "0s between " << first0 << " & " << last0;

    int margin = buffer + sample;
    
    if ((first0 > margin) && ((last0 + margin + 1) < diff_y_.size())) {

      double avg_left, avg_right;
      for (int i=buffer; i < margin; i++) {
        avg_left  += diff_y_[first0 - i];
        avg_right += diff_y_[last0 + i];
      }
      avg_left /= sample;
      avg_right /= sample;

      double slope = (avg_right - avg_left) / (last0 - first0 + sample + (2*buffer) );
      double xoffset = first0 - buffer - (sample/2);
      PL_INFO << "will connect empty space with y = " << avg_left << " + (x- " << xoffset << ")*" << slope;
      for (int i = first0; i <= last0; i++)
        filled_y_[i] = avg_left + ((i - xoffset) * slope);
    }
}




void UtilXY::find_peaks(int min_width) {
  peaks_.clear();
  if (y_.size() < 3)
    return;
  std::vector<int> temp_peaks;
  for (std::size_t i=1; (i+1) < y_.size(); i++) {
    if ((y_[i] > y_[i-1]) && (y_[i] > y_[i+1]))
      temp_peaks.push_back(i);
  }
  for (int q = 0; q < y_.size(); q++) {
    bool left = true, right = true;
    for (int d = 1; d <= min_width; d++) {
      if (((q-d) >= 0) && (y_[q-d] >= y_[q-(d-1)]))
        left = false;
      if (((q+d) < y_.size()) && (y_[q+d] >= y_[q+(d-1)]))
        right = false;
    }
    if (left && right)
      peaks_.push_back(q);
  }
}


bool poly_fit(std::vector<double> xx, std::vector<double> yy, std::vector<double>& coefs)
{
  double chisq;
  gsl_matrix *X, *cov;
  gsl_vector *y, *c;

  int n = xx.size();
  int k = coefs.size();
  
  if (!k || (xx.size() != yy.size()))
    return false;

  X = gsl_matrix_alloc (n, k);
  y = gsl_vector_alloc (n);

  c = gsl_vector_alloc (k);
  cov = gsl_matrix_alloc (k, k);

  for (int i = 0; i < n; i++)
  {
    for (int j = 0; j < k; j++)
      gsl_matrix_set (X, i, j, pow(xx[i], j));     
    gsl_vector_set (y, i, yy[i]);
  }

  {
    gsl_multifit_linear_workspace * work = gsl_multifit_linear_alloc (n, k);
    gsl_multifit_linear (X, y, c, cov, &chisq, work);
    gsl_multifit_linear_free (work);
  }

  for (int i = 0; i < k; i++)
    coefs[i] = gsl_vector_get(c,(i));

  gsl_matrix_free (X);
  gsl_vector_free (y);
  gsl_vector_free (c);
  gsl_matrix_free (cov);

  return true;
}

/*void UtilXY::find_peaks2(int max) {
  height_.clear();
  hwhm_.clear();
  center_.clear();

  fityk::Fityk *f = new fityk::Fityk;
  //f->redir_messages(g_custom_logger::get());
  for (int i = 0; i < x_.size(); ++i) {
      f->add_point(x_[i], y_[i], 1);
  }
  for (int i = 0; i < max; ++i) {
    f->execute("guess Gaussian");
    f->execute("fit");
  }

  std::vector<fityk::Func*> allfuncs = f->all_functions();
  for (auto &q : allfuncs) {
    center_.push_back(q->get_param_value("center"));
    hwhm_.push_back(q->get_param_value("hwhm"));
    height_.push_back(q->get_param_value("height"));

    //delete?
  }

  delete f;
}*/

bool poly_fit2(std::vector<double> xx, std::vector<double> yy, std::vector<double>& coefs)
{
  int n = xx.size();
  int k = coefs.size();

  if (!k || (xx.size() != yy.size()))
    return false;

  fityk::Fityk *f = new fityk::Fityk;
  //f->redir_messages(g_custom_logger::get());
  for (int i = 0; i < n; ++i) {
      f->add_point(xx[i], yy[i], 1);
  }
  std::string param = "a0=0", expr = "a0";
  for (int i = 1; i < k; ++i) {
    std::string coefn = std::to_string(i);
    param += ", a" + coefn + "=0";
    expr += " + a" + coefn + "*x^" + coefn;
  }
  std::string definition = "define MyPoly(" + param + ") = " + expr;
  PL_INFO << "Fitting data to: " << definition;
  f->execute(definition);
  f->execute("guess MyPoly");
  f->execute("fit");
  coefs = f->all_parameters();
  delete f;

  return true;
}


bool poly_fit_w(std::vector<double> xx, std::vector<double> yy, std::vector<double> err,
              std::vector<double>& coefs)
{
  double chisq;
  gsl_matrix *X, *cov;
  gsl_vector *y, *w, *c;

  std::size_t n = xx.size();
  std::size_t k = coefs.size();
  
  if (err.empty())
    err.resize(xx.size(), 1.0);

  if (!k || (n != yy.size()) || (n != err.size()))
    return false;

  X = gsl_matrix_alloc (n, k);
  y = gsl_vector_alloc (n);
  w = gsl_vector_alloc (n);

  c = gsl_vector_alloc (k);
  cov = gsl_matrix_alloc (k, k);

  for (std::size_t i = 0; i < n; i++)
  {      
    for (std::size_t j = 0; j < k; j++)
      gsl_matrix_set (X, i, j, pow(xx[i], j));      
    gsl_vector_set (y, i, yy[i]);
    gsl_vector_set (w, i, 1.0/(err[i]*err[i]));
  }

  {
    gsl_multifit_linear_workspace * work = gsl_multifit_linear_alloc (n, k);
    gsl_multifit_wlinear (X, w, y, c, cov, &chisq, work);
    gsl_multifit_linear_free (work);
  }

  for (std::size_t i = 0; i < k; i++)
    coefs[i] = gsl_vector_get(c,(i));

  gsl_matrix_free (X);
  gsl_vector_free (y);
  gsl_vector_free (w);
  gsl_vector_free (c);
  gsl_matrix_free (cov);

  return true;
}

void PeakFitter::run() {
  peaks.clear();
  sum.clear();

  QMutexLocker locker(&mutex_);
  std::vector<double> xx = x.toStdVector(),
                      yy = y.toStdVector(),
                      sigma(x.size(), 1);

  sum.resize(x.size());

  fityk::Fityk *f = new fityk::Fityk;
  f->load_data(0, xx, yy, sigma);

  int n = 0;
  double width_sum = 0.0;
  while ((stop.load() != 1) && (n < max_)){
    f->execute("guess Gaussian");
    //f->execute("fit");
    fityk::Func* lastfn = f->all_functions().back();
    Gaussian pk(lastfn->get_param_value("center"),
                lastfn->get_param_value("height"),
                lastfn->get_param_value("hwhm"));

    PL_INFO << pk;

    if (n == 0) {
      width_sum += pk.hwhm_;
      peaks.push_back(pk);
    } else if ((pk.hwhm_ * n / width_sum) < max_ratio_) {
      width_sum += pk.hwhm_;
      peaks.push_back(pk);
    } else
      PL_INFO << "rejected";

    n++;
  }

  emit newPeak(new QVector<Gaussian>(peaks), new QVector<double>(sum));

  delete f;
}


  /*#define C(i) (gsl_vector_get(c,(i)))
    #define COV(i,j) (gsl_matrix_get(cov,(i),(j)))

  {
    printf ("# best fit: Y = %g + %g X + %g X^2\n", 
            C(0), C(1), C(2));

    printf ("# covariance matrix:\n");
    printf ("[ %+.5e, %+.5e, %+.5e  \n",
               COV(0,0), COV(0,1), COV(0,2));
    printf ("  %+.5e, %+.5e, %+.5e  \n", 
               COV(1,0), COV(1,1), COV(1,2));
    printf ("  %+.5e, %+.5e, %+.5e ]\n", 
               COV(2,0), COV(2,1), COV(2,2));
    printf ("# chisq = %g\n", chisq);
  }
  */
