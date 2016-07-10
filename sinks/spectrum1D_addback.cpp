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


#include "spectrum1D_addback.h"
#include "daq_sink_factory.h"
#include "custom_logger.h"

namespace Qpx {

static SinkRegistrar<SpectrumAddback1D> registrar("Addback 1D");

SpectrumAddback1D::SpectrumAddback1D()
  : Spectrum1D()
{
  Setting base_options = metadata_.attributes();
  metadata_ = Metadata("Addback 1D", "Addback spectrum. Sums energies of all channels selected in add pattern.", 1,
  {}, metadata_.output_types());
  metadata_.overwrite_all_attributes(base_options);
}

void SpectrumAddback1D::addEvent(const Event& newEvent)
{
  uint32_t sum = 0;
  for (auto &h : newEvent.hits)
    if (pattern_add_.relevant(h.first))
      sum += h.second.value(energy_idx_.at(h.second.source_channel())).val(bits_);

  if ((sum < cutoff_bin_) || (sum >= spectrum_.size()))
    return;

  ++spectrum_[sum];
  total_hits_++;

  if (sum > maxchan_)
    maxchan_ = sum;
}


}
