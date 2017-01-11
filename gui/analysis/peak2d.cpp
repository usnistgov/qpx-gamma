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
//#include "custom_logger.h"

bool Bounds::operator== (const Bounds& other) const
{
  return (lower_ == other.lower_)
      && (upper_ == other.upper_)
      && (centroid_ == other.centroid_);
}

bool Bounds::operator!= (const Bounds& other) const
{
  return !operator==(other);
}

Bounds Bounds::calibrate(const Qpx::Calibration& c, uint16_t bits) const
{
  Bounds ret;
  ret.set_centroid(c.transform(centroid_, bits));
  ret.set_bounds(c.transform(lower_, bits),
                 c.transform(upper_, bits));
  return ret;
}

std::string Bounds::bounds_to_string() const
{
  std::stringstream ss;
  ss << "[" << lower_ << "," << upper_ << "]";
  return ss.str();
}

void Bounds::set_bounds(double l, double r, bool discretize)
{
  if (discretize)
  {
    lower_ = std::floor(std::min(l, r));
    upper_ = std::ceil(std::max(l, r));
  }
  else
  {
    lower_ = std::min(l, r);
    upper_ = std::max(l, r);
  }
}

double Bounds::width() const
{
  return upper_ - lower_ + 1;
}

void Bounds::set_centroid(double c)
{
  centroid_ = c;
}

double Bounds::lower() const
{
  return lower_;
}

double Bounds::upper() const
{
  return upper_;
}

double Bounds::centroid() const
{
  return centroid_;
}




bool Bounds2D::operator== (const Bounds2D& other) const
{
  return (x == other.x) && (y == other.y);
}

bool Bounds2D::operator!= (const Bounds2D& other) const
{
  return !operator==(other);
}

bool Bounds2D::intersects(const Bounds2D& other) const
{
  return ((x.lower() < other.x.upper())
          && (x.upper() > other.x.lower())
          && (y.lower() < other.y.upper())
          && (y.upper() > other.y.lower()));
}

Bounds2D Bounds2D::reflect() const
{
  auto ret = *this;
  std::swap(ret.x, ret.y);
  return ret;
}

bool Bounds2D::northwest() const
{
  return ((x.lower() < y.lower()) && (x.upper() < y.upper()));
}

bool Bounds2D::southeast() const
{
  return ((x.lower() > y.lower()) && (x.upper() > y.upper()));
}

double Bounds2D::chan_area() const
{
  return x.width() * y.width();
}


void Bounds2D::integrate(Qpx::SinkPtr spectrum)
{
  integral = 0;
  variance = 0;
  if (!spectrum)
    return;

  Qpx::Metadata md = spectrum->metadata();

  if (md.dimensions() != 2)
    return;

  std::shared_ptr<Qpx::EntryList> spectrum_data
      = std::move(spectrum->data_range({{x.lower(), x.upper()}, {y.lower(), y.upper()}}));
  for (auto &entry : *spectrum_data)
    integral += to_double( entry.second );

  variance = integral / pow(chan_area(), 2);
}

void Sum2D::adjust_x(const Qpx::ROI &roi, double center)
{
  Qpx::Peak pk;

  if (roi.contains(center))
    pk = roi.peaks().at(center);
  else
    return;

  area[1][1].x.set_centroid(pk.center().value());
  area[0][1] = area[2][1] = area[1][1];

  area[1][1].x.set_bounds(pk.sum4().left(), pk.sum4().right());
  area[0][1].x.set_bounds(pk.sum4().LB().left(), pk.sum4().LB().right());
  area[2][1].x.set_bounds(pk.sum4().RB().left(), pk.sum4().RB().right());

  energy_x = pk.energy();
}

void Sum2D::adjust_y(const Qpx::ROI &roi, double center)
{
  Qpx::Peak pk;

  if (roi.contains(center))
    pk = roi.peaks().at(center);
  else
    return;

  area[1][1].y.set_centroid(pk.center().value());
  area[1][0] = area[1][2] = area[1][1];

  area[1][1].y.set_bounds(pk.sum4().left(), pk.sum4().right());
  area[1][0].y.set_bounds(pk.sum4().LB().left(), pk.sum4().LB().right());
  area[1][2].y.set_bounds(pk.sum4().RB().left(), pk.sum4().RB().right());

  energy_y = pk.energy();
}

void Sum2D::adjust_diag_x(const Qpx::ROI &roi, double center)
{
  Qpx::Peak pk;

  if (roi.contains(center))
    pk = roi.peaks().at(center);
  else
    return;

  Bounds2D bckg;
  bckg.x.set_centroid(area[1][1].x.centroid());
  bckg.y.set_centroid(area[1][1].y.centroid());

  bckg.y.set_bounds(area[0][0].y.lower(), area[0][0].y.upper());
  bckg.x.set_bounds(pk.sum4().LB().left(), pk.sum4().LB().right());
  area[0][0] = bckg;

  bckg.y.set_bounds(area[0][2].y.lower(), area[0][2].y.upper());
  bckg.x.set_bounds(pk.sum4().LB().left(), pk.sum4().LB().right());
  area[0][2] = bckg;

  bckg.y.set_bounds(area[2][0].y.lower(), area[2][0].y.upper());
  bckg.x.set_bounds(pk.sum4().RB().left(), pk.sum4().RB().right());
  area[2][0] = bckg;

  bckg.y.set_bounds(area[2][2].y.lower(), area[2][2].y.upper());
  bckg.x.set_bounds(pk.sum4().RB().left(), pk.sum4().RB().right());
  area[2][2] = bckg;
}

