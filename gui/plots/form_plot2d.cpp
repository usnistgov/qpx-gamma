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

#include "form_plot2d.h"
#include "ui_form_plot2d.h"
#include "dialog_spectrum.h"
#include "custom_timer.h"

FormPlot2D::FormPlot2D(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormPlot2D)
{
  ui->setupUi(this);

  connect(ui->coincPlot, SIGNAL(markers_set(Marker,Marker)), this, SLOT(markers_moved(Marker,Marker)));
  connect(ui->coincPlot, SIGNAL(stuff_selected()), this, SLOT(selection_changed()));

  ui->spectrumSelector->set_only_one(true);
  connect(ui->spectrumSelector, SIGNAL(itemSelected(SelectorItem)), this, SLOT(choose_spectrum(SelectorItem)));
  connect(ui->spectrumSelector, SIGNAL(itemDoubleclicked(SelectorItem)), this, SLOT(spectrumDoubleclicked(SelectorItem)));

  fractions_["Full"] = 1;
  fractions_["9/16"] = 9.0/16.0;
  fractions_["4/9"]  = 4.0/9.0;
  fractions_["1/4"]  = 1.0/4.0;
  fractions_["1/9"]  = 1.0/9.0;
  fractions_["1/16"] = 1.0/16.0;

  for (auto &q : fractions_)
    cropFraction.addAction(QString::fromStdString(q.first));
  for (auto &q : cropFraction.actions())
    q->setCheckable(true);
  ui->toolCrop->setMenu(&cropFraction);
  ui->toolCrop->setPopupMode(QToolButton::InstantPopup);
  connect(&cropFraction, SIGNAL(triggered(QAction*)), this, SLOT(crop_changed(QAction*)));

  ui->labelBlank->setVisible(false);

  new_zoom = zoom_2d = 0.5;
  adjrange =0;
  //scaling-related stuff
  bits = 0;
}

FormPlot2D::~FormPlot2D()
{
  delete ui;
}

void FormPlot2D::selection_changed() {
  emit stuff_selected();
}

void FormPlot2D::set_range_x(MarkerBox2D range) {
  range_ = range;
  ui->coincPlot->set_range_x(range);
}

void FormPlot2D::spectrumDoubleclicked(SelectorItem item)
{
  on_pushDetails_clicked();
}

void FormPlot2D::crop_changed(QAction* action) {
  QString choice = action->text();
  for (auto &q : cropFraction.actions())
    q->setChecked(choice == q->text());
  if (fractions_.count(choice.toStdString())) {
    new_zoom = fractions_.at(choice.toStdString());
    PL_DBG << "new zoom";
    if (this->isVisible() && (mySpectra != nullptr))
      mySpectra->activate();
  }
}

void FormPlot2D::setSpectra(Qpx::SpectraSet& new_set, QString spectrum) {
//  PL_DBG << "setSpectra with " << spectrum.toStdString();
  mySpectra = &new_set;

  name_2d = spectrum;

  updateUI();
}

void FormPlot2D::set_gates_movable(bool mov) {
  gates_movable_ = mov;
}

void FormPlot2D::set_show_boxes(bool show) {
  show_boxes_ = show;
  ui->coincPlot->replot_markers();
}

std::list<MarkerBox2D> FormPlot2D::get_selected_boxes() {
  return ui->coincPlot->get_selected_boxes();
}


void FormPlot2D::set_boxes(std::list<MarkerBox2D> boxes) {
  boxes_ = boxes;
  replot_markers();
}


void FormPlot2D::reset_content() {
  //PL_DBG << "reset content";
  ui->coincPlot->reset_content();
  ui->coincPlot->refresh();
  name_2d.clear();
  x_marker.visible = false;
  y_marker.visible = false;
  ext_marker.visible = false;
  replot_markers();
}

void FormPlot2D::choose_spectrum(SelectorItem item)
{
  QString id = ui->spectrumSelector->selected().text;

  if (id == name_2d)
    return;
  std::list<Qpx::Spectrum::Spectrum*> spectra = mySpectra->spectra(2, -1);

  for (auto &q : spectra)
    if (q->name() == id.toStdString())
      q->set_visible(true);
    else
      q->set_visible(false);

  //name_2d = arg1;
  update_plot(true);
}

void FormPlot2D::updateUI()
{
  QVector<SelectorItem> items;

  std::string newname;
  std::set<std::string> names;

  for (auto &q : mySpectra->spectra(2, -1)) {
    Qpx::Spectrum::Metadata md;
    if (q != nullptr)
      md = q->metadata();

    names.insert(md.name);
    if (md.visible)
      newname = md.name;

    SelectorItem new_spectrum;
    new_spectrum.text = QString::fromStdString(md.name);
    new_spectrum.color = QColor::fromRgba(md.appearance);
    items.push_back(new_spectrum);
  }

  if (newname.empty() && !names.empty())
    newname = *names.begin();

  if (names.count(name_2d.toStdString()))
    newname = name_2d.toStdString();

  for (auto &q : items) {
    q.visible = (q.text.toStdString() == newname);
  }

  ui->spectrumSelector->setItems(items);
}

