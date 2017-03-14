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
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *
 ******************************************************************************/

#pragma once

#include "fitter.h"
#include <QString>
#include "daq_sink.h"
#include "qp_2d.h"

class Bounds
{
public:
  bool operator== (const Bounds& other) const;
  bool operator!= (const Bounds& other) const;

  Bounds calibrate(const Qpx::Calibration& c, uint16_t bits) const;
  std::string bounds_to_string() const;

  void set_bounds(double l, double r, bool discretize = true);

  double width() const;

  void set_centroid(double c);

  double lower() const;
  double upper() const;
  double centroid() const;

protected:
  double lower_ {0}, upper_ {0};
  double centroid_{0};
};

struct Bounds2D
{
  bool operator== (const Bounds2D& other) const;
  bool operator!= (const Bounds2D& other) const;

  bool intersects(const Bounds2D& other) const;
  bool northwest() const;
  bool southeast() const;
  Bounds2D reflect() const;

  void integrate(Qpx::SinkPtr spectrum);

  double chan_area() const;

  int64_t id {-1};
  bool selectable {false};
  bool selected {false};
  Bounds x, y;
  bool horizontal {true};
  bool vertical {true};
  bool labelfloat {false};
  QColor color;

  double integral  {0};
  double variance  {0};
};

struct Sum2D
{
  Sum2D();

  void adjust(const Qpx::Fitter &fitter_x, const Qpx::Fitter &fitter_y);
  void adjust_diag(const Qpx::Fitter &fitter_x, const Qpx::Fitter &fitter_y);

  void integrate(Qpx::SinkPtr source);

  Bounds2D area[3][3];

  UncertainDouble energy_x, energy_y;

  UncertainDouble gross, xback, yback, dback, net;

  Qpx::Fitter x, y, dx, dy;

  int currie_quality_indicator {-1};
  bool approved {false};

  Qpx::Peak get_one_peak(const Qpx::Fitter &fitter);
};
