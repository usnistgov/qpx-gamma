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

  qRegisterMetaType<Gaussian>("Gaussian");
  qRegisterMetaType<QVector<Gaussian>>("QVector<Gaussian>");

  ui->plot1D->use_calibrated(true);
  ui->plot1D->set_markers_selectable(true);

  range_.visible = false;
  range_.center.visible = true;
  range_.center.appearance.themes["light"] = QPen(Qt::darkMagenta, 3);
  range_.center.appearance.themes["dark"] = QPen(Qt::magenta, 3);
  range_.l.appearance.themes["light"] = QPen(Qt::darkMagenta, 10);
  range_.l.appearance.themes["dark"] = QPen(Qt::magenta, 10);
  range_.l.visible = true;
  range_.r = range_.l;
  range_.base.themes["light"] = QPen(Qt::darkMagenta, 2, Qt::DashLine);
  range_.base.themes["dark"] = QPen(Qt::magenta, 2, Qt::DashLine);
  range_.top.themes["light"] = QPen(Qt::darkMagenta, 0, Qt::DashLine);
  range_.top.themes["dark"] = QPen(Qt::magenta, 0, Qt::DashLine);


  list.appearance.themes["light"] = QPen(Qt::darkGray, 1);
  list.appearance.themes["dark"] = QPen(Qt::white, 1);
  list.selected_appearance.themes["light"] = QPen(Qt::black, 2);
  list.selected_appearance.themes["dark"] = QPen(Qt::yellow, 2);
  list.visible = true;

  main_graph_.default_pen = QPen(Qt::gray, 1);
  prelim_peak_.default_pen = QPen(Qt::black, 4);
  filtered_peak_.default_pen = QPen(Qt::blue, 6);
  gaussian_.default_pen = QPen(Qt::darkBlue, 0);

  QColor flagged_color;
  flagged_color.setHsv(QColor(Qt::green).hsvHue(), QColor(Qt::green).hsvSaturation(), 192);

  flagged_.default_pen =  QPen(flagged_color, 1);
  pseudo_voigt_.default_pen = QPen(Qt::darkCyan, 0);

  multiplet_.default_pen = QPen(Qt::red, 2);

  baseline_.default_pen = QPen(Qt::darkGray, 0);
  rise_.default_pen = QPen(Qt::green, 0);
  fall_.default_pen = QPen(Qt::red, 0);
  even_.default_pen = QPen(Qt::black, 0);

  connect(ui->plot1D, SIGNAL(markers_selected()), this, SLOT(user_selected_peaks()));
  connect(ui->plot1D, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->plot1D, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));
  connect(ui->plot1D, SIGNAL(range_moved()), this, SLOT(range_moved()));

}

FormPeaks::~FormPeaks()
{
  delete ui;
}

void FormPeaks::setFit(Gamma::Fitter* fit) {
  fit_data_ = fit;
  update_fit(true);
}

void FormPeaks::set_visible_elements(ShowFitElements elems, bool interactive) {
  ui->checkShowMovAvg->setChecked(elems & ShowFitElements::movavg);
  ui->checkShowPrelimPeaks->setChecked(elems & ShowFitElements::prelim);
  ui->checkShowFilteredPeaks->setChecked(elems & ShowFitElements::filtered);
  ui->checkShowGaussians->setChecked(elems & ShowFitElements::gaussians);
  ui->checkShowBaselines->setChecked(elems & ShowFitElements::baselines);
  ui->checkShowPseudoVoigt->setChecked(elems & ShowFitElements::voigt);

  ui->widgetElements->setVisible(interactive);
  ui->line->setVisible(interactive);

}


void FormPeaks::loadSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  ui->spinMinPeakWidth->setValue(settings_.value("min_peak_width", 5).toInt());
  ui->doubleOverlapWidth->setValue(settings_.value("overlap_width", 2.70).toDouble());
  ui->spinMovAvg->setValue(settings_.value("moving_avg_window", 15).toInt());
  ui->checkShowMovAvg->setChecked(settings_.value("show_moving_avg", false).toBool());
  ui->checkShowFilteredPeaks->setChecked(settings_.value("show_filtered_peaks", false).toBool());
  ui->checkShowPrelimPeaks->setChecked(settings_.value("show_prelim_peaks", false).toBool());
  ui->checkShowGaussians->setChecked(settings_.value("show_gaussians", false).toBool());
  ui->checkShowBaselines->setChecked(settings_.value("show_baselines", false).toBool());
  ui->checkShowPseudoVoigt->setChecked(settings_.value("show_pseudovoigt", false).toBool());
  ui->plot1D->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->plot1D->set_plot_style(settings_.value("plot_style", "Step").toString());
  ui->plot1D->set_marker_labels(settings_.value("marker_labels", true).toBool());
  settings_.endGroup();
}

