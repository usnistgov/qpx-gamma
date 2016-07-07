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
 *
 ******************************************************************************/

#include "form_gates_plot2d.h"
#include "ui_form_gates_plot2d.h"
#include "dialog_spectrum.h"
#include "custom_timer.h"

using namespace Qpx;

FormGatesPlot2D::FormGatesPlot2D(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormGatesPlot2D),
  current_spectrum_(0)
{
  ui->setupUi(this);

  connect(ui->coincPlot, SIGNAL(markers_set(Coord,Coord)), this, SLOT(markers_moved(Coord,Coord)));
  connect(ui->coincPlot, SIGNAL(stuff_selected()), this, SLOT(selection_changed()));

  ui->coincPlot->set_antialiased(false);

  adjrange = 0;

  my_marker_.selectable = false;
  my_marker_.selected = false;

  bits = 0;
}

FormGatesPlot2D::~FormGatesPlot2D()
{
  delete ui;
}

void FormGatesPlot2D::selection_changed() {
  emit stuff_selected();
}

void FormGatesPlot2D::set_range_x(MarkerBox2D range) {
  range_ = range;
  ui->coincPlot->set_range_x(range);
}

void FormGatesPlot2D::setSpectra(Project& new_set, int64_t idx) {
  mySpectra = &new_set;
  current_spectrum_ = idx;

  //  update_plot();
}

void FormGatesPlot2D::set_gates_movable(bool mov) {
  gates_movable_ = mov;
}

void FormGatesPlot2D::set_show_boxes(bool show) {
  show_boxes_ = show;
  ui->coincPlot->replot_markers();
}

std::list<MarkerBox2D> FormGatesPlot2D::get_selected_boxes() {
  return ui->coincPlot->get_selected_boxes();
}


void FormGatesPlot2D::set_boxes(std::list<MarkerBox2D> boxes) {
  boxes_ = boxes;
  replot_markers();
}


void FormGatesPlot2D::reset_content() {
  //DBG << "reset content";
  ui->coincPlot->reset_content();
  ui->coincPlot->refresh();
  current_spectrum_ = 0;
  my_marker_.visible = false;
  replot_markers();
}

void FormGatesPlot2D::refresh()
{
  ui->coincPlot->refresh();
}

void FormGatesPlot2D::replot_markers() {
  //DBG << "replot markers";

  std::list<MarkerBox2D> boxes;
  std::list<MarkerLabel2D> labels;
  MarkerLabel2D label;

  if (show_boxes_)
    boxes = boxes_;

  for (auto &q : boxes) {
    label.selectable = q.selectable;
    label.selected = q.selected;

    if (q.horizontal && q.vertical) {
      label.x = q.x2;
      label.y = q.y2;
      label.vertical = false;
      label.text = QString::number(q.y_c.energy());
      labels.push_back(label);

      label.x = q.x2;
      label.y = q.y2;
      label.vertical = true;
      label.text = QString::number(q.x_c.energy());
      labels.push_back(label);
    } else if (q.horizontal) {
      label.x = q.x_c;
      label.y = q.y2;
      label.vertical = false;
      label.hfloat = q.labelfloat;
      label.text = QString::number(q.y_c.energy());
      labels.push_back(label);
    } else if (q.vertical) {
      label.x = q.x2;
      label.y = q.y_c;
      label.vertical = true;
      label.vfloat = q.labelfloat;
      label.text = QString::number(q.x_c.energy());
      labels.push_back(label);
    }
  }

  boxes.push_back(my_marker_);

  label.selectable = false;
  label.selected = false;
  label.vertical = false;

  if (my_marker_.visible && my_marker_.horizontal && my_marker_.vertical) {
    label.vfloat = false;
    label.hfloat = false;
    label.x = my_marker_.x2;
    label.y = my_marker_.y_c;
    label.vertical = false;
    label.text = QString::number(my_marker_.y_c.energy());
    labels.push_back(label);

    label.x = my_marker_.x_c;
    label.y = my_marker_.y2;
    label.vertical = true;
    label.text = QString::number(my_marker_.x_c.energy());
    labels.push_back(label);
  } else if (my_marker_.visible && my_marker_.horizontal) {
    label.vfloat = false;
    label.hfloat = true;
    label.x = my_marker_.x2;
    label.y = my_marker_.y_c;
    label.vertical = false;
    label.text = QString::number(my_marker_.y_c.energy());
    labels.push_back(label);
  } else if (my_marker_.visible && my_marker_.vertical) {
    label.vfloat = true;
    label.hfloat = false;
    label.x = my_marker_.x_c;
    label.y = my_marker_.y2;
    label.vertical = true;
    label.text = QString::number(my_marker_.x_c.energy());
    labels.push_back(label);
  }

  ui->coincPlot->set_boxes(boxes);
  ui->coincPlot->set_labels(labels);
  ui->coincPlot->replot_markers();
}

