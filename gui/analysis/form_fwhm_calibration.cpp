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
 *      FormFwhmCalibration - 
 *
 ******************************************************************************/

#include "form_fwhm_calibration.h"
#include "widget_detectors.h"
#include "ui_form_fwhm_calibration.h"
#include "gamma_fitter.h"
#include "qt_util.h"
#include "sqrt_poly.h"

FormFwhmCalibration::FormFwhmCalibration(QSettings &settings, XMLableDB<Gamma::Detector>& dets, Gamma::Fitter& fit, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormFwhmCalibration),
  settings_(settings),
  fit_data_(fit),
  detectors_(dets)
{
  ui->setupUi(this);

  loadSettings();

  QColor point_color;
  QColor selected_color;

  point_color.setHsv(180, 215, 150, 40);
  style_pts.default_pen = QPen(point_color, 9);
  selected_color.setHsv(225, 255, 230, 210);
  style_pts.themes["selected"] = QPen(selected_color, 9);

  point_color.setHsv(180, 215, 150, 140);
  style_relevant.default_pen = QPen(point_color, 9);
  selected_color.setHsv(225, 255, 230, 210);
  style_relevant.themes["selected"] = QPen(selected_color, 9);

  style_fit.default_pen = QPen(Qt::darkCyan, 2);

  ui->PlotCalib->setLabels("energy", "FWHM");

  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(8);
  ui->tablePeaks->setHorizontalHeaderLabels({"chan", "energy", "fwhm(s4)", "w %err(s4)", "CQI (s4)", "fwmw(hyp)", "w %err(hyp)", "fit %err(hyp)"});
  ui->tablePeaks->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePeaks->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tablePeaks->setEditTriggers(QTableView::NoEditTriggers);
  ui->tablePeaks->horizontalHeader()->setStretchLastSection(true);
  ui->tablePeaks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tablePeaks->show();

  connect(ui->tablePeaks, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));
  connect(ui->PlotCalib, SIGNAL(selection_changed()), this, SLOT(selection_changed_in_plot()));

}

FormFwhmCalibration::~FormFwhmCalibration()
{
  delete ui;
}

bool FormFwhmCalibration::save_close() {
  saveSettings();
  return true;
}

void FormFwhmCalibration::loadSettings() {
  settings_.beginGroup("Program");
  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();


  settings_.beginGroup("FWHM_calibration");
  ui->spinTerms->setValue(settings_.value("fit_function_terms", 2).toInt());
  ui->doubleMaxFitErr->setValue(settings_.value("max_fit_err", 1).toDouble());
  ui->doubleMaxWidthErr->setValue(settings_.value("max_width_err", 5).toDouble());

  settings_.endGroup();

}

void FormFwhmCalibration::saveSettings() {

  settings_.beginGroup("FWHM_calibration");
  settings_.setValue("fit_function_terms", ui->spinTerms->value());
  settings_.setValue("max_fit_err", ui->doubleMaxFitErr->value());
  settings_.setValue("max_width_err", ui->doubleMaxWidthErr->value());
  settings_.endGroup();
}

void FormFwhmCalibration::clear() {
  new_calibration_ = Gamma::Calibration();
  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(0);
  toggle_push();
  ui->PlotCalib->clear_data();
  ui->PlotCalib->redraw();
  ui->pushApplyCalib->setEnabled(false);
  ui->pushFromDB->setEnabled(false);
}

void FormFwhmCalibration::newSpectrum() {
  new_calibration_ = fit_data_.settings().cali_fwhm_;
  update_data();
}

void FormFwhmCalibration::update_data() {
  rebuild_table();
  replot_calib();

  if (fit_data_.peaks().empty())
    selected_peaks_.clear();

  select_in_table();
  select_in_plot();
  toggle_push();
}

void FormFwhmCalibration::rebuild_table() {
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);

  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(fit_data_.peaks().size());
  int i=0;
  for (auto &q : fit_data_.peaks()) {
    bool significant = (( (1 - q.second.hypermet_.rsq_) * 100 < ui->doubleMaxFitErr->value())
                        && (q.second.hypermet_.width_.err() < ui->doubleMaxWidthErr->value()));
    add_peak_to_table(q.second, i, significant);
    ++i;
  }

  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}

