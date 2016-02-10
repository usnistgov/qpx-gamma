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
 *      Gamma::SUM4
 *
 ******************************************************************************/

#ifndef GAMMA_SUM4_H
#define GAMMA_SUM4_H

#include <vector>
#include <cstdint>
#include "polynomial.h"

namespace Gamma {

class SUM4Edge {
  uint32_t start_, end_;
  double sum_, w_, avg_, variance_;

public:
  SUM4Edge() : start_(0), end_(0), sum_(0), w_(0), avg_(0), variance_(0) {}
  SUM4Edge(const std::vector<double> &y,
           uint32_t left, uint32_t right);

  uint32_t start()  const {return start_;}
  uint32_t end()    const {return end_;}
  double sum()      const {return sum_;}
  double width()    const {return w_;}
  double average()  const {return avg_;}
  double variance() const {return variance_;}

};

class SUM4 {

  SUM4Edge LB_, RB_;
  Polynomial background_;

public:
  double background_area, background_variance;
  std::vector<double> bx, by;

  int32_t Lpeak, Rpeak;
  double peak_width;
  double peak_area, peak_variance;
  double centroid, centroid_variance;
  double fwhm;
  double err;
  int currie_quality_indicator;

  SUM4();

  SUM4(const std::vector<double> &y,
       uint32_t left, uint32_t right,
       uint16_t samples);

  SUM4(const std::vector<double> &y,
       uint32_t left, uint32_t right,
       SUM4Edge LB, SUM4Edge RB);

  SUM4Edge LB() const {return LB_;}
  SUM4Edge RB() const {return RB_;}

  void recalc(const std::vector<double> &y);

};

}

#endif
