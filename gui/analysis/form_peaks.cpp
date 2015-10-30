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
  range_.top.themes["light"] = QPen(Qt::darkMagenta, 1);
  range_.top.themes["dark"] = QPen(Qt::magenta, 1);
  range_.base.themes["light"] = QPen(Qt::darkMagenta, 2, Qt::DashLine);
  range_.base.themes["dark"] = QPen(Qt::magenta, 2, Qt::DashLine);


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

  flagged_.default_pen =  QPen(flagged_color, 0);
  pseudo_voigt_.default_pen = QPen(Qt::darkCyan, 0);

  multiplet_.default_pen = QPen(Qt::red, 2);

  baseline_.default_pen = QPen(Qt::darkGray, 0);
  rise_.default_pen = QPen(Qt::green, 0);
  fall_.default_pen = QPen(Qt::red, 0);
  even_.default_pen = QPen(Qt::black, 0);

  sum4edge_.default_pen = QPen(Qt::darkYellow, 3);


  connect(ui->plot1D, SIGNAL(markers_selected()), this, SLOT(user_selected_peaks()));
  connect(ui->plot1D, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->plot1D, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));
  connect(ui->plot1D, SIGNAL(range_moved(double, double)), this, SLOT(range_moved(double, double)));

  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Insert), ui->plot1D);
  connect(shortcut, SIGNAL(activated()), this, SLOT(on_pushAdd_clicked()));


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
  ui->checkShowEdges->setChecked(elems & ShowFitElements::edges);

  ui->widgetElements->setVisible(interactive);
  ui->line->setVisible(interactive);

}


void FormPeaks::loadSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  ui->spinMinPeakWidth->setValue(settings_.value("min_peak_width", 5).toInt());
  ui->spinSum4EdgeW->setValue(settings_.value("sum4_edge_width", 3).toInt());
  ui->doubleOverlapWidth->setValue(settings_.value("overlap_width", 2.70).toDouble());
  ui->spinMovAvg->setValue(settings_.value("moving_avg_window", 15).toInt());
  ui->checkShowMovAvg->setChecked(settings_.value("show_moving_avg", false).toBool());
  ui->checkShowFilteredPeaks->setChecked(settings_.value("show_filtered_peaks", false).toBool());
  ui->checkShowPrelimPeaks->setChecked(settings_.value("show_prelim_peaks", false).toBool());
  ui->checkShowGaussians->setChecked(settings_.value("show_gaussians", false).toBool());
  ui->checkShowBaselines->setChecked(settings_.value("show_baselines", false).toBool());
  ui->checkShowPseudoVoigt->setChecked(settings_.value("show_pseudovoigt", false).toBool());
  ui->checkShowEdges->setChecked(settings_.value("show_edges", false).toBool());
  ui->plot1D->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->plot1D->set_plot_style(settings_.value("plot_style", "Step").toString());
  ui->plot1D->set_marker_labels(settings_.value("marker_labels", true).toBool());
  ui->plot1D->set_grid_style(settings_.value("grid_style", "Grid + subgrid").toString());
  settings_.endGroup();
}

void FormPeaks::saveSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  settings_.setValue("min_peak_width", ui->spinMinPeakWidth->value());
  settings_.setValue("sum4_edge_width", ui->spinSum4EdgeW->value());
  settings_.setValue("moving_avg_window", ui->spinMovAvg->value());
  settings_.setValue("overlap_width", ui->doubleOverlapWidth->value());
  settings_.setValue("show_moving_avg", ui->checkShowMovAvg->isChecked());
  settings_.setValue("show_prelim_peaks", ui->checkShowPrelimPeaks->isChecked());
  settings_.setValue("show_filtered_peaks", ui->checkShowFilteredPeaks->isChecked());
  settings_.setValue("show_gaussians", ui->checkShowGaussians->isChecked());
  settings_.setValue("show_baselines", ui->checkShowBaselines->isChecked());
  settings_.setValue("show_pseudovoigt", ui->checkShowPseudoVoigt->isChecked());
  settings_.setValue("show_edges", ui->checkShowEdges->isChecked());
  settings_.setValue("scale_type", ui->plot1D->scale_type());
  settings_.setValue("plot_style", ui->plot1D->plot_style());
  settings_.setValue("marker_labels", ui->plot1D->marker_labels());
  settings_.setValue("grid_style", ui->plot1D->grid_style());
  settings_.endGroup();
}

