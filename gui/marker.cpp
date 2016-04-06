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

#include "marker.h"
#include "custom_logger.h"

QPen AppearanceProfile::get_pen(QString theme) {
  if (themes.count(theme))
    return themes[theme];
  else
    return default_pen;
}


void Coord::set_energy(double nrg, Qpx::Calibration cali) {
  bin_ = nan("");
  bits_ = 0;
  energy_ = nrg;
  if (!isnan(nrg)) {
    bits_ = cali.bits_;
    bin_ = cali.inverse_transform(nrg);
  }
}

void Coord::set_bin(double bin, uint16_t bits, Qpx::Calibration cali) {
  bin_ = bin;
  bits_ = bits;
  energy_ = nan("");
  if (!isnan(bin)/* && cali.valid()*/)
    energy_ = cali.transform(bin_, bits_);
}

double Coord::energy() const {
  return energy_;
}

double Coord::bin(const uint16_t to_bits) const {
  if (!to_bits || !bits_)
    return nan("");

  if (bits_ > to_bits)
    return bin_ / pow(2, bits_ - to_bits);
  if (bits_ < to_bits)
    return bin_ * pow(2, to_bits - bits_);
  else
    return bin_;
}

void Peak2D::adjust_x(Qpx::ROI &roi, double center)
{
  Qpx::Peak pk;

  if (roi.peaks_.count(center))
    pk = roi.peaks_.at(center);
  else
    return;

  x = roi;

  MarkerBox2D newpeak = area[1][1];
  newpeak.visible = true;
  newpeak.mark_center = true;
  newpeak.vertical = true;
  newpeak.horizontal = true;
  newpeak.labelfloat = false;
  newpeak.selectable = true;
  newpeak.selected = true;

  newpeak.x_c.set_bin(pk.center, roi.settings_.bits_, roi.settings_.cali_nrg_);
  newpeak.x1.set_bin(pk.sum4_.bx[0], roi.settings_.bits_, roi.settings_.cali_nrg_);
  newpeak.x2.set_bin(pk.sum4_.bx[pk.sum4_.bx.size()-1], roi.settings_.bits_, roi.settings_.cali_nrg_);

  area[1][1] = newpeak;

  MarkerBox2D bckg;
  bckg.x_c = newpeak.x_c;
  bckg.y_c = newpeak.y_c;
  bckg.y1 = newpeak.y1;
  bckg.y2 = newpeak.y2;

  bckg.x1.set_bin(roi.finder_.x_[pk.sum4_.LB().start()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.x2.set_bin(roi.finder_.x_[pk.sum4_.LB().end()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[0][1] = bckg;

  bckg.x1.set_bin(roi.finder_.x_[pk.sum4_.RB().start()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.x2.set_bin(roi.finder_.x_[pk.sum4_.RB().end()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[2][1] = bckg;
}

void Peak2D::adjust_y(Qpx::ROI &roi, double center)
{
  Qpx::Peak pk;

  if (roi.peaks_.count(center))
    pk = roi.peaks_.at(center);
  else
    return;

  y = roi;

  MarkerBox2D newpeak = area[1][1];
  newpeak.visible = true;
  newpeak.mark_center = true;
  newpeak.vertical = true;
  newpeak.horizontal = true;
  newpeak.labelfloat = false;
  newpeak.selectable = true;
  newpeak.selected = true;

  newpeak.y_c.set_bin(pk.center, roi.settings_.bits_, roi.settings_.cali_nrg_);
  newpeak.y1.set_bin(pk.sum4_.bx[0], roi.settings_.bits_, roi.settings_.cali_nrg_);
  newpeak.y2.set_bin(pk.sum4_.bx[pk.sum4_.bx.size()-1], roi.settings_.bits_, roi.settings_.cali_nrg_);

  area[1][1] = newpeak;

  MarkerBox2D bckg;
  bckg.x_c = newpeak.x_c;
  bckg.y_c = newpeak.y_c;
  bckg.x1 = newpeak.x1;
  bckg.x2 = newpeak.x2;

  bckg.y1.set_bin(roi.finder_.x_[pk.sum4_.LB().start()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.y2.set_bin(roi.finder_.x_[pk.sum4_.LB().end()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[1][0] = bckg;

  bckg.y1.set_bin(roi.finder_.x_[pk.sum4_.RB().start()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.y2.set_bin(roi.finder_.x_[pk.sum4_.RB().end()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[1][2] = bckg;
}

void Peak2D::adjust_diag_x(Qpx::ROI &roi, double center)
{
  Qpx::Peak pk;

  if (roi.peaks_.count(center))
    pk = roi.peaks_.at(center);
  else
    return;

  dx = roi;

  MarkerBox2D bckg;
  bckg.x_c = area[1][1].x_c;
  bckg.y_c = area[1][1].y_c;

  bckg.y1 = area[0][0].y1;
  bckg.y2 = area[0][0].y2;
  bckg.x1.set_bin(roi.finder_.x_[pk.sum4_.LB().start()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.x2.set_bin(roi.finder_.x_[pk.sum4_.LB().end()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[0][0] = bckg;

  bckg.y1 = area[0][2].y1;
  bckg.y2 = area[0][2].y2;
  bckg.x1.set_bin(roi.finder_.x_[pk.sum4_.LB().start()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.x2.set_bin(roi.finder_.x_[pk.sum4_.LB().end()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[0][2] = bckg;

  bckg.y1 = area[2][0].y1;
  bckg.y2 = area[2][0].y2;
  bckg.x1.set_bin(roi.finder_.x_[pk.sum4_.RB().start()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.x2.set_bin(roi.finder_.x_[pk.sum4_.RB().end()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[2][0] = bckg;

  bckg.y1 = area[2][2].y1;
  bckg.y2 = area[2][2].y2;
  bckg.x1.set_bin(roi.finder_.x_[pk.sum4_.RB().start()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.x2.set_bin(roi.finder_.x_[pk.sum4_.RB().end()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[2][2] = bckg;
}

void Peak2D::adjust_diag_y(Qpx::ROI &roi, double center)
{
  Qpx::Peak pk;

  if (roi.peaks_.count(center))
    pk = roi.peaks_.at(center);
  else
    return;

  dy = roi;

  MarkerBox2D bckg;
  bckg.x_c = area[1][1].x_c;
  bckg.y_c = area[1][1].y_c;

  bckg.x1 = area[0][0].x1;
  bckg.x2 = area[0][0].x2;
  bckg.y1.set_bin(roi.finder_.x_[pk.sum4_.LB().start()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.y2.set_bin(roi.finder_.x_[pk.sum4_.LB().end()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[0][0] = bckg;

  bckg.x1 = area[0][2].x1;
  bckg.x2 = area[0][2].x2;
  bckg.y1.set_bin(roi.finder_.x_[pk.sum4_.RB().start()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.y2.set_bin(roi.finder_.x_[pk.sum4_.RB().end()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[0][2] = bckg;

  bckg.x1 = area[2][0].x1;
  bckg.x2 = area[2][0].x2;
  bckg.y1.set_bin(roi.finder_.x_[pk.sum4_.LB().start()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.y2.set_bin(roi.finder_.x_[pk.sum4_.LB().end()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[2][0] = bckg;

  bckg.x1 = area[2][2].x1;
  bckg.x2 = area[2][2].x2;
  bckg.y1.set_bin(roi.finder_.x_[pk.sum4_.RB().start()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.y2.set_bin(roi.finder_.x_[pk.sum4_.RB().end()], roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[2][2] = bckg;
}

Peak2D::Peak2D(Qpx::ROI &x_roi, Qpx::ROI &y_roi, double x_center, double y_center) {
//  approved = false;
  Qpx::Peak xx, yy;

  if (x_roi.peaks_.count(x_center))
    xx = x_roi.peaks_.at(x_center);
  else
    return;

  if (y_roi.peaks_.count(y_center))
    yy = y_roi.peaks_.at(y_center);
  else
    return;

  x = x_roi;
  y = y_roi;

  MarkerBox2D newpeak;
  newpeak.visible = true;
  newpeak.mark_center = true;
  newpeak.vertical = true;
  newpeak.horizontal = true;
  newpeak.labelfloat = false;
  newpeak.selectable = true;
  newpeak.selected = true;

  newpeak.x_c.set_bin(xx.center, x_roi.settings_.bits_, x_roi.settings_.cali_nrg_);
  newpeak.x1.set_bin(xx.sum4_.bx[0], x_roi.settings_.bits_, x_roi.settings_.cali_nrg_);
  newpeak.x2.set_bin(xx.sum4_.bx[xx.sum4_.bx.size()-1], x_roi.settings_.bits_, x_roi.settings_.cali_nrg_);

  newpeak.y_c.set_bin(yy.center, y_roi.settings_.bits_, y_roi.settings_.cali_nrg_);
  newpeak.y1.set_bin(yy.sum4_.bx[0], y_roi.settings_.bits_, y_roi.settings_.cali_nrg_);
  newpeak.y2.set_bin(yy.sum4_.bx[yy.sum4_.bx.size()-1], y_roi.settings_.bits_, y_roi.settings_.cali_nrg_);

  area[1][1] = newpeak;

  adjust_x(x_roi, x_center);
  adjust_y(y_roi, y_center);
}


