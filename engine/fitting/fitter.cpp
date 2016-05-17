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

#include "fitter.h"
#include <numeric>
#include <algorithm>
#include "custom_logger.h"

namespace Qpx {

void Fitter::setData(SinkPtr spectrum)
{
//  clear();
  if (spectrum) {
    Metadata md = spectrum->metadata();
    if ((md.dimensions() != 1) || (md.bits <= 0) || (md.total_count <= 0))
      return;

    metadata_ = md;

    settings_.bits_ = md.bits;

    if (!md.detectors.empty())
      detector_ = md.detectors[0];

    if (detector_.energy_calibrations_.has_a(Calibration("Energy", md.bits)))
      settings_.cali_nrg_ = detector_.energy_calibrations_.get(Calibration("Energy", md.bits));
    //best?

    if (detector_.fwhm_calibration_.valid())
      settings_.cali_fwhm_ = detector_.fwhm_calibration_;

    settings_.live_time = md.attributes.branches.get(Setting("live_time")).value_duration;
    settings_.real_time = md.attributes.branches.get(Setting("real_time")).value_duration;

    std::shared_ptr<EntryList> spectrum_dump = std::move(spectrum->data_range({{0, pow(2,md.bits)}}));
    std::vector<double> x;
    std::vector<double> y;

    int i = 0, j = 0;
    int x_bound = 0;
    bool go = false;
    for (auto it : *spectrum_dump) {
      if (it.second > 0)
        go = true;
      if (go) {
        x.push_back(static_cast<double>(i));
        y.push_back(it.second.convert_to<double>());
        if (it.second > 0)
          x_bound = j+1;
        j++;
      }
      i++;
    }

    x.resize(x_bound);
    y.resize(x_bound);

    finder_.setNewData(x, y);
    apply_settings(settings_);
  }
}

void Fitter::clear() {
  detector_ = Detector();
  metadata_ = Metadata();
  settings_.cali_nrg_ = Calibration();
  settings_.cali_fwhm_ = Calibration();
  finder_.clear();
  regions_.clear();
}


void Fitter::find_regions() {
  regions_.clear();
//  DBG << "Fitter: looking for " << filtered.size()  << " peaks";

  finder_.find_peaks();

  if (finder_.filtered.empty())
    return;

  std::vector<size_t> Ls;
  std::vector<size_t> Rs;

  size_t L = finder_.lefts[0];
  size_t R = finder_.rights[0];
  for (int i=1; i < finder_.filtered.size(); ++i) {
    double margin = 0;
    if (!finder_.fw_theoretical_bin.empty())
      margin = settings_.ROI_extend_background * finder_.fw_theoretical_bin[R];
//    DBG << "Margin = " << margin;

    if (finder_.lefts[i] < (R + 2 * margin) ) {
//      DBG << "cat ROI " << L << " " << R << " " << finder_.lefts[i] << " " << finder_.rights[i];
      L = std::min(L, finder_.lefts[i]);
      R = std::max(R, finder_.rights[i]);
//      DBG << "postcat ROI " << L << " " << R;
    } else {
//      DBG << "<Fitter> Creating ROI " << L << "-" << R;
      L -= margin; if (L < 0) L = 0;
      R += margin; if (R >= finder_.x_.size()) R = finder_.x_.size() - 1;
      if (settings_.cali_nrg_.transform(R, settings_.bits_) > settings_.finder_cutoff_kev) {
//        DBG << "<Fitter> region " << L << "-" << R;
        Ls.push_back(L);
        Rs.push_back(R);
      }
      L = finder_.lefts[i];
      R = finder_.rights[i];
    }
  }
  double margin = 0;
  if (!finder_.fw_theoretical_bin.empty())
    margin = settings_.ROI_extend_background * finder_.fw_theoretical_bin[R];
  R += margin; if (R >= finder_.x_.size()) R = finder_.x_.size() - 1;
  Ls.push_back(L);
  Rs.push_back(R);


  //extend limits of ROI to edges of neighbor ROIs (grab more background)
  if (Ls.size() > 2) {
    for (int i=0; (i+1) < Ls.size(); ++i) {
      if (Rs[i] < Ls[i+1]) {
        int mid = (Ls[i+1] + Rs[i]) / 2;
        Rs[i] = mid - 1;
        Ls[i+1] = mid + 1;
      }

//      int32_t Rthis = Rs[i];
//      Rs[i] = Ls[i+1] - 1;
//      Ls[i+1] = Rthis + 1;
    }
  }

  for (int i=0; i < Ls.size(); ++i) {
    ROI newROI(settings_);
    newROI.set_data(finder_, finder_.x_[Ls[i]], finder_.x_[Rs[i]]);
    if (newROI.width())
      regions_[newROI.L()] = newROI;
  }
//  DBG << "<Fitter> Created " << regions_.size() << " regions";

}

std::map<double, Peak> Fitter::peaks() {
  std::map<double, Peak> peaks;
  for (auto &q : regions_)
    if (q.second.peak_count()) {
      std::map<double, Peak> roipeaks(q.second.peaks());
      peaks.insert(roipeaks.begin(), roipeaks.end());
    }
  return peaks;
}

std::set<double> Fitter::relevant_regions(double left, double right)
{
  std::set<double> ret;
  for (auto & r : regions_)
  {
    if (
        ((left <= r.second.L()) && (r.second.L() <= right))
        ||
        ((left <= r.second.R()) && (r.second.R() <= right))
        ||
        r.second.overlaps(left)
        ||
        r.second.overlaps(right)
       )
      ret.insert(r.second.L());
  }
  return ret;
}


void Fitter::delete_ROI(double bin) {
  if (regions_.count(bin)) {
    auto it = regions_.find(bin);
    regions_.erase(it);
  }
}

ROI *Fitter::parent_of(double center) {
  ROI *parent = nullptr;
  for (auto &m : regions_)
    if (m.second.contains(center)) {
      parent = &m.second;
      break;
    }
  return parent;
}


bool Fitter::adj_LB(double regionL, double left, double right, boost::atomic<bool>& interruptor) {
  if (!regions_.count(regionL))
    return false;

  DBG << "<Fitter> adj LB";

  ROI newroi = regions_[regionL];
  newroi.adj_LB(finder_, left, right, interruptor);
  regions_.erase(regionL);
  regions_[newroi.L()] = newroi;
}

bool Fitter::adj_RB(double regionL, double left, double right, boost::atomic<bool>& interruptor) {
  if (!regions_.count(regionL))
    return false;

  ROI newroi = regions_[regionL];
  newroi.adj_RB(finder_, left, right, interruptor);
  regions_.erase(regionL);
  regions_[newroi.L()] = newroi;
}

bool Fitter::merge_regions(double left, double right, boost::atomic<bool>& interruptor) {
  std::set<double> rois = relevant_regions(left, right);
  double min = std::min(left, right);
  double max = std::max(left, right);

  for (auto & r : rois)
  {
    if (regions_.count(r) && (regions_.at(r).L() < min))
      min = regions_.at(r).L();
    if (regions_.count(r) && (regions_.at(r).R() > max))
      max = regions_.at(r).R();
    regions_.erase(r);
  }

  ROI newROI(settings_);
  newROI.set_data(finder_, min, max);

  //add old peaks?
  newROI.auto_fit(interruptor);
  if (newROI.width())
    regions_[newROI.L()] = newROI;
}


bool Fitter::adjust_sum4(double &peak_center, double left, double right)
{
  ROI *parent = parent_of(peak_center);
  if (!parent)
    return false;

  return parent->adjust_sum4(peak_center, left, right);
}


void Fitter::add_peak(double left, double right, boost::atomic<bool>& interruptor) {
  if (finder_.x_.empty())
    return;

  for (auto &q : regions_) {
    if (q.second.overlaps(left, right)) {
      q.second.add_peak(finder_, left, right, interruptor);
      return;
    }
  }

//  DBG << "<Fitter> making new ROI to add peak manually " << left << " " << right;
  ROI newROI(settings_);
  newROI.set_data(finder_, left, right);
//  newROI.add_peak(finder_.x_, finder_.y_, left, right, interruptor);
  newROI.auto_fit(interruptor);
  if (newROI.width())
    regions_[newROI.L()] = newROI;
}

void Fitter::remove_peaks(std::set<double> bins) {
  for (auto &m : regions_) {
    if (m.second.remove_peaks(bins)) {

    }
  }
}

void Fitter::replace_peak(const Peak& pk) {
  for (auto &m : regions_)
    if (m.second.replace_peak(pk))
      return;
}

void Fitter::apply_settings(FitSettings settings) {
  settings_ = settings;
  finder_.settings_ = settings_;
  if (regions_.empty())
    finder_.find_peaks();
}

//void Fitter::apply_energy_calibration(Calibration cal) {
//  settings_.cali_nrg_ = cal;
//  apply_settings(settings_);
//}

//void Fitter::apply_fwhm_calibration(Calibration cal) {
//  settings_.cali_fwhm_ = cal;
//  apply_settings(settings_);
//}

void Fitter::save_report(std::string filename) {
  std::ofstream file(filename, std::ios::out | std::ios::app);
  file << "Spectrum \"" << metadata_.name << "\"" << std::endl;
  file << "========================================================" << std::endl;
  file << "Bits: " << settings_.bits_ << "    Resolution: " << pow(2,metadata_.bits) << std::endl;

//  file << "Match pattern:  ";
//  for (auto &q : metadata_.match_pattern)
//    file << q << " ";
//  file << std::endl;
  
//  file << "Add pattern:    ";
//  for (auto &q : metadata_.add_pattern)
//    file << q << " ";
//  file << std::endl;

  file << "Spectrum type: " << metadata_.type() << std::endl;

  if (!metadata_.attributes.branches.empty()) {
    file << "Attributes" << std::endl;
    file << metadata_.attributes;
  }

  if (!metadata_.detectors.empty()) {
    file << "Detectors" << std::endl;
    for (auto &q : metadata_.detectors)
      file << "   " << q.name_ << " (" << q.type_ << ")" << std::endl;
  }
  
  file << "========================================================" << std::endl;
  file << std::endl;

  file << "Acquisition start time:  " << boost::posix_time::to_iso_extended_string(metadata_.attributes.branches.get(Setting("start_time")).value_time) << std::endl;
  double lt = settings_.live_time.total_milliseconds() * 0.001;
  double rt = settings_.real_time.total_milliseconds() * 0.001;
  file << "Live time(s):   " << lt << std::endl;
  file << "Real time(s):   " << rt << std::endl;
  if ((lt < rt) && (rt > 0))
    file << "Dead time(%):   " << (rt-lt)/rt*100 << std::endl;
  double tc = metadata_.total_count.convert_to<double>();
  file << "Total count:    " << tc << std::endl;
  if ((tc > 0) && (lt > 0))
    file << "Count rate:     " << tc/lt << " cps(total/live)"<< std::endl;
  if ((tc > 0) && (rt > 0))
    file << "Count rate:     " << tc/rt << " cps(total/real)"<< std::endl;
  file << std::endl;

  file.fill(' ');
  file << "========================================================" << std::endl;
  file << "===========QPX Fitter analysis results===========" << std::endl;
  file << "========================================================" << std::endl;

  file << std::endl;
  file.fill('-');
  file << std::setw( 15 ) << "center(Hyp)" << "--|"
       << std::setw( 15 ) << "energy(Hyp)" << "--|"
       << std::setw( 15 ) << "FWHM(Hyp)" << "--|"
       << std::setw( 25 ) << "area(Hyp)" << "--|"
       << std::setw( 16 ) << "cps(Hyp)"  << "-||"
      
       << std::setw( 25 ) << "center(S4)" << "--|"
       << std::setw( 15 ) << "cntr-err(S4)" << "--|"
       << std::setw( 15 ) << "FWHM(S4)" << "--|"
       << std::setw( 25 ) << "bckg-area(S4)" << "--|"
       << std::setw( 15 ) << "bckg-err(S4)" << "--|"
       << std::setw( 25 ) << "area(S4)" << "--|"
       << std::setw( 15 ) << "area-err(S4)" << "--|"
       << std::setw( 15 ) << "cps(S4)"  << "--|"
       << std::setw( 5 ) << "CQI"  << "--|"
       << std::endl;
  file.fill(' ');
  for (auto &q : peaks()) {
    file << std::setw( 16 ) << std::setprecision( 10 ) << q.second.center().to_string() << " | "
         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.energy().to_string() << " | "
//         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.fwhm_hyp << " | "
         << std::setw( 26 ) << q.second.area_hyp().to_string() << " | "
//         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.cps_hyp << " || "

//      file << std::setw( 26 ) << q.second.sum4_.centroid.val_uncert(10) << " | "
//           << std::setw( 15 ) << q.second.sum4_.centroid.err(10) << " | "

//           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.fwhm_sum4 << " | "
//           << std::setw( 26 ) << q.second.sum4_.background_area.val_uncert(10) << " | "
//           << std::setw( 15 ) << q.second.sum4_.background_area.err(10) << " | "
//           << std::setw( 26 ) << q.second.sum4_.peak_area.val_uncert(10) << " | "
//           << std::setw( 15 ) << q.second.sum4_.peak_area.err(10) << " | "
//           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.cps_sum4 << " | "
           << std::setw( 5 ) << std::setprecision( 10 ) << q.second.good() << " |";

    file << std::endl;
  }
  
  file.close();
}

void Fitter::to_xml(pugi::xml_node &root) const
{
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  settings_.to_xml(node);

  for (auto &r : regions_)
    r.second.to_xml(node);
}

void Fitter::from_xml(const pugi::xml_node &node, SinkPtr spectrum)
{
  if (node.child(settings_.xml_element_name().c_str()))
    settings_.from_xml(node.child(settings_.xml_element_name().c_str()));

  setData(spectrum);

  //clear automatically found rois?

  for (auto &r : node.children())
  {
    ROI region;
    if (std::string(r.name()) == region.xml_element_name())
    {
      region.from_xml(r, finder_, settings_);
      if (region.width())
        regions_[region.L()] = region;
    }
  }
}



}
