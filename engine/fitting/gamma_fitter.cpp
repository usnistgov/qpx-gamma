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

    if (!md.detectors.empty())
      detector_ = md.detectors[0];

    if (detector_.energy_calibrations_.has_a(Gamma::Calibration("Energy", md.bits))) {
      nrg_cali_ = detector_.energy_calibrations_.get(Gamma::Calibration("Energy", md.bits));
      //PL_INFO << "<Gamma::Fitter> Energy calibration used from detector \"" << detector_.name_ << "\"   " << nrg_cali_.to_string();
    } //else
      //PL_INFO << "<Gamma::Fitter> No existing calibration for this resolution";

    if (detector_.fwhm_calibration_.valid()) {
      fwhm_cali_ = detector_.fwhm_calibration_;
      //PL_INFO << "<Gamma::Fitter> FWHM calibration used from detector \"" << detector_.name_ << "\"   "  << fwhm_cali_.to_string();
    } //else
      //PL_INFO << "<Gamma::Fitter> No existing FWHM calibration";

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
  }
}

void Fitter::clear() {
  detector_ = Gamma::Detector();
  metadata_ = Qpx::Spectrum::Metadata();
  nrg_cali_ = Gamma::Calibration();
  fwhm_cali_ = Gamma::Calibration();
  finder_.clear();
  peaks_.clear();
  regions_.clear();
}


void Fitter::filter_by_theoretical_fwhm(double range) {
  std::set<double> to_remove;
  for (auto &q : peaks_) {
    double frac = q.second.fwhm_gaussian / q.second.fwhm_theoretical;
    if ((frac < (1 - range)) || (frac > (1 + range)))
      to_remove.insert(q.first);
  }
  for (auto &q : to_remove)
    peaks_.erase(q);
}


void Fitter::find_peaks() {

  peaks_.clear();
  regions_.clear();
//  PL_DBG << "Fitter: looking for " << filtered.size()  << " peaks";

  if (finder_.filtered.empty())
    return;

  uint16_t L = finder_.lefts[0];
  uint16_t R = finder_.rights[0];
  for (int i=1; i < finder_.filtered.size(); ++i) {
    if (finder_.lefts[i] < R) {
      PL_DBG << "cat ROI " << L << " " << R << " " << finder_.lefts[i] << " " << finder_.rights[i];
      L = std::min(L, finder_.lefts[i]);
      R = std::max(R, finder_.rights[i]);
      PL_DBG << "postcat ROI " << L << " " << R;
    } else {
      PL_DBG << "making new ROI " << L << "-" << R;
      ROI newROI(nrg_cali_, fwhm_cali_, metadata_.live_time.total_milliseconds() * 0.001);
      newROI.set_data(finder_.x_, finder_.y_, L, R);
      if (!newROI.peaks_.empty())
        regions_.push_back(newROI);
      L = finder_.lefts[i];
      R = finder_.rights[i];
    }
  }
  ROI newROI(nrg_cali_, fwhm_cali_, metadata_.live_time.total_milliseconds() * 0.001);
  newROI.set_data(finder_.x_, finder_.y_, L, R);
  if (!newROI.peaks_.empty())
    regions_.push_back(newROI);

//  PL_DBG << "peaks before filtering by theoretical fwhm " << peaks_.size();

//  if (fwhm_cali_.valid()) {
//    //    PL_DBG << "<GammaFitter> Valid FWHM calib found, performing filtering/deconvolution";
//    filter_by_theoretical_fwhm(fw_tolerance_);

//    PL_DBG << "filtered by theoretical fwhm " << peaks_.size();

//    make_multiplets();
//  }

  //  PL_INFO << "Preliminary search found " << prelim.size() << " potential peaks";
  //  PL_INFO << "After minimum width filter: " << filtered.size();
  //  PL_INFO << "Fitted peaks: " << peaks_.size();

  for (auto &q : regions_) {
    for (auto &p : q.peaks_) {
      peaks_[p.center] = p;
    }
  }

}

void Fitter::add_peak(uint32_t left, uint32_t right) {
  //std::vector<double> xx(x_.begin() + left, x_.begin() + right + 1);
  //std::vector<double> yy(y_.begin() + left, y_.begin() + right + 1);
  //std::vector<double> bckgr = make_background(x_, y_, left, right, 3);
//  if (finder_.x_.empty())
//    return;

//  Peak newpeak = Gamma::Peak(finder_.x_, finder_.y_, left, right, nrg_cali_, fwhm_cali_, metadata_.live_time.total_seconds(), sum4edge_samples);
//  //PL_DBG << "new peak center = " << newpeak.center;

//  if (fwhm_cali_.valid()) {
//    newpeak.lim_L = newpeak.energy - overlap_ * newpeak.fwhm_theoretical;
//    newpeak.lim_R = newpeak.energy + overlap_ * newpeak.fwhm_theoretical;
//    newpeak.intersects_R = false;
//    newpeak.intersects_L = false;

//    if (regions_.empty()) {
//      peaks_[newpeak.center] = newpeak;
//      make_multiplets();
//    } else {
//      for (auto &q : regions_) {
//        if (q.overlaps(newpeak)) {
//          q.add_peak(newpeak, finder_.x_, finder_.y_);
//          newpeak.subpeak = true;
//          peaks_[newpeak.center] = newpeak;
//          return;
//        }
//      }

//      std::set<Peak> multiplet;
//      std::set<double> to_remove;
//      for (auto &q : peaks_) {
//        if ((newpeak.energy > q.second.lim_L) && (newpeak.energy < q.second.lim_R)) {
//          to_remove.insert(q.first);
//          multiplet.insert(q.second);
//        }
//      }
//      if (!to_remove.empty()) {
//        ROI new_roi(nrg_cali_, fwhm_cali_, metadata_.live_time.total_seconds());
//        new_roi.add_peaks(multiplet, finder_.x_, finder_.y_);
//        regions_.push_back(new_roi);
//        for (auto &q : to_remove)
//          peaks_.erase(q);
//        newpeak.subpeak = true;
//        peaks_[newpeak.center] = newpeak;
//      } else
//        peaks_[newpeak.center] = newpeak;
//    }
//  } else {
//    peaks_[newpeak.center] = newpeak;
//  }
}

