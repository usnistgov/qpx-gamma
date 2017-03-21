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
 *      Qpx::Finder
 *
 ******************************************************************************/

#pragma once

#include <vector>
#include <cinttypes>
#include "fit_settings.h"

namespace Qpx {

class Finder {

public:
  Finder() {}
  Finder(const std::vector<double> &x, const std::vector<double> &y, const FitSettings &settings);

  void clear();
  void reset();
  bool empty() const;
  
  bool cloneRange(const Finder &other, double l, double r);
  void setFit(const std::vector<double> &x_fit,
              const std::vector<double> &y_fit,
              const std::vector<double> &y_background);
  void find_peaks();

  double find_left(double chan) const;
  double find_right(double chan) const ;
  int32_t find_index(double chan_val) const;

  //DATA

  std::vector<double> x_, y_;
  std::vector<double> y_fit_, y_background_, y_resid_, y_resid_on_background_;
  std::vector<double> fw_theoretical_nrg;
  std::vector<double> fw_theoretical_bin;
  std::vector<double> x_kon, x_conv;

  std::vector<size_t> prelim, filtered, lefts, rights;

  FitSettings settings_;

private:
  void calc_kon();

  size_t left_edge(size_t idx) const;
  size_t right_edge(size_t idx) const;

  void setNewData(const std::vector<double> &x, const std::vector<double> &y);

};

}
