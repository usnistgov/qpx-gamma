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
#include "custom_logger.h"

namespace Gamma {

void Fitter::setXY(std::vector<double> x, std::vector<double> y, uint16_t avg_window)
{
  x_.clear(); y_.clear();
  if (y.size() == x.size()) {
    x_ = x;
    y_ = y;
  }
  x_nrg_ = nrg_cali_.transform(x);
  set_mov_avg(avg_window);
  deriv();
}


void Fitter::setXY(std::vector<double> x, std::vector<double> y,  uint16_t min, uint16_t max, uint16_t avg_window)
{
  x_.clear(); y_.clear(); x_nrg_.clear();
  if ((y.size() == x.size()) && (min < max) && ((max+1) < x.size())) {
    for (int i=min; i<=max; ++i) {
      x_.push_back(x[i]);
      y_.push_back(y[i]);
    }
    x_nrg_ = nrg_cali_.transform(x_);
  }  
  set_mov_avg(avg_window);
}

void Fitter::setData(Qpx::Spectrum::Spectrum* spectrum)
{
  clear();
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

    int i = 0;
    for (auto it : *spectrum_dump) {
      x_.push_back(static_cast<double>(i));
      y_.push_back(it.second);
      i++;
    }

    x_nrg_ = nrg_cali_.transform(x_);
    set_mov_avg(avg_window_);
  }
}

void Fitter::clear() {
  detector_ = Gamma::Detector();
  metadata_ = Qpx::Spectrum::Metadata();
  nrg_cali_ = Gamma::Calibration();
  fwhm_cali_ = Gamma::Calibration();
  x_.clear();
  x_nrg_.clear();
  y_.clear();
  y_avg_.clear();
  deriv1.clear();
  deriv2.clear();
  prelim.clear();
  filtered.clear();
  lefts.clear();
  rights.clear();
  lefts_t.clear();
  rights_t.clear();
  peaks_.clear();
  multiplets_.clear();
}

void Fitter::set_mov_avg(uint16_t window) {
  y_avg_ = y_;

  if ((window % 2) == 0)
    window++;

  if (y_.size() < window)
    return;

  uint16_t half = (window - 1) / 2;

  //assume values under 0 are same as for index 0
  double avg = (half + 1) * y_[0];

  //begin averaging over the first few
  for (int i = 0; i < half; ++i)
    avg += y_[i];

  avg /= window;

  double remove, add;
  for (int i=0; i < y_.size(); i++) {
    if (i < (half+1))
      remove = y_[0] / window;
    else
      remove = y_[i-(half+1)] / window;

    if ((i + half) > y_.size())
      add = y_[y_.size() - 1] / window;
    else
      add = y_[i + half] / window;

    avg = avg - remove + add;

    y_avg_[i] = avg;
  }

  deriv();
}


void Fitter::deriv() {
  if (!y_avg_.size())
    return;

  deriv1.clear();
  deriv2.clear();

  deriv1.push_back(0);
  for (int i=1; i < y_avg_.size(); ++i) {
    deriv1.push_back(y_avg_[i] - y_avg_[i-1]);
  }

  deriv2.push_back(0);
  for (int i=1; i < deriv1.size(); ++i) {
    deriv2.push_back(deriv1[i] - deriv1[i-1]);
  }
}


void Fitter::find_prelim() {
  prelim.clear();
  std::string dbg_list("prelim peaks ");

  int was = 0, is = 0;

  for (int i = 0; i < deriv1.size(); ++i) {
    if (deriv1[i] > 0)
      is = 1;
    else if (deriv1[i] < 0)
      is = -1;
    else
      is = 0;

    if ((was == 1) && (is == -1)) {
      prelim.push_back(i);
      dbg_list += std::to_string(i) + " ";
    }

    was = is;
  }
  //PL_DBG << dbg_list;
}

void Fitter::filter_prelim(uint16_t min_width) {
  filtered.clear();
  lefts.clear();
  rights.clear();

  std::string dbg_list("filtered (minw=");
  dbg_list += std::to_string(min_width) + ") peaks ";

  if ((y_.size() < 3) || !prelim.size())
    return;

  for (auto &q : prelim) {
    if (!q)
      continue;
    uint16_t left = 0, right = 0;
    for (int i=q-1; i >= 0; --i)
      if (deriv1[i] > 0)
        left++;
      else
        break;

    for (int i=q; i < deriv1.size(); ++i)
      if (deriv1[i] < 0)
        right++;
      else
        break;

    if ((left >= min_width) && (right >= min_width)) {
      lefts.push_back(q-left-1);
      filtered.push_back(q-1);
      rights.push_back(q+right-1);
      dbg_list += std::to_string(q-left-1) + "/" + std::to_string(q-1) + "\\" + std::to_string(q+right-1) + " ";
    }
  }
  //  PL_DBG << dbg_list;
}

void Fitter::refine_edges(double threshl, double threshr) {
  lefts_t.clear();
  rights_t.clear();

  for (int i=0; i<filtered.size(); ++i) {
    uint16_t left = lefts[i],
        right = rights[i];

    for (int j=lefts[i]; j < filtered[i]; ++j) {
      if (deriv1[j] < threshl)
        left = j;
    }

    for (int j=rights[i]; j > filtered[i]; --j) {
      if ((-deriv1[j]) < threshr)
        right = j;
    }

    lefts_t.push_back(left);
    rights_t.push_back(right);
  }
}

