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
 *      FormCoincPeaks -
 *
 ******************************************************************************/

#include "form_coinc_peaks.h"
#include "widget_detectors.h"
#include "ui_form_coinc_peaks.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormCoincPeaks::FormCoincPeaks(QWidget *parent) :
  QWidget(parent),
  spectrum_(nullptr),
  fit_data_(nullptr),
  ui(new Ui::FormCoincPeaks)
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
  gaussian_.default_pen = QPen(Qt::darkBlue, 0);

  multiplet_.default_pen = QPen(Qt::red, 2);

  sel_gaussian_.default_pen = QPen(Qt::green, 0);

  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(6);
  ui->tablePeaks->setHorizontalHeaderLabels({"chan", "edges", "energy", "fwhm", "area", "cps"});
  ui->tablePeaks->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePeaks->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tablePeaks->horizontalHeader()->setStretchLastSection(true);
  ui->tablePeaks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tablePeaks->show();
  connect(ui->tablePeaks, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));

  connect(ui->plot1D, SIGNAL(markers_selected()), this, SLOT(user_selected_peaks()));
  connect(ui->plot1D, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->plot1D, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));
  connect(ui->plot1D, SIGNAL(range_moved()), this, SLOT(range_moved()));

}

FormCoincPeaks::~FormCoincPeaks()
{
  delete ui;
}

void FormCoincPeaks::setFit(Gamma::Fitter* fit) {
  fit_data_ = fit;
  update_fit(true);
}

void FormCoincPeaks::loadSettings(QSettings &settings_) {
  settings_.beginGroup("CoincPeaks");
  ui->spinMinPeakWidth->setValue(settings_.value("min_peak_width", 5).toInt());
  ui->doubleOverlapWidth->setValue(settings_.value("overlap_width", 2.70).toDouble());
  ui->spinMovAvg->setValue(settings_.value("moving_avg_window", 15).toInt());
  ui->plot1D->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->plot1D->set_plot_style(settings_.value("plot_style", "Step").toString());
  ui->plot1D->set_marker_labels(settings_.value("marker_labels", true).toBool());
  settings_.endGroup();
}

void FormCoincPeaks::saveSettings(QSettings &settings_) {
  settings_.beginGroup("CoincPeaks");
  settings_.setValue("min_peak_width", ui->spinMinPeakWidth->value());
  settings_.setValue("moving_avg_window", ui->spinMovAvg->value());
  settings_.setValue("overlap_width", ui->doubleOverlapWidth->value());
  settings_.setValue("scale_type", ui->plot1D->scale_type());
  settings_.setValue("plot_style", ui->plot1D->plot_style());
  settings_.setValue("marker_labels", ui->plot1D->marker_labels());
  settings_.endGroup();
}

void FormCoincPeaks::clear() {
  nrg_calibration_ = Gamma::Calibration();
  fwhm_calibration_ = Gamma::Calibration();
  detector_ = Gamma::Detector();
  fit_data_->clear();
  maxima.clear();
  minima.clear();
  toggle_push();
  ui->plot1D->setTitle("");
  ui->plot1D->clearGraphs();
  ui->plot1D->clearExtras();
  ui->plot1D->reset_scales();
  replot_all();
  sel_L = 0;
  sel_R = 0;
}


void FormCoincPeaks::setSpectrum(Pixie::Spectrum::Spectrum *newspectrum, uint16_t L, uint16_t R) {
  clear();
  spectrum_ = newspectrum;
  sel_L = L;
  sel_R = R;

  if (spectrum_ && spectrum_->resolution()) {
    int bits = spectrum_->bits();
    for (std::size_t i=0; i < spectrum_->add_pattern().size(); i++) {
      if (spectrum_->add_pattern()[i] == 1) {
        detector_ = spectrum_->get_detectors()[i];
        QString title = "Spectrum=" + QString::fromStdString(spectrum_->name()) + "  resolution=" + QString::number(bits) + "bits  Detector=" + QString::fromStdString(detector_.name_);
        ui->plot1D->setFloatingText(title);
        ui->plot1D->setTitle(title);
      }
    }
  }

  ui->plot1D->reset_scales();

  update_spectrum();

  if (spectrum_ && (spectrum_->total_count() > 0)) {
    on_pushFindPeaks_clicked();
    replot_all();
  }

}

