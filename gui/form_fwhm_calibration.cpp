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

  bits_ = 0;

  style_pts.default_pen = QPen(Qt::darkCyan, 7);
  style_pts.themes["selected"] = QPen(Qt::darkRed, 7);
  style_fit.default_pen = QPen(Qt::blue, 0);

  ui->PlotCalib->setLabels("energy", "FWHM");

  ui->tableFWHM->verticalHeader()->hide();
  ui->tableFWHM->setColumnCount(4);
  ui->tableFWHM->setHorizontalHeaderLabels({"chan", "energy", "fwmw (gaussian)", "fwhm (pseudo-Voigt)"});
  ui->tableFWHM->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableFWHM->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tableFWHM->horizontalHeader()->setStretchLastSection(true);
  ui->tableFWHM->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableFWHM->show();

  connect(ui->tableFWHM, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));
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
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
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
  detector_ = Gamma::Detector();
  bits_ = 0;
  ui->tableFWHM->clearContents();
  ui->tableFWHM->setRowCount(0);
  toggle_push();
  ui->PlotCalib->setFloatingText("");
  ui->pushApplyCalib->setEnabled(false);

  ui->pushFromDB->setEnabled(false);
}

void FormFwhmCalibration::setData(Gamma::Calibration fwhm_calib, uint16_t bits) {
  bits_ = bits;
  new_fwhm_calibration_ = old_fwhm_calibration_ = fwhm_calib;
  if (detectors_.has_a(detector_) && (detectors_.get(detector_).fwhm_calibration_.units_ == "keV"))
    ui->pushFromDB->setEnabled(true);
  else
    ui->pushFromDB->setEnabled(false);
  replot_markers();
}

void FormFwhmCalibration::update_peaks(bool contents_changed) {
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
  ui->tableFWHM->setItem(row, 2, new QTableWidgetItem( QString::number(p.fwhm_gaussian) ));
  ui->tableFWHM->setItem(row, 3, new QTableWidgetItem( QString::number(p.fwhm_pseudovoigt) ));
}

void FormFwhmCalibration::replot_markers() {
  QVector<double> xx, yy;

  double xmin = std::numeric_limits<double>::max();
  double xmax = - std::numeric_limits<double>::max();

  for (auto &q : fit_data_.peaks_) {
    double x = q.second.energy;
    double y = q.second.fwhm_pseudovoigt;

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
    if ((new_fwhm_calibration_.coefficients_ != Gamma::Calibration().coefficients_) && (new_fwhm_calibration_.units_ == "keV")) {

      double step = (xmax-xmin) / 50.0;
      xx.clear(); yy.clear();

      for (double x=xmin; x <= xmax; x+=step) {
        double y = new_fwhm_calibration_.transform(x);
        xx.push_back(x);
        yy.push_back(y);
      }

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

void FormFwhmCalibration::update_peak_selection(std::set<double> pks) {
  for (auto &q : fit_data_.peaks_)
    q.second.selected = (pks.count(q.second.energy) > 0);
  toggle_push();
  replot_markers();
  if (isVisible())
    emit peaks_changed(false);
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
  if (static_cast<int>(fit_data_.peaks_.size()) >= 2) {
    ui->pushFit->setEnabled(true);
  } else {
    ui->pushFit->setEnabled(false);
  }
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
    yy.push_back(q.second.fwhm_pseudovoigt);
    i++;
  }

  //PL_DBG << "Calibrating " << i << " points";

  Polynomial p = Polynomial(xx, yy, ui->spinTerms->value());

  if (p.coeffs_.size()) {
    //    PL_DBG << "Calibration succeeded";
    new_fwhm_calibration_.type_ = "FWHM";
    new_fwhm_calibration_.bits_ = bits_;
    new_fwhm_calibration_.coefficients_ = p.coeffs_;
    new_fwhm_calibration_.calib_date_ = boost::posix_time::microsec_clock::local_time();  //spectrum timestamp instead?
    new_fwhm_calibration_.units_ = "keV";
    new_fwhm_calibration_.model_ = Gamma::CalibrationModel::polynomial;
    ui->PlotCalib->setFloatingText("E = " + QString::fromStdString(p.to_UTF8(true)));
    ui->pushApplyCalib->setEnabled(new_fwhm_calibration_ != old_fwhm_calibration_);
  }
  else
    PL_INFO << "<WFHM calibration> Gamma::Calibration failed";

  return p;
}

void FormFwhmCalibration::on_pushApplyCalib_clicked()
{
  emit update_detector(ui->checkToSpectra->isChecked(), ui->checkToDB->isChecked());
}

void FormFwhmCalibration::on_pushFromDB_clicked()
{
  Gamma::Detector newdet = detectors_.get(detector_);
  new_fwhm_calibration_ = newdet.fwhm_calibration_;
}

void FormFwhmCalibration::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(detectors_, data_directory_);
  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
  det_widget->exec();
}

double FormFwhmCalibration::find_outlier()
{
  double furthest = -1;
  double furthest_dist = 0;
  for (auto &q : fit_data_.peaks_) {
    double dist = std::abs(q.second.fwhm_pseudovoigt - new_fwhm_calibration_.transform(q.second.energy));
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
    fit_data_.peaks_.erase(furthest);
    update_peaks(true);
    emit peaks_changed(true);
  }
}

void FormFwhmCalibration::on_pushCullUntil_clicked()
{
  Polynomial p = fit_calibration();

  while ((fit_data_.peaks_.size() > 0) && (p.rsq < ui->doubleRsqGoal->value())) {
    double furthest = find_outlier();
    if (furthest >= 0) {
      fit_data_.peaks_.erase(furthest);
      p = fit_calibration();
    } else
      break;
  }

  p = fit_calibration();
  update_peaks(true);
  emit peaks_changed(true);
}
