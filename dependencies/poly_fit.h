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
#include <numeric>

std::string to_str_precision(double, uint16_t);

class Polynomial {
public:
  Polynomial() : degree_(-1), xoffset_(0.0) {}
  Polynomial(std::vector<double> coeffs, double center = 0);
  Polynomial(std::vector<double> &x, std::vector<double> &y, uint16_t degree, double center=0);
       
  std::string to_string();
  std::string to_UTF8();
  std::string to_markup();

  double evaluate(double x);
  std::vector<double> evaluate_array(std::vector<double> x);
  
  std::vector<double> coeffs_;
  double xoffset_;
  int degree_;
};

class Gaussian {
public:
  Gaussian() : height_(0), hwhm_(0), center_(0) {}
  Gaussian(const std::vector<double> &x, const std::vector<double> &y);
  Gaussian(double center, double height, double hwhm) : height_(height), hwhm_(hwhm), center_(center) {}

  std::string to_string();

  double evaluate(double x);
  std::vector<double> evaluate_array(std::vector<double> x);
  
  double height_, hwhm_, center_;
};


class Peak {
public:
  Peak() {}
  Peak(const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> &y_baseline);
  Peak(const std::vector<double> &x, const std::vector<double> &y, uint32_t left, uint32_t right, uint32_t BL_samples);


  std::vector<double> x_, y_, y_baseline_, y_fullfit_;
  Gaussian gaussian_;

private:
  void fit(const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> &y_baseline);
};

class UtilXY {
public:
  UtilXY() {}
  UtilXY(const std::vector<double> &x, const std::vector<double> &y, uint16_t avg_window = 1):
    x_(x), y_(y), y_avg_(y_) {set_mov_avg(avg_window); deriv();}
  UtilXY(const std::vector<double> &x, const std::vector<double> &y, uint16_t min, uint16_t max, uint16_t avg_window = 1);

  void setXY(std::vector<double> x, std::vector<double> y)
    {*this = UtilXY(x, y);}

  void set_mov_avg(uint16_t);
  void deriv();
  void find_prelim();
  void filter_prelim(uint16_t min_width);
  void refine_edges(double threshl, double threshr);
  void find_peaks(int min_width);

  uint16_t find_left(uint16_t, uint16_t);
  uint16_t find_right(uint16_t, uint16_t);
  std::vector<double> make_baseline(uint16_t left, uint16_t right, uint16_t BL_samples);

  std::vector<double> x_, y_, y_avg_, deriv1, deriv2;

  std::vector<uint16_t> prelim, filtered, lefts, rights, lefts_t, rights_t;

  std::vector<Peak> peaks_;
};

#endif
