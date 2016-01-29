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

FormFwhmCalibration::FormFwhmCalibration(QSettings &settings, XMLableDB<Gamma::Detector>& dets, Gamma::Fitter& fit, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormFwhmCalibration),
  settings_(settings),
  fit_data_(fit),
  detectors_(dets)
{
  ui->setupUi(this);

  loadSettings();

  style_pts.default_pen = QPen(Qt::darkBlue, 7);
  style_pts.themes["selected"] = QPen(Qt::cyan, 7);
  style_fit.default_pen = QPen(Qt::blue, 0);

  ui->PlotCalib->setLabels("energy", "FWHM");

  ui->tableFWHM->verticalHeader()->hide();
  ui->tableFWHM->setColumnCount(4);
  ui->tableFWHM->setHorizontalHeaderLabels({"chan", "energy", "fwhm(sum4)", "fwmw (hypermet)", "fwhm (pseudo-Voigt)"});
  ui->tableFWHM->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableFWHM->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tableFWHM->setEditTriggers(QTableView::NoEditTriggers);
  ui->tableFWHM->horizontalHeader()->setStretchLastSection(true);
  ui->tableFWHM->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableFWHM->show();

  connect(ui->tableFWHM, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));
  connect(ui->PlotCalib, SIGNAL(selection_changed()), this, SLOT(selection_changed_in_plot()));

  QShortcut* shortcut = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tableFWHM);
  connect(shortcut, SIGNAL(activated()), this, SLOT(on_pushRemovePeak_clicked()));
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
  ui->doubleRsqGoal->setValue(settings_.value("r_squared_goal", 0.85).toDouble());

  settings_.endGroup();

}

void FormFwhmCalibration::saveSettings() {

  settings_.beginGroup("FWHM_calibration");
  settings_.setValue("fit_function_terms", ui->spinTerms->value());
  settings_.setValue("r_squared_goal", ui->doubleRsqGoal->value());
  settings_.endGroup();
}

void FormFwhmCalibration::clear() {
  ui->tableFWHM->clearContents();
  ui->tableFWHM->setRowCount(0);
  toggle_push();
  ui->PlotCalib->setFloatingText("");
  ui->PlotCalib->clearGraphs();
  ui->pushApplyCalib->setEnabled(false);

  ui->pushFromDB->setEnabled(false);
}

void FormFwhmCalibration::newSpectrum() {
  old_fwhm_calibration_ = fit_data_.fwhm_cali_;
  replot_markers();
  toggle_push();
}

void FormFwhmCalibration::update_peaks(bool contents_changed) {
  ui->tableFWHM->blockSignals(true);
  this->blockSignals(true);

  if (contents_changed) {
    ui->tableFWHM->clearContents();
    ui->tableFWHM->setRowCount(fit_data_.peaks_.size());
    int i=0;
    for (auto &q : fit_data_.peaks_) {
      add_peak_to_table(q.second, i);
      ++i;
    }
  }
  
  toggle_push();
  replot_markers();
}

void FormFwhmCalibration::add_peak_to_table(const Gamma::Peak &p, int row) {
  QTableWidgetItem *center = new QTableWidgetItem(QString::number(p.center));
  center->setData(Qt::EditRole, QVariant::fromValue(p.center));
  ui->tableFWHM->setItem(row, 0, center);
  
  ui->tableFWHM->setItem(row, 1, new QTableWidgetItem( QString::number(p.energy) ));
  ui->tableFWHM->setItem(row, 2, new QTableWidgetItem( QString::number(p.fwhm_sum4) ));
  ui->tableFWHM->setItem(row, 3, new QTableWidgetItem( QString::number(p.fwhm_hyp) ));
//  ui->tableFWHM->setItem(row, 4, new QTableWidgetItem( QString::number(p.fwhm_pseudovoigt) ));
}

