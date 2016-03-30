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
 *      Manip2d set of functions for transforming spectra
 *
 ******************************************************************************/

#include "manip2d.h"
#include "custom_logger.h"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>

namespace Qpx {

bool slice_diagonal(std::shared_ptr<Sink> source, std::shared_ptr<Sink> destination, uint32_t xc, uint32_t yc, uint32_t width, uint32_t minx, uint32_t maxx) {
  if (source == nullptr)
    return false;
  if (destination == nullptr)
    return false;

  Metadata md = source->metadata();

  if ((md.total_count > 0) && (md.dimensions() == 2))
  {
    destination->set_detectors(md.detectors);

    int diag_width = std::round(std::sqrt((width*width)/2.0));
    if ((diag_width % 2) == 0)
      diag_width++;

    int tot = xc + yc;
    for (int i=0; i < tot; ++i) {
      if ((i >= minx) && (i < maxx)) {
        Entry entry({i}, sum_diag(source, i, tot-i, diag_width));
        destination->append(entry);
      }
    }

  }

  return (destination->metadata().total_count > 0);
}


bool slice_rectangular(std::shared_ptr<Sink> source, std::shared_ptr<Sink> destination, std::initializer_list<Pair> bounds) {
  if (source == nullptr)
    return false;
  if (destination == nullptr)
    return false;

  Metadata md = source->metadata();

  if ((md.total_count > 0) && (md.dimensions() == 2))
  {
    destination->set_detectors(md.detectors);

    std::shared_ptr<EntryList> spectrum_data = std::move(source->data_range(bounds));
    for (auto it : *spectrum_data)
      destination->append(it);

  }

  return (destination->metadata().total_count > 0);
}

PreciseFloat sum_with_neighbors(std::shared_ptr<Sink> source, uint16_t x, uint16_t y)
{
  PreciseFloat ans = 0;
  ans += source->data({x,y}) + 0.25 * (source->data({x+1,y}) + source->data({x,y+1}));
  if (x != 0)
    ans += 0.25 * source->data({x-1,y});
  if (y != 0)
    ans += 0.25 * source->data({x,y-1});
  return ans;
}

PreciseFloat sum_diag(std::shared_ptr<Sink> source, uint16_t x, uint16_t y, uint16_t width)
{
  PreciseFloat ans = sum_with_neighbors(source, x, y);
  int w = (width-1)/2;
  for (int i=1; i < w; ++i)
    ans += sum_with_neighbors(source, x-i, y-i) + sum_with_neighbors(source, x+i, y+i);
  return ans;
}

bool symmetrize(std::shared_ptr<Sink> source, std::shared_ptr<Sink> destination)
{
  if (source == nullptr)
    return false;
  if (destination == nullptr)
    return false;


  Metadata md = source->metadata();

  if ((md.total_count == 0) || (md.dimensions() != 2)) {
    PL_WARN << "<symmetrize> " << md.name << " has no events or is not 2d";
    return false;
  }

  Detector detector1;
  Detector detector2;

  if (md.detectors.size() > 1) {
    detector1 = md.detectors[0];
    detector2 = md.detectors[1];
  } else {
    PL_WARN << "<symmetrize> " << md.name << " does not have 2 detector definitions";
    return false;
  }

  Calibration gain_match_cali = detector2.get_gain_match(md.bits, detector1.name_);

  if (gain_match_cali.to_ == detector1.name_)
    PL_INFO << "<symmetrize> using gain match calibration from " << detector2.name_ << " to " << detector1.name_ << " " << gain_match_cali.to_string();
  else {
    PL_WARN << "<symmetrize> no appropriate gain match calibration";
    return false;
  }

  uint32_t adjrange = pow(2, md.bits);

  boost::random::mt19937 gen;
  boost::random::uniform_real_distribution<> dist(-0.5, 0.5);

  uint16_t e2 = 0;
  std::unique_ptr<std::list<Entry>> spectrum_data = std::move(source->data_range({{0, adjrange}, {0, adjrange}}));
  for (auto it : *spectrum_data) {
    PreciseFloat count = it.second;

    //PL_DBG << "adding " << it.first[0] << "+" << it.first[1] << "  x" << it.second;
    double xformed = gain_match_cali.transform(it.first[1]);
    double e1 = it.first[0];

    it.second = 1;
    for (int i=0; i < count; ++i) {
      double plus = dist(gen);
      double xfp = xformed + plus;

      if (xfp > 0)
        e2 = static_cast<uint16_t>(std::round(xfp));
      else
        e2 = 0;

      //PL_DBG << xformed << " plus " << plus << " = " << xfp << " round to " << e2;

      it.first[0] = e1;
      it.first[1] = e2;

      destination->append(it);

      it.first[0] = e2;
      it.first[1] = e1;

      destination->append(it);

    }
  }

  for (auto &p : md.detectors) {
    if (p.shallow_equals(detector1) || p.shallow_equals(detector2)) {
      p = Detector(detector1.name_ + std::string("*") + detector2.name_);
      p.energy_calibrations_.add(detector1.energy_calibrations_.get(Calibration("Energy", md.bits)));
    }
  }

  destination->set_detectors(md.detectors);

  return (destination->metadata().total_count > 0);
}


}
