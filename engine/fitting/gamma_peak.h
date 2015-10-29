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
 *      Gamma::Peak
 *
 ******************************************************************************/

#ifndef GAMMA_PEAK_H
#define GAMMA_PEAK_H

#include <vector>
#include <set>
#include "polynomial.h"
#include "gaussian.h"
#include "voigt_.h"
#include "detector.h"

namespace Gamma {

enum class BaselineType : int {linear = 0, step = 1, step_polynomial = 2};


class SUM4 {
public:
  std::vector<double> x_, y_, bx_, by_;
  int32_t Lpeak, Rpeak, LBstart, LBend, RBstart, RBend;
  double peak_width, Lw, Rw;
  double Lsum, Rsum;
  double B_area, B_variance;
  double P_area, net_variance;
  double net_area;
  double sumYnet, CsumYnet, C2sumYnet;
  double centroid, centroid_var, fwhm;
  double err;
  double currieLQ, currieLD, currieLC;
  int currie_quality_indicator;

  SUM4();

  SUM4(const std::vector<double> &x,
       const std::vector<double> &y,
       uint32_t left, uint32_t right,
       uint16_t samples);

};


  double local_avg(const std::vector<double> &x,
                   const std::vector<double> &y,
                   uint16_t chan,
                   uint16_t samples = 1);
  
  std::vector<double> make_background(const std::vector<double> &x,
                                      const std::vector<double> &y,
                                      uint16_t left, uint16_t right,
                                      uint16_t samples,
                                      BaselineType type = BaselineType::linear);

class Peak {
public:
  Peak()
      : subpeak (false)
      , center(0)
      , energy(0)
      , height(0)
      , fwhm_sum4(0)
      , fwhm_gaussian (0)
      , fwhm_pseudovoigt (0)
      , hwhm_L (0)
      , hwhm_R (0)
      , fwhm_theoretical(0)
      , lim_L (0)
      , lim_R (0)
      , intersects_L (false)
      , intersects_R (false)
      , selected (false)
      , flagged (false)
      , area_net_(0.0)
      , area_gross_(0.0)
      , area_bckg_(0.0)
      , area_gauss_(0.0)
      , live_seconds_(0.0)
      , cts_per_sec_net_(0.0)
      , cts_per_sec_gauss_(0.0)
      , area_best_(0.0)
      , cps_best_(0.0)
      , intensity_theoretical_(0.0)
      , efficiency_relative_(0.0)
  {}

  Peak(const std::vector<double> &x, const std::vector<double> &y, uint32_t L, uint32_t R,
       Calibration cali_nrg = Calibration(), Calibration cali_fwhm = Calibration(), double live_seconds = 0.0, uint16_t sum4edge_samples = 3);
  
  void construct(Calibration cali_nrg, Calibration cali_fwhm);

  std::vector<double> x_, y_, y_baseline_, y_fullfit_gaussian_, y_fullfit_pseudovoigt_;
  SUM4 sum4_;
  Gaussian gaussian_;
  SplitPseudoVoigt pseudovoigt_;

  double center, energy, height, fwhm_sum4, fwhm_gaussian, fwhm_pseudovoigt, hwhm_L, hwhm_R,
      fwhm_theoretical, lim_L, lim_R;
  bool intersects_L, intersects_R;
  bool selected, flagged;

  double area_net_, area_gross_, area_bckg_, area_gauss_;
  double live_seconds_, cts_per_sec_net_, cts_per_sec_gauss_;
  double area_best_, cps_best_;
  double intensity_theoretical_, efficiency_relative_;
  
  bool subpeak;

  static bool by_center (const Peak& a, const Peak& b)
  {
    return (a.center < b.center);
  }

  bool operator<(const Peak &other) const {
    return (center < other.center);
  }

  bool operator==(const Peak &other) const {
    return (center == other.center);
  }

  bool operator>(const Peak &other) const {
    return (center > other.center);
  }
  
};

struct Multiplet {
  Multiplet(const Calibration &nrg, const Calibration &fw, double live_seconds = 0.0)
  : cal_nrg_ (nrg)
  , cal_fwhm_ (fw)
  , live_seconds_(live_seconds) 
  {}

  bool contains(double bin);
  bool overlaps(const Peak &);
  void add_peaks(const std::set<Peak> &pks, const std::vector<double> &x, const std::vector<double> &y);
  void add_peak(const Peak &pk, const std::vector<double> &x, const std::vector<double> &y);
  void remove_peak(const Peak &pk);
  void remove_peak(double bin);
  void rebuild();

  std::set<Peak> peaks_;
  std::vector<double> x_, y_,
    y_background_, y_fullfit_;
  Calibration cal_nrg_, cal_fwhm_;
  double live_seconds_;
};


}

#endif