void FormFwhmCalibration::update_selection(std::set<double> selected_peaks) {
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (changed) {
    select_in_table();
    select_in_plot();
  }
}

void FormFwhmCalibration::select_in_table() {
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);
  ui->tablePeaks->clearSelection();

  QItemSelectionModel *selectionModel = ui->tablePeaks->selectionModel();
  QItemSelection itemSelection = selectionModel->selection();

  for (int i=0; i < ui->tablePeaks->rowCount(); ++i)
    if (selected_peaks_.count(ui->tablePeaks->item(i, 0)->data(Qt::EditRole).toDouble())) {
      ui->tablePeaks->selectRow(i);
      itemSelection.merge(selectionModel->selection(), QItemSelectionModel::Select);
    }

  selectionModel->clearSelection();
  selectionModel->select(itemSelection,QItemSelectionModel::Select);

  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}

void FormFwhmCalibration::select_in_plot() {
  std::set<double> selected_energies;
  for (auto &p : fit_data_.peaks())
    if (selected_peaks_.count(p.first))
      selected_energies.insert(p.second.energy);
  ui->PlotCalib->set_selected_pts(selected_energies);
  ui->PlotCalib->redraw();
}

void FormFwhmCalibration::add_peak_to_table(const Gamma::Peak &p, int row, bool gray) {
  QBrush background(gray ? Qt::lightGray : Qt::white);

  data_to_table(row, 0, p.center, background);
  data_to_table(row, 1, p.energy, background);
  data_to_table(row, 2, p.fwhm_sum4, background);
  data_to_table(row, 3, p.sum4_.peak_area.err(), background);
  data_to_table(row, 4, p.sum4_.currie_quality_indicator, background);
  data_to_table(row, 5, p.fwhm_hyp, background);
  data_to_table(row, 6, p.hypermet_.width_.err(), background);
  data_to_table(row, 7, (1 - p.hypermet_.rsq_) * 100, background);
}

void FormFwhmCalibration::data_to_table(int row, int column, double value, QBrush background) {
  QTableWidgetItem *item = new QTableWidgetItem(QString::number(value));
  item->setData(Qt::EditRole, QVariant::fromValue(value));
  item->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, column, item);
}

void FormFwhmCalibration::replot_calib() {
  ui->PlotCalib->clear_data();
  QVector<double> xx_relevant, yy_relevant, yy_relevant_sigma;
  QVector<double> xx, yy, yy_sigma;

  double xmin = std::numeric_limits<double>::max();
  double xmax = - std::numeric_limits<double>::max();

  for (auto &q : fit_data_.peaks()) {
    double x = q.second.energy;
    double y = q.second.fwhm_hyp;
    double sigma = q.second.hypermet_.width_.uncert * 2 * sqrt(log(2));

    if (( (1 - q.second.hypermet_.rsq_) * 100 < ui->doubleMaxFitErr->value())
        && (q.second.hypermet_.width_.err() < ui->doubleMaxWidthErr->value())) {
      xx_relevant.push_back(x);
      yy_relevant.push_back(y);
      yy_relevant_sigma.push_back(sigma);
    } else {
      xx.push_back(x);
      yy.push_back(y);
      yy_sigma.push_back(0);
    }

    if (x < xmin)
      xmin = x;
    if (x > xmax)
      xmax = x;
  }

  double x_margin = (xmax - xmin) / 10;
  xmax += x_margin;
  xmin -= x_margin;

  if (xx.size() > 0) {
    ui->PlotCalib->addPoints(xx, yy, yy_sigma, style_pts);
    ui->PlotCalib->addPoints(xx_relevant, yy_relevant, yy_relevant_sigma, style_relevant);
    if (new_calibration_.valid()) {

      double step = (xmax-xmin) / 50.0;
      xx.clear(); yy.clear();

      for (double x=xmin; x < xmax; x+=step) {
        double y = new_calibration_.transform(x);
        xx.push_back(x);
        yy.push_back(y);
      }

      ui->PlotCalib->setFloatingText("FWHM = " + QString::fromStdString(new_calibration_.fancy_equation(5, true)));
      ui->PlotCalib->addFit(xx, yy, style_fit);
    }
  }
}