void FormPeaks::saveSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  settings_.setValue("min_peak_width", ui->spinMinPeakWidth->value());
  settings_.setValue("moving_avg_window", ui->spinMovAvg->value());
  settings_.setValue("overlap_width", ui->doubleOverlapWidth->value());
  settings_.setValue("show_moving_avg", ui->checkShowMovAvg->isChecked());
  settings_.setValue("show_prelim_peaks", ui->checkShowPrelimPeaks->isChecked());
  settings_.setValue("show_filtered_peaks", ui->checkShowFilteredPeaks->isChecked());
  settings_.setValue("show_gaussians", ui->checkShowGaussians->isChecked());
  settings_.setValue("show_baselines", ui->checkShowBaselines->isChecked());
  settings_.setValue("show_pseudovoigt", ui->checkShowPseudoVoigt->isChecked());
  settings_.setValue("scale_type", ui->plot1D->scale_type());
  settings_.setValue("plot_style", ui->plot1D->plot_style());
  settings_.setValue("marker_labels", ui->plot1D->marker_labels());
  settings_.endGroup();
}

void FormPeaks::clear() {
  maxima.clear();
  minima.clear();
  toggle_push();
  ui->plot1D->setTitle("");
  ui->plot1D->clearGraphs();
  ui->plot1D->clearExtras();
  ui->plot1D->reset_scales();
  replot_all();
}


void FormPeaks::new_spectrum() {
  if (fit_data_->peaks_.empty()) {
    fit_data_->set_mov_avg(ui->spinMovAvg->value());
    fit_data_->find_peaks(ui->spinMinPeakWidth->value());
    on_pushFindPeaks_clicked();
  }

  QString title = "Spectrum=" + QString::fromStdString(fit_data_->metadata_.name) + "  resolution=" + QString::number(fit_data_->metadata_.bits) + "bits  Detector=" + QString::fromStdString(fit_data_->detector_.name_);
  ui->plot1D->setFloatingText(title);
  ui->plot1D->setTitle(title);

  ui->plot1D->reset_scales();
  update_spectrum();
  toggle_push();
}

void FormPeaks::update_spectrum() {
  if (fit_data_ == nullptr)
    return;

  if (fit_data_->metadata_.resolution == 0) {
    clear();
    replot_all();
    return;
  }

  minima.clear();
  maxima.clear();
  QColor plotcolor;
  plotcolor.setHsv(QColor::fromRgba(fit_data_->metadata_.appearance).hsvHue(), 48, 160);
  main_graph_.default_pen = QPen(plotcolor, 1);

  for (int i=0; i < fit_data_->x_.size(); ++i) {
    double yy = fit_data_->y_[i];
    double xx = fit_data_->x_[i];
    if (!minima.count(xx) || (minima[xx] > yy))
      minima[xx] = yy;
    if (!maxima.count(xx) || (maxima[xx] < yy))
      maxima[xx] = yy;
  }

  replot_all();
}


