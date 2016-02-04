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

    if (detector_.energy_calibrations_.has_a(Gamma::Calibration("Energy", md.bits)))
      nrg_cali_ = detector_.energy_calibrations_.get(Gamma::Calibration("Energy", md.bits));
    //best?

    if (detector_.fwhm_calibration_.valid())
      fwhm_cali_ = detector_.fwhm_calibration_;

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


void Fitter::find_peaks() {

  peaks_.clear();
  regions_.clear();
//  PL_DBG << "Fitter: looking for " << filtered.size()  << " peaks";

  if (finder_.filtered.empty())
    return;

  int32_t L = finder_.lefts[0];
  int32_t R = finder_.rights[0];
  for (int i=1; i < finder_.filtered.size(); ++i) {
    double margin = 0;
    if (fwhm_cali_.valid() && nrg_cali_.valid())
      margin = overlap_ * fwhm_cali_.transform(nrg_cali_.transform(R, metadata_.bits));
    if (finder_.lefts[i] < (R + 2 * margin) ) {
//      PL_DBG << "cat ROI " << L << " " << R << " " << finder_.lefts[i] << " " << finder_.rights[i];
      L = std::min(L, static_cast<int32_t>(finder_.lefts[i]));
      R = std::max(R, static_cast<int32_t>(finder_.rights[i]));
//      PL_DBG << "postcat ROI " << L << " " << R;
    } else {
//      PL_DBG << "<Fitter> Creating ROI " << L << "-" << R;
      ROI newROI(nrg_cali_, fwhm_cali_, metadata_.bits);
      L -= margin; if (L < 0) L = 0;
      R += margin; if (R >= finder_.x_.size()) R = finder_.x_.size() - 1;
      newROI.set_data(finder_.x_, finder_.y_, L, R);
      regions_.push_back(newROI);
      L = finder_.lefts[i];
      R = finder_.rights[i];
    }
  }
  double margin = 0;
  if (fwhm_cali_.valid() && nrg_cali_.valid())
    margin = overlap_ * fwhm_cali_.transform(nrg_cali_.transform(R, metadata_.bits));
  R += margin; if (R >= finder_.x_.size()) R = finder_.x_.size() - 1;
  ROI newROI(nrg_cali_, fwhm_cali_, metadata_.bits);
  newROI.set_data(finder_.x_, finder_.y_, L, R);
  regions_.push_back(newROI);

  PL_DBG << "<Fitter> Created " << regions_.size() << " regions";

//  int current = 0;
//  for (auto &q : regions_) {
//    PL_DBG << "<Fitter> Fitting region " << current << " of " << regions_.size() << "...";
//    q.auto_fit();
//    current++;
//  }

//  remap_peaks();

}

void Fitter::remap_peaks() {
  peaks_.clear();
  for (auto &q : regions_)
    for (auto &p : q.peaks_) {
      Peak pk = p;
      pk.construct(nrg_cali_, metadata_.live_time.total_milliseconds() * 0.001, metadata_.bits);
      peaks_[pk.center] = pk;
    }

//  PL_DBG << "<Fitter> Mapped " << peaks_.size() << " peaks";
}

void Fitter::add_peak(uint32_t left, uint32_t right) {
//  PL_DBG << "Add new peak " << left << "-" << right;

  bool added = false;
  for (auto &q : regions_) {
    if (q.overlaps(left, right)) {
//      PL_DBG << "<Fitter> adding to existing region";
      q.add_peak(finder_.x_, finder_.y_, left, right);
      added = true;
      break;
    }
  }

  if (!added) {
    PL_DBG << "<Fitter> making new ROI to add peak manually";
    ROI newROI(nrg_cali_, fwhm_cali_, metadata_.bits);
    newROI.set_data(finder_.x_, finder_.y_, left, right);
    newROI.auto_fit();
    if (!newROI.peaks_.empty()) {
      PL_DBG << "<Fitter> succeeded making new ROI";
      regions_.push_back(newROI);
      added = true;
    }
  }

  if (added)
    remap_peaks();

}

void Fitter::remove_peaks(std::set<double> bins) {
  for (auto &m : regions_)
    m.remove_peaks(bins);

  std::list<ROI> regions;
  for (auto &r : regions_)
    if (!r.peaks_.empty())
      regions.push_back(r);

  regions_ = regions;
  remap_peaks();
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
         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.fwhm_hyp << "  |"
         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.area_hyp_ << "  |"
         << std::setw( 15 ) << std::setprecision( 10 ) << q.second.cts_per_sec_hyp_ << " ||";

    if (!q.second.sum4_.fwhm > 0) {
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
