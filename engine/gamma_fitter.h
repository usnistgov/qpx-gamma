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
 *      Gamma::Fitter
 *
 ******************************************************************************/

#ifndef GAMMA_FITTER_H
#define GAMMA_FITTER_H

#include <vector>
#include "gamma_peak.h"
#include "detector.h"

namespace Gamma {

class Fitter {
  
public:
  Fitter() {}
  Fitter(const std::vector<double> &x, const std::vector<double> &y, uint16_t avg_window = 1):
    x_(x), y_(y), y_avg_(y_) {set_mov_avg(avg_window); deriv();}

  Fitter(const std::vector<double> &x, const std::vector<double> &y, uint16_t min, uint16_t max, uint16_t avg_window = 1);

  void setXY(std::vector<double> x, std::vector<double> y)
    {*this = Fitter(x, y);}

  void set_mov_avg(uint16_t);
  void deriv();
  void find_prelim();
  void filter_prelim(uint16_t min_width);
  void refine_edges(double threshl, double threshr);
  void find_peaks(int min_width, Calibration nrg_cali = Calibration(), Calibration fwhm_cali = Calibration(), double overlap = 4.0);

  Peak make_multiplet(std::vector<Peak> peaks, Calibration nrg_cali, Calibration fwhm_cali);

  uint16_t find_left(uint16_t chan, uint16_t grace = 0);
  uint16_t find_right(uint16_t chan, uint16_t grace = 0);
  double local_avg(uint16_t chan, uint16_t samples = 1);
  std::vector<double> make_baseline(uint16_t left, uint16_t right, uint16_t BL_samples, BaselineType type = BaselineType::linear);

  static void filter_by_theoretical_fwhm(std::vector<Peak>&, double range);

  std::vector<double> x_, y_, y_avg_, deriv1, deriv2;

  std::vector<uint16_t> prelim, filtered, lefts, rights, lefts_t, rights_t;

  std::vector<Peak> peaks_, multiplets_;
};

}

#endif
