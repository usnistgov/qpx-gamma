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
#include "spectrum.h"

namespace Gamma {

class Fitter {
  
public:
  Fitter()
      : overlap_(4.0)
      , avg_window_(1)
      , activity_scale_factor_(1.0)
      , x_bound(0)
  {}
  
//  Fitter(const std::vector<double> &x, const std::vector<double> &y, uint16_t avg_window = 1)
//      : Fitter()
//  {setXY(x, y, avg_window);}

//  Fitter(const std::vector<double> &x, const std::vector<double> &y, uint16_t min, uint16_t max, uint16_t avg_window = 1)
//      : Fitter()
//  {setXY(x, y, min, max, avg_window);}

  Fitter(Qpx::Spectrum::Spectrum *spectrum, uint16_t avg_window = 1)
      : Fitter()
  {avg_window_ = avg_window; setData(spectrum);}

  void clear();
  
  void setData(Qpx::Spectrum::Spectrum *spectrum);

  void set_mov_avg(uint16_t);
  void deriv();
  void find_prelim();
  void filter_prelim(uint16_t min_width);
  void refine_edges(double threshl, double threshr);
  void find_peaks(int min_width);

  void add_peak(uint32_t left, uint32_t right);
  void remove_peak(double bin);
  void remove_peaks(std::set<double> bins);

  void make_multiplets();

  uint16_t find_left(uint16_t chan, uint16_t grace = 0);
  uint16_t find_right(uint16_t chan, uint16_t grace = 0);

  void filter_by_theoretical_fwhm(double range);

  void save_report(std::string filename);

  //DATA

  std::string sample_name_;
  double activity_scale_factor_;
  
  std::vector<double> x_, x_nrg_, y_, y_avg_, deriv1, deriv2;
  uint32_t x_bound;

  std::vector<uint16_t> prelim, filtered, lefts, rights, lefts_t, rights_t;
  
  Qpx::Spectrum::Metadata metadata_;
  Gamma::Detector detector_;
  
  std::map<double, Peak> peaks_;
  std::list<Multiplet> multiplets_;
  Calibration nrg_cali_, fwhm_cali_;
  double overlap_;

private:
  uint16_t avg_window_;
};

}

#endif
