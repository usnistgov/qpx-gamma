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

FormGatesPlot2D::FormGatesPlot2D(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormGatesPlot2D)
{
  ui->setupUi(this);

  connect(ui->coincPlot, SIGNAL(markers_set(Marker,Marker)), this, SLOT(markers_moved(Marker,Marker)));
  connect(ui->coincPlot, SIGNAL(stuff_selected()), this, SLOT(selection_changed()));

  adjrange = 0;

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

void FormGatesPlot2D::setSpectra(Qpx::SpectraSet& new_set, QString spectrum) {
  mySpectra = &new_set;
  name_2d = spectrum;

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
  //PL_DBG << "reset content";
  ui->coincPlot->reset_content();
  ui->coincPlot->refresh();
  name_2d.clear();
  x_marker.visible = false;
  y_marker.visible = false;
  replot_markers();
}

void FormGatesPlot2D::set_gate_width(int w) {
  ui->spinGateWidth->setValue(w);
}

int FormGatesPlot2D::gate_width() {
  return ui->spinGateWidth->value();
}

void FormGatesPlot2D::refresh()
{
  ui->coincPlot->refresh();
}

void FormGatesPlot2D::replot_markers() {
  //PL_DBG << "replot markers";

  std::list<MarkerBox2D> boxes;
  std::list<MarkerLabel2D> labels;
  MarkerLabel2D label;

  if (show_boxes_)
    boxes = boxes_;

  int width = ui->spinGateWidth->value() / 2;

  for (auto &q : boxes) {
    label.selectable = q.selectable;
    label.selected = q.selected;

    if ((q.horizontal) && (q.vertical)) {
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

  MarkerBox2D gate, gatex, gatey;
  gate.selectable = false;
  gate.selected = false;

  if (!ui->spinGateWidth->isVisible())
    width = 0;

  //PL_DBG << "FormGatesPlot2D marker width = " << width;

  gate.x_c = x_marker.pos;
  gate.y_c = y_marker.pos;

  gate.visible = y_marker.visible;
  gate.x1.set_bin(0, bits, calib_x_);
  gate.x2.set_bin(adjrange, bits, calib_x_);
  gate.y1.set_bin(y_marker.pos.bin(bits) - width, bits, calib_x_);
  gate.y2.set_bin(y_marker.pos.bin(bits) + width, bits, calib_x_);
  gatey = gate;
  boxes.push_back(gate);

  gate.visible = x_marker.visible;
  gate.x1.set_bin(x_marker.pos.bin(bits) - width, bits, calib_x_);
  gate.x2.set_bin(x_marker.pos.bin(bits) + width, bits, calib_x_);
  gate.y1.set_bin(0, bits, calib_x_);
  gate.y2.set_bin(adjrange, bits, calib_x_);
  gatex = gate;
  boxes.push_back(gate);


  label.selectable = false;
  label.selected = false;
  label.vertical = false;

  if (y_marker.visible && x_marker.visible) {
    label.vfloat = false;
    label.hfloat = false;
    label.x = gatex.x2;
    label.y = gatey.y_c;
    label.vertical = false;
    label.text = QString::number(y_marker.pos.energy());
    labels.push_back(label);

    label.x = gatex.x_c;
    label.y = gatey.y2;
    label.vertical = true;
    label.text = QString::number(x_marker.pos.energy());
    labels.push_back(label);
  } else if (y_marker.visible) {
    label.vfloat = false;
    label.hfloat = true;
    label.x = gatex.x2;
    label.y = gatey.y_c;
    label.vertical = false;
    label.text = QString::number(y_marker.pos.energy());
    labels.push_back(label);
  } else if (x_marker.visible) {
    label.vfloat = true;
    label.hfloat = false;
    label.x = gatex.x_c;
    label.y = gatey.y2;
    label.vertical = true;
    label.text = QString::number(x_marker.pos.energy());
    labels.push_back(label);
  }



  ui->coincPlot->set_boxes(boxes);
  ui->coincPlot->set_labels(labels);
  ui->coincPlot->replot_markers();
}

void FormGatesPlot2D::update_plot() {

  this->setCursor(Qt::WaitCursor);
  CustomTimer guiside(true);

  Qpx::Spectrum::Spectrum* some_spectrum = mySpectra->by_name(name_2d.toStdString());

  Qpx::Spectrum::Metadata md;
  if (some_spectrum)
    md = some_spectrum->metadata();

  if ((md.total_count > 0) && (md.dimensions == 2) && (adjrange = md.resolution) )
  {
    //      PL_DBG << "really really updating 2d total count = " << some_spectrum->total_count();

    std::shared_ptr<Qpx::Spectrum::EntryList> spectrum_data =
        std::move(some_spectrum->get_spectrum({{0, adjrange}, {0, adjrange}}));
    ui->coincPlot->update_plot(adjrange, spectrum_data);

    //        PL_DBG << "rescaling 2d";
    name_2d = name_2d;

    int newbits = some_spectrum->bits();
    if (bits != newbits)
      bits = newbits;

    Qpx::Detector detector_x_;
    Qpx::Detector detector_y_;
    if (md.detectors.size() > 1) {
      detector_x_ = md.detectors[0];
      detector_y_ = md.detectors[1];
    }

    calib_x_ = detector_x_.best_calib(bits);
    calib_y_ = detector_y_.best_calib(bits);

    ui->coincPlot->set_axes(calib_x_, calib_y_, bits);

  } else {
    ui->coincPlot->reset_content();
    ui->coincPlot->refresh();
  }

  replot_markers();


  PL_DBG << "<Plot2D> plotting took " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}

void FormGatesPlot2D::markers_moved(Marker x, Marker y) {
  if (gates_movable_) {
    x_marker = x;
    y_marker = y;
    replot_markers();
  }

  emit markers_set(x, y);
}

void FormGatesPlot2D::set_markers(Marker x, Marker y) {
  x_marker = x;
  y_marker = y;

  replot_markers();
}

void FormGatesPlot2D::on_spinGateWidth_valueChanged(int arg1)
{
  /*int width = ui->spinGateWidth->value() / 2;
  if ((ui->spinGateWidth->value() % 2) == 0)
    ui->spinGateWidth->setValue(width*2 + 1);*/

  replot_markers();
  //emit markers_set(x_marker, y_marker);
}

void FormGatesPlot2D::set_gates_visible(bool vertical, bool horizontal, bool diagonal)
{
  gate_vertical_ = vertical;
  gate_horizontal_ = horizontal;
  gate_diagonal_ = diagonal;

  replot_markers();
}


void FormGatesPlot2D::on_spinGateWidth_editingFinished()
{
  int width = ui->spinGateWidth->value() / 2;
  if ((ui->spinGateWidth->value() % 2) == 0)
    ui->spinGateWidth->setValue(width*2 + 1);

  replot_markers();
  emit markers_set(x_marker, y_marker);
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
