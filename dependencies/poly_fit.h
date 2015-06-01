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
 *      Gaussian, Polynomial and composite peaks
 *
 ******************************************************************************/

#ifndef POLY_FIT_H
#define POLY_FIT_H

#include <vector>
#include <iostream>

class Polynomial {
public:
  Polynomial() : degree_(-1), xoffset_(0.0) {}
  Polynomial(std::vector<double> coeffs, double xoff = 0);
  Polynomial(std::vector<double> &x, std::vector<double> &y, uint16_t degree, double center=0);
       
  double evaluate(double x);
  friend std::ostream &operator<<(std::ostream &out, Polynomial d);
  std::vector<double> evaluate_array(std::vector<double> x);
  
  std::vector<double> coeffs_;
  int degree_;
  double xoffset_;
};

class Gaussian {
public:
  Gaussian() : height_(0), hwhm_(0), center_(0) {}
  Gaussian(const std::vector<double> &x, const std::vector<double> &y);
  Gaussian(double center, double height, double hwhm) : height_(height), hwhm_(hwhm), center_(center) {}

  double evaluate(double x);
  friend std::ostream &operator<<(std::ostream &out, Gaussian d);
  std::vector<double> evaluate_array(std::vector<double> x);
  
  double height_, hwhm_, center_;
};


class Peak {
public:
  Peak() {}
  Peak(const std::vector<double> &x, const std::vector<double> &y, int min, int max);

  std::vector<double> x_, y_, filled_y_, y_nobase_, y_fullfit_;
  Gaussian rough_, refined_;

private:
  //  Polynomial find_p(std::vector<double> x, std::vector<double> y, double center);
  //  void fill0_linear(int buffer);  
};

class UtilXY {
public:
  UtilXY();
  UtilXY(const std::vector<double> &x, const std::vector<double> &y):
    x_(x), y_(y) {}

  void setXY(std::vector<double> x, std::vector<double> y)
    {*this = UtilXY(x, y);}

  void find_peaks(int min_width, int max_width);
  //void find_peaks2(int max);

  std::vector<double> x_, y_;

  std::vector<Peak> peaks_;
};

#endif