void FormPeaks::clear() {
  //PL_DBG << "FormPeaks::clear()";
  maxima.clear();
  minima.clear();
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
  if (fit_data_->peaks_.empty()) {
    fit_data_->set_mov_avg(ui->spinMovAvg->value());
    fit_data_->find_peaks(ui->spinMinPeakWidth->value());
//    PL_DBG << "number of peaks found " << fit_data_->peaks_.size();
    on_pushFindPeaks_clicked();
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

  minima.clear();
  maxima.clear();
  QColor plotcolor;
  plotcolor.setHsv(QColor::fromRgba(fit_data_->metadata_.appearance).hsvHue(), 48, 160);
  main_graph_.default_pen = QPen(plotcolor, 1);

  ui->plot1D->use_calibrated(fit_data_->nrg_cali_.valid());
  ui->plot1D->setLabels(QString::fromStdString(fit_data_->nrg_cali_.units_), "count");

  for (int i=0; i < fit_data_->x_.size(); ++i) {
    double yy = fit_data_->y_[i];
    double xx = fit_data_->nrg_cali_.transform(fit_data_->x_[i], fit_data_->metadata_.bits);
    if (!minima.count(xx) || (minima[xx] > yy))
      minima[xx] = yy;
    if (!maxima.count(xx) || (maxima[xx] < yy))
      maxima[xx] = yy;
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

  ui->plot1D->use_calibrated(fit_data_->nrg_cali_.valid());

  ui->plot1D->addGraph(QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(fit_data_->x_)),
                       QVector<double>::fromStdVector(fit_data_->y_),
                       main_graph_, true,
                       fit_data_->metadata_.bits);

  if (ui->checkShowMovAvg->isChecked())
    plot_derivs();

  QVector<double> xx, yy;

  if (ui->checkShowPrelimPeaks->isChecked()) {
    xx.clear(); yy.clear();
    for (auto &q : fit_data_->prelim) {
      xx.push_back(fit_data_->nrg_cali_.transform(fit_data_->x_[q], fit_data_->metadata_.bits));
      yy.push_back(fit_data_->y_[q]);
    }
    if (yy.size())
      ui->plot1D->addPoints(xx, yy, prelim_peak_, QCPScatterStyle::ssDiamond);
  }

  if (ui->checkShowFilteredPeaks->isChecked()) {
    xx.clear(); yy.clear();
    for (auto &q : fit_data_->filtered) {
      xx.push_back(fit_data_->nrg_cali_.transform(fit_data_->x_[q], fit_data_->metadata_.bits));
      yy.push_back(fit_data_->y_[q]);
    }
    if (yy.size())
      ui->plot1D->addPoints(xx, yy, filtered_peak_, QCPScatterStyle::ssDiamond);
  }

  for (auto &q : fit_data_->multiplets_) {
    if (ui->checkShowGaussians->isChecked()) {
      std::vector<double> y_fit = q.y_fullfit_;
      for (auto &p : y_fit)
        if (p < 1)
          p = std::floor(p * 10 + 0.5)/10;
      ui->plot1D->addGraph(QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(q.x_)), QVector<double>::fromStdVector(y_fit), multiplet_);
    }
  }
  
  for (auto &q : fit_data_->peaks_) {
    if (ui->checkShowPseudoVoigt->isChecked()) {
      std::vector<double> y_fit = q.second.y_fullfit_pseudovoigt_;
      for (auto &p : y_fit)
        if (p < 1)
          p = std::floor(p * 10 + 0.5)/10;
      ui->plot1D->addGraph(QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(q.second.x_)),
                           QVector<double>::fromStdVector(y_fit),
                           pseudo_voigt_);
    }

    if (ui->checkShowGaussians->isChecked()) {
      std::vector<double> y_fit = q.second.y_fullfit_gaussian_;
      for (auto &p : y_fit)
        if (p < 1)
          p = std::floor(p * 10 + 0.5)/10;

      AppearanceProfile prof = gaussian_;
      if (q.second.flagged)
        prof = flagged_;
      ui->plot1D->addGraph(QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(q.second.x_)),
                           QVector<double>::fromStdVector(y_fit),
                           prof);
    }
    if (ui->checkShowBaselines->isChecked()) {
      std::vector<double> y_fit = q.second.y_baseline_;
      for (auto &p : y_fit)
        if (p < 1)
          p = std::floor(p * 10 + 0.5)/10;
      ui->plot1D->addGraph(QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(q.second.x_)),
                           QVector<double>::fromStdVector(y_fit),
                           baseline_);
    }
    if (ui->checkShowEdges->isChecked() && (!q.second.subpeak)) {
//      PL_DBG << "edges for " << q.second.center << " L:" << q.second.sum4_.LBstart << "-" << q.second.sum4_.LBend
//                                                << " R:" << q.second.sum4_.RBstart << "-" << q.second.sum4_.RBend;


      std::vector<double> x_edge, y_edge;
      for (int i = q.second.sum4_.LBstart; i <= q.second.sum4_.LBend; ++i) {
        x_edge.push_back(q.second.sum4_.x_[i]);
        y_edge.push_back(q.second.sum4_.Lsum / q.second.sum4_.Lw);
      }
      ui->plot1D->addGraph(QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(x_edge)),
                           QVector<double>::fromStdVector(y_edge),
                           sum4edge_);