void FormCoincPeaks::setData(Gamma::Calibration nrg_calib, Gamma::Calibration fwhm_calib) {
  fwhm_calibration_ = fwhm_calib;
  nrg_calibration_ = nrg_calib;
  replot_markers();
}


void FormCoincPeaks::update_spectrum() {
  if (fit_data_ == nullptr)
    return;

  if ((!spectrum_) || (!spectrum_->resolution())) {
    clear();
    replot_all();
    return;
  }

  minima.clear();
  maxima.clear();

  std::vector<double> x_chan(spectrum_->resolution());
  std::vector<double> y(spectrum_->resolution());

  double live_seconds = spectrum_->live_time().total_seconds();

  std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_dump =
      std::move(spectrum_->get_spectrum({{0, y.size()}}));

  int i = 0;
  for (auto it : *spectrum_dump) {
    double yy = it.second;
    double xx = static_cast<double>(i);
    x_chan[i] = xx;
    double x_nrg = nrg_calibration_.transform(xx);
    y[i] = yy;
    if (!minima.count(x_nrg) || (minima[xx] > yy))
      minima[x_nrg] = yy;
    if (!maxima.count(x_nrg) || (maxima[xx] < yy))
      maxima[x_nrg] = yy;
    ++i;
  }

  fit_data_->setXY(x_chan, y, ui->spinMovAvg->value());
  fit_data_->find_prelim();
  fit_data_->filter_prelim(ui->spinMinPeakWidth->value());
  fit_data_->live_seconds_ = live_seconds;

  replot_all();
}


void FormCoincPeaks::replot_all() {
  if (fit_data_ == nullptr)
    return;
  
  ui->plot1D->clearGraphs();
  ui->plot1D->addGraph(QVector<double>::fromStdVector(nrg_calibration_.transform(fit_data_->x_)), QVector<double>::fromStdVector(fit_data_->y_), main_graph_);

  QVector<double> xx, yy;

  for (auto &q : fit_data_->multiplets_)
      ui->plot1D->addGraph(QVector<double>::fromStdVector(nrg_calibration_.transform(q.x_)), QVector<double>::fromStdVector(q.y_fullfit_), multiplet_);
  
  AppearanceProfile app;
  for (auto &q : fit_data_->peaks_) {
    if ((q.first > sel_L) && (q.first < sel_R))
      app = sel_gaussian_;
    else
      app = gaussian_;

    ui->plot1D->addGraph(QVector<double>::fromStdVector(nrg_calibration_.transform(q.second.x_)),
                         QVector<double>::fromStdVector(q.second.y_fullfit_gaussian_),
                         app);
    ui->plot1D->addGraph(QVector<double>::fromStdVector(nrg_calibration_.transform(q.second.x_)),
                         QVector<double>::fromStdVector(q.second.y_baseline_),
                         app);
  }

  ui->plot1D->use_calibrated(nrg_calibration_.units_ != "channels");
  ui->plot1D->setLabels(QString::fromStdString(nrg_calibration_.units_), "count");
  //ui->plot1D->setLabels("channel", "counts");

  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(fit_data_->peaks_.size());
  bool gray = false;
  QColor background_col;
  int i=0;
  for (auto &q : fit_data_->peaks_) {
    if ((q.first > sel_L) && (q.first < sel_R))
      background_col = Qt::green;
    else if (gray)
      background_col = Qt::lightGray;
    else
      background_col = Qt::white;
    add_peak_to_table(q.second, i, background_col);
    ++i;
    if (!q.second.intersects_R)
      gray = !gray;
  }

  ui->plot1D->setYBounds(minima, maxima); //no baselines or avgs
  replot_markers();
}