void FormPlot2D::set_show_selector(bool vis) {
  ui->spectrumSelector->setVisible(vis);
  ui->labelBlank->setVisible(!vis);
}

void FormPlot2D::set_gate_width(int w) {
  ui->spinGateWidth->setValue(w);
}

void FormPlot2D::set_show_gate_width(bool vis) {
  ui->labelGateWidth->setVisible(vis);
  ui->spinGateWidth->setVisible(vis);
}

int FormPlot2D::gate_width() {
  return ui->spinGateWidth->value();
}

void FormPlot2D::refresh()
{
  ui->coincPlot->refresh();
}

void FormPlot2D::replot_markers() {
  //PL_DBG << "replot markers";

  std::list<MarkerBox2D> boxes;
  if (show_boxes_)
    boxes = boxes_;

  int width = ui->spinGateWidth->value() / 2;

  if (!ui->spinGateWidth->isVisible())
    width = 0;

  MarkerBox2D gate;
  gate.selectable = false;
  gate.selected = false;
  gate.x_c = x_marker.channel;
  gate.y_c = y_marker.channel;
  gate.x1 = x_marker; gate.x1.chan_valid = true; gate.x1.energy_valid = false;
  gate.x2 = x_marker; gate.x2.chan_valid = true; gate.x2.energy_valid = false;
  gate.y1 = y_marker; gate.y1.chan_valid = true; gate.y1.energy_valid = false;
  gate.y2 = y_marker; gate.y2.chan_valid = true; gate.y2.energy_valid = false;

  gate.visible = y_marker.visible;
  gate.x1.channel = 0;
  gate.x2.channel = adjrange;
  gate.y1.channel -= width;
  gate.y2.channel += width;
  calibrate_marker(gate.x1);
  calibrate_marker(gate.x2);
  calibrate_marker(gate.y1);
  calibrate_marker(gate.y2);
  boxes.push_back(gate);

  gate.visible = x_marker.visible;
  gate.x1 = x_marker; gate.x1.chan_valid = true; gate.x1.energy_valid = false;
  gate.x2 = x_marker; gate.x2.chan_valid = true; gate.x2.energy_valid = false;
  gate.y1 = y_marker; gate.y1.chan_valid = true; gate.y1.energy_valid = false;
  gate.y2 = y_marker; gate.y2.chan_valid = true; gate.y2.energy_valid = false;
  gate.x1.channel -= width;
  gate.x2.channel += width;
  gate.y1.channel = 0;
  gate.y2.channel = adjrange;
  calibrate_marker(gate.x1);
  calibrate_marker(gate.x2);
  calibrate_marker(gate.y1);
  calibrate_marker(gate.y2);
  boxes.push_back(gate);

  ui->coincPlot->set_boxes(boxes);

  ui->coincPlot->replot_markers();
}

void FormPlot2D::update_plot(bool force) {
//  PL_DBG << "updating 2d";

  this->setCursor(Qt::WaitCursor);
  CustomTimer guiside(true);

  bool new_data = mySpectra->new_data();
  bool rescale2d = (zoom_2d != new_zoom);

  if (rescale2d || new_data || force) {
//    PL_DBG << "really updating 2d " << name_2d.toStdString();

    QString newname = ui->spectrumSelector->selected().text;
    //PL_DBG << "<Plot2D> getting " << newname.toStdString();

    Qpx::Spectrum::Spectrum* some_spectrum = mySpectra->by_name(newname.toStdString());

    ui->pushDetails->setEnabled((some_spectrum != nullptr));
    zoom_2d = new_zoom;

    Qpx::Spectrum::Metadata md;
    if (some_spectrum)
      md = some_spectrum->metadata();

    if ((md.total_count > 0) && (md.dimensions == 2) && (adjrange = static_cast<uint32_t>(md.resolution * zoom_2d)) )
    {
//      PL_DBG << "really really updating 2d total count = " << some_spectrum->total_count();

      std::shared_ptr<Qpx::Spectrum::EntryList> spectrum_data =
          std::move(some_spectrum->get_spectrum({{0, adjrange}, {0, adjrange}}));
      ui->coincPlot->update_plot(adjrange, spectrum_data);

      if (rescale2d || force || (name_2d != newname)) {
//        PL_DBG << "rescaling 2d";
        name_2d = newname;

        int newbits = some_spectrum->bits();
        if (bits != newbits) {
          bits = newbits;
          ext_marker.shift(bits);
          x_marker.shift(bits);
          y_marker.shift(bits);
        }

        Gamma::Detector detector_x_;
        Gamma::Detector detector_y_;
        if (md.detectors.size() > 1) {
          detector_x_ = md.detectors[0];
          detector_y_ = md.detectors[1];
        }

        if (detector_x_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits)))
          calib_x_ = detector_x_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
        else
          calib_x_ = detector_x_.highest_res_calib();

        if (detector_y_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits)))
          calib_y_ = detector_y_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
        else
          calib_y_ = detector_y_.highest_res_calib();

        if (!calib_x_.bits_)
          calib_x_.bits_ = bits;
        if (!calib_y_.bits_)
          calib_y_.bits_ = bits;

        calibrate_marker(x_marker);
        calibrate_marker(y_marker);

        ui->coincPlot->set_axes(calib_x_, calib_y_, bits);
      }

    } else {
      ui->coincPlot->reset_content();
      ui->coincPlot->refresh();
    }

    replot_markers();
  }

  PL_DBG << "<Plot2D> plotting took " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}