//      PL_DBG << "Plot left edge " << x_edge.size();

      x_edge.clear();
      y_edge.clear();

      for (int i = q.second.sum4_.RBstart; i <= q.second.sum4_.RBend; ++i) {
        x_edge.push_back(q.second.sum4_.x_[i]);
        y_edge.push_back(q.second.sum4_.Rsum / q.second.sum4_.Rw);
      }
      ui->plot1D->addGraph(QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(x_edge)),
                           QVector<double>::fromStdVector(y_edge),
                           sum4edge_);

//      PL_DBG << "Plot right edge " << x_edge.size();
    }


  }

  //ui->plot1D->setLabels("channel", "counts");

  ui->plot1D->setYBounds(minima, maxima); //no baselines or avgs
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
//  if (fit_data_->nrg_cali_.valid())
  //fit_data_->nrg_cali_.inverse_transform(x, fit_data_->metadata_.bits);


  uint16_t ch = static_cast<uint16_t>(x);

  range_.center.set_bin(x, fit_data_->metadata_.bits, fit_data_->nrg_cali_);

  uint16_t ch_l = fit_data_->find_left(ch, ui->spinMinPeakWidth->value());
  range_.l.set_bin(ch_l, fit_data_->metadata_.bits, fit_data_->nrg_cali_);

  uint16_t ch_r = fit_data_->find_right(ch, ui->spinMinPeakWidth->value());
  range_.r.set_bin(ch_r, fit_data_->metadata_.bits, fit_data_->nrg_cali_);


  ui->pushAdd->setEnabled(true);
//  PL_INFO << "<Peak finder> Marker at chan=" << x << " (" << range_.center.energy() << " " << fit_data_->nrg_cali_.units_ << ")"
//          << ", edges=[" << ch_l << ", " << ch_r << "] "
//          << ", en_edges=[" << range_.l.energy() << ", " << range_.r.energy() << "]";


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
  //PL_DBG << "<Peak finder> replot markers";

  if (fit_data_ == nullptr)
    return;

  ui->plot1D->use_calibrated(fit_data_->nrg_cali_.valid());

  ui->plot1D->blockSignals(true);
  this->blockSignals(true);

  ui->plot1D->clearExtras();

  if (range_.visible) {
    //PL_DBG << "setting visible range";
    ui->plot1D->set_range(range_);
  }

  std::list<Marker> markers;

  for (auto &q : fit_data_->peaks_) {
    Marker m = list;
    m.pos.set_bin(q.second.center, fit_data_->metadata_.bits, fit_data_->nrg_cali_);
    //PL_DBG << "FormPlot1D  marker at c=" << m.pos.bin(fit_data_->metadata_.bits) <<  " e=" <<  m.pos.energy();

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
  //PL_DBG << "FormPeaks::user_selected_peaks()";

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

void FormPeaks::on_pushFindPeaks_clicked()
{
  if (fit_data_ == nullptr)
    return;

  this->setCursor(Qt::WaitCursor);

  fit_data_->set_mov_avg(ui->spinMovAvg->value());
  fit_data_->overlap_ = ui->doubleOverlapWidth->value();
  fit_data_->sum4edge_samples = ui->spinSum4EdgeW->value();
  fit_data_->find_peaks(ui->spinMinPeakWidth->value());
//  PL_DBG << "number of peaks found " << fit_data_->peaks_.size();

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
      temp_x.clear(); temp_x.push_back(fit_data_->nrg_cali_.transform(fit_data_->x_[i-1]));
      temp_y.clear(); temp_y.push_back(fit_data_->y_avg_[i-1]);
    }

    was = is;
    temp_y.push_back(fit_data_->y_avg_[i]);
    temp_x.push_back(fit_data_->nrg_cali_.transform(fit_data_->x_[i]));
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

void FormPeaks::on_checkShowEdges_clicked()
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
  if (content_changed) {
    update_spectrum();
    replot_all();
  } else
    replot_markers();
  ui->plot1D->setLabels(QString::fromStdString(fit_data_->nrg_cali_.units_), "count");
  toggle_push();
}

void FormPeaks::on_doubleOverlapWidth_editingFinished()
{
  fit_data_->overlap_ = ui->doubleOverlapWidth->value();
}