void FormCoincPeaks::addMovingMarker(double x) {
  if (fit_data_ == nullptr)
    return;
  
  range_.visible = true;

  range_.center.channel = x;
  range_.center.chan_valid = true;
  range_.center.energy = nrg_calibration_.transform(x);
  range_.center.energy_valid = (range_.center.channel != range_.center.energy);

  uint16_t ch = static_cast<uint16_t>(x);

  uint16_t ch_l = fit_data_->find_left(ch, ui->spinMinPeakWidth->value());
  range_.l.channel = ch_l;
  range_.l.chan_valid = true;
  range_.l.energy = nrg_calibration_.transform(ch_l);
  range_.l.energy_valid = (range_.l.channel != range_.l.energy);

  uint16_t ch_r = fit_data_->find_right(ch, ui->spinMinPeakWidth->value());
  range_.r.channel = ch_r;
  range_.r.chan_valid = true;
  range_.r.energy = nrg_calibration_.transform(ch_r);
  range_.r.energy_valid = (range_.r.channel != range_.r.energy);

  ui->pushAdd->setEnabled(true);
  PL_INFO << "<Peak finder> Marker at chan=" << range_.center.channel << " (" << range_.center.energy << " " << nrg_calibration_.units_ << ")"
          << ", edges=[" << range_.l.channel << ", " << range_.r.channel << "]";

  replot_markers();
}

void FormCoincPeaks::removeMovingMarker(double x) {
  range_.visible = false;
  toggle_push();
  replot_markers();
}


void FormCoincPeaks::replot_markers() {
  if (fit_data_ == nullptr)
    return;

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
}

void FormCoincPeaks::on_pushAdd_clicked()
{
  if (range_.l.channel >= range_.r.channel)
    return;

  fit_data_->nrg_cali_ = nrg_calibration_;
  fit_data_->fwhm_cali_ = fwhm_calibration_;
  fit_data_->overlap_ = ui->doubleOverlapWidth->value();
  fit_data_->add_peak(range_.l.channel, range_.r.channel);
  ui->pushAdd->setEnabled(false);
  removeMovingMarker(0);
  toggle_push();
  replot_all();
  emit peaks_changed(true);
}

void FormCoincPeaks::user_selected_peaks() {
  if (fit_data_ == nullptr)
    return;

  std::set<double> chosen_peaks = ui->plot1D->get_selected_markers();
  for (auto &q : fit_data_->peaks_)
    q.second.selected = (chosen_peaks.count(q.second.center) > 0);

  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);
  ui->tablePeaks->clearSelection();
  int i = 0;
  for (auto &q : fit_data_->peaks_) {
    if (q.second.selected) {
      ui->tablePeaks->selectRow(i);
    }
    ++i;
  }
  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);

  toggle_push();
  replot_markers();
  if (isVisible())
    emit peaks_changed(false);
}

void FormCoincPeaks::toggle_push() {
  if (fit_data_ == nullptr)
    return;

  bool sel = false;
  for (auto &q : fit_data_->peaks_)
    if (q.second.selected)
      sel = true;
  ui->pushAdd->setEnabled(range_.visible);
  ui->pushMarkerRemove->setEnabled(sel);
}

void FormCoincPeaks::on_pushMarkerRemove_clicked()
{
  if (fit_data_ == nullptr)
    return;

  std::set<double> chosen_peaks;
  for (auto &q : fit_data_->peaks_)
    if (q.second.selected)
      chosen_peaks.insert(q.second.center);

  fit_data_->nrg_cali_ = nrg_calibration_;
  fit_data_->fwhm_cali_ = fwhm_calibration_;
  fit_data_->overlap_ = ui->doubleOverlapWidth->value();
  fit_data_->remove_peaks(chosen_peaks);

  toggle_push();
  replot_all();
  emit peaks_changed(true);
}

