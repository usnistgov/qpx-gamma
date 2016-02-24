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
 *      Qpx::Spectrum::Spectrum1D_LFC for loss-free counting by statistical
 *      extrapolation.
 *
 ******************************************************************************/


#include "spectrum1D_LFC.h"
#include "custom_logger.h"

namespace Qpx {
namespace Spectrum {

static Registrar<Spectrum1D_LFC> registrar("LFC1D");

/////////////LFC 1D////////////////

bool Spectrum1D_LFC::initialize() {
  //add pattern must have exactly one channel
  int adds = 0;
  for (auto &q : metadata_.add_pattern)
    if (q == 1)
      adds++;

  if (adds != 1) {
    PL_WARN << "Invalid 1D_LFC spectrum requested (only 1 channel allowed)";
    return false;
  }
  
  for (int i=0; i < metadata_.add_pattern.size(); i++)
    if (metadata_.add_pattern[i] == 1)
      my_channel_ = i;

  metadata_.resolution = pow(2, 16 - shift_by_);
  channels_all_.resize(metadata_.resolution,0);
  channels_run_.resize(metadata_.resolution,0);

  time_sample_ = get_attr("time_sample").value_dbl;

  return Spectrum1D::initialize();
}


void Spectrum1D_LFC::addHit(const Hit& newHit)
{
    channels_run_[ (newHit.energy >> shift_by_) ] ++;
    count_current_++;
}

void Spectrum1D_LFC::addStats(const StatsUpdate& newStats)
{
  Spectrum1D::addStats(newStats);

  if ((newStats.source_channel < 0) || (newStats.source_channel >= metadata_.add_pattern.size())
      || (newStats.source_channel != my_channel_))
    return;

  if (time1_ == StatsUpdate()) {
    time2_ = time1_ = newStats;
    return;
  }

  double d_lab_time  = (newStats.lab_time - time1_.lab_time).total_microseconds() / 1000000;


  //NO MORE SPILL NUMBERS. MUST IDENTIFY LAST SPILL!!!!
  if (d_lab_time > time_sample_) {
    time2_ = newStats;
    StatsUpdate diff = time2_ - time1_;
    time1_ = time2_;

    PL_TRC << "LFC update chan[" << my_channel_ << "]"
           << " device_time=" << diff.total_time
           << " lab_time=" << d_lab_time
           << " live_time=" << diff.live_time
           << " ftdt=" << diff.ftdt
           << " fast_peaks=" << diff.fast_peaks
           << " FAST_LIVE(live_time-ftdt)=" << (diff.live_time - diff.ftdt);

    
    if (diff.total_time == 0.0)
      return;
    double scale_factor = d_lab_time / diff.total_time;
    double fast_scaled  = (diff.live_time - diff.ftdt) * scale_factor;// * 1000000;

    PL_TRC << "LFC update chan[" << my_channel_ << "]"
           << " scale_factor=" << scale_factor
           << " fast_scaled=" << fast_scaled;
     
    if (fast_scaled == 0.0)
      return;
    double fast_peaks_compensated = diff.fast_peaks * d_lab_time / fast_scaled;
    time1_ = time2_;
  
    count_total_ += fast_peaks_compensated;
    metadata_.total_count = count_total_;

    PL_DBG << "<SpectrumLFC1D> '" << metadata_.name << "' update chan[" << my_channel_ << "]"
           << " fast_peaks_compensated=" << fast_peaks_compensated
           << " true_count=" << count_current_;

    for (uint32_t i = 0; i < metadata_.resolution; i++) {
      channels_all_[i] += fast_peaks_compensated * channels_run_[i] / count_current_;
      if (channels_all_[i] > 0.0)
        spectrum_[i] = channels_all_[i];
      channels_run_[i] = 0.0;
    }
    metadata_.live_time = metadata_.real_time; //think about this...
    count_current_ = 0;
  } else {
    for (uint32_t i = 0; i < metadata_.resolution; i++) {
      if ((channels_run_[i] > 0.0) || (channels_all_[i] > 0.0))
        spectrum_[i] = channels_run_[i] + channels_all_[i].convert_to<uint64_t>();
    }
    metadata_.total_count += count_current_;
    time2_ = newStats;
  }
}

void Spectrum1D_LFC::addRun(const RunInfo& run_info) {
  addStats(time2_);
  Spectrum1D::addRun(run_info);
  metadata_.total_count = 0.0;
  for (auto &q : spectrum_)
    metadata_.total_count += q;
  metadata_.live_time = metadata_.real_time; //think about this...
}

}}
