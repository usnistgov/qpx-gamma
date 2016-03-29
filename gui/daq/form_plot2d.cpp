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

  ui->spectrumSelector->set_only_one(true);
  connect(ui->spectrumSelector, SIGNAL(itemSelected(SelectorItem)), this, SLOT(choose_spectrum(SelectorItem)));
  connect(ui->spectrumSelector, SIGNAL(itemDoubleclicked(SelectorItem)), this, SLOT(spectrumDoubleclicked(SelectorItem)));


  QWidget *popup = new QWidget(this);

  crop_slider_ = new QSlider(Qt::Vertical, popup);
  crop_slider_->setRange(0, 100);

  crop_label_ = new QLabel(popup);
  crop_label_->setAlignment(Qt::AlignCenter);
  crop_label_->setNum(100);
  crop_label_->setMinimumWidth(crop_label_->sizeHint().width());

  typedef void(QLabel::*IntSlot)(int);
  connect(crop_slider_, &QAbstractSlider::valueChanged, crop_label_, static_cast<IntSlot>(&QLabel::setNum));

  QBoxLayout *popupLayout = new QHBoxLayout(popup);
  popupLayout->setMargin(2);
  popupLayout->addWidget(crop_slider_);
  popupLayout->addWidget(crop_label_);

  QWidgetAction *action = new QWidgetAction(this);
  action->setDefaultWidget(popup);

  crop_menu_ = new QMenu(this);
  crop_menu_->addAction(action);


  ui->toolCrop->setMenu(crop_menu_);
  ui->toolCrop->setPopupMode(QToolButton::InstantPopup);
  connect(crop_menu_, SIGNAL(aboutToHide()), this, SLOT(crop_changed()));


  ui->labelBlank->setVisible(false);

  zoom_2d = 0.5;
  crop_slider_->setValue(50);
  crop_changed();

  adjrange = 0;

  bits = 0;
}

FormPlot2D::~FormPlot2D()
{
  delete ui;
}

void FormPlot2D::setDetDB(XMLableDB<Qpx::Detector>& detDB) {
  detectors_ = &detDB;
}

void FormPlot2D::spectrumDoubleclicked(SelectorItem item)
{
  on_pushDetails_clicked();
}

void FormPlot2D::crop_changed() {
  PL_DBG << "changing zoom";
  new_zoom = crop_slider_->value() / 100.0;
  ui->toolCrop->setText(QString::number(crop_slider_->value()) + "% ");
  if (this->isVisible() && (mySpectra != nullptr))
    mySpectra->activate();
}

void FormPlot2D::setSpectra(Qpx::Project& new_set, QString spectrum) {
//  PL_DBG << "setSpectra with " << spectrum.toStdString();
  mySpectra = &new_set;

  name_2d = spectrum;

  updateUI();
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
  std::list<std::shared_ptr<Qpx::Sink>> spectra = mySpectra->spectra(2, -1);

  for (auto &q : spectra) {
    Qpx::Setting vis = q->metadata().attributes.branches.get(Qpx::Setting("visible"));
    if (q->name() == id.toStdString())
      vis.value_int = true;
    else
      vis.value_int = false;
    q->set_generic_attr(vis);
  }

  //name_2d = arg1;
  update_plot(true);
}

