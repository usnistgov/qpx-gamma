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

#include "peak2d.h"
#include "custom_logger.h"

void MarkerBox2D::integrate(Qpx::SinkPtr spectrum)
{
  integral = 0;
  chan_area = 0;
  if (!spectrum)
    return;

  Qpx::Metadata md = spectrum->metadata();

  if ((md.total_count <= 0) || (md.dimensions() != 2))
    return;

  //bits match?

  uint32_t xmin = std::ceil(x1.bin(md.bits));
  uint32_t xmax = std::floor(x2.bin(md.bits));
  uint32_t ymin = std::ceil(y1.bin(md.bits));
  uint32_t ymax = std::floor(y2.bin(md.bits));

  chan_area = (xmax - xmin + 1) * (ymax - ymin + 1);

  std::shared_ptr<Qpx::EntryList> spectrum_data = std::move(spectrum->data_range({{xmin, xmax}, {ymin, ymax}}));
  for (auto &entry : *spectrum_data)
    integral += entry.second.convert_to<double>();

  variance = integral / pow(chan_area, 2);
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

  newpeak.x_c.set_bin(pk.center.val, roi.settings_.bits_, roi.settings_.cali_nrg_);
  newpeak.x1.set_bin(pk.sum4_.bx[0] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  newpeak.x2.set_bin(pk.sum4_.bx[pk.sum4_.bx.size()-1] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);

  area[1][1] = newpeak;

  MarkerBox2D bckg;
  bckg.x_c = newpeak.x_c;
  bckg.y_c = newpeak.y_c;
  bckg.y1 = newpeak.y1;
  bckg.y2 = newpeak.y2;

  bckg.x1.set_bin(roi.finder_.x_[pk.sum4_.LB().start()] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.x2.set_bin(roi.finder_.x_[pk.sum4_.LB().end()] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[0][1] = bckg;

  bckg.x1.set_bin(roi.finder_.x_[pk.sum4_.RB().start()] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.x2.set_bin(roi.finder_.x_[pk.sum4_.RB().end()] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[2][1] = bckg;

  energy_x = pk.energy;
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

  newpeak.y_c.set_bin(pk.center.val, roi.settings_.bits_, roi.settings_.cali_nrg_);
  newpeak.y1.set_bin(pk.sum4_.bx[0] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  newpeak.y2.set_bin(pk.sum4_.bx[pk.sum4_.bx.size()-1] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);

  area[1][1] = newpeak;

  MarkerBox2D bckg;
  bckg.x_c = newpeak.x_c;
  bckg.y_c = newpeak.y_c;
  bckg.x1 = newpeak.x1;
  bckg.x2 = newpeak.x2;

  bckg.y1.set_bin(roi.finder_.x_[pk.sum4_.LB().start()] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.y2.set_bin(roi.finder_.x_[pk.sum4_.LB().end()] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[1][0] = bckg;

  bckg.y1.set_bin(roi.finder_.x_[pk.sum4_.RB().start()] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.y2.set_bin(roi.finder_.x_[pk.sum4_.RB().end()] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[1][2] = bckg;

  energy_y = pk.energy;
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
  bckg.x1.set_bin(roi.finder_.x_[pk.sum4_.LB().start()] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.x2.set_bin(roi.finder_.x_[pk.sum4_.LB().end()] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[0][0] = bckg;

  bckg.y1 = area[0][2].y1;
  bckg.y2 = area[0][2].y2;
  bckg.x1.set_bin(roi.finder_.x_[pk.sum4_.LB().start()] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.x2.set_bin(roi.finder_.x_[pk.sum4_.LB().end()] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[0][2] = bckg;

  bckg.y1 = area[2][0].y1;
  bckg.y2 = area[2][0].y2;
  bckg.x1.set_bin(roi.finder_.x_[pk.sum4_.RB().start()] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.x2.set_bin(roi.finder_.x_[pk.sum4_.RB().end()] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[2][0] = bckg;

  bckg.y1 = area[2][2].y1;
  bckg.y2 = area[2][2].y2;
  bckg.x1.set_bin(roi.finder_.x_[pk.sum4_.RB().start()] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.x2.set_bin(roi.finder_.x_[pk.sum4_.RB().end()] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
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
  bckg.y1.set_bin(roi.finder_.x_[pk.sum4_.LB().start()] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.y2.set_bin(roi.finder_.x_[pk.sum4_.LB().end()] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[0][0] = bckg;

  bckg.x1 = area[0][2].x1;
  bckg.x2 = area[0][2].x2;
  bckg.y1.set_bin(roi.finder_.x_[pk.sum4_.RB().start()] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.y2.set_bin(roi.finder_.x_[pk.sum4_.RB().end()] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[0][2] = bckg;

  bckg.x1 = area[2][0].x1;
  bckg.x2 = area[2][0].x2;
  bckg.y1.set_bin(roi.finder_.x_[pk.sum4_.LB().start()] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.y2.set_bin(roi.finder_.x_[pk.sum4_.LB().end()] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[2][0] = bckg;

  bckg.x1 = area[2][2].x1;
  bckg.x2 = area[2][2].x2;
  bckg.y1.set_bin(roi.finder_.x_[pk.sum4_.RB().start()] - 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  bckg.y2.set_bin(roi.finder_.x_[pk.sum4_.RB().end()] + 0.5,
      roi.settings_.bits_, roi.settings_.cali_nrg_);
  area[2][2] = bckg;
}

Peak2D::Peak2D(Qpx::ROI &x_roi, Qpx::ROI &y_roi, double x_center, double y_center):
  currie_quality_indicator(-1),
  approved(false)
{
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

  newpeak.x_c.set_bin(xx.center.val, x_roi.settings_.bits_, x_roi.settings_.cali_nrg_);
  newpeak.x1.set_bin(xx.sum4_.bx[0] - 0.5,
      x_roi.settings_.bits_, x_roi.settings_.cali_nrg_);
  newpeak.x2.set_bin(xx.sum4_.bx[xx.sum4_.bx.size()-1] + 0.5,
      x_roi.settings_.bits_, x_roi.settings_.cali_nrg_);

  newpeak.y_c.set_bin(yy.center.val, y_roi.settings_.bits_, y_roi.settings_.cali_nrg_);
  newpeak.y1.set_bin(yy.sum4_.bx[0] - 0.5,
      y_roi.settings_.bits_, y_roi.settings_.cali_nrg_);
  newpeak.y2.set_bin(yy.sum4_.bx[yy.sum4_.bx.size()-1] + 0.5,
      y_roi.settings_.bits_, y_roi.settings_.cali_nrg_);

  area[1][1] = newpeak;

  adjust_x(x_roi, x_center);
  adjust_y(y_roi, y_center);
}

void Peak2D::integrate(Qpx::SinkPtr source)
{
  if (!area[1][1].visible)
    return;
  area[1][1].integrate(source);

  xback = ValUncert(0);
  if ((area[0][1] != MarkerBox2D()) && (area[2][1] != MarkerBox2D())) {
    area[0][1].integrate(source);
    area[2][1].integrate(source);

    xback.val = area[1][1].chan_area *
        (area[0][1].integral / area[0][1].chan_area + area[2][1].integral / area[2][1].chan_area)
        / 2.0;
    double backvariance = pow(area[1][1].chan_area / 2.0, 2)
        * (area[0][1].variance + area[2][1].variance);
    xback.uncert = sqrt(backvariance);
  }

  yback = ValUncert(0);
  if ((area[1][0] != MarkerBox2D()) && (area[1][2] != MarkerBox2D())) {
    area[1][0].integrate(source);
    area[1][2].integrate(source);

    yback.val = area[1][1].chan_area *
        (area[1][0].integral / area[1][0].chan_area + area[1][2].integral / area[1][2].chan_area)
        / 2.0;
    double backvariance = pow(area[1][1].chan_area / 2.0, 2)
        * (area[1][0].variance + area[1][2].variance);
    yback.uncert = sqrt(backvariance);
  }

  dback = ValUncert(0);
  if ((area[0][2] != MarkerBox2D()) && (area[2][0] != MarkerBox2D())) {
    area[0][2].integrate(source);
    area[2][0].integrate(source);

    dback.val = area[1][1].chan_area *
        (area[2][0].integral / area[2][0].chan_area + area[0][2].integral / area[0][2].chan_area)
        / 2.0;
    double backvariance = pow(area[1][1].chan_area / 2.0, 2)
        * (area[2][0].variance + area[0][2].variance);
    dback.uncert = sqrt(backvariance);
  }

  gross = ValUncert(0);
  gross.val = area[1][1].integral;
  gross.uncert = sqrt(area[1][1].variance);

  net = ValUncert(0);
  net.val = gross.val - xback.val - yback.val - dback.val;

  double background_variance = pow(xback.uncert, 2) + pow(yback.uncert, 2) + pow(dback.uncert, 2);
  double netvariance = gross.val + background_variance;

  net.uncert = sqrt(netvariance);

  double currieLQ(0), currieLD(0), currieLC(0);
  currieLQ = 50 * (1 + sqrt(1 + background_variance / 12.5));
  currieLD = 2.71 + 4.65 * sqrt(background_variance);
  currieLC = 2.33 * sqrt(background_variance);

  if (net.val > currieLQ)
    currie_quality_indicator = 1;
  else if (net.val > currieLD)
    currie_quality_indicator = 2;
  else if (net.val > currieLC)
    currie_quality_indicator = 3;
  else if (net.val > 0)
    currie_quality_indicator = 4;
  else
    currie_quality_indicator = -1;

}


