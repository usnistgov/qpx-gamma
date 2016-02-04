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
 *      FormPeaks -
 *
 ******************************************************************************/

#include "form_peaks.h"
#include "widget_detectors.h"
#include "ui_form_peaks.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormPeaks::FormPeaks(QWidget *parent) :
  QWidget(parent),
  fit_data_(nullptr),
  ui(new Ui::FormPeaks)
{
  ui->setupUi(this);

  range_.visible = false;
  range_.top.default_pen = QPen(Qt::darkMagenta, 1);
  range_.base.default_pen = QPen(Qt::darkMagenta, 2, Qt::DashLine);

  list.appearance.default_pen = QPen(Qt::darkGray, 1);
  list.selected_appearance.default_pen = QPen(Qt::black, 2);
  list.visible = true;

  connect(ui->plot1D, SIGNAL(markers_selected()), this, SLOT(user_selected_peaks()));
  connect(ui->plot1D, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->plot1D, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));
  connect(ui->plot1D, SIGNAL(range_moved(double, double)), this, SLOT(range_moved(double, double)));

  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Insert), ui->plot1D);
  connect(shortcut, SIGNAL(activated()), this, SLOT(on_pushAdd_clicked()));

  connect(&thread_fitter_, SIGNAL(fit_updated(Gamma::Fitter)), this, SLOT(fit_updated(Gamma::Fitter)));

  thread_fitter_.start();
}

FormPeaks::~FormPeaks()
{
  thread_fitter_.stop_work();
  thread_fitter_.terminate(); //in thread itself?

  delete ui;
}

void FormPeaks::setFit(Gamma::Fitter* fit) {
  fit_data_ = fit;
  update_fit(true);
}

void FormPeaks::loadSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  ui->spinSqWidth->setValue(settings_.value("square_width", 3).toInt());
  ui->spinSum4EdgeW->setValue(settings_.value("sum4_edge_width", 3).toInt());
  ui->doubleOverlapWidth->setValue(settings_.value("overlap_width", 2.70).toDouble());
  ui->doubleThresh->setValue(settings_.value("peak_threshold", 3.0).toDouble());
  ui->plot1D->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  settings_.endGroup();
}

void FormPeaks::saveSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  settings_.setValue("square_width", ui->spinSqWidth->value());
  settings_.setValue("sum4_edge_width", ui->spinSum4EdgeW->value());
  settings_.setValue("peak_threshold", ui->doubleThresh->value());
  settings_.setValue("overlap_width", ui->doubleOverlapWidth->value());
  settings_.setValue("scale_type", ui->plot1D->scale_type());
  settings_.endGroup();
}

void FormPeaks::clear() {
  //PL_DBG << "FormPeaks::clear()";
  toggle_push();
  ui->plot1D->setTitle("");
  ui->plot1D->clearGraphs();
  ui->plot1D->clearExtras();
  ui->plot1D->reset_scales();
  replot_all();
}


void FormPeaks::make_range(Marker marker) {
  if (marker.visible)
    addMovingMarker(marker.pos);
  else
    removeMovingMarker(0);
}


void FormPeaks::new_spectrum(QString title) {
  if (fit_data_ == nullptr)
    return;

  if (fit_data_->peaks_.empty()) {
    fit_data_->finder_.find_peaks(ui->spinSqWidth->value(), ui->doubleThresh->value());

    //    PL_DBG << "number of peaks found " << fit_data_->peaks_.size();
    //    on_pushFindPeaks_clicked();
  }


  if (title.isEmpty())
    title = "Spectrum=" + QString::fromStdString(fit_data_->metadata_.name) + "  resolution=" + QString::number(fit_data_->metadata_.bits) + "bits  Detector=" + QString::fromStdString(fit_data_->detector_.name_);

  ui->plot1D->setTitle(title);

  ui->plot1D->reset_scales();
  update_spectrum();
  toggle_push();
}

void FormPeaks::tighten() {
  ui->plot1D->tight_x();
}


