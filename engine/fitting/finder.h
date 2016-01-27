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

namespace Gamma {

class Finder {

private:
  static constexpr uint16_t default_width  = 3;
  static constexpr double   default_thresh = 3.0;
  
public:
  Finder(uint16_t width = default_width, double thresh = default_thresh) :
      square_width_(default_width),
      threshold_(default_thresh)
  {}


  void clear();
  
  void setData(std::vector<double> x, std::vector<double> y);
  void find_peaks();
  void find_peaks(uint16_t width, double thresh);

  uint16_t find_left(uint16_t chan);
  uint16_t find_right(uint16_t chan);

  //DATA

  std::vector<double> x_, y_;
  std::vector<double> x_kon, x_conv;

  std::vector<uint16_t> prelim, filtered, lefts, rights;

private:
  uint16_t square_width_;
  double threshold_;

  void calc_kon();

  uint16_t left_edge(uint16_t idx);
  uint16_t right_edge(uint16_t idx);

};

}

#endif