void FormCoincPeaks::on_pushFindPeaks_clicked()
{
  if (fit_data_ == nullptr)
    return;

  this->setCursor(Qt::WaitCursor);

  fit_data_->nrg_cali_ = nrg_calibration_;
  fit_data_->fwhm_cali_ = fwhm_calibration_;
  fit_data_->overlap_ = ui->doubleOverlapWidth->value();

  fit_data_->find_peaks(ui->spinMinPeakWidth->value());

  toggle_push();
  replot_all();

  emit peaks_changed(true);
  this->setCursor(Qt::ArrowCursor);
}

void FormCoincPeaks::on_checkShowMovAvg_clicked()
{
  replot_all();
}

void FormCoincPeaks::on_checkShowPrelimPeaks_clicked()
{
  replot_all();
}

void FormCoincPeaks::on_checkShowGaussians_clicked()
{
  replot_all();
}

void FormCoincPeaks::on_checkShowBaselines_clicked()
{
  replot_all();
}

void FormCoincPeaks::on_checkShowFilteredPeaks_clicked()
{
  replot_all();
}

void FormCoincPeaks::on_checkShowPseudoVoigt_clicked()
{
  replot_all();
}

void FormCoincPeaks::on_spinMovAvg_editingFinished()
{
  if (fit_data_ == nullptr)
    return;

  fit_data_->set_mov_avg(ui->spinMovAvg->value());
  fit_data_->find_prelim();
  fit_data_->filter_prelim(ui->spinMinPeakWidth->value());
  replot_all();
}

void FormCoincPeaks::on_spinMinPeakWidth_editingFinished()
{
  if (fit_data_ == nullptr)
    return;

  fit_data_->filter_prelim(ui->spinMinPeakWidth->value());
  replot_all();
}

void FormCoincPeaks::range_moved() {
  range_ = ui->plot1D->get_range();
  toggle_push();
  replot_markers();
}

void FormCoincPeaks::update_fit(bool content_changed) {
  if (content_changed)
    replot_all();
  else
    replot_markers();
  toggle_push();
}

void FormCoincPeaks::add_peak_to_table(const Gamma::Peak &p, int row, QColor bckg) {
  QBrush background(bckg);

  QTableWidgetItem *center = new QTableWidgetItem(QString::number(p.center));
  center->setData(Qt::EditRole, QVariant::fromValue(p.center));
  center->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 0, center);

  QTableWidgetItem *edges = new QTableWidgetItem( QString::number(p.x_[0]) + "-" + QString::number(p.x_[p.x_.size() - 1]) );
  edges->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 1, edges);

  QTableWidgetItem *nrg = new QTableWidgetItem(QString::number(p.energy));
  nrg->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 2, nrg);

  QTableWidgetItem *fwhm = new QTableWidgetItem(QString::number(p.fwhm_gaussian));
  fwhm->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 3, fwhm);

  if (p.area_net_ != 0) {
    QTableWidgetItem *area_net = new QTableWidgetItem(QString::number(p.area_net_));
    area_net->setData(Qt::BackgroundRole, background);
    ui->tablePeaks->setItem(row, 4, area_net);

    QTableWidgetItem *cps_net = new QTableWidgetItem(QString::number(p.cts_per_sec_net_));
    cps_net->setData(Qt::BackgroundRole, background);
    ui->tablePeaks->setItem(row, 5, cps_net);
  } else {
    QTableWidgetItem *area_gauss = new QTableWidgetItem(QString::number(p.area_gauss_));
    area_gauss->setData(Qt::BackgroundRole, background);
    ui->tablePeaks->setItem(row, 4, area_gauss);

    QTableWidgetItem *cps_gauss = new QTableWidgetItem(QString::number(p.cts_per_sec_gauss_));
    cps_gauss->setData(Qt::BackgroundRole, background);
    ui->tablePeaks->setItem(row, 5, cps_gauss);
  }
}

void FormCoincPeaks::selection_changed_in_table() {
  for (auto &q : fit_data_->peaks_)
    q.second.selected = false;
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows()) {
    fit_data_->peaks_[ui->tablePeaks->item(i.row(), 0)->data(Qt::EditRole).toDouble()].selected = true;
  }
  toggle_push();
  replot_markers();
}


