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

#include "sum4.h"
#include <cmath>
#include "custom_logger.h"

namespace Gamma {

SUM4Edge::SUM4Edge(const std::vector<double> &y, uint32_t left, uint32_t right)
  : SUM4Edge()
{
  if (y.empty()
      || (left > right)
      || (left >= y.size())
      || (right >= y.size()))
    return;

  start_ = left;
  end_   = right;

  w_ = right - left + 1;
  midpoint_ = left + (w_ * 0.5);


  min_ = std::numeric_limits<double>::max();
  max_ = std::numeric_limits<double>::min();

  for (int i=left; i <= right; ++i) {
    sum_ += y[i];
    min_ = std::min(min_, y[i]);
    max_ = std::max(max_, y[i]);
  }

  avg_ = sum_ / w_;

  variance_ = sum_ / pow(w_, 2);
}

SUM4::SUM4()
    :peak_width(0)
    ,background_area(0)
    ,background_variance(0)
    ,peak_area(0)
    ,peak_variance(0)
    ,err(0)
    ,Lpeak(0)
    ,Rpeak(0)
    ,currie_quality_indicator(-1)
    ,centroid(0)
    ,centroid_variance(0)
    ,fwhm(0)
{}



SUM4::SUM4(const std::vector<double> &y,
           uint32_t left, uint32_t right,
           uint16_t samples)
    :SUM4()
{

  if (y.empty()
      || (left > right)
      || (left >= y.size())
      || (right >= y.size())
      || (samples >= y.size()))
    return;

  Lpeak = left;
  Rpeak = right;

  int32_t LBend = left - 1;
  if (LBend < 0)
    LBend = 0;

  int32_t RBstart = right + 1;
  if (RBstart >= y.size())
    RBstart = y.size() - 1;

  int32_t LBstart = LBend - samples;
  if (LBstart < 0)
    LBstart = 0;

  int32_t RBend = RBstart + samples;
  if (RBend >= y.size())
    RBend = y.size() - 1;

  LB_ = SUM4Edge(y, LBstart, LBend);
  RB_ = SUM4Edge(y, RBstart, RBend);

//  PL_DBG << "<SUM4> edgel " << LB_.start() << "-" << LB_.end();
//  PL_DBG << "<SUM4> edger " << RB_.start() << "-" << RB_.end();

  recalc(y);
}

SUM4::SUM4(const std::vector<double> &y,
           uint32_t left, uint32_t right,
           SUM4Edge LB, SUM4Edge RB)
  : SUM4()
{
  if (y.empty()
      || (left > right)
      || (left >= y.size())
      || (right >= y.size())
      || (LB.average() <= 0)
      || (RB.average() <= 0))
    return;

  LB_ = LB;
  RB_ = RB;
  Lpeak = left;
  Rpeak = right;
  recalc(y);
}


void SUM4::recalc(const std::vector<double> &y)
{
  peak_width = Rpeak - Lpeak + 1;

  background_area = peak_width * (LB_.average() + RB_.average()) / 2.0;

  double gross_area(0);
  for (int i=Lpeak; i <=Rpeak; ++i)
    gross_area += y[i];

  peak_area = gross_area - background_area;

  background_variance = pow((peak_width / 2.0), 2) * (LB_.variance() + RB_.variance());
  peak_variance = gross_area + background_variance;

  //uncertainty = sqrt(peak_variance);

  err = 100 * sqrt(peak_variance) / peak_area;

  double currieLQ(0), currieLD(0), currieLC(0);
  currieLQ = 50 * (1 + sqrt(1 + background_variance / 12.5));
  currieLD = 2.71 + 4.65 * sqrt(background_variance);
  currieLC = 2.33 * sqrt(background_variance);

  if (peak_area > currieLQ)
    currie_quality_indicator = 1;
  else if (peak_area > currieLD)
    currie_quality_indicator = 2;
  else if (peak_area > currieLC)
    currie_quality_indicator = 3;
  else if (peak_area > 0)
    currie_quality_indicator = 4;
  else
    currie_quality_indicator = -1;

  //by default, linear
  double slope = (RB_.average() - LB_.average()) / (RB_.start() - LB_.end()) ;
  background_ = Polynomial({LB_.average(), slope}, LB_.end());

  bx.resize(peak_width);
  by.resize(peak_width);

  double sumYnet(0), CsumYnet(0), C2sumYnet(0);
  for (int32_t i = Lpeak; i <= Rpeak; ++i) {
    double B_chan  = background_.eval(i);
    double yn = y[i] - B_chan;
    sumYnet += yn;
    CsumYnet += i * yn;
    C2sumYnet += pow(i, 2) * yn;

    bx[i-Lpeak] = i;
    by[i-Lpeak] = B_chan;
  }

//  PL_DBG << "<SUM4> bx " << bx[0] << "-" << bx[bx.size() -1];

  centroid = CsumYnet / sumYnet;
  centroid_variance = (C2sumYnet / sumYnet) - pow(centroid, 2);
  fwhm = 2.0 * sqrt(centroid_variance * log(4));
}


}