void FormFwhmCalibration::replot_markers() {
  ui->PlotCalib->setFloatingText("");
  ui->PlotCalib->clearGraphs();
  QVector<double> xx, yy;

  double xmin = std::numeric_limits<double>::max();
  double xmax = - std::numeric_limits<double>::max();

  for (auto &q : fit_data_.peaks_) {
    double x = q.second.energy;
    double y = q.second.fwhm_hyp;

    xx.push_back(x);
    yy.push_back(y);

    if (x < xmin)
      xmin = x;
    if (x > xmax)
      xmax = x;
  }

  double x_margin = (xmax - xmin) / 10;
  xmax += x_margin;
  xmin -= x_margin;

  if (xx.size() > 0) {
    ui->PlotCalib->addPoints(xx, yy, style_pts);
    if ((fit_data_.fwhm_cali_.coefficients_ != Gamma::Calibration().coefficients_) && (fit_data_.fwhm_cali_.valid())) {

      double step = (xmax-xmin) / 50.0;
      xx.clear(); yy.clear();

      for (double x=xmin; x < xmax; x+=step) {
        double y = fit_data_.fwhm_cali_.transform(x);
        xx.push_back(x);
        yy.push_back(y);
      }

      ui->PlotCalib->setFloatingText("FWHM = " + QString::fromStdString(fit_data_.fwhm_cali_.fancy_equation(3, true)));
      ui->PlotCalib->addFit(xx, yy, style_fit);
    }
  }

  std::set<double> chosen_peaks_nrg;

  ui->tableFWHM->blockSignals(true);
  this->blockSignals(true);
  ui->tableFWHM->clearSelection();
  int i = 0;
  for (auto &q : fit_data_.peaks_) {
    if (q.second.selected) {
      ui->tableFWHM->selectRow(i);
      chosen_peaks_nrg.insert(q.second.energy);
    }
    ++i;
  }
  ui->PlotCalib->set_selected_pts(chosen_peaks_nrg);
  ui->tableFWHM->blockSignals(false);
  this->blockSignals(false);
}

void FormFwhmCalibration::selection_changed_in_plot() {
  std::set<double> chosen_peaks_nrg = ui->PlotCalib->get_selected_pts();
  for (auto &q : fit_data_.peaks_)
    q.second.selected = (chosen_peaks_nrg.count(q.second.energy) > 0);
  toggle_push();
  replot_markers();
  if (isVisible())
    emit peaks_changed(false);
}

void FormFwhmCalibration::selection_changed_in_table() {
  //PL_DBG << "fwhm changed in table";


  for (auto &q : fit_data_.peaks_)
    q.second.selected = false;
  foreach (QModelIndex i, ui->tableFWHM->selectionModel()->selectedRows()) {
    fit_data_.peaks_[ui->tableFWHM->item(i.row(), 0)->data(Qt::EditRole).toDouble()].selected = true;
  }
 
  toggle_push();
  replot_markers();
  if (isVisible())
    emit peaks_changed(false);
}

void FormFwhmCalibration::toggle_push() {
  int sel = 0;
  for (auto &q : fit_data_.peaks_)
    if (q.second.selected)
      sel++;
  ui->pushRemovePeak->setEnabled(sel > 0);

  ui->pushCullOne->setEnabled(false);
  ui->pushCullUntil->setEnabled(false);
  ui->doubleRsqGoal->setEnabled(false);

  if (fit_data_.fwhm_cali_.valid()) {
    ui->pushCullOne->setEnabled(true);
    ui->pushCullUntil->setEnabled(true);
    ui->doubleRsqGoal->setEnabled(true);
  }

  if (detectors_.has_a(fit_data_.detector_) && (detectors_.get(fit_data_.detector_).fwhm_calibration_.valid()))
    ui->pushFromDB->setEnabled(true);
  else
    ui->pushFromDB->setEnabled(false);

  if (static_cast<int>(fit_data_.peaks_.size()) > 1) {
    ui->spinTerms->setEnabled(true);
    ui->pushFit->setEnabled(true);
  } else {
    ui->pushFit->setEnabled(false);
    ui->spinTerms->setEnabled(false);
  }

  ui->pushApplyCalib->setEnabled(fit_data_.fwhm_cali_ != old_fwhm_calibration_);
}

