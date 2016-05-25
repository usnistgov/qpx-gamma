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
 *      Fitter
 *
 ******************************************************************************/

#ifndef QPX_FITTER_H
#define QPX_FITTER_H

#include "peak.h"
#include "roi.h"
#include "daq_sink.h"
#include "finder.h"

namespace Qpx {

class Fitter {
  
public:
  Fitter() : activity_scale_factor_(1.0) {}

  Fitter(SinkPtr spectrum) : Fitter()
  { setData(spectrum); }

  const FitSettings &settings() { return finder_.settings_; }
  void apply_settings(FitSettings settings);
  const Finder &finder() const { return finder_; }
//  void apply_energy_calibration(Calibration cal);
//  void apply_fwhm_calibration(Calibration cal);


  void clear();
  void setData(SinkPtr spectrum);
  void find_regions();

  //access peaks
  size_t peak_count() const;
  bool contains_peak(double bin) const;
  Peak peak(double peakID) const;
  std::map<double, Peak> peaks();

  //access regions
  size_t region_count() const;
  bool contains_region(double bin) const;
  ROI region(double bin) const;
  const std::map<double, ROI> &regions() const;
  ROI parent_region(double peakID) const;
  std::set<double> relevant_regions(double left, double right);

  //manupulation, may invoke optimizer
  bool auto_fit(double regionID, boost::atomic<bool>& interruptor);
  bool add_peak(double left, double right, boost::atomic<bool>& interruptor);
  bool adj_LB(double regionID, double left, double right, boost::atomic<bool>& interruptor);
  bool adj_RB(double regionID, double left, double right, boost::atomic<bool>& interruptor);
  bool merge_regions(double left, double right, boost::atomic<bool>& interruptor);
  bool refit_region(double regionID, boost::atomic<bool>& interruptor);
  bool override_ROI_settings(double regionID, const FitSettings &fs, boost::atomic<bool>& interruptor);
  bool remove_peaks(std::set<double> peakIDs, boost::atomic<bool>& interruptor);
  //manipulation, no optimizer
  bool adjust_sum4(double &peakID, double left, double right);
  bool replace_hypermet(double &peakID, Hypermet hyp);
  bool rollback_ROI(double regionID, size_t point);
  bool delete_ROI(double regionID);

  //export results
  void save_report(std::string filename);

  //XMLable
  void to_xml(pugi::xml_node &node) const;
  void from_xml(const pugi::xml_node &node, SinkPtr spectrum);
  std::string xml_element_name() const {return "Fitter";}




  //for efficiency stuff
  std::string sample_name_;
  double activity_scale_factor_; //should be in spectrum?

  //data from spectrum
  Metadata metadata_;
  Detector detector_; //need this? metadata?


private:
  std::map<double, ROI> regions_;
//  FitSettings settings_;
  Finder finder_;

  void render_all();
  ROI *parent_of(double peakID);

};

typedef std::shared_ptr<Fitter> FitterPtr;

}

#endif
