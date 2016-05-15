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
 *      Qpx::SUM4
 *
 ******************************************************************************/

#ifndef QPX_SUM4_H
#define QPX_SUM4_H

#include <vector>
#include <cstdint>
#include "polynomial.h"
#include "UncertainDouble.h"

namespace Qpx {

class SUM4Edge {
//  uint32_t start_, end_;
  double Lchan_, Rchan_;
  double min_, max_;
  UncertainDouble dsum_, davg_;

public:
  SUM4Edge();
  SUM4Edge(const std::vector<double> &x,
           const std::vector<double> &y,
           uint32_t left, uint32_t right);

//  uint32_t start()  const {return start_;}
//  uint32_t end()    const {return end_;}
  double left()  const {return Lchan_;}
  double right() const {return Rchan_;}
  double sum()      const {return dsum_.value();}
  double width()    const;
  double average()  const {return davg_.value();}
  double variance() const {return pow(davg_.uncertainty(),2);}

  double min()      const {return min_;}
  double max()      const {return max_;}
  double midpoint() const;
};

class SUM4 {

public:
  SUM4();
  SUM4(const std::vector<double> &x,
       const std::vector<double> &y,
       uint32_t Lindex, uint32_t Rindex,
       PolyBounded background,
       SUM4Edge LB, SUM4Edge RB);

  SUM4Edge LB() const {return LB_;}
  SUM4Edge RB() const {return RB_;}

  double left()  const {return Lchan_;}
  double right() const {return Rchan_;}

  double peak_width() const;
  int    quality() const;

  UncertainDouble gross_area()      const {return gross_area_;}
  UncertainDouble background_area() const {return background_area_;}
  UncertainDouble peak_area()       const {return peak_area_;}
  UncertainDouble centroid()        const {return centroid_;}
  UncertainDouble fwhm()            const {return fwhm_;}

  static int get_currie_quality_indicator(double peak_net_area, double background_variance);

private:
  SUM4Edge LB_, RB_;
  PolyBounded background_;
  double Lchan_, Rchan_;

  UncertainDouble gross_area_;
  UncertainDouble background_area_;
  UncertainDouble peak_area_;
  UncertainDouble centroid_;
  UncertainDouble fwhm_;

};

}

#endif
