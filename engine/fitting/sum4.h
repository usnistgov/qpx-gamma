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

namespace Gamma {

class SUM4 {
public:
  int32_t Lpeak, Rpeak, LBstart, LBend, RBstart, RBend;
  double peak_width, Lw, Rw;
  double Lsum, Rsum;
  double offset, slope;
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

}

#endif