void Sum2D::adjust_diag_y(const Qpx::ROI &roi, double center)
{
  Qpx::Peak pk;

  if (roi.contains(center))
    pk = roi.peaks().at(center);
  else
    return;

  Bounds2D bckg;
  bckg.x.set_centroid(area[1][1].x.centroid());
  bckg.y.set_centroid(area[1][1].y.centroid());

  bckg.x.set_bounds(area[0][0].x.lower(), area[0][0].x.upper());
  bckg.y.set_bounds(pk.sum4().LB().left(), pk.sum4().LB().right());
  area[0][0] = bckg;

  bckg.x.set_bounds(area[0][2].x.lower(), area[0][2].x.upper());
  bckg.y.set_bounds(pk.sum4().RB().left(), pk.sum4().RB().right());
  area[0][2] = bckg;

  bckg.x.set_bounds(area[2][0].x.lower(), area[2][0].x.upper());
  bckg.y.set_bounds(pk.sum4().LB().left(), pk.sum4().LB().right());
  area[2][0] = bckg;

  bckg.x.set_bounds(area[2][2].x.lower(), area[2][2].x.upper());
  bckg.y.set_bounds(pk.sum4().RB().left(), pk.sum4().RB().right());
  area[2][2] = bckg;
}

Sum2D::Sum2D(const Qpx::ROI &x_roi, const Qpx::ROI &y_roi, double x_center, double y_center):
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

  Bounds2D newpeak;
  newpeak.selected = true;

  newpeak.x.set_centroid(xx.center().value());
  newpeak.x.set_bounds(xx.sum4().left(), xx.sum4().right());

  newpeak.y.set_centroid(yy.center().value());
  newpeak.y.set_bounds(yy.sum4().left(), yy.sum4().right());

  area[1][1] = newpeak;

  adjust_x(x_roi, x_center);
  adjust_y(y_roi, y_center);
}

void Sum2D::integrate(Qpx::SinkPtr source)
{
  if (area[1][1] == Bounds2D())
    return;
  area[1][1].integrate(source);

  xback = UncertainDouble();
  if ((area[0][1] != Bounds2D()) && (area[2][1] != Bounds2D())) {
    area[0][1].integrate(source);
    area[2][1].integrate(source);

    double val = area[1][1].chan_area() *
        (area[0][1].integral / area[0][1].chan_area() + area[2][1].integral / area[2][1].chan_area())
        / 2.0;
    double backvariance = pow(area[1][1].chan_area() / 2.0, 2)
        * (area[0][1].variance + area[2][1].variance);
    xback = UncertainDouble::from_double(val, sqrt(backvariance));
  }

  yback = UncertainDouble();
  if ((area[1][0] != Bounds2D()) && (area[1][2] != Bounds2D())) {
    area[1][0].integrate(source);
    area[1][2].integrate(source);

    double val = area[1][1].chan_area() *
        (area[1][0].integral / area[1][0].chan_area() + area[1][2].integral / area[1][2].chan_area())
        / 2.0;
    double backvariance = pow(area[1][1].chan_area() / 2.0, 2)
        * (area[1][0].variance + area[1][2].variance);
    yback = UncertainDouble::from_double(val, sqrt(backvariance));
  }

  gross = UncertainDouble::from_double(area[1][1].integral, sqrt(area[1][1].variance));
  double netval = gross.value() - xback.value() - yback.value();
  double background_variance = pow(xback.uncertainty(), 2)
                             + pow(yback.uncertainty(), 2);

  dback = UncertainDouble();
  if ((area[0][2] != Bounds2D()) && (area[2][0] != Bounds2D())) {
    area[0][2].integrate(source);
    area[2][0].integrate(source);

    double val = area[1][1].chan_area() *
        (area[2][0].integral / area[2][0].chan_area() + area[0][2].integral / area[0][2].chan_area())
        / 2.0;
    double backvariance = pow(area[1][1].chan_area() / 2.0, 2)
        * (area[2][0].variance + area[0][2].variance);
    dback = UncertainDouble::from_double(val, sqrt(backvariance));

    netval -= dback.value();
    background_variance += pow(dback.uncertainty(), 2);
  }

  double netvariance = gross.value() + background_variance;

  net = UncertainDouble::from_double(netval, sqrt(netvariance));
  currie_quality_indicator = Qpx::SUM4::get_currie_quality_indicator(net.value(), background_variance);
}


