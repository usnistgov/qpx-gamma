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
 *      Pixie::Simulator generates events based on discrete distribution
 *
 ******************************************************************************/


#include "simulator.h"

#include <utility>
#include "custom_logger.h"

namespace Pixie {

Simulator::Simulator(SpectraSet* all_spectra, std::array<int,2> chans,
                     int source_res, int dest_res) {
  channels_ = chans;
  shift_by_ = 0;
  resolution_ = 0;
  count_ = 0;

  PL_INFO << "Creating simulation with precision of " << dest_res
          << " bits from source with " << source_res << " bits";

  Spectrum::Spectrum* co = nullptr;
  if ((chans[0] < 0) || (chans[1] < 0) || (chans[0] >= kNumChans) || (chans[1] >= kNumChans)) {
    PL_WARN << "channels requested for simulation not in physical range"; return;
  }

  if (source_res < dest_res) {
    PL_WARN << "Cannot create simulation. Source resolution must be >= resolution precision.";
    return;
  }
  
  std::list<Spectrum::Spectrum*> co_found = all_spectra->spectra(2, source_res);
  if (co_found.empty()) { PL_WARN << "no 2d source found"; return; }

  for (auto &q: co_found)
    if ((q->add_pattern()[chans[0]] && q->add_pattern()[chans[1]]) &&
        (!q->match_pattern()[chans[0]] && !q->match_pattern()[chans[1]]))
      co = q;

  if (co == nullptr) { PL_WARN << "no 2d source with matching pattern"; return; }

  PL_INFO << " source total events coinc " << co->total_count();

  settings = all_spectra->runInfo().p4_state;
  lab_time = (all_spectra->runInfo().time_stop - all_spectra->runInfo().time_start).total_milliseconds() * 0.001;

  if (lab_time == 0.0) {
    PL_WARN << "Lab time = 0. Cannot create simulation.";
    return;
  }
  
  count_ = co->total_count();

  time_factor = settings.get_mod("TOTAL_TIME") / lab_time;
  OCR = static_cast<double>(count_) / lab_time;

  int adjust_bits = source_res - dest_res;
    
  shift_by_ = 16 - dest_res;
  resolution_ = pow(2, dest_res);
  
  PL_INFO << " building matrix for simulation";
  std::vector<double> distribution(resolution_*resolution_, 0.0);   //optimize somehow

  std::unique_ptr<std::list<Pixie::Spectrum::Entry>> spec_list(co->get_spectrum({{0,co->resolution()},{0,co->resolution()}}));
  
  for (auto it : *spec_list)
    distribution[(it.first[0] >> adjust_bits) * resolution_
                 + (it.first[1] >> adjust_bits)]
        = static_cast<double>(it.second) / static_cast<double> (count_);

  PL_INFO << " creating discrete distribution for simulation";
  dist_ = boost::random::discrete_distribution<>(distribution);

  if (shift_by_) {
    int size_granular = pow(2, shift_by_);
    std::vector<double> distri_granular(size_granular, 1.0);
    refined_dist_ = boost::random::discrete_distribution<>(distri_granular);
    //Not physically accurate, of course
  }
  valid_ = true;
}

Hit Simulator::getHit() {
  Hit newHit;
  
  newHit.run_type = 0x0103;
    
  if (resolution_ > 0) {
    uint64_t newpoint = dist_(gen);
    uint32_t en1 = newpoint / resolution_;
    uint32_t en2 = newpoint % resolution_;

    if (en1)
      newHit.pattern[channels_[0]+8] = 1;
    if (en2)
      newHit.pattern[channels_[1]+8] = 1;

    en1 = en1 << shift_by_;
    en2 = en2 << shift_by_;
    
    if (shift_by_) {
      en1 += refined_dist_(gen);
      en2 += refined_dist_(gen);
    }
    
    newHit.energy[channels_[0]] = en1;
    newHit.energy[channels_[1]] = en2;
  }

  return newHit;
};

StatsUpdate Simulator::getBlock(double duration) {
  StatsUpdate newBlock;
  double fraction;
  
  newBlock.total_time = duration * time_factor;
  newBlock.event_rate = duration * OCR;

  if (lab_time == 0.0)
    fraction = duration;
  else
    fraction = duration / lab_time;

  for (int i = 0; i<2; i++) {
    newBlock.fast_peaks[channels_[i]] = settings.get_chan("FAST_PEAKS", Channel(channels_[i])) * fraction;
    newBlock.live_time[channels_[i]]  = settings.get_chan("LIVE_TIME", Channel(channels_[i])) * fraction;
    newBlock.ftdt[channels_[i]]       = settings.get_chan("FTDT", Channel(channels_[i])) * fraction;
    newBlock.sfdt[channels_[i]]       = settings.get_chan("SFDT", Channel(channels_[i])) * fraction;
  }
      

  return newBlock;
}



}