void FormPeaks::replot_all() {
  if (fit_data_ == nullptr)
    return;

  ui->plot1D->blockSignals(true);
  this->blockSignals(true);
  
  ui->plot1D->clearGraphs();
  ui->plot1D->clearExtras();

  ui->plot1D->addGraph(QVector<double>::fromStdVector(fit_data_->x_), QVector<double>::fromStdVector(fit_data_->y_), main_graph_, true);

  if (ui->checkShowMovAvg->isChecked())
    plot_derivs();

  QVector<double> xx, yy;

  if (ui->checkShowPrelimPeaks->isChecked()) {
    xx.clear(); yy.clear();
    for (auto &q : fit_data_->prelim) {
      xx.push_back(q);
      yy.push_back(fit_data_->y_[q]);
    }
    if (yy.size())
      ui->plot1D->addPoints(xx, yy, prelim_peak_, QCPScatterStyle::ssDiamond);
  }

  if (ui->checkShowFilteredPeaks->isChecked()) {
    xx.clear(); yy.clear();
    for (auto &q : fit_data_->filtered) {
      xx.push_back(q);
      yy.push_back(fit_data_->y_[q]);
    }
    if (yy.size())
      ui->plot1D->addPoints(xx, yy, filtered_peak_, QCPScatterStyle::ssDiamond);
  }

  for (auto &q : fit_data_->multiplets_) {
    if (ui->checkShowGaussians->isChecked())
      ui->plot1D->addGraph(QVector<double>::fromStdVector(q.x_), QVector<double>::fromStdVector(q.y_fullfit_), multiplet_);
  }
  
  for (auto &q : fit_data_->peaks_) {
    if (ui->checkShowPseudoVoigt->isChecked())
      ui->plot1D->addGraph(QVector<double>::fromStdVector(q.second.x_),
                           QVector<double>::fromStdVector(q.second.y_fullfit_pseudovoigt_),
                           pseudo_voigt_);
    if (ui->checkShowGaussians->isChecked()) {
      AppearanceProfile prof = gaussian_;
      if (q.second.flagged)
        prof = flagged_;
      ui->plot1D->addGraph(QVector<double>::fromStdVector(q.second.x_),
                           QVector<double>::fromStdVector(q.second.y_fullfit_gaussian_),
                           prof);
    }
    if (ui->checkShowBaselines->isChecked())
      ui->plot1D->addGraph(QVector<double>::fromStdVector(q.second.x_),
                           QVector<double>::fromStdVector(q.second.y_baseline_),
                           baseline_);
  }

  ui->plot1D->setLabels("channel", "counts");

  ui->plot1D->setYBounds(minima, maxima); //no baselines or avgs
  replot_markers();
}

void FormPeaks::addMovingMarker(double x) {
  if (fit_data_ == nullptr)
    return;
  
  range_.visible = true;

  range_.center.channel = x;
  range_.center.chan_valid = true;
  range_.center.energy = fit_data_->nrg_cali_.transform(x);
  range_.center.energy_valid = (range_.center.channel != range_.center.energy);

  uint16_t ch = static_cast<uint16_t>(x);

  uint16_t ch_l = fit_data_->find_left(ch, ui->spinMinPeakWidth->value());
  range_.l.channel = ch_l;
  range_.l.chan_valid = true;
  range_.l.energy = fit_data_->nrg_cali_.transform(ch_l);
  range_.l.energy_valid = (range_.l.channel != range_.l.energy);

  uint16_t ch_r = fit_data_->find_right(ch, ui->spinMinPeakWidth->value());
  range_.r.channel = ch_r;
  range_.r.chan_valid = true;
  range_.r.energy = fit_data_->nrg_cali_.transform(ch_r);
  range_.r.energy_valid = (range_.r.channel != range_.r.energy);

  ui->pushAdd->setEnabled(true);
  PL_INFO << "<Peak finder> Marker at chan=" << range_.center.channel << " (" << range_.center.energy << " " << fit_data_->nrg_cali_.units_ << ")"
          << ", edges=[" << range_.l.channel << ", " << range_.r.channel << "]";

  replot_markers();
}

void FormPeaks::removeMovingMarker(double x) {
  range_.visible = false;
  toggle_push();
  replot_markers();
}


void FormPeaks::replot_markers() {

  if (fit_data_ == nullptr)
    return;

  ui->plot1D->blockSignals(true);
  this->blockSignals(true);

  ui->plot1D->clearExtras();

  if (range_.visible)
    ui->plot1D->set_range(range_);

  std::list<Marker> markers;

  for (auto &q : fit_data_->peaks_) {
    Marker m = list;
    m.channel = q.second.center;
    m.energy = q.second.energy;
    m.chan_valid = true;
    m.energy_valid = (m.channel != m.energy);

    //if (selected_peaks_.count(m.channel) == 1)
    //  m.selected = true;
    m.selected = q.second.selected;
    markers.push_back(m);
  }

  ui->plot1D->set_markers(markers);
  ui->plot1D->replot_markers();
  ui->plot1D->redraw();

  ui->plot1D->blockSignals(false);
  this->blockSignals(false);

}

