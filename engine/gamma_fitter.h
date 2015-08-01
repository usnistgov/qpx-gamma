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

  void clear();
  
  void setXY(std::vector<double> x, std::vector<double> y, uint16_t avg_window = 1)
  {*this = Fitter(x, y, avg_window);}

  void set_mov_avg(uint16_t);
  void deriv();
  void find_prelim();
  void filter_prelim(uint16_t min_width);
  void refine_edges(double threshl, double threshr);
  void find_peaks(int min_width, Calibration nrg_cali = Calibration(), Calibration fwhm_cali = Calibration(), double overlap = 4.0);

  void add_peak(uint32_t left, uint32_t right, Calibration nrg_cali = Calibration(), Calibration fwhm_cali = Calibration(), double overlap = 4.0);
  void remove_peak(double bin, Calibration nrg_cali = Calibration(), Calibration fwhm_cali = Calibration());
  void remove_peaks(std::set<double> bins, Calibration nrg_cali = Calibration(), Calibration fwhm_cali = Calibration());

  void make_multiplets(Calibration nrg_cali, Calibration fwhm_cali, double overlap = 4.0);

  uint16_t find_left(uint16_t chan, uint16_t grace = 0);
  uint16_t find_right(uint16_t chan, uint16_t grace = 0);

  void filter_by_theoretical_fwhm(double range);

  std::vector<double> x_, y_, y_avg_, deriv1, deriv2;

  std::vector<uint16_t> prelim, filtered, lefts, rights, lefts_t, rights_t;
  
  //  Calibration cal_nrg, cal_fwhm;

  std::map<double, Peak> peaks_;
  std::list<Multiplet> multiplets_;
};

}

#endif
