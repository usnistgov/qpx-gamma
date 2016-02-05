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

namespace Gamma {

SUM4::SUM4()
    :peak_width(0)
    ,Lw(0)
    ,Rw(0)
    ,offset(0)
    ,slope(0)
    ,Lsum(0)
    ,Rsum(0)
    ,P_area(0)
    ,B_area(0)
    ,B_variance(0)
    ,net_area(0)
    ,sumYnet(0)
    ,CsumYnet(0)
    ,C2sumYnet(0)
    ,net_variance(0)
    ,err(0)
    ,currieLQ(0)
    ,currieLD(0)
    ,currieLC(0)
    ,Lpeak(0)
    ,Rpeak(0)
    ,currie_quality_indicator(-1)
    ,centroid(0)
    ,centroid_var(0)
    ,fwhm(0)
{}



SUM4::SUM4(const std::vector<double> &x,
           const std::vector<double> &y,
           uint32_t left, uint32_t right,
           uint16_t samples)
    :SUM4()
{

  if ((x.size() != y.size())
      || x.empty()
      || (left > right)
      || (left >= x.size())
      || (right >= x.size())
      || (samples >= x.size()))
    return;

  Lpeak = left;
  Rpeak = right;

  peak_width = right - left + 1;

  LBend = left - 1;
  if (LBend < 0)
    LBend = 0;

  RBstart = right + 1;
  if (RBstart >= x.size())
    RBstart = x.size() - 1;

  LBstart = LBend - samples;
  if (LBstart < 0)
    LBstart = 0;

  RBend = RBstart + samples;
  if (RBend >= x.size())
    RBend = x.size() - 1;

  Lw = LBend - LBstart + 1;
  Rw = RBend - RBstart + 1;

  for (int i=LBstart; i <= LBend; ++i)
    Lsum += y[i];

  for (int i=RBstart; i <= RBend; ++i)
    Rsum += y[i];

  
  B_area = peak_width * (Lsum / Lw + Rsum / Rw) / 2.0;

  for (int i=left; i <=right; ++i)
    P_area += y[i];

  net_area = P_area - B_area;

  B_variance = pow((peak_width / 2.0), 2) * (Lsum / pow(Lw, 2) + Rsum / pow(Rw, 2));
  net_variance = P_area + B_variance;

  //uncertainty = sqrt(net_variance);

  err = 100 * sqrt(net_variance) / net_area;

  
  currieLQ = 50 * (1 + sqrt(1 + B_variance / 12.5));
  currieLD = 2.71 + 4.65 * sqrt(B_variance);
  currieLC = 2.33 * sqrt(B_variance);

  if (net_area > currieLQ)
    currie_quality_indicator = 1;
  else if (net_area > currieLD)
    currie_quality_indicator = 2;
  else if (net_area > currieLC)
    currie_quality_indicator = 3;
  else if (net_area > 0)
    currie_quality_indicator = 4;
  else
    currie_quality_indicator = -1;


  //by default, linear
  slope = (Rsum / Rw - Lsum / Lw) / (Rpeak - Lpeak);
  offset = Lsum / Lw - slope * Lpeak;

  for (int32_t i = Lpeak; i <= Rpeak; ++i) {
    double B_chan  = offset + i*slope; 
    double yn = y[i] - B_chan;
    sumYnet += yn;
    CsumYnet += i * yn;
    C2sumYnet += pow(i, 2) * yn;
  }

  centroid = CsumYnet / sumYnet;
  centroid_var = (C2sumYnet / sumYnet) - pow(centroid, 2);
  fwhm = 2.0 * sqrt(centroid_var * log(4));

}

}