void FormFwhmCalibration::selection_changed_in_plot() {
  std::set<double> selected_energies = ui->PlotCalib->get_selected_pts();
  selected_peaks_.clear();
  for (auto &p : fit_data_.peaks())
    if (selected_energies.count(p.second.energy))
      selected_peaks_.insert(p.first);

  select_in_table();
  if (isVisible())
    emit selection_changed(selected_peaks_);
  toggle_push();
}

void FormFwhmCalibration::selection_changed_in_table() {
  selected_peaks_.clear();
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows())
    selected_peaks_.insert(ui->tablePeaks->item(i.row(), 0)->data(Qt::EditRole).toDouble());

  select_in_plot();
  if (isVisible())
    emit selection_changed(selected_peaks_);
  toggle_push();
}

void FormFwhmCalibration::toggle_push() {
  if (detectors_.has_a(fit_data_.detector_) && (detectors_.get(fit_data_.detector_).fwhm_calibration_.valid()))
    ui->pushFromDB->setEnabled(true);
  else
    ui->pushFromDB->setEnabled(false);

  if (static_cast<int>(fit_data_.peaks().size()) > 1) {
    ui->spinTerms->setEnabled(true);
    ui->pushFit->setEnabled(true);
  } else {
    ui->pushFit->setEnabled(false);
    ui->spinTerms->setEnabled(false);
  }

  ui->pushApplyCalib->setEnabled(fit_data_.settings().cali_fwhm_ != new_calibration_);
}

void FormFwhmCalibration::on_pushFit_clicked()
{  
  fit_calibration();
  replot_calib();
  select_in_plot();
  toggle_push();
  emit new_fit();
}

void FormFwhmCalibration::fit_calibration()
{
  std::vector<double> xx, yy, yy_sigma;
  for (auto &q : fit_data_.peaks()) {
    if (( (1 - q.second.hypermet_.rsq_) * 100 < ui->doubleMaxFitErr->value())
        && (q.second.hypermet_.width_.err() < ui->doubleMaxWidthErr->value())) {
      xx.push_back(q.second.energy);
      yy.push_back(q.second.fwhm_hyp);
      yy_sigma.push_back(q.second.hypermet_.width_.uncert * 2 * sqrt(log(2)));
    }
  }

  SqrtPoly p;
  for (int i=0; i <= ui->spinTerms->value(); ++i)
    p.add_coeff(i, -30, 30, 1);

  p.fit(xx, yy, yy_sigma);

  if (p.coeffs_.size()) {
    new_calibration_.type_ = "FWHM";
    new_calibration_.bits_ = fit_data_.metadata_.bits;
    new_calibration_.coefficients_ = p.coeffs();
    new_calibration_.r_squared_ = p.rsq_;
    new_calibration_.calib_date_ = boost::posix_time::microsec_clock::local_time();  //spectrum timestamp instead?
    new_calibration_.units_ = "keV";
    new_calibration_.model_ = Gamma::CalibrationModel::sqrt_poly;
  }
  else
    PL_INFO << "<WFHM calibration> Gamma::Calibration failed";

}

void FormFwhmCalibration::on_pushApplyCalib_clicked()
{
  emit update_detector();
}

void FormFwhmCalibration::on_pushFromDB_clicked()
{
  Gamma::Detector newdet = detectors_.get(fit_data_.detector_);
  new_calibration_ = newdet.fwhm_calibration_;
  replot_calib();
  select_in_plot();
  toggle_push();
  emit new_fit();
}

void FormFwhmCalibration::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(detectors_, settings_directory_);
  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
  det_widget->exec();
}

void FormFwhmCalibration::on_doubleMaxFitErr_valueChanged(double arg1)
{
  replot_calib();
  select_in_plot();
}

void FormFwhmCalibration::on_doubleMaxWidthErr_valueChanged(double arg1)
{
  replot_calib();
  select_in_plot();
}