void Fitter::make_multiplets()
{
//  if (peaks_.size() > 1) {

//    for (auto &q : peaks_) {
//      q.second.lim_L = q.second.energy - overlap_ * q.second.fwhm_theoretical;
//      q.second.lim_R = q.second.energy + overlap_ * q.second.fwhm_theoretical;
//      q.second.intersects_R = false;
//      q.second.intersects_L = false;
//    }
    
//    std::map<double, Peak>::iterator pk1 = peaks_.begin();
//    std::map<double, Peak>::iterator pk2 = peaks_.begin();
//    pk2++;

//    int juncs=0;
//    while (pk2 != peaks_.end()) {
//      if ((pk1->second.energy > pk2->second.lim_L) || (pk1->second.lim_R > pk2->second.energy)) {
//        pk1->second.intersects_R = true;
//        pk2->second.intersects_L = true;
//        juncs++;
//      }
//      pk1++;
//      pk2++;
//    }
//    //PL_DBG << "<Gamma::Fitter> found " << juncs << " peak overlaps";

//    std::set<Peak> multiplet;
//    std::set<double> to_remove;

//    pk1 = peaks_.begin();
//    pk2 = peaks_.begin();
//    pk2++;
    
//    while (pk2 != peaks_.end()) {
//      if (pk1->second.intersects_R && pk2->second.intersects_L) {
//        multiplet.insert(pk1->second);
//        to_remove.insert(pk1->first);
//      }
//      if (pk2->second.intersects_L && !pk2->second.intersects_R) {
//        multiplet.insert(pk2->second);
//        to_remove.insert(pk2->first);

//        if (!multiplet.empty()) {
//          ROI multi(nrg_cali_, fwhm_cali_, metadata_.live_time.total_seconds());
//          multi.add_peaks(multiplet, finder_.x_, finder_.y_);
//          regions_.push_back(multi);
//        }

//        multiplet.clear();
//      }
//      pk1++;
//      pk2++;
//    }
//    for (auto &q : to_remove)
//      peaks_.erase(q);
//    for (auto &q : regions_) {
//      for (auto &p : q.peaks_)
//        peaks_[p.center] = p;
//    }
//  }
}

void Fitter::remove_peak(double bin) {
  peaks_.erase(bin);
  if (fwhm_cali_.valid()) {
    regions_.clear();
    make_multiplets();
  }
}


void Fitter::remove_peaks(std::set<double> bins) {
  for (auto &q : bins)
    peaks_.erase(q);
  if (fwhm_cali_.valid()) {
    regions_.clear();
    make_multiplets();
  }
}

void Fitter::save_report(std::string filename) {
  std::ofstream file(filename, std::ios::out | std::ios::app);
  file << "Spectrum \"" << metadata_.name << "\"" << std::endl;
  file << "========================================================" << std::endl;
  file << "Bits: " << metadata_.bits << "    Resolution: " << metadata_.resolution << std::endl;

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
  file << std::setw( 15 ) << "center(Gauss)" << "--|" 
       << std::setw( 15 ) << "energy(Gauss)" << "--|"
       << std::setw( 15 ) << "FWHM(Gauss)" << "--|"
       << std::setw( 15 ) << "area(Gauss)" << "--|"
       << std::setw( 15 ) << "cps(Gauss)"  << "-||"
      
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
  for (auto &q : peaks_) {
    file << std::setw( 15 ) << std::setprecision( 10 ) << q.second.center << "  |"
         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.energy << "  |"
         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.fwhm_gaussian << "  |"
         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.area_gauss_ << "  |"
         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.cts_per_sec_gauss_ << " ||";

    if (!q.second.subpeak) {
      file << std::setw( 15 ) << std::setprecision( 10 ) << q.second.sum4_.centroid << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.sum4_.centroid_var << "  |"
           << std::setw( 7 ) << std::setprecision( 10 ) << q.second.sum4_.Lpeak << "  |"
           << std::setw( 7 ) << std::setprecision( 10 ) << q.second.sum4_.Rpeak << "  |"

           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.fwhm_sum4 << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.area_bckg_ << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.sum4_.B_variance << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.area_net_ << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.sum4_.net_variance << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.sum4_.err << "  |"
           << std::setw( 15 ) << std::setprecision( 10 ) << q.second.cts_per_sec_net_ << "  |"
           << std::setw( 5 ) << std::setprecision( 10 ) << q.second.sum4_.currie_quality_indicator << "  |";
    }

    file << std::endl;
  }
  
  file.close();
}


}