uint16_t Fitter::find_left(uint16_t chan, uint16_t grace) {
  if ((chan - grace < 0) || (chan >= x_.size()))
    return 0;

  int i = chan - grace;
  while ((i >= 0) && (deriv1[i] > 0))
    i--;
  return i;

  
}

uint16_t Fitter::find_right(uint16_t chan, uint16_t grace) {
  if ((chan < 0) || (chan + grace >= x_.size()))
    return x_.size() - 1;

  int i = chan + grace;
  while ((i < x_.size()) && (deriv1[i] < 0))
    i++;
  return i;
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


void Fitter::find_peaks(int min_width) {
  find_prelim();
  filter_prelim(min_width);

  peaks_.clear();
  multiplets_.clear();
  for (int i=0; i < filtered.size(); ++i) {
    //std::vector<double> baseline = make_background(x_, y_, lefts[i], rights[i], 3);
    //std::vector<double> xx(x_.begin() + lefts[i], x_.begin() + rights[i] + 1);
    //std::vector<double> yy(y_.begin() + lefts[i], y_.begin() + rights[i] + 1);
    Peak fitted = Peak(x_, y_, lefts[i],rights[i], nrg_cali_, fwhm_cali_, metadata_.live_time.total_seconds());

    if (
        (fitted.height > 0) &&
        (fitted.fwhm_sum4 > 0) &&
        (fitted.fwhm_gaussian > 0) &&
        (fitted.fwhm_pseudovoigt > 0) &&
        (lefts[i] < fitted.center) &&
        (fitted.center < rights[i])
       )
    {
      peaks_[fitted.center] = fitted;
    }
  }

  if (fwhm_cali_.valid()) {
    //    PL_DBG << "<GammaFitter> Valid FWHM calib found, performing filtering/deconvolution";
    filter_by_theoretical_fwhm(0.25);

    //    PL_DBG << "filtered by theoretical fwhm " << peaks_.size();

    make_multiplets();
  }

  //  PL_INFO << "Preliminary search found " << prelim.size() << " potential peaks";
  //  PL_INFO << "After minimum width filter: " << filtered.size();
  //  PL_INFO << "Fitted peaks: " << peaks_.size();
}

void Fitter::add_peak(uint32_t left, uint32_t right) {
  //std::vector<double> xx(x_.begin() + left, x_.begin() + right + 1);
  //std::vector<double> yy(y_.begin() + left, y_.begin() + right + 1);
  //std::vector<double> bckgr = make_background(x_, y_, left, right, 3);
  
  Peak newpeak = Gamma::Peak(x_, y_, left, right, nrg_cali_, fwhm_cali_, metadata_.live_time.total_seconds());
  PL_DBG << "new peak center = " << newpeak.center;

  peaks_[newpeak.center] = newpeak;
  if (fwhm_cali_.valid()) {
    multiplets_.clear();
    make_multiplets();
  }
}

void Fitter::make_multiplets()
{
  if (peaks_.size() > 1) {

    for (auto &q : peaks_) {
      q.second.lim_L = q.second.energy - overlap_ * q.second.fwhm_theoretical;
      q.second.lim_R = q.second.energy + overlap_ * q.second.fwhm_theoretical;
      q.second.intersects_R = false;
      q.second.intersects_L = false;
    }
    
    std::map<double, Peak>::iterator pk1 = peaks_.begin();
    std::map<double, Peak>::iterator pk2 = peaks_.begin();
    pk2++;

    int juncs=0;
    while (pk2 != peaks_.end()) {
      if ((pk1->second.energy > pk2->second.lim_L) || (pk1->second.lim_R > pk2->second.energy)) {
        pk1->second.intersects_R = true;
        pk2->second.intersects_L = true;
        juncs++;
      }
      pk1++;
      pk2++;
    }
    //    PL_DBG << "<Gamma::Fitter> found " << juncs << " peak overlaps";

    std::set<Peak> multiplet;
    std::set<double> to_remove;

    pk1 = peaks_.begin();
    pk2 = peaks_.begin();
    pk2++;
    
    while (pk2 != peaks_.end()) {
      if (pk1->second.intersects_R && pk2->second.intersects_L) {
        multiplet.insert(pk1->second);
        to_remove.insert(pk1->first);
      }
      if (pk2->second.intersects_L && !pk2->second.intersects_R) {
        multiplet.insert(pk2->second);
        to_remove.insert(pk2->first);

        if (!multiplet.empty()) {
          Multiplet multi(nrg_cali_, fwhm_cali_, metadata_.live_time.total_seconds());
          multi.add_peaks(multiplet, x_, y_);
          multiplets_.push_back(multi);
        }

        multiplet.clear();
      }
      pk1++;
      pk2++;
    }
    for (auto &q : to_remove)
      peaks_.erase(q);
    for (auto &q : multiplets_) {
      for (auto &p : q.peaks_)
        peaks_[p.center] = p;
    }
  }  
}

void Fitter::remove_peak(double bin) {
  peaks_.erase(bin);
  if (fwhm_cali_.valid()) {
    multiplets_.clear();
    make_multiplets();
  }
}


void Fitter::remove_peaks(std::set<double> bins) {
  for (auto &q : bins)
    peaks_.erase(q);
  if (fwhm_cali_.valid()) {
    multiplets_.clear();
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