void FormFwhmCalibration::on_pushFit_clicked()
{  
  fit_calibration();
  toggle_push();
  replot_markers();
}

Polynomial FormFwhmCalibration::fit_calibration()
{
  std::vector<double> xx, yy;
  int i=0;
  for (auto &q : fit_data_.peaks_) {
    xx.push_back(q.second.energy);
    yy.push_back(q.second.fwhm_hyp);
    i++;
  }

  //PL_DBG << "Calibrating " << i << " points";

  Polynomial p = Polynomial(xx, yy, ui->spinTerms->value());

  if (p.coeffs_.size()) {
    //    PL_DBG << "Calibration succeeded";
    fit_data_.fwhm_cali_.type_ = "FWHM";
    fit_data_.fwhm_cali_.bits_ = fit_data_.metadata_.bits;
    fit_data_.fwhm_cali_.coefficients_ = p.coeffs_;
    fit_data_.fwhm_cali_.calib_date_ = boost::posix_time::microsec_clock::local_time();  //spectrum timestamp instead?
    fit_data_.fwhm_cali_.units_ = "keV";
    fit_data_.fwhm_cali_.model_ = Gamma::CalibrationModel::polynomial;
  }
  else
    PL_INFO << "<WFHM calibration> Gamma::Calibration failed";

  return p;
}

void FormFwhmCalibration::on_pushApplyCalib_clicked()
{
  emit update_detector();
}

void FormFwhmCalibration::on_pushFromDB_clicked()
{
  Gamma::Detector newdet = detectors_.get(fit_data_.detector_);
  fit_data_.fwhm_cali_ = newdet.fwhm_calibration_;
  replot_markers();
  toggle_push();
}

void FormFwhmCalibration::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(detectors_, settings_directory_);
  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
  det_widget->exec();
}

double FormFwhmCalibration::find_outlier()
{
  double furthest = -1;
  double furthest_dist = 0;
  for (auto &q : fit_data_.peaks_) {
    double dist = std::abs(q.second.fwhm_hyp - fit_data_.fwhm_cali_.transform(q.second.energy));
    if (dist > furthest_dist) {
      furthest_dist = dist;
      furthest = q.second.center;
    }
  }

  return furthest;
}

void FormFwhmCalibration::on_pushCullOne_clicked()
{
  double furthest = find_outlier();
  if (furthest >= 0) {
    std::set<double> pks;
    pks.insert(furthest);
    fit_data_.remove_peaks(pks);
    update_peaks(true);
    emit peaks_changed(true);
  }
}

void FormFwhmCalibration::on_pushCullUntil_clicked()
{
  Polynomial p = fit_calibration();

  std::set<double> pks;
  while ((fit_data_.peaks_.size() > 0) && (p.rsq < ui->doubleRsqGoal->value())) {
    double furthest = find_outlier();
    if (furthest >= 0) {
      pks.insert(furthest);
      fit_data_.peaks_.erase(furthest);
      p = fit_calibration();
    } else
      break;
  }

  p = fit_calibration();
  fit_data_.remove_peaks(pks);
  update_peaks(true);
  emit peaks_changed(true);
}

void FormFwhmCalibration::on_pushRemovePeak_clicked()
{
  std::set<double> chosen_peaks;
  double last_sel = -1;
  for (auto &q : fit_data_.peaks_)
    if (q.second.selected) {
      chosen_peaks.insert(q.second.center);
      last_sel = q.first;
    }

  fit_data_.remove_peaks(chosen_peaks);

  for (auto &q : fit_data_.peaks_)
    if (q.first > last_sel) {
      q.second.selected = true;
      break;
    }

  update_peaks(true);
  emit peaks_changed(true);
}
