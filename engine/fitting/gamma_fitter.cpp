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

#include "gamma_fitter.h"
#include <numeric>
#include <algorithm>
#include "custom_logger.h"

namespace Gamma {

void Fitter::setData(Qpx::Spectrum::Spectrum* spectrum)
{
//  clear();
  if (spectrum != nullptr) {
    Qpx::Spectrum::Metadata md = spectrum->metadata();
    if ((md.dimensions != 1) || (md.resolution <= 0) || (md.total_count <= 0))
      return;

    metadata_ = md;

    settings_.bits_ = md.bits;

    if (!md.detectors.empty())
      detector_ = md.detectors[0];

    if (detector_.energy_calibrations_.has_a(Gamma::Calibration("Energy", md.bits)))
      settings_.cali_nrg_ = detector_.energy_calibrations_.get(Gamma::Calibration("Energy", md.bits));
    //best?

    if (detector_.fwhm_calibration_.valid())
      settings_.cali_fwhm_ = detector_.fwhm_calibration_;

    std::shared_ptr<Qpx::Spectrum::EntryList> spectrum_dump = std::move(spectrum->get_spectrum({{0, md.resolution}}));
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

    finder_.setData(x, y);
    apply_settings(settings_);
  }
}

void Fitter::clear() {
  detector_ = Gamma::Detector();
  metadata_ = Qpx::Spectrum::Metadata();
  settings_.cali_nrg_ = Gamma::Calibration();
  settings_.cali_fwhm_ = Gamma::Calibration();
  finder_.clear();
  regions_.clear();
}


void Fitter::find_regions() {
  regions_.clear();
//  PL_DBG << "Fitter: looking for " << filtered.size()  << " peaks";

  if (finder_.filtered.empty())
    return;

  std::vector<int32_t> Ls;
  std::vector<int32_t> Rs;

  int32_t L = finder_.lefts[0];
  int32_t R = finder_.rights[0];
  for (int i=1; i < finder_.filtered.size(); ++i) {
    double margin = 0;
    if (!finder_.fw_theoretical_bin.empty())
      margin = settings_.ROI_extend_background * finder_.fw_theoretical_bin[R];
//    PL_DBG << "Margin = " << margin;

    if (finder_.lefts[i] < (R + 2 * margin) ) {
//      PL_DBG << "cat ROI " << L << " " << R << " " << finder_.lefts[i] << " " << finder_.rights[i];
      L = std::min(L, static_cast<int32_t>(finder_.lefts[i]));
      R = std::max(R, static_cast<int32_t>(finder_.rights[i]));
//      PL_DBG << "postcat ROI " << L << " " << R;
    } else {
//      PL_DBG << "<Fitter> Creating ROI " << L << "-" << R;
      L -= margin; if (L < 0) L = 0;
      R += margin; if (R >= finder_.x_.size()) R = finder_.x_.size() - 1;
      if (settings_.cali_nrg_.transform(R, settings_.bits_) > settings_.finder_cutoff_kev) {
//        PL_DBG << "<Fitter> region " << L << "-" << R;
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
    newROI.set_data(finder_.x_, finder_.y_, Ls[i], Rs[i]);
    regions_.push_back(newROI);
  }
//  PL_DBG << "<Fitter> Created " << regions_.size() << " regions";

}

std::map<double, Peak> Fitter::peaks() {
  std::map<double, Peak> peaks;
  for (auto &q : regions_)
    for (auto &p : q.peaks_) {
      peaks[p.second.center] = p.second;
    }
  return peaks;
}

void Fitter::delete_ROI(double start_energy) {
  std::list<ROI> regions;
  for (auto &r : regions_)
    if (!r.hr_x_nrg.empty() && (r.hr_x_nrg[0] != start_energy))
      regions.push_back(r);
  regions_ = regions;
}

ROI* Fitter::parent_of(double center) {
  ROI *parent = nullptr;
  for (auto &m : regions_)
    if (m.contains(center)) {
      parent = &m;
      break;
    }
  return parent;
}


ROI* Fitter::region_at_nrg(double nrg) {
  ROI *ret = nullptr;
  for (auto &m : regions_)
    if (!m.hr_x_nrg.empty() && (m.hr_x_nrg.front() == nrg)) {
      ret = &m;
      break;
    }
  return ret;
}

void Fitter::remap_region(ROI &region) {
  for (auto &p : region.peaks_) {
    if (p.second.sum4_.peak_width == 0) {
      double edge =  p.second.hypermet_.width_.val * sqrt(log(2)) * 3;
      uint32_t edgeL = region.finder_.find_index(p.second.hypermet_.center_.val - edge);
      uint32_t edgeR = region.finder_.find_index(p.second.hypermet_.center_.val + edge);
      p.second.sum4_ = SUM4(region.finder_.y_, edgeL, edgeR, region.LB(), region.RB());
    }
    p.second.construct(settings_.cali_nrg_, metadata_.live_time.total_milliseconds() * 0.001, settings_.bits_);
  }
}

void Fitter::adj_bounds(ROI &target, uint32_t left, uint32_t right, boost::atomic<bool>& interruptor) {
  ROI temproi = target;
  temproi.adjust_bounds(finder_.x_, finder_.y_, left, right, interruptor);
  remap_region(temproi);
  if (!temproi.hr_x.empty())
    target = temproi;
}


void Fitter::add_peak(uint32_t left, uint32_t right, boost::atomic<bool>& interruptor) {
//  PL_DBG << "Add new peak " << left << "-" << right;

  bool added = false;
  for (auto &q : regions_) {
    if (q.overlaps(left, right)) {
//      PL_DBG << "<Fitter> adding to existing region";
      q.add_peak(finder_.x_, finder_.y_, left, right, interruptor);
      added = true;
      remap_region(q);
      break;
    }
  }

  if (!added) {
    PL_DBG << "<Fitter> making new ROI to add peak manually";
    ROI newROI(settings_);
    newROI.set_data(finder_.x_, finder_.y_, left, right);
    newROI.auto_fit(interruptor);
    if (!newROI.peaks_.empty()) {
      remap_region(newROI);
      PL_DBG << "<Fitter> succeeded making new ROI";
      regions_.push_back(newROI);
      added = true;
    }
  }
}

void Fitter::remove_peaks(std::set<double> bins) {
  for (auto &m : regions_) {
    if (m.remove_peaks(bins))
      remap_region(m);
  }
}

void Fitter::replace_peak(const Peak& pk) {
  for (auto &m : regions_)
    if (m.contains(pk.center)) {
      m.peaks_[pk.center] = pk; //use function that will rebuild
//      PL_DBG << "replacing " << pk.center;
    }
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
  file << "Bits: " << settings_.bits_ << "    Resolution: " << metadata_.resolution << std::endl;

  file << "Match pattern:  ";
  for (auto &q : metadata_.match_pattern)
    file << q << " ";
  file << std::endl;
  
  file << "Add pattern:    ";
  for (auto &q : metadata_.add_pattern)
    file << q << " ";
  file << std::endl;

  file << "Spectrum type: " << metadata_.type << std::endl;

  if (!metadata_.attributes.empty()) {
    file << "Attributes" << std::endl;
    for (auto &q : metadata_.attributes.my_data_)
      file << "   " << q.id_ << " = " << q.value_dbl << std::endl; //other types?
  }

  if (!metadata_.detectors.empty()) {
    file << "Detectors" << std::endl;
    for (auto &q : metadata_.detectors)
      file << "   " << q.name_ << " (" << q.type_ << ")" << std::endl;
  }
  
  file << "========================================================" << std::endl;
  file << std::endl;

  file << "Acquisition start time:  " << boost::posix_time::to_iso_extended_string(metadata_.start_time) << std::endl;
  double rt = metadata_.real_time.total_seconds();
  double lt = metadata_.live_time.total_seconds();
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
  file << "===========QPX Gamma::Fitter analysis results===========" << std::endl;
  file << "========================================================" << std::endl;

  file << std::endl;
  file.fill('-');
  file << std::setw( 15 ) << "center(Hyp)" << "--|"
       << std::setw( 15 ) << "energy(Hyp)" << "--|"
       << std::setw( 15 ) << "FWHM(Hyp)" << "--|"
       << std::setw( 15 ) << "area(Hyp)" << "--|"
       << std::setw( 15 ) << "cps(Hyp)"  << "-||"
      
       << std::setw( 15 ) << "center(S4)" << "--|" 
       << std::setw( 15 ) << "cntr-var(S4)" << "--|" 
       << std::setw( 7 ) << "L" << "--|"
       << std::setw( 7 ) << "R" << "--|"
       << std::setw( 15 ) << "FWHM(S4)" << "--|"
       << std::setw( 15 ) << "bckg-area(S4)" << "--|"
       << std::setw( 15 ) << "bckg-var(S4)" << "--|"
       << std::setw( 15 ) << "net-area(S4)" << "--|"
       << std::setw( 15 ) << "net-var(S4)" << "--|"
       << std::setw( 15 ) << "%err(S4)" << "--|"      
       << std::setw( 15 ) << "cps(S4)"  << "--|"
       << std::setw( 5 ) << "CQI"  << "--|"
       << std::endl;
  file.fill(' ');
  for (auto &q : peaks()) {
    file << std::setw( 15 ) << std::setprecision( 10 ) << q.second.center << "  |"
         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.energy << "  |"
         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.fwhm_hyp << "  |"
         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.area_hyp << "  |"
         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.cps_hyp << " ||";

    if (!q.second.sum4_.fwhm > 0) {
      file << std::setw( 15 ) << std::setprecision( 10 ) << q.second.sum4_.centroid << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.sum4_.centroid_variance << "  |"
           << std::setw( 7 ) << std::setprecision( 10 ) << q.second.sum4_.Lpeak << "  |"
           << std::setw( 7 ) << std::setprecision( 10 ) << q.second.sum4_.Rpeak << "  |"

           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.fwhm_sum4 << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.sum4_.background_area << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.sum4_.background_variance << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.area_sum4 << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.sum4_.peak_variance << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.sum4_.err << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.cps_sum4 << "  |"
           << std::setw( 5 ) << std::setprecision( 10 ) << q.second.sum4_.currie_quality_indicator << "  |";
    }

    file << std::endl;
  }
  
  file.close();
}


}
