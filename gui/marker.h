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

#ifndef Q_MARKER_H
#define Q_MARKER_H

#include "detector.h"
#include <QPen>
#include <QMap>

struct AppearanceProfile {
  QMap<QString, QPen> themes;
  QPen default_pen;

  AppearanceProfile() : default_pen(Qt::gray, 1, Qt::SolidLine) {}
  QPen get_pen(QString theme) {
    if (themes.count(theme))
      return themes[theme];
    else
      return default_pen;
  }
};

class Coord {
public:
  Coord() : bits_(-1), energy_(nan("")), bin_(nan("")) {}

  void set_energy(double nrg, Gamma::Calibration cali) {
    bin_ = nan("");
    bits_ = 0;
    energy_ = nrg;
    if (!isnan(nrg)) {
      bits_ = cali.bits_;
      bin_ = cali.inverse_transform(nrg);
    }
  }

  void set_bin(double bin, uint16_t bits, Gamma::Calibration cali) {
    bin_ = bin;
    bits_ = bits;
    energy_ = nan("");
    if (!isnan(bin) && cali.valid())
      energy_ = cali.transform(bin_, bits_);
  }

  double energy() const {
    return energy_;
  }

  double bin(const uint16_t to_bits) const {
    if (!to_bits || !bits_)
      return nan("");

    if (bits_ > to_bits)
      return bin_ / pow(2, bits_ - to_bits);
    if (bits_ < to_bits)
      return bin_ * pow(2, to_bits - bits_);
    else
      return bin_;
  }

  bool operator!= (const Coord& other) const { return (!operator==(other)); }
  bool operator== (const Coord& other) const {
    if (energy_ != other.energy_) return false;
    if (bin(bits_) != other.bin(bits_)) return false;
    return true;
  }

private:
  double energy_;
  double bin_;
  uint16_t bits_;
};

struct Marker {
  bool operator!= (const Marker& other) const { return (!operator==(other)); }
  bool operator== (const Marker& other) const {
    if (pos != other.pos) return false;
    return true;
  }

  Coord pos;

  AppearanceProfile appearance, selected_appearance;

  bool visible;
  bool selected;

  Marker() : visible(false), selected(false) {}
};

struct Range {
  bool visible;
  AppearanceProfile base, top;
  Coord center, l, r;
};

struct MarkerLabel2D {
  MarkerLabel2D() : selected(false), selectable(false), vertical(false) {}

  Coord x, y;
  QString text;
  bool selected, selectable, vertical;
};

struct MarkerBox2D {
  MarkerBox2D() : visible(false), selected(false), selectable (true), horizontal(false), vertical(false), mark_center(false) {}
  bool operator== (const MarkerBox2D& other) const {
    if (x1 != other.x1) return false;
    if (x2 != other.x2) return false;
    if (y1 != other.y1) return false;
    if (y2 != other.y2) return false;
    if (x_c != other.x_c) return false;
    if (y_c != other.y_c) return false;
    return true;
  }

  bool visible;
  bool selected;
  bool selectable;
  bool horizontal, vertical;
  bool mark_center;
  Coord x1, x2, y1, y2, x_c, y_c;
};

#endif
