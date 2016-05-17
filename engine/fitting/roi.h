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

struct Fit {
  std::map<double, Peak> peaks_;
  SUM4Edge  LB_, RB_;
  PolyBounded background_;
  FitSettings settings_;
};


struct ROI {
  ROI(FitSettings fs = FitSettings())
  {
    finder_.settings_ = fs;
  }

  void set_data(const Finder &parentfinder, double min, double max);

  double L() const;
  double R() const;
  double width() const;
  bool overlaps(double bin) const;
  bool overlaps(double Lbin, double Rbin) const;
  bool overlaps(const ROI& other) const;

  size_t peak_count() const;
  bool contains(double bin) const;
  const std::map<double, Peak> &peaks() const;

  SUM4Edge LB() const {return LB_;}
  SUM4Edge RB() const {return RB_;}
  FitSettings fit_settings() const { return finder_.settings_; }
  PolyBounded sum4_background();

  void override_settings(FitSettings &fs);

  void auto_fit(boost::atomic<bool>& interruptor);
  void iterative_fit(boost::atomic<bool>& interruptor);

  //  void add_peaks(const std::set<Peak> &pks, const std::vector<double> &x, const std::vector<double> &y);
  bool adj_LB(const Finder &parentfinder, double left, double right, boost::atomic<bool>& interruptor);
  bool adj_RB(const Finder &parentfinder, double left, double right, boost::atomic<bool>& interruptor);
  void add_peak(const Finder &parentfinder, double left, double right, boost::atomic<bool>& interruptor);
  bool adjust_sum4(double &peak_center, double left, double right);

  bool replace_peak(const Peak& pk);
  bool remove_peaks(const std::set<double> &pks);
  bool rollback(const Finder &parent_finder, int i);


  void rebuild();

  void save_current_fit();


  void to_xml(pugi::xml_node &node) const;
  void from_xml(const pugi::xml_node &node, const Finder &finder, const FitSettings &parentsettings);
  std::string xml_element_name() const {return "Region";}


  //history
  std::vector<Fit> fits_;
  //  Fit current_fit_;

  //rendition
  std::vector<double> hr_x, hr_x_nrg,
                      hr_background, hr_back_steps,
                      hr_fullfit, hr_sum4_background_;

  const Finder &finder() const { return finder_; }


private:
  //intrinsic, these are saved
  SUM4Edge LB_, RB_;
  PolyBounded background_;
  std::map<double, Peak> peaks_;


  Finder finder_;           // x & y data needed from fitter


  std::vector<double> remove_background();
  void init_edges();
  void init_LB();
  void init_RB();
  void init_background();
  bool remove_peak(double bin);

  bool add_from_resid(int32_t centroid_hint = -1);
  void rebuild_as_hypermet();
  void rebuild_as_gaussian();
  void cull_peaks();
  void render();
};


}

#endif
