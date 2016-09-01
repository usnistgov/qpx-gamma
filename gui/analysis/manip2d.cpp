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
#include "daq_sink_factory.h"

namespace Qpx {

SinkPtr slice_rectangular(SinkPtr source, std::initializer_list<Pair> bounds, bool det1) {
  if (!source)
    return nullptr;

  Metadata md = source->metadata();
  if ((md.get_attribute("total_events").value_precise <= 0) || (md.dimensions() != 2))
    return nullptr;


  Metadata temp = SinkFactory::getInstance().create_prototype("1D");

  Setting name = md.get_attribute("name");
  name.value_text += " projection";
  temp.set_attribute(name);

  temp.set_attribute(md.get_attribute("resolution"));
  //GENERALIZE!!!

  Setting pattern;
  pattern = temp.get_attribute("pattern_coinc");
  pattern.value_pattern.set_gates(std::vector<bool>({1,1}));
  pattern.value_pattern.set_theshold(2);
  temp.set_attribute(pattern);

  pattern = temp.get_attribute("pattern_add");
  if (det1)
    pattern.value_pattern.set_gates(std::vector<bool>({1,0}));
  else
    pattern.value_pattern.set_gates(std::vector<bool>({0,1}));
  pattern.value_pattern.set_theshold(1);
  temp.set_attribute(pattern);

  SinkPtr ret = SinkFactory::getInstance().create_from_prototype(temp);
  if (!ret)
    return nullptr;

  ret->set_detectors(md.detectors);

  std::shared_ptr<EntryList> spectrum_data = std::move(source->data_range(bounds));
  for (auto it : *spectrum_data)
    ret->append(it);

  if (ret->metadata().get_attribute("total_events").value_precise > 0)
    return ret;
  else
    return nullptr;
}

bool slice_diagonal_x(SinkPtr source, SinkPtr destination, size_t xc, size_t yc, size_t width, size_t minx, size_t maxx) {
  if (source == nullptr)
    return false;
  if (destination == nullptr)
    return false;

  Metadata md = source->metadata();

  if ((md.get_attribute("total_events").value_precise > 0) && (md.dimensions() == 2))
  {
    destination->set_detectors(md.detectors);

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

  }

  return (destination->metadata().get_attribute("total_events").value_precise > 0);
}

bool slice_diagonal_y(SinkPtr source, SinkPtr destination, size_t xc, size_t yc, size_t width, size_t miny, size_t maxy) {
  if (source == nullptr)
    return false;
  if (destination == nullptr)
    return false;

  Metadata md = source->metadata();

  if ((md.get_attribute("total_events").value_precise > 0) && (md.dimensions() == 2))
  {
    destination->set_detectors(md.detectors);

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

  }

  return (destination->metadata().get_attribute("total_events").value_precise > 0);
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

  Metadata md = source->metadata();

  if ((md.get_attribute("total_events").value_precise <= 0) || (md.dimensions() != 2)) {
    WARN << "<::MakeSymmetrize> " << md.get_attribute("name").value_text << " has no events or is not 2d";
    return nullptr;
  }

  std::vector<size_t> chans;
  Setting pattern;
  pattern = md.get_attribute("pattern_add");
  std::vector<bool> gts = pattern.value_pattern.gates();

  for (size_t i=0; i < gts.size(); ++i) {
    if (gts[i])
      chans.push_back(i);
  }

  if (chans.size() != 2) {
    WARN << "<::MakeSymmetrize> " << md.get_attribute("name").value_text << " does not have 2 channels in add pattern";
    return nullptr;
  }

  pattern = md.get_attribute("pattern_coinc");
  pattern.value_pattern.set_gates(gts);
  pattern.value_pattern.set_theshold(2);
  md.set_attribute(pattern);

  SinkPtr ret = SinkFactory::getInstance().create_from_prototype(md); //assume 2D?

  if (!ret) {
    WARN << "<::MakeSymmetrize> symmetrization of " << md.get_attribute("name").value_text << " could not be created";
    return nullptr;
  }

  Detector detector1;
  Detector detector2;

  if (md.detectors.size() > 1) {
    detector1 = md.detectors[0];
    detector2 = md.detectors[1];
  } else {
    WARN << "<::MakeSymmetrize> " << md.get_attribute("name").value_text << " does not have 2 detector definitions";
    return nullptr;
  }

  size_t bits = md.get_attribute("resolution").value_int;
  Calibration gain_match_cali = detector2.get_gain_match(bits, detector1.name_);

  if (gain_match_cali.to_ == detector1.name_)
    LINFO << "<::MakeSymmetrize> using gain match calibration from " << detector2.name_ << " to " << detector1.name_ << " " << gain_match_cali.to_string();
  else {
    WARN << "<::MakeSymmetrize> no appropriate gain match calibration";
    return nullptr;
  }

  size_t adjrange = pow(2, bits);

  boost::random::mt19937 gen;
  boost::random::uniform_real_distribution<> dist(-0.5, 0.5);

  size_t e2 = 0;
  std::unique_ptr<std::list<Entry>> spectrum_data = std::move(source->data_range({{0, adjrange}, {0, adjrange}}));
  for (auto it : *spectrum_data) {
    PreciseFloat count = it.second;

    //DBG << "adding " << it.first[0] << "+" << it.first[1] << "  x" << it.second;
    double xformed = gain_match_cali.transform(it.first[1]);
    double e1 = it.first[0];

    it.second = 1;
    for (int i=0; i < count; ++i) {
      double plus = dist(gen);
      double xfp = xformed + plus;

      if (xfp > 0)
        e2 = static_cast<size_t>(std::round(xfp));
      else
        e2 = 0;

      //DBG << xformed << " plus " << plus << " = " << xfp << " round to " << e2;

      it.first[0] = e1;
      it.first[1] = e2;

      ret->append(it);

      it.first[0] = e2;
      it.first[1] = e1;

      ret->append(it);

    }
  }

  for (auto &p : md.detectors) {
    if (p.shallow_equals(detector1) || p.shallow_equals(detector2)) {
      p = Detector(detector1.name_ + std::string("*") + detector2.name_);
      p.energy_calibrations_.add(detector1.energy_calibrations_.get(Calibration("Energy", bits)));
    }
  }

  ret->set_detectors(md.detectors);

  Qpx::Setting app = md.get_attribute("symmetrized");
  app.value_int = true;
  ret->set_attribute(app);

  if (ret->metadata().get_attribute("total_events").value_precise > 0)
    return ret;
  else
    return nullptr;
}


}
