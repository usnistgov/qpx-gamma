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
 *      Gamma::Finder
 *
 ******************************************************************************/

#ifndef GAMMA_FINDER_H
#define GAMMA_FINDER_H

#include <vector>
#include <cinttypes>
#include "fit_param.h"

namespace Gamma {

class Finder {

public:

  void clear();
  
  void setData(const std::vector<double> &x, const std::vector<double> &y);
  void setFit(const std::vector<double> &y_fit, const std::vector<double> &y_background);
  void find_peaks();

  uint16_t find_left(uint16_t chan);
  uint16_t find_right(uint16_t chan);
  int32_t find_index(double chan_val);

  //DATA

  std::vector<double> x_, y_, y_fit_, y_resid_, y_resid_on_background_;
  std::vector<double> fw_theoretical_nrg;
  std::vector<double> fw_theoretical_bin;
  std::vector<double> x_kon, x_conv;

  std::vector<uint16_t> prelim, filtered, lefts, rights;

  FitSettings settings_;

private:
  void calc_kon();

  uint16_t left_edge(uint16_t idx);
  uint16_t right_edge(uint16_t idx);

};

}

#endif
