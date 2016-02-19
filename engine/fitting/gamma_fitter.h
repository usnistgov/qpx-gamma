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
  Fitter() :
    activity_scale_factor_(1.0)
  {
    finder_.settings_ = settings_;
  }

  const FitSettings &settings() {
    return settings_;
  }

  void apply_settings(FitSettings settings);
//  void apply_energy_calibration(Calibration cal);
//  void apply_fwhm_calibration(Calibration cal);

  Fitter(Qpx::Spectrum::Spectrum *spectrum)
      : Fitter()
  {setData(spectrum);}

  void clear();
  
  void setData(Qpx::Spectrum::Spectrum *spectrum);

  void find_regions();
  ROI *parent_of(double center);

  void add_peak(uint32_t left, uint32_t right, boost::atomic<bool>& interruptor);
  void adj_bounds(ROI &target, uint32_t left, uint32_t right, boost::atomic<bool>& interruptor);

  void remove_peaks(std::set<double> bins);
  void delete_ROI(double bin);
  void replace_peak(const Peak&);

  void save_report(std::string filename);

  std::map<double, Peak> peaks();

  //DATA

  Finder finder_;

  Qpx::Spectrum::Metadata metadata_;
  Gamma::Detector detector_; //need this? metadata?
  std::string sample_name_;
  double activity_scale_factor_; //should be in spectrum?
  
  std::map<double, ROI> regions_;

private:
  FitSettings settings_;

};

}

#endif
