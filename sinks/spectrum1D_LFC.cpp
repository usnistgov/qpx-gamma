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
 *      Qpx::Sink1D_LFC for loss-free counting by statistical
 *      extrapolation.
 *
 ******************************************************************************/


#include "spectrum1D_LFC.h"
#include "daq_sink_factory.h"
#include "custom_logger.h"

namespace Qpx {

static SinkRegistrar<Spectrum1D_LFC> registrar("LFC1D");

Spectrum1D_LFC::Spectrum1D_LFC()
  : Spectrum1D()
{
  Setting base_options = metadata_.attributes;
  metadata_ = Metadata("LFC1D", "One detector loss-free spectrum", 1,
                       {}, metadata_.output_types());
  metadata_.attributes = base_options;

  Qpx::Setting t_sample;
  t_sample.id_ = "time_sample";
  t_sample.metadata.setting_type = Qpx::SettingType::floating;
  t_sample.metadata.unit = "seconds";
  t_sample.value_dbl = 20.0;
  t_sample.metadata.minimum = 0;
  t_sample.metadata.step = 1;
  t_sample.metadata.maximum = 3600.0;
  t_sample.metadata.description = "minimum \u0394t before compensating";
  t_sample.metadata.writable = true;
  t_sample.metadata.flags.insert("preset");
  metadata_.attributes.branches.add(t_sample);
}

bool Spectrum1D_LFC::_initialize() {
  if (!Spectrum1D::_initialize())
    return false;

  //add pattern must have exactly one channel
  int adds = 0;
  std::vector<bool> gts = pattern_add_.gates();
  for (int i=0; i < gts.size(); ++i) {
    if (gts[i]) {
      adds++;
      my_channel_ = i;
    }
  }

  if (adds != 1) {
    PL_WARN << "Invalid 1D_LFC spectrum requested (only 1 channel allowed)";
    return false;
  }
  
  channels_all_.resize(pow(2, metadata_.bits),0);
  channels_run_.resize(pow(2, metadata_.bits),0);

  time_sample_ = get_attr("time_sample").value_dbl;

  return true;
}


void Spectrum1D_LFC::addHit(const Hit& newHit)
{
    channels_run_[ newHit.energy.val(metadata_.bits) ] ++;
    count_current_++;
}

void Spectrum1D_LFC::_push_stats(const StatsUpdate& newStats)
{
  Spectrum1D::_push_stats(newStats);

  if (newStats.source_channel != my_channel_)
    return;

  if (time1_ == StatsUpdate()) {
    time2_ = time1_ = newStats;
    return;
  }

  PreciseFloat d_lab_time  = (newStats.lab_time - time1_.lab_time).total_microseconds() * 0.000001;

  //NO MORE SPILL NUMBERS. MUST IDENTIFY LAST SPILL!!!!
  if (d_lab_time > time_sample_) {
    time2_ = newStats;
    StatsUpdate diff = time2_ - time1_;
    time1_ = time2_;

    PreciseFloat scale_factor = 1;
    if (diff.items.count("native_time") && (diff.items.at("native_time") > 0))
      scale_factor = d_lab_time / diff.items.at("native_time");

    PreciseFloat live = d_lab_time;
    if (diff.items.count("live_trigger"))
      live = diff.items.at(("live_trigger"));
    else if (diff.items.count("live_time"))
      live = diff.items.at(("live_time"));

    PreciseFloat fast_scaled  = live * scale_factor;

    PreciseFloat fast_peaks_compensated = count_current_;

    if ((fast_scaled > 0) && diff.items.count("trigger_count"))
      fast_peaks_compensated = diff.items.at("trigger_count") * d_lab_time / fast_scaled;

    time1_ = time2_;
  
    count_total_ += fast_peaks_compensated;
    metadata_.total_count = count_total_;

    PL_DBG << "<SpectrumLFC1D> '" << metadata_.name << "' update chan[" << my_channel_ << "]"
           << " fast_peaks_compensated=" << fast_peaks_compensated
           << " true_count=" << count_current_;

    uint32_t res = pow(2, metadata_.bits);
    for (uint32_t i = 0; i < res; i++) {
      channels_all_[i] += fast_peaks_compensated * channels_run_[i] / count_current_;
      if (channels_all_[i] > 0.0)
        spectrum_[i] = channels_all_[i];
      channels_run_[i] = 0.0;
    }
    Setting real_time = get_attr("real_time");
    Setting live_time = get_attr("live_time");
    live_time.value_duration = real_time.value_duration;
    metadata_.attributes.branches.replace(live_time);

    count_current_ = 0;
  } else {
    uint32_t res = pow(2, metadata_.bits);
    for (uint32_t i = 0; i < res; i++) {
      if ((channels_run_[i] > 0.0) || (channels_all_[i] > 0.0))
        spectrum_[i] = channels_run_[i] + channels_all_[i].convert_to<uint64_t>();
    }
    metadata_.total_count += count_current_;
    time2_ = newStats;
  }
}

}