void FormPlot2D::markers_moved(Marker x, Marker y) {
  calibrate_marker(x);
  calibrate_marker(y);

  if (gates_movable_) {
    x_marker = x;
    y_marker = y;
    ext_marker.visible = ext_marker.visible & x.visible;
    replot_markers();
  }

  emit markers_set(x, y);
}

void FormPlot2D::calibrate_marker(Marker &marker) {
  if (!marker.energy_valid && marker.chan_valid) {
    marker.energy = calib_x_.transform(marker.channel, marker.bits);
    marker.energy_valid = (calib_x_.units_ != "channels");
  }
}


void FormPlot2D::set_marker(Marker n) {
  ext_marker = n;
  ext_marker.shift(bits);
  if (!ext_marker.visible) {
    x_marker.visible = false;
    y_marker.visible = false;
  }
  replot_markers();
}

void FormPlot2D::set_markers(Marker x, Marker y) {
  x_marker = x;
  y_marker = y;

  x_marker.shift(bits);
  y_marker.shift(bits);

  replot_markers();
}

void FormPlot2D::on_pushDetails_clicked()
{
  Qpx::Spectrum::Spectrum* someSpectrum = mySpectra->by_name(name_2d.toStdString());
  if (someSpectrum == nullptr)
    return;

  dialog_spectrum* newSpecDia = new dialog_spectrum(*someSpectrum, this);
  connect(newSpecDia, SIGNAL(finished(bool)), this, SLOT(spectrumDetailsClosed(bool)));
  connect(newSpecDia, SIGNAL(delete_spectrum()), this, SLOT(spectrumDetailsDelete()));
  connect(newSpecDia, SIGNAL(analyse()), this, SLOT(analyse()));
  newSpecDia->exec();
}

void FormPlot2D::spectrumDetailsClosed(bool changed) {
  if (changed) {
    //replot?
  }
}

void FormPlot2D::spectrumDetailsDelete()
{
  mySpectra->delete_spectrum(name_2d.toStdString());

  updateUI();

  QString name = ui->spectrumSelector->selected().text;
  std::list<Qpx::Spectrum::Spectrum*> spectra = mySpectra->spectra(2, -1);

  for (auto &q : spectra)
    if (q->name() == name.toStdString())
      q->set_visible(true);
    else
      q->set_visible(false);

  update_plot(true);
}

void FormPlot2D::set_zoom(double zm) {
  new_zoom = zm;
  if (this->isVisible() && (mySpectra != nullptr))
    mySpectra->activate();
}

double FormPlot2D::zoom() {
  return zoom_2d;
}

void FormPlot2D::analyse()
{
  emit requestAnalysis(name_2d);
}

void FormPlot2D::on_spinGateWidth_valueChanged(int arg1)
{
  /*int width = ui->spinGateWidth->value() / 2;
  if ((ui->spinGateWidth->value() % 2) == 0)
    ui->spinGateWidth->setValue(width*2 + 1);*/

  replot_markers();
  //emit markers_set(x_marker, y_marker);
}

void FormPlot2D::set_gates_visible(bool vertical, bool horizontal, bool diagonal)
{
  gate_vertical_ = vertical;
  gate_horizontal_ = horizontal;
  gate_diagonal_ = diagonal;

  replot_markers();
}


void FormPlot2D::on_spinGateWidth_editingFinished()
{
  int width = ui->spinGateWidth->value() / 2;
  if ((ui->spinGateWidth->value() % 2) == 0)
    ui->spinGateWidth->setValue(width*2 + 1);

  replot_markers();
  emit markers_set(x_marker, y_marker);
}

void FormPlot2D::set_scale_type(QString st) {
  ui->coincPlot->set_scale_type(st);
}

void FormPlot2D::set_gradient(QString gr) {
  ui->coincPlot->set_gradient(gr);
}

void FormPlot2D::set_show_legend(bool sl) {
  ui->coincPlot->set_show_legend(sl);
}

QString FormPlot2D::scale_type() {
  return ui->coincPlot->scale_type();
}

QString FormPlot2D::gradient() {
  return ui->coincPlot->gradient();
}

bool FormPlot2D::show_legend() {
  return ui->coincPlot->show_legend();
}

