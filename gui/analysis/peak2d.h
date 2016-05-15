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
 *      Marker - for marking a channel or energt in a plot
 *
 ******************************************************************************/

#ifndef Q_PEAK2D_H
#define Q_PEAK2D_H

#include "coord.h"
#include "roi.h"
#include <QString>
#include "daq_sink.h"

struct MarkerLabel2D {
  MarkerLabel2D() : selected(false), selectable(false), vertical(false), vfloat(false), hfloat(false) {}

  Coord x, y;
  QString text;
  bool selected, selectable, vertical;
  bool vfloat, hfloat;
};

struct MarkerBox2D {
  MarkerBox2D()
    : visible(false)
    , selected(false)
    , selectable (true)
    , horizontal(false)
    , vertical(false)
    , mark_center(false)
    , labelfloat(false)
    , integral(0)
  {}

  bool operator== (const MarkerBox2D& other) const {
    if (x1 != other.x1) return false;
    if (x2 != other.x2) return false;
    if (y1 != other.y1) return false;
    if (y2 != other.y2) return false;
    if (x_c != other.x_c) return false;
    if (y_c != other.y_c) return false;
    return true;
  }

  bool operator!= (const MarkerBox2D& other) const
    { return !operator==(other); }

  bool intersects(const MarkerBox2D& other) const {
    return ((x1.energy() < other.x2.energy())
            && (x2.energy() > other.x1.energy())
            && (y1.energy() < other.y2.energy())
            && (y2.energy() > other.y1.energy()));
  }

  bool northwest() const { return ((x1.energy() < y1.energy()) && (x2.energy() < y2.energy())); }
  bool southeast() const { return ((x1.energy() > y1.energy()) && (x2.energy() > y2.energy())); }

  void integrate(Qpx::SinkPtr spectrum);

  bool visible;
  bool selected;
  bool selectable;
  bool horizontal, vertical, labelfloat;
  bool mark_center;
  Coord x1, x2, y1, y2, x_c, y_c;
  double integral;
  double chan_area;
  double variance;
  QString label;
};

struct Peak2D {
  Peak2D() :
    currie_quality_indicator(-1),
    approved(false)
  {}
  Peak2D(Qpx::ROI &x_roi, Qpx::ROI &y_roi, double x_center, double y_center);

  void adjust_x(Qpx::ROI &roi, double center);
  void adjust_y(Qpx::ROI &roi, double center);
  void adjust_diag_x(Qpx::ROI &roi, double center);
  void adjust_diag_y(Qpx::ROI &roi, double center);

  void integrate(Qpx::SinkPtr source);

  MarkerBox2D area[3][3];
  Qpx::ROI x, y, dx, dy;

  UncertainDouble energy_x, energy_y;

  UncertainDouble gross, xback, yback, dback, net;
  int currie_quality_indicator;

  bool approved;
};

#endif