void FormPeaks::update_spectrum() {
  if (fit_data_ == nullptr)
    return;

  if (fit_data_->metadata_.resolution == 0) {
    clear();
    replot_all();
    return;
  }

  replot_all();
  ui->plot1D->redraw();
}


void FormPeaks::replot_all() {
  if (fit_data_ == nullptr)
    return;

  //PL_DBG << "FormPeaks::replot_all()";

  ui->plot1D->blockSignals(true);
  this->blockSignals(true);
  
  ui->plot1D->clearGraphs();
  ui->plot1D->clearExtras();

  ui->plot1D->updateData();

  replot_markers();
}

void FormPeaks::addMovingMarker(double x) {
  if (fit_data_ == nullptr)
    return;

  Coord c;

  if (fit_data_->nrg_cali_.valid())
    c.set_energy(x, fit_data_->nrg_cali_);
  else
    c.set_bin(x, fit_data_->metadata_.bits, fit_data_->nrg_cali_);

  addMovingMarker(c);
}

void FormPeaks::addMovingMarker(Coord c) {
  if (fit_data_ == nullptr)
    return;
  
  range_.visible = true;

  double x = c.bin(fit_data_->metadata_.bits);

  uint16_t ch = static_cast<uint16_t>(x);
  uint16_t ch_l = fit_data_->finder_.find_left(ch);
  uint16_t ch_r = fit_data_->finder_.find_right(ch);

  for (auto &q : fit_data_->regions_) {
    if (q.overlaps(x)) {
      ch_l = q.finder_.find_left(ch);
      ch_r = q.finder_.find_right(ch);
    }
  }

  range_.center.set_bin(x, fit_data_->metadata_.bits, fit_data_->nrg_cali_);
  range_.l.set_bin(ch_l, fit_data_->metadata_.bits, fit_data_->nrg_cali_);
  range_.r.set_bin(ch_r, fit_data_->metadata_.bits, fit_data_->nrg_cali_);

  ui->pushAdd->setEnabled(true);

  for (auto &q : fit_data_->peaks_)
    q.second.selected = false;
  toggle_push();
  replot_markers();

  emit peaks_changed(false);
  emit range_changed(range_);
}

void FormPeaks::removeMovingMarker(double dummy) {
  range_.visible = false;
  toggle_push();
  replot_markers();
  emit range_changed(range_);
}


void FormPeaks::replot_markers() {

  if (fit_data_ == nullptr)
    return;

  ui->plot1D->use_calibrated(fit_data_->nrg_cali_.valid());

  ui->plot1D->blockSignals(true);
  this->blockSignals(true);

  ui->plot1D->clearExtras();

  if (range_.visible)
    ui->plot1D->set_range(range_);

  std::list<Marker> markers;

  for (auto &q : fit_data_->peaks_) {
    Marker m = list;
    m.pos.set_bin(q.second.center, fit_data_->metadata_.bits, fit_data_->nrg_cali_);
    m.selected = q.second.selected;
    markers.push_back(m);
  }

  ui->plot1D->set_markers(markers);
  ui->plot1D->replot_markers();
  ui->plot1D->redraw();

  ui->plot1D->blockSignals(false);
  this->blockSignals(false);

}

void FormPeaks::on_pushFindPeaks_clicked()
{
  this->setCursor(Qt::WaitCursor);

  perform_fit();

  emit peaks_changed(true);
  this->setCursor(Qt::ArrowCursor);
}


void FormPeaks::perform_fit() {
  if (fit_data_ == nullptr)
    return;

  fit_data_->finder_.find_peaks(ui->spinSqWidth->value(),ui->doubleThresh->value());
  fit_data_->overlap_ = ui->doubleOverlapWidth->value();
  fit_data_->sum4edge_samples = ui->spinSum4EdgeW->value();
  fit_data_->find_peaks();
  //  PL_DBG << "number of peaks found " << fit_data_->peaks_.size();

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.fit_peaks();

  toggle_push();
  replot_all();

}

