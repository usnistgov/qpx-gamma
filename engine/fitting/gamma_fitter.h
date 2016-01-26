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

#include "gamma_peak.h"
#include "roi.h"
#include "detector.h"
#include "spectrum.h"
#include "finder.h"

namespace Gamma {

class Fitter {
  
public:
  Fitter()
      : overlap_(4.0)
      , fw_tolerance_(0.5)
      , avg_window_(3)
      , activity_scale_factor_(1.0)
      , sum4edge_samples(3)
      , x_bound(0)
  {}
  
  Fitter(Qpx::Spectrum::Spectrum *spectrum)
      : Fitter()
  {setData(spectrum);}

  void clear();
  
  void setData(Qpx::Spectrum::Spectrum *spectrum);

  void find_peaks();

  void add_peak(uint32_t left, uint32_t right);
  void remove_peak(double bin);
  void remove_peaks(std::set<double> bins);

  void make_multiplets();

  void filter_by_theoretical_fwhm(double range);

  void save_report(std::string filename);

  //DATA

  Finder finder_;

  std::string sample_name_;
  double activity_scale_factor_;
  
  std::vector<double> x_nrg_;

  uint32_t x_bound;
  uint16_t sum4edge_samples;

  Qpx::Spectrum::Metadata metadata_;
  Gamma::Detector detector_;
  
  std::map<double, Peak> peaks_;
  std::list<ROI> multiplets_;
  Calibration nrg_cali_, fwhm_cali_;
  double overlap_, fw_tolerance_;

private:
  uint16_t avg_window_;
};

}

#endif
