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