void FormPlot2D::updateUI()
{
  QVector<SelectorItem> items;

  std::string newname;
  std::set<std::string> names;

  for (auto &q : mySpectra->spectra(2, -1)) {
    Qpx::Metadata md;
    if (q != nullptr)
      md = q->metadata();

    names.insert(md.name);
    if (md.attributes.branches.get(Qpx::Setting("visible")).value_int)
      newname = md.name;

    SelectorItem new_spectrum;
    new_spectrum.text = QString::fromStdString(md.name);
    new_spectrum.color = QColor(QString::fromStdString(md.attributes.branches.get(Qpx::Setting("appearance")).value_text));
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

void FormPlot2D::refresh()
{
  ui->coincPlot->refresh();
}

void FormPlot2D::replot_markers() {
  //PL_DBG << "replot markers";

  std::list<MarkerBox2D> boxes;
  std::list<MarkerLabel2D> labels;
  MarkerLabel2D label;

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
      label.text = QString::number(q.y_c.energy());
      labels.push_back(label);
    } else if (q.vertical) {
      label.x = q.x2;
      label.y = q.y_c;
      label.vertical = true;
      label.text = QString::number(q.x_c.energy());
      labels.push_back(label);
    }
  }

  MarkerBox2D gate, gatex, gatey;
  gate.selectable = false;
  gate.selected = false;


  //PL_DBG << "FormPlot2d marker width = " << width;

  gate.x_c = x_marker.pos;
  gate.y_c = y_marker.pos;

  gate.visible = y_marker.visible;
  gate.x1.set_bin(0, bits, calib_x_);
  gate.x2.set_bin(adjrange, bits, calib_x_);
  gate.y1.set_bin(y_marker.pos.bin(bits), bits, calib_x_);
  gate.y2.set_bin(y_marker.pos.bin(bits), bits, calib_x_);
  gatey = gate;
  boxes.push_back(gate);

  gate.visible = x_marker.visible;
  gate.x1.set_bin(x_marker.pos.bin(bits), bits, calib_x_);
  gate.x2.set_bin(x_marker.pos.bin(bits), bits, calib_x_);
  gate.y1.set_bin(0, bits, calib_x_);
  gate.y2.set_bin(adjrange, bits, calib_x_);
  gatex = gate;
  boxes.push_back(gate);


  label.selectable = false;
  label.selected = false;
  label.vertical = false;

//  if (y_marker.visible && x_marker.visible) {
//    label.x = gatex.x2;
//    label.y = gatey.y_c;
//    label.text = QString::number(y_marker.pos.energy());
//    labels.push_back(label);

//    label.x = gatex.x_c;
//    label.y = gatey.y2;
//    label.vertical = true;
//    label.text = QString::number(x_marker.pos.energy());
//    labels.push_back(label);
//  }
  if (y_marker.visible) {
    label.x = gatex.x2;
    label.y = gatey.y_c;
    label.vertical = false;
    label.text = QString::number(y_marker.pos.energy());
    labels.push_back(label);
  }
  if (x_marker.visible) {
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

void FormPlot2D::update_plot(bool force) {
//  PL_DBG << "updating 2d";

  this->setCursor(Qt::WaitCursor);
  CustomTimer guiside(true);

  bool new_data = mySpectra->new_data();
  bool rescale2d = (zoom_2d != new_zoom);

  if (rescale2d || new_data || force) {
//    PL_DBG << "really updating 2d " << name_2d.toStdString();

    ui->pushSymmetrize->setEnabled(false);

    QString newname = ui->spectrumSelector->selected().text;
    //PL_DBG << "<Plot2D> getting " << newname.toStdString();

    std::shared_ptr<Qpx::Sink> some_spectrum = mySpectra->by_name(newname.toStdString());

    ui->pushDetails->setEnabled((some_spectrum != nullptr));
    zoom_2d = new_zoom;

    Qpx::Metadata md;
    if (some_spectrum)
      md = some_spectrum->metadata();


    if ((md.total_count > 0) && (md.dimensions() == 2) && (adjrange = pow(2,md.bits) * zoom_2d))
    {
//      PL_DBG << "really really updating 2d total count = " << some_spectrum->total_count();

      Qpx::Setting sym = md.attributes.branches.get(Qpx::Setting("symmetrized"));

      //PL_DBG << "Sym :" << sym.id_ << "=" << sym.value_int;

      ui->pushSymmetrize->setEnabled(sym.value_int == 0);

      std::shared_ptr<Qpx::EntryList> spectrum_data =
          std::move(some_spectrum->get_spectrum({{0, adjrange}, {0, adjrange}}));
      ui->coincPlot->update_plot(adjrange, spectrum_data);

      if (rescale2d || force || (name_2d != newname)) {
//        PL_DBG << "rescaling 2d";
        name_2d = newname;

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
  x_marker = x;
  y_marker = y;
  ext_marker.visible = ext_marker.visible & x.visible;
  //PL_DBG << "markers changed";
  replot_markers();

  emit markers_set(x, y);
}

void FormPlot2D::set_marker(Marker n) {
  ext_marker = n;
  if (!ext_marker.visible) {
    x_marker.visible = false;
    y_marker.visible = false;
  }
  replot_markers();
}

void FormPlot2D::set_markers(Marker x, Marker y) {
  x_marker = x;
  y_marker = y;

  replot_markers();
}

void FormPlot2D::on_pushDetails_clicked()
{
  std::shared_ptr<Qpx::Sink> someSpectrum = mySpectra->by_name(name_2d.toStdString());
  if (someSpectrum == nullptr)
    return;

  if (detectors_ == nullptr)
    return;

  dialog_spectrum* newSpecDia = new dialog_spectrum(*someSpectrum, *detectors_, this);
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
  std::list<std::shared_ptr<Qpx::Sink>> spectra = mySpectra->spectra(2, -1);

  for (auto &q : spectra) {
    Qpx::Setting vis = q->metadata().attributes.branches.get(Qpx::Setting("visible"));
    if (q->name() == name.toStdString())
      vis.value_int = true;
    else
      vis.value_int = false;
    q->set_generic_attr(vis);
  }

  update_plot(true);
}

void FormPlot2D::set_zoom(double zm) {
  if (zm > 1.0)
    zm = 1.0;
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

void FormPlot2D::on_pushSymmetrize_clicked()
{
  emit requestSymmetrize(name_2d);
}