void FormGatesPlot2D::update_plot() {

  this->setCursor(Qt::WaitCursor);
  CustomTimer guiside(true);

  SinkPtr some_spectrum = mySpectra->get_sink(current_spectrum_);

  Metadata md;
  if (some_spectrum)
    md = some_spectrum->metadata();

  uint16_t newbits = md.attributes.branches.get(Qpx::Setting("resolution")).value_int;

  if ((md.dimensions() == 2) && (adjrange = pow(2,newbits)) )
  {
    //      DBG << "really really updating 2d total count = " << some_spectrum->total_count();

    std::shared_ptr<EntryList> spectrum_data =
        std::move(some_spectrum->data_range({{0, adjrange}, {0, adjrange}}));
    ui->coincPlot->update_plot(adjrange, adjrange, spectrum_data);

    if (bits != newbits)
      bits = newbits;

    Detector detector_x_;
    Detector detector_y_;
    if (md.detectors.size() > 1) {
      detector_x_ = md.detectors[0];
      detector_y_ = md.detectors[1];
    }

    calib_x_ = detector_x_.best_calib(bits);
    calib_y_ = detector_y_.best_calib(bits);

    ui->coincPlot->set_axes(calib_x_, calib_y_, bits, "Event count");

  } else {
    ui->coincPlot->reset_content();
    ui->coincPlot->refresh();
  }

  replot_markers();


//  DBG << "<Plot2D> plotting took " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}

void FormGatesPlot2D::markers_moved(Coord x, Coord y) {
  MarkerBox2D m = my_marker_;
  m.visible = !(x.null() || y.null());
  m.x_c = x;
  m.y_c = y;

  if (gates_movable_) {
    my_marker_ = m;
    replot_markers();
  }

  emit marker_set(m);
}

void FormGatesPlot2D::set_marker(MarkerBox2D m) {
  my_marker_ = m;
  my_marker_.selectable = false;
  my_marker_.selected = false;
  replot_markers();
}

void FormGatesPlot2D::set_gates_visible(bool vertical, bool horizontal, bool diagonal)
{
  gate_vertical_ = vertical;
  gate_horizontal_ = horizontal;
  gate_diagonal_ = diagonal;

  replot_markers();
}

void FormGatesPlot2D::set_scale_type(QString st) {
  ui->coincPlot->set_scale_type(st);
}

void FormGatesPlot2D::set_gradient(QString gr) {
  ui->coincPlot->set_gradient(gr);
}

void FormGatesPlot2D::set_show_legend(bool sl) {
  ui->coincPlot->set_show_legend(sl);
}

QString FormGatesPlot2D::scale_type() {
  return ui->coincPlot->scale_type();
}

QString FormGatesPlot2D::gradient() {
  return ui->coincPlot->gradient();
}

bool FormGatesPlot2D::show_legend() {
  return ui->coincPlot->show_legend();
}
