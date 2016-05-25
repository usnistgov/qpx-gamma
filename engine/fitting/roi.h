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
 *      Qpx::ROI
 *
 ******************************************************************************/

#ifndef QPX_ROI_H
#define QPX_ROI_H

#include "peak.h"
#include "polynomial.h"
#include "finder.h"
#include <boost/atomic.hpp>
#include "xmlable.h"

namespace Qpx {

struct FitDescription
{
  std::string description;
  int    peaknum;
  double rsq;
  double sum4aggregate;

  FitDescription()
    : peaknum(0), rsq(0), sum4aggregate(0) {}
};

struct Fit {
  Fit(const SUM4Edge &lb, const SUM4Edge &rb,
      const PolyBounded &backg,
      const std::map<double, Peak> &peaks,
      const Finder &finder,
      std::string descr);

  FitDescription description;

  std::map<double, Peak> peaks_;
  SUM4Edge  LB_, RB_;
  PolyBounded background_;
  FitSettings settings_;
};


struct ROI {
  ROI() {}
  ROI(const Finder &parentfinder, double min, double max);

  //bounds
  double ID() const;
  double left_bin() const;
  double right_bin() const;
  double width() const;
  double left_nrg() const;  //may return NaN
  double right_nrg() const; //may return NaN

  bool overlaps(double bin) const;
  bool overlaps(double Lbin, double Rbin) const;
  bool overlaps(const ROI& other) const;

  //access peaks
  size_t peak_count() const;
  bool contains(double peakID) const;
  Peak peak(double peakID) const;
  const std::map<double, Peak> &peaks() const;

  //access other
  SUM4Edge LB() const {return LB_;}
  SUM4Edge RB() const {return RB_;}
  FitSettings fit_settings() const { return finder_.settings_; }
  const Finder &finder() const { return finder_; }
  PolyBounded sum4_background();

  //access history
  int current_fit() const;
  size_t history_size() const;
  std::vector<FitDescription> history() const;

  //manipulation, no optimizer
  bool rollback(const Finder &parent_finder, size_t i);
  bool adjust_sum4(double &peakID, double left, double right);
  bool replace_hypermet(double &peakID, Hypermet hyp);

  //manupulation, may invoke optimizer
  bool auto_fit(boost::atomic<bool>& interruptor);
  bool refit(boost::atomic<bool>& interruptor);
  bool adjust_LB(const Finder &parentfinder, double left, double right, boost::atomic<bool>& interruptor);
  bool adjust_RB(const Finder &parentfinder, double left, double right, boost::atomic<bool>& interruptor);
  bool add_peak(const Finder &parentfinder, double left, double right, boost::atomic<bool>& interruptor);
  bool remove_peaks(const std::set<double> &pks, boost::atomic<bool>& interruptor);
  bool override_settings(const FitSettings &fs, boost::atomic<bool>& interruptor);


  //XMLable
  void to_xml(pugi::xml_node &node, const Finder &parent_finder) const;
  void from_xml(const pugi::xml_node &node, const Finder &finder);
  std::string xml_element_name() const {return "Region";}

  //as rendered for graphing
  std::vector<double>
      hr_x,
      hr_x_nrg,
      hr_background,
      hr_back_steps,
      hr_fullfit,
      hr_sum4_background_;

private:
  //intrinsic, these are saved
  SUM4Edge LB_, RB_;
  PolyBounded background_;
  std::map<double, Peak> peaks_;

  Finder finder_;           // gets x & y data from fitter

  //history
  std::vector<Fit> fits_;
  int current_fit_;


  void set_data(const Finder &parentfinder, double min, double max);

  std::vector<double> remove_background();
  void init_edges();
  void init_LB();
  void init_RB();
  void init_background();

  void cull_peaks();
  bool remove_peak(double bin);

  bool add_from_resid(boost::atomic<bool>& interruptor, int32_t centroid_hint = -1);
  bool rebuild(boost::atomic<bool>& interruptor);
  bool rebuild_as_hypermet(boost::atomic<bool>& interruptor);
  bool rebuild_as_gaussian(boost::atomic<bool>& interruptor);
  void iterative_fit(boost::atomic<bool>& interruptor);

  void render();
  void save_current_fit(std::string description);
};


}

#endif
