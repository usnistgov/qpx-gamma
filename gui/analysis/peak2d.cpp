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

  if (md.dimensions() != 2)
    return;

  //bits match?
  uint16_t bits = md.get_attribute("resolution").value_int;

  uint32_t xmin = std::ceil(x1.bin(bits));
  uint32_t xmax = std::floor(x2.bin(bits));
  uint32_t ymin = std::ceil(y1.bin(bits));
  uint32_t ymax = std::floor(y2.bin(bits));

  chan_area = (xmax - xmin + 1) * (ymax - ymin + 1);

  std::shared_ptr<Qpx::EntryList> spectrum_data = std::move(spectrum->data_range({{xmin, xmax}, {ymin, ymax}}));
  for (auto &entry : *spectrum_data)
    integral += entry.second.convert_to<double>();

  variance = integral / pow(chan_area, 2);
}

void Peak2D::adjust_x(const Qpx::ROI &roi, double center)
{
  Qpx::Peak pk;

  if (roi.contains(center))
    pk = roi.peaks().at(center);
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

  newpeak.x_c.set_bin(pk.center().value(),
                      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  newpeak.x1.set_bin(pk.sum4().left() - 0.5,
                     roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  newpeak.x2.set_bin(pk.sum4().right() + 0.5,
                     roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);

  area[1][1] = newpeak;

  MarkerBox2D bckg;
  bckg.x_c = newpeak.x_c;
  bckg.y_c = newpeak.y_c;
  bckg.y1 = newpeak.y1;
  bckg.y2 = newpeak.y2;

  bckg.x1.set_bin(pk.sum4().LB().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  bckg.x2.set_bin(pk.sum4().LB().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  area[0][1] = bckg;

  bckg.x1.set_bin(pk.sum4().RB().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  bckg.x2.set_bin(pk.sum4().RB().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  area[2][1] = bckg;

  energy_x = pk.energy();
}

void Peak2D::adjust_y(const Qpx::ROI &roi, double center)
{
  Qpx::Peak pk;

  if (roi.contains(center))
    pk = roi.peaks().at(center);
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

  newpeak.y_c.set_bin(pk.center().value(), roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  newpeak.y1.set_bin(pk.sum4().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  newpeak.y2.set_bin(pk.sum4().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);

  area[1][1] = newpeak;

  MarkerBox2D bckg;
  bckg.x_c = newpeak.x_c;
  bckg.y_c = newpeak.y_c;
  bckg.x1 = newpeak.x1;
  bckg.x2 = newpeak.x2;

  bckg.y1.set_bin(pk.sum4().LB().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  bckg.y2.set_bin(pk.sum4().LB().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  area[1][0] = bckg;

  bckg.y1.set_bin(pk.sum4().RB().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  bckg.y2.set_bin(pk.sum4().RB().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  area[1][2] = bckg;

  energy_y = pk.energy();
}

void Peak2D::adjust_diag_x(const Qpx::ROI &roi, double center)
{
  Qpx::Peak pk;

  if (roi.contains(center))
    pk = roi.peaks().at(center);
  else
    return;

  dx = roi;

  MarkerBox2D bckg;
  bckg.x_c = area[1][1].x_c;
  bckg.y_c = area[1][1].y_c;

  bckg.y1 = area[0][0].y1;
  bckg.y2 = area[0][0].y2;
  bckg.x1.set_bin(pk.sum4().LB().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  bckg.x2.set_bin(pk.sum4().LB().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  area[0][0] = bckg;

  bckg.y1 = area[0][2].y1;
  bckg.y2 = area[0][2].y2;
  bckg.x1.set_bin(pk.sum4().LB().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  bckg.x2.set_bin(pk.sum4().LB().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  area[0][2] = bckg;

  bckg.y1 = area[2][0].y1;
  bckg.y2 = area[2][0].y2;
  bckg.x1.set_bin(pk.sum4().RB().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  bckg.x2.set_bin(pk.sum4().RB().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  area[2][0] = bckg;

  bckg.y1 = area[2][2].y1;
  bckg.y2 = area[2][2].y2;
  bckg.x1.set_bin(pk.sum4().RB().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  bckg.x2.set_bin(pk.sum4().RB().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  area[2][2] = bckg;
}

void Peak2D::adjust_diag_y(const Qpx::ROI &roi, double center)
{
  Qpx::Peak pk;

  if (roi.contains(center))
    pk = roi.peaks().at(center);
  else
    return;

  dy = roi;

  MarkerBox2D bckg;
  bckg.x_c = area[1][1].x_c;
  bckg.y_c = area[1][1].y_c;

  bckg.x1 = area[0][0].x1;
  bckg.x2 = area[0][0].x2;
  bckg.y1.set_bin(pk.sum4().LB().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  bckg.y2.set_bin(pk.sum4().LB().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  area[0][0] = bckg;

  bckg.x1 = area[0][2].x1;
  bckg.x2 = area[0][2].x2;
  bckg.y1.set_bin(pk.sum4().RB().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  bckg.y2.set_bin(pk.sum4().RB().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  area[0][2] = bckg;

  bckg.x1 = area[2][0].x1;
  bckg.x2 = area[2][0].x2;
  bckg.y1.set_bin(pk.sum4().LB().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  bckg.y2.set_bin(pk.sum4().LB().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  area[2][0] = bckg;

  bckg.x1 = area[2][2].x1;
  bckg.x2 = area[2][2].x2;
  bckg.y1.set_bin(pk.sum4().RB().left() - 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  bckg.y2.set_bin(pk.sum4().RB().right() + 0.5,
      roi.fit_settings().bits_, roi.fit_settings().cali_nrg_);
  area[2][2] = bckg;
}

Peak2D::Peak2D(const Qpx::ROI &x_roi, const Qpx::ROI &y_roi, double x_center, double y_center):
  currie_quality_indicator(-1),
  approved(false)
{
//  approved = false;
  Qpx::Peak xx, yy;

  if (x_roi.contains(x_center))
    xx = x_roi.peaks().at(x_center);
  else
    return;

  if (y_roi.contains(y_center))
    yy = y_roi.peaks().at(y_center);
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

  newpeak.x_c.set_bin(xx.center().value(), x_roi.fit_settings().bits_, x_roi.fit_settings().cali_nrg_);
  newpeak.x1.set_bin(xx.sum4().left() - 0.5,
      x_roi.fit_settings().bits_, x_roi.fit_settings().cali_nrg_);
  newpeak.x2.set_bin(xx.sum4().right() + 0.5,
      x_roi.fit_settings().bits_, x_roi.fit_settings().cali_nrg_);

  newpeak.y_c.set_bin(yy.center().value(), y_roi.fit_settings().bits_, y_roi.fit_settings().cali_nrg_);
  newpeak.y1.set_bin(yy.sum4().left() - 0.5,
      y_roi.fit_settings().bits_, y_roi.fit_settings().cali_nrg_);
  newpeak.y2.set_bin(yy.sum4().right() + 0.5,
      y_roi.fit_settings().bits_, y_roi.fit_settings().cali_nrg_);

  area[1][1] = newpeak;

  adjust_x(x_roi, x_center);
  adjust_y(y_roi, y_center);
}

void Peak2D::integrate(Qpx::SinkPtr source)
{
  if (!area[1][1].visible)
    return;
  area[1][1].integrate(source);

  xback = UncertainDouble();
  if ((area[0][1] != MarkerBox2D()) && (area[2][1] != MarkerBox2D())) {
    area[0][1].integrate(source);
    area[2][1].integrate(source);

    double val = area[1][1].chan_area *
        (area[0][1].integral / area[0][1].chan_area + area[2][1].integral / area[2][1].chan_area)
        / 2.0;
    double backvariance = pow(area[1][1].chan_area / 2.0, 2)
        * (area[0][1].variance + area[2][1].variance);
    xback = UncertainDouble::from_double(val, sqrt(backvariance));
  }

  yback = UncertainDouble();
  if ((area[1][0] != MarkerBox2D()) && (area[1][2] != MarkerBox2D())) {
    area[1][0].integrate(source);
    area[1][2].integrate(source);

    double val = area[1][1].chan_area *
        (area[1][0].integral / area[1][0].chan_area + area[1][2].integral / area[1][2].chan_area)
        / 2.0;
    double backvariance = pow(area[1][1].chan_area / 2.0, 2)
        * (area[1][0].variance + area[1][2].variance);
    yback = UncertainDouble::from_double(val, sqrt(backvariance));
  }

  dback = UncertainDouble();
  if ((area[0][2] != MarkerBox2D()) && (area[2][0] != MarkerBox2D())) {
    area[0][2].integrate(source);
    area[2][0].integrate(source);

    double val = area[1][1].chan_area *
        (area[2][0].integral / area[2][0].chan_area + area[0][2].integral / area[0][2].chan_area)
        / 2.0;
    double backvariance = pow(area[1][1].chan_area / 2.0, 2)
        * (area[2][0].variance + area[0][2].variance);
    dback = UncertainDouble::from_double(val, sqrt(backvariance));
  }

  gross = UncertainDouble::from_double(area[1][1].integral, sqrt(area[1][1].variance));

  double netval = gross.value() - xback.value() - yback.value() - dback.value();
  double background_variance = pow(xback.uncertainty(), 2)
                             + pow(yback.uncertainty(), 2)
                             + pow(dback.uncertainty(), 2);
  double netvariance = gross.value() + background_variance;

  net = UncertainDouble::from_double(netval, sqrt(netvariance));
  currie_quality_indicator = Qpx::SUM4::get_currie_quality_indicator(net.value(), background_variance);
}