void FormPeaks::fit_updated(Gamma::Fitter fitter) {
  *fit_data_ = fitter;
  toggle_push();
  replot_all();
  emit peaks_changed(true);
}

void FormPeaks::on_pushAdd_clicked()
{
  if (!ui->pushAdd->isEnabled())
    return;

  if (range_.l.energy() >= range_.r.energy())
    return;

  fit_data_->add_peak(range_.l.bin(fit_data_->metadata_.bits), range_.r.bin(fit_data_->metadata_.bits));
  ui->pushAdd->setEnabled(false);
  removeMovingMarker(0);
  toggle_push();
  replot_all();
  emit peaks_changed(true);
}

void FormPeaks::user_selected_peaks() {

  if (fit_data_ == nullptr)
    return;

  bool changed = false;
  std::set<double> chosen_peaks = ui->plot1D->get_selected_markers();
  for (auto &q : fit_data_->peaks_) {
    if (q.second.selected != (chosen_peaks.count(q.second.center) > 0))
      changed = true;
    q.second.selected = (chosen_peaks.count(q.second.center) > 0);
  }

  if (changed) {
    range_.visible = false;
    toggle_push();
    replot_markers();
    if (isVisible()) {
      emit peaks_changed(false);
      emit range_changed(range_);
    }
  }
}

void FormPeaks::toggle_push() {
  if (fit_data_ == nullptr)
    return;

  bool sel = false;
  for (auto &q : fit_data_->peaks_)
    if (q.second.selected)
      sel = true;
  ui->pushAdd->setEnabled(range_.visible);
  ui->pushMarkerRemove->setEnabled(sel);
}

void FormPeaks::on_pushMarkerRemove_clicked()
{
  if (fit_data_ == nullptr)
    return;

  std::set<double> chosen_peaks;
  for (auto &q : fit_data_->peaks_)
    if (q.second.selected)
      chosen_peaks.insert(q.second.center);

  fit_data_->remove_peaks(chosen_peaks);

  toggle_push();
  replot_all();
  emit peaks_changed(true);
}

void FormPeaks::on_spinSqWidth_editingFinished()
{
  if (fit_data_ == nullptr)
    return;

  fit_data_->finder_.find_peaks(ui->spinSqWidth->value(), ui->doubleThresh->value());
  replot_all();
}

void FormPeaks::on_spinSum4EdgeW_editingFinished()
{
  fit_data_->sum4edge_samples = ui->spinSum4EdgeW->value();
}


void FormPeaks::range_moved(double l, double r) {

  //PL_DBG << "<peaks> range moved";

  if (r < l) {
    double t = l;
    l = r;
    r = t;
  }

  //double c = (r + l) / 2.0;

  if (fit_data_->nrg_cali_.valid()) {
    range_.l.set_energy(l, fit_data_->nrg_cali_);
    range_.r.set_energy(r, fit_data_->nrg_cali_);
    //range_.center.set_energy(c, fit_data_->nrg_cali_);
  } else {
    range_.l.set_bin(l, fit_data_->metadata_.bits, fit_data_->nrg_cali_);
    range_.r.set_bin(r, fit_data_->metadata_.bits, fit_data_->nrg_cali_);
    //range_.center.set_bin(c, fit_data_->metadata_.bits, fit_data_->nrg_cali_);
  }

  toggle_push();
  replot_markers();

  emit range_changed(range_);
}

void FormPeaks::update_fit(bool content_changed) {
  ui->plot1D->setData(fit_data_);
  if (content_changed) {
    update_spectrum();
    replot_all();
  } else
    replot_markers();
  toggle_push();
}

void FormPeaks::on_doubleOverlapWidth_editingFinished()
{
  fit_data_->overlap_ = ui->doubleOverlapWidth->value();
}


void FormPeaks::on_doubleThresh_editingFinished()
{
  if (fit_data_ == nullptr)
    return;

  fit_data_->finder_.find_peaks(ui->spinSqWidth->value(), ui->doubleThresh->value());
  replot_all();
}