void FormPeaks::on_pushAdd_clicked()
{
  if (range_.l.channel >= range_.r.channel)
    return;

  fit_data_->add_peak(range_.l.channel, range_.r.channel);
  ui->pushAdd->setEnabled(false);
  removeMovingMarker(0);
  toggle_push();
  replot_all();
  emit peaks_changed(true);
}

void FormPeaks::user_selected_peaks() {
  PL_DBG << "User selectd peaks";

  if (fit_data_ == nullptr)
    return;

  std::set<double> chosen_peaks = ui->plot1D->get_selected_markers();
  for (auto &q : fit_data_->peaks_)
    q.second.selected = (chosen_peaks.count(q.second.center) > 0);
  toggle_push();
  replot_markers();
  if (isVisible())
    emit peaks_changed(false);
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

void FormPeaks::on_pushFindPeaks_clicked()
{
  if (fit_data_ == nullptr)
    return;

  this->setCursor(Qt::WaitCursor);

  fit_data_->find_peaks(ui->spinMinPeakWidth->value());

  toggle_push();
  replot_all();

  emit peaks_changed(true);
  this->setCursor(Qt::ArrowCursor);
}

void FormPeaks::plot_derivs()
{
  if (fit_data_ == nullptr)
    return;

  QVector<double> temp_y, temp_x;
  int was = 0, is = 0;

  for (int i = 0; i < fit_data_->deriv1.size(); ++i) {
    if (fit_data_->deriv1[i] > 0)
      is = 1;
    else if (fit_data_->deriv1[i] < 0)
      is = -1;
    else
      is = 0;

    if ((was != is) && (temp_x.size()))
    {
      if (temp_x.size() > ui->spinMinPeakWidth->value()) {
        if (was == 1)
          ui->plot1D->addGraph(temp_x, temp_y, rise_);
        else if (was == -1)
          ui->plot1D->addGraph(temp_x, temp_y, fall_);
        else
          ui->plot1D->addGraph(temp_x, temp_y, even_);
      }
      temp_x.clear(); temp_x.push_back(i-1);
      temp_y.clear(); temp_y.push_back(fit_data_->y_avg_[i-1]);
    }

    was = is;
    temp_y.push_back(fit_data_->y_avg_[i]);
    temp_x.push_back(i);
  }

  if (temp_x.size())
  {
    if (was == 1)
      ui->plot1D->addGraph(temp_x, temp_y, rise_);
    else if (was == -1)
      ui->plot1D->addGraph(temp_x, temp_y, fall_);
    else
      ui->plot1D->addGraph(temp_x, temp_y, even_);
  }
}

void FormPeaks::on_checkShowMovAvg_clicked()
{
  replot_all();
}

void FormPeaks::on_checkShowPrelimPeaks_clicked()
{
  replot_all();
}

void FormPeaks::on_checkShowGaussians_clicked()
{
  replot_all();
}

void FormPeaks::on_checkShowBaselines_clicked()
{
  replot_all();
}

void FormPeaks::on_checkShowFilteredPeaks_clicked()
{
  replot_all();
}

void FormPeaks::on_checkShowPseudoVoigt_clicked()
{
  replot_all();
}

void FormPeaks::on_spinMovAvg_editingFinished()
{
  if (fit_data_ == nullptr)
    return;

  fit_data_->set_mov_avg(ui->spinMovAvg->value());
  fit_data_->find_prelim();
  fit_data_->filter_prelim(ui->spinMinPeakWidth->value());
  replot_all();
}

void FormPeaks::on_spinMinPeakWidth_editingFinished()
{
  if (fit_data_ == nullptr)
    return;

  fit_data_->filter_prelim(ui->spinMinPeakWidth->value());
  replot_all();
}

void FormPeaks::range_moved() {
  range_ = ui->plot1D->get_range();
  toggle_push();
  replot_markers();
}

void FormPeaks::update_fit(bool content_changed) {
  if (content_changed)
    replot_all();
  else
    replot_markers();
  toggle_push();
}

void FormPeaks::on_doubleOverlapWidth_editingFinished()
{
  fit_data_->overlap_ = ui->doubleOverlapWidth->value();
}
