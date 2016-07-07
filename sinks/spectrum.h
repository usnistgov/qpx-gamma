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
 *      Qpx::Sink generic spectrum type.
 *                       All public methods are thread-safe.
 *                       When deriving override protected methods.
 *
 *      Qpx::SinkFactory creates spectra of appropriate type
 *                               by type name, from template, or
 *                               from file.
 *
 *      Qpx::SinkRegistrar for registering new spectrum types.
 *
 ******************************************************************************/

#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "daq_sink.h"

namespace Qpx {

class Spectrum : public Sink
{
public:
  Spectrum();

protected:
  
  bool _initialize() override;
  void _push_hit(const Hit&) override;
  void _push_stats(const StatsUpdate&) override;
  void _flush() override;

  void _set_detectors(const std::vector<Qpx::Detector>& dets) override;
  void _recalc_axes() override;

  virtual bool validateEvent(const Event&) const;
  virtual void addEvent(const Event&) = 0;


protected:

  std::vector<int32_t> cutoff_logic_;
  std::vector<double>  delay_ns_;

  double max_delay_;
  double coinc_window_;

  std::map<int, std::list<StatsUpdate>> stats_list_;
  std::map<int, boost::posix_time::time_duration> real_times_;
  std::map<int, boost::posix_time::time_duration> live_times_;
  std::vector<int> energy_idx_;

  std::list<Event> backlog;

  uint64_t recent_count_;
  StatsUpdate recent_start_, recent_end_;

  Pattern pattern_coinc_, pattern_anti_, pattern_add_;
  uint16_t bits_;

  PreciseFloat total_hits_;
  PreciseFloat total_events_;
};

}

#endif // SPECTRUM_H
