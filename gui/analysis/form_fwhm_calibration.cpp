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
#include "fitter.h"
#include "qt_util.h"
#include "sqrt_poly.h"

FormFwhmCalibration::FormFwhmCalibration(XMLableDB<Qpx::Detector>& dets,
                                         Qpx::Fitter& fit, QWidget *parent)
  : QWidget(parent)
  , ui(new Ui::FormFwhmCalibration)
  , fit_data_(fit)
  , detectors_(dets)
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

  ui->PlotCalib->setAxisLabels("energy", "FWHM");

  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(6);
  ui->tablePeaks->setHorizontalHeaderLabels({"energy", "err(energy)", "fwhm", "err(fwhm)", "Quality", "fit rsq(hyp)"});
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

bool FormFwhmCalibration::save_close()
{
  saveSettings();
  return true;
}

void FormFwhmCalibration::loadSettings()
{
  QSettings settings;

  settings.beginGroup("Program");
  data_directory_ = settings.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings.endGroup();


  settings.beginGroup("FWHM_calibration");
  ui->spinTerms->setValue(settings.value("fit_function_terms", 2).toInt());
  ui->doubleMaxFitErr->setValue(settings.value("max_fit_err", 1).toDouble());
  ui->doubleMaxWidthErr->setValue(settings.value("max_width_err", 5).toDouble());

  settings.endGroup();
}

void FormFwhmCalibration::saveSettings()
{
  QSettings settings;
  settings.beginGroup("FWHM_calibration");
  settings.setValue("fit_function_terms", ui->spinTerms->value());
  settings.setValue("max_fit_err", ui->doubleMaxFitErr->value());
  settings.setValue("max_width_err", ui->doubleMaxWidthErr->value());
  settings.endGroup();
}

void FormFwhmCalibration::clear()
{
  new_calibration_ = Qpx::Calibration();
  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(0);
  toggle_push();
  ui->PlotCalib->clearAll();
  ui->PlotCalib->replot();
  ui->pushApplyCalib->setEnabled(false);
  ui->pushFromDB->setEnabled(false);
}

void FormFwhmCalibration::newSpectrum()
{
  new_calibration_ = fit_data_.settings().cali_fwhm_;
  update_data();
}

void FormFwhmCalibration::update_data()
{
  rebuild_table();
  replot_calib();

  if (fit_data_.peaks().empty())
    selected_peaks_.clear();

  select_in_table();
  select_in_plot();
  toggle_push();
}

void FormFwhmCalibration::rebuild_table()
{
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);

  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(fit_data_.peaks().size());
  int i=0;
  for (auto &q : fit_data_.peaks())
  {
    bool significant = (( (1 - q.second.hypermet().chi2()) * 100 < ui->doubleMaxFitErr->value())
                        && (q.second.fwhm().error() < ui->doubleMaxWidthErr->value()));
    add_peak_to_table(q.second, i, significant);
    ++i;
  }

  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}

void FormFwhmCalibration::update_selection(std::set<double> selected_peaks)
{
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (changed) {
    select_in_table();
    select_in_plot();
  }
}

void FormFwhmCalibration::select_in_table()
{
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);
  ui->tablePeaks->clearSelection();

  QItemSelectionModel *selectionModel = ui->tablePeaks->selectionModel();
  QItemSelection itemSelection = selectionModel->selection();

  for (int i=0; i < ui->tablePeaks->rowCount(); ++i)
    if (selected_peaks_.count(ui->tablePeaks->item(i, 0)->data(Qt::UserRole).toDouble())) {
      ui->tablePeaks->selectRow(i);
      itemSelection.merge(selectionModel->selection(), QItemSelectionModel::Select);
    }

  selectionModel->clearSelection();
  selectionModel->select(itemSelection,QItemSelectionModel::Select);

  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}

void FormFwhmCalibration::select_in_plot()
{
  std::set<double> selected_energies;
  for (auto &p : fit_data_.peaks())
    if (selected_peaks_.count(p.first))
      selected_energies.insert(p.second.energy().value());
  ui->PlotCalib->set_selected_pts(selected_energies);
  ui->PlotCalib->replotAll();
}

void FormFwhmCalibration::add_peak_to_table(const Qpx::Peak &p, int row, bool gray)
{
  QBrush background(gray ? Qt::lightGray : Qt::white);

  add_to_table(ui->tablePeaks, row, 0, p.energy().to_string(),
               QVariant::fromValue(p.center().value()), background);
  add_to_table(ui->tablePeaks, row, 1, p.energy().error_percent(), QVariant(), background);
  add_to_table(ui->tablePeaks, row, 2, p.fwhm().to_string(), QVariant(), background);
  add_to_table(ui->tablePeaks, row, 3, p.fwhm().error_percent(), QVariant(), background);
  add_to_table(ui->tablePeaks, row, 4, std::to_string(p.good()), QVariant(), background);
  UncertainDouble rsq(1, (1 - p.hypermet().chi2()), 2);
  add_to_table(ui->tablePeaks, row, 5, rsq.error_percent(), QVariant(), background);
}

