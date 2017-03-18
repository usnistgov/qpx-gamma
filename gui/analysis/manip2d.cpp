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
#include "consumer_factory.h"

namespace Qpx
{

SinkPtr derive_empty_1d(SinkPtr source, bool det1)
{
  if (!source)
    return nullptr;

  ConsumerMetadata md = source->metadata();
  if (md.dimensions() != 2)
    return nullptr;

  Setting pattern = md.get_attribute("pattern_add");
  auto gates = pattern.value_pattern.gates();
  int chan1{-1};
  int chan2{-1};
  for (size_t i=0; i < gates.size(); ++i)
  {
    if (gates[i] && (chan1 == -1))
      chan1 = i;
    else if (gates[i] && (chan2 == -1))
      chan2 = i;
    else if (gates[i])
      return nullptr;
  }

  if ((chan1 == -1) && (chan2 == -1))
    return nullptr;

  if (det1)
    gates[chan2] = false;
  else
    gates[chan1] = false;

  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  md.set_attribute(pattern);

  ConsumerMetadata temp = ConsumerFactory::getInstance().create_prototype("1D");
  temp.set_attributes(md.get_all_attributes());

  auto ret = ConsumerFactory::getInstance().create_from_prototype(temp);
  if (ret)
    ret->set_detectors(md.detectors);
  return ret;
}


SinkPtr slice_rectangular(SinkPtr source, std::initializer_list<Pair> bounds, bool det1)
{
  if (bounds.size() != 2)
    return nullptr;

  SinkPtr ret = derive_empty_1d(source, det1);
  if (!ret)
    return nullptr;

  std::shared_ptr<EntryList> spectrum_data = std::move(source->data_range(bounds));
  if (det1)
    for (auto it : *spectrum_data)
    {
      Entry entry({it.first[0]}, it.second);
      ret->append(entry);
    }
  else
    for (auto it : *spectrum_data)
    {
      Entry entry({it.first[1]}, it.second);
      ret->append(entry);
    }

  ret->flush();

  return ret;
}

SinkPtr slice_diagonal_x(SinkPtr source, size_t xc, size_t yc, size_t width,
                         size_t minx, size_t maxx)
{
  SinkPtr destination = derive_empty_1d(source, true);
  if (!destination)
    return nullptr;

  int diag_width = std::round(std::sqrt((width*width)/2.0));
  if ((diag_width % 2) == 0)
    diag_width++;

  size_t tot = xc + yc;
  for (size_t i=0; i < tot; ++i) {
    if ((i >= minx) && (i < maxx)) {
      Entry entry({i}, sum_diag(source, i, tot-i, diag_width));
      destination->append(entry);
    }
  }

  destination->flush();
  return destination;
}

SinkPtr slice_diagonal_y(SinkPtr source, size_t xc, size_t yc, size_t width,
                         size_t miny, size_t maxy)
{
  SinkPtr destination = derive_empty_1d(source, false);
  if (!destination)
    return nullptr;

  int diag_width = std::round(std::sqrt((width*width)/2.0));
  if ((diag_width % 2) == 0)
    diag_width++;

  size_t tot = xc + yc;
  for (size_t i=0; i < tot; ++i) {
    if ((i >= miny) && (i < maxy)) {
      Entry entry({i}, sum_diag(source, tot-i, i, diag_width));
      destination->append(entry);
    }
  }

  destination->flush();
  return destination;
}

PreciseFloat sum_diag(SinkPtr source, size_t x, size_t y, size_t width)
{
  PreciseFloat ans = sum_with_neighbors(source, x, y);
  int w = (width-1)/2;
  for (int i=1; i < w; ++i)
    ans += sum_with_neighbors(source, x-i, y-i) + sum_with_neighbors(source, x+i, y+i);
  return ans;
}


PreciseFloat sum_with_neighbors(SinkPtr source, size_t x, size_t y)
{
  PreciseFloat ans = 0;
  ans += source->data({x,y}) + 0.25 * (source->data({x+1,y}) + source->data({x,y+1}));
  if (x != 0)
    ans += 0.25 * source->data({x-1,y});
  if (y != 0)
    ans += 0.25 * source->data({x,y-1});
  return ans;
}

SinkPtr make_symmetrized(SinkPtr source)
{
  if (!source)
    return nullptr;

  ConsumerMetadata md = source->metadata();

  if (md.dimensions() != 2)
    return nullptr;

  SinkPtr ret = ConsumerFactory::getInstance().create_from_prototype(md); //assume 2D?

  if (!ret)
    return nullptr;

  Detector detector1;
  Detector detector2;

  if (md.detectors.size() > 1)
  {
    detector1 = md.detectors[0];
    detector2 = md.detectors[1];
  } else {
    WARN << "<::MakeSymmetrize> " << md.get_attribute("name").value_text
         << " does not have 2 detector definitions";
    return nullptr;
  }

  size_t bits = md.get_attribute("resolution").value_int;
  Calibration gain_match_cali = detector2.get_gain_match(bits, detector1.name_);

  if (gain_match_cali.to_ == detector1.name_)
    LINFO << "<::MakeSymmetrize> using gain match calibration from "
          << detector2.name_ << " to " << detector1.name_
          << " " << gain_match_cali.to_string();
  else {
    WARN << "<::MakeSymmetrize> no appropriate gain match calibration from "
         << detector2.name_ << " to " << detector1.name_;
    return nullptr;
  }

  size_t adjrange = pow(2, bits);

  boost::random::mt19937 gen;
  boost::random::uniform_real_distribution<> dist(-0.5, 0.5);

  std::unique_ptr<std::list<Entry>> spectrum_data = std::move(source->data_range({{0, adjrange}, {0, adjrange}}));
  for (auto it : *spectrum_data)
  {
    size_t e1 = it.first[0];

    double xformed = gain_match_cali.transform(it.first[1]);

    for (int i=0; i < it.second; ++i)
    {
      size_t e2 = std::max(static_cast<size_t>(0),
                           static_cast<size_t>(std::round(xformed + dist(gen))));
      ret->append(Entry({e1, e2}, 1));
      ret->append(Entry({e2, e1}, 1));
    }
  }

  for (auto &p : md.detectors)
  {
    if (p.shallow_equals(detector1) || p.shallow_equals(detector2))
    {
      p = Detector(detector1.name_ + std::string("*") + detector2.name_);
      p.energy_calibrations_.add(detector1.energy_calibrations_.get(Calibration("Energy", bits)));
    }
  }

  ret->set_detectors(md.detectors);

  Qpx::Setting app = md.get_attribute("symmetrized");
  app.value_int = true;
  ret->set_attribute(app);

  ret->flush();

  if (ret->metadata().get_attribute("total_events").value_precise > 0)
    return ret;
  else
    return nullptr;
}


}