void FormFwhmCalibration::replot_calib()
{
  ui->PlotCalib->clearAll();
  QVector<double> xx_relevant, yy_relevant,
      xx_relevant_sigma, yy_relevant_sigma;
  QVector<double> xx, yy;

  double xmin = std::numeric_limits<double>::max();
  double xmax = - std::numeric_limits<double>::max();

  for (auto &q : fit_data_.peaks()) {
    double x = q.second.energy().value();
    double y = q.second.fwhm().value();
    double x_sigma = q.second.energy().uncertainty();
    double y_sigma = q.second.fwhm().uncertainty();

    if (( (1 - q.second.hypermet().chi2()) * 100 < ui->doubleMaxFitErr->value())
        && (q.second.fwhm().error() < ui->doubleMaxWidthErr->value()))
    {
      xx_relevant.push_back(x);
      yy_relevant.push_back(y);
      xx_relevant_sigma.push_back(x_sigma);
      yy_relevant_sigma.push_back(y_sigma);
    }
    else
    {
      xx.push_back(x);
      yy.push_back(y);
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
    ui->PlotCalib->addPoints(style_pts, xx, yy, QVector<double>(), QVector<double>());
    ui->PlotCalib->addPoints(style_relevant, xx_relevant, yy_relevant, xx_relevant_sigma, yy_relevant_sigma);
    if (new_calibration_.valid()) {

      double step = (xmax-xmin) / 50.0;
      xx.clear(); yy.clear();

      for (double x=xmin; x < xmax; x+=step) {
        double y = new_calibration_.transform(x);
        xx.push_back(x);
        yy.push_back(y);
      }

      ui->PlotCalib->setTitle("FWHM = " + QString::fromStdString(new_calibration_.fancy_equation(5, true)));
      ui->PlotCalib->setFit(xx, yy, style_fit);
    }
  }
}

void FormFwhmCalibration::selection_changed_in_plot()
{
  std::set<double> selected_energies = ui->PlotCalib->get_selected_pts();
  selected_peaks_.clear();
  for (auto &p : fit_data_.peaks())
    if (selected_energies.count(p.second.energy().value()))
      selected_peaks_.insert(p.first);

  select_in_table();
  if (isVisible())
    emit selection_changed(selected_peaks_);
  toggle_push();
}

void FormFwhmCalibration::selection_changed_in_table()
{
  selected_peaks_.clear();
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows())
    selected_peaks_.insert(ui->tablePeaks->item(i.row(), 0)->data(Qt::UserRole).toDouble());

  select_in_plot();
  if (isVisible())
    emit selection_changed(selected_peaks_);
  toggle_push();
}

void FormFwhmCalibration::toggle_push()
{
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
  std::vector<double> xx, yy, xx_sigma, yy_sigma;
  for (auto &q : fit_data_.peaks())
  {
    if (( (1 - q.second.hypermet().chi2()) * 100 < ui->doubleMaxFitErr->value())
        && (q.second.fwhm().error() < ui->doubleMaxWidthErr->value()))
    {
//      DBG << "Adding point e=" << q.second.energy().to_string()
//          << " w=" << q.second.fwhm().to_string();
      xx.push_back(q.second.energy().value());
//      if (std::isfinite(q.second.energy().uncertainty()))
//        xx_sigma.push_back(q.second.energy().uncertainty());
//      else
        xx_sigma.push_back(0);
      yy.push_back(pow(q.second.fwhm().value(), 2));
//      if (std::isfinite(q.second.fwhm().uncertainty()))
//        yy_sigma.push_back(2*q.second.fwhm().uncertainty()*q.second.fwhm().value());
//      else
        yy_sigma.push_back(0);
    }
  }

//  SqrtPoly p;
  Polynomial p;
  for (int i=0; i <= ui->spinTerms->value(); ++i)
    p.add_coeff(i, 0, 50, 0);

  p.fit(xx, yy, xx_sigma, yy_sigma);

  if (p.coeff_count())
  {
    new_calibration_.type_ = "FWHM";
    new_calibration_.bits_ = fit_data_.settings().bits_; //irrelevant?
    new_calibration_.coefficients_ = p.coeffs_consecutive();
    new_calibration_.r_squared_ = p.chi2();
    new_calibration_.calib_date_ = boost::posix_time::microsec_clock::universal_time();  //spectrum timestamp instead?
    new_calibration_.units_ = "keV";
    new_calibration_.model_ = Qpx::CalibrationModel::sqrt_poly;
//    new_calibration_.model_ = Qpx::CalibrationModel::polynomial;
  }
  else
    LINFO << "<WFHM calibration> Qpx::Calibration failed";
}

void FormFwhmCalibration::on_pushApplyCalib_clicked()
{
  emit update_detector();
}

void FormFwhmCalibration::on_pushFromDB_clicked()
{
  Qpx::Detector newdet = detectors_.get(fit_data_.detector_);
  new_calibration_ = newdet.fwhm_calibration_;
  replot_calib();
  select_in_plot();
  toggle_push();
  emit new_fit();
}

void FormFwhmCalibration::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(detectors_);
  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
  det_widget->exec();
}

void FormFwhmCalibration::on_doubleMaxFitErr_valueChanged(double)
{
  replot_calib();
  select_in_plot();
}

void FormFwhmCalibration::on_doubleMaxWidthErr_valueChanged(double)
{
  replot_calib();
  select_in_plot();
}
