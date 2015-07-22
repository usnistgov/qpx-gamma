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
 *      FormEnergyCalibration - 
 *
 ******************************************************************************/

#include "form_energy_calibration.h"
#include "widget_detectors.h"
#include "ui_form_energy_calibration.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormEnergyCalibration::FormEnergyCalibration(QSettings &settings, XMLableDB<Gamma::Detector>& dets, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormEnergyCalibration),
  marker_table_(my_markers_),
  settings_(settings),
  detectors_(dets),
  selection_model_(&marker_table_)
{
  ui->setupUi(this);

  loadSettings();
  bits_ = 0;

  style_fit.default_pen = QPen(Qt::blue, 0);
  style_pts.default_pen = QPen(Qt::darkBlue, 7);
  style_pts.themes["selected"] = QPen(Qt::darkRed, 7);

  ui->PlotCalib->setLabels("channel", "energy");

  ui->tableMarkers->setModel(&marker_table_);
  ui->tableMarkers->setSelectionModel(&selection_model_);
  ui->tableMarkers->setItemDelegate(&special_delegate_);
  ui->tableMarkers->verticalHeader()->hide();
  ui->tableMarkers->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableMarkers->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tableMarkers->horizontalHeader()->setStretchLastSection(true);
  ui->tableMarkers->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableMarkers->show();

  connect(&selection_model_, SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
          this, SLOT(selection_changed_in_table(QItemSelection, QItemSelection)));
  connect(ui->PlotCalib, SIGNAL(selection_changed()), this, SLOT(selection_changed_in_plot()));

  ui->isotopes->show();
  connect(ui->isotopes, SIGNAL(energiesSelected()), this, SLOT(isotope_energies_chosen()));
}

FormEnergyCalibration::~FormEnergyCalibration()
{
  delete ui;
}

bool FormEnergyCalibration::save_close() {
  if (ui->isotopes->save_close()) {
    saveSettings();
    return true;
  }
  else
    return false;
}

void FormEnergyCalibration::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

  ui->isotopes->setDir(data_directory_);

  settings_.beginGroup("Energy_calibration");
  ui->spinTerms->setValue(settings_.value("fit_function_terms", 2).toInt());
  ui->isotopes->set_current_isotope(settings_.value("current_isotope", "Co-60").toString());

  settings_.endGroup();

}

void FormEnergyCalibration::saveSettings() {

  settings_.beginGroup("Energy_calibration");
  settings_.setValue("fit_function_terms", ui->spinTerms->value());
  settings_.setValue("current_isotope", ui->isotopes->current_isotope());
  settings_.endGroup();
}

void FormEnergyCalibration::clear() {
  peaks_.clear();
  my_markers_.clear();
  bits_ = 0;
  new_calibration_ = Gamma::Calibration();
  old_calibration_ = Gamma::Calibration();
  detector_ = Gamma::Detector();
  selection_model_.reset();
  marker_table_.update();
  toggle_push();
  ui->PlotCalib->setFloatingText("");
  ui->pushApplyCalib->setEnabled(false);
  ui->pushFromDB->setEnabled(false);
}

void FormEnergyCalibration::setData(Gamma::Calibration nrg_calib, uint16_t bits) {
  bits_ = bits;
  new_calibration_ = old_calibration_ = nrg_calib;
  PL_DBG << "Energy calibration received new calib coefs = " << new_calibration_.coef_to_string();  
  if (detectors_.has_a(detector_) && detectors_.get(detector_).energy_calibrations_.has_a(old_calibration_))
    ui->pushFromDB->setEnabled(true);
  else
    ui->pushFromDB->setEnabled(false);
  replot_markers();
}

void FormEnergyCalibration::update_peaks(std::vector<Gamma::Peak> pks) {
  peaks_ = pks;
  my_markers_.clear();
  for (auto &q : pks)
    my_markers_[q.center] = q.energy;

  selection_model_.reset();
  marker_table_.update();
  toggle_push();
  replot_markers();
}

void FormEnergyCalibration::replot_markers() {

  QVector<double> xx, yy;

  double xmin = std::numeric_limits<double>::max();
  double xmax = - std::numeric_limits<double>::max();

  if (peaks_.size()) {
    xmax = xmin = peaks_[0].energy;
  }

  for (auto &q : my_markers_) {
    double x = q.first;
    double y = q.second;

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

  if (xx.size()) {
    ui->PlotCalib->addPoints(xx, yy, style_pts);
    if (new_calibration_.units_ != "channels") {

      double step = (xmax-xmin) / 50.0;
      xx.clear(); yy.clear();

      for (double i=xmin; i <= xmax; i+=step) {
        xx.push_back(i);
        yy.push_back(new_calibration_.transform(i));
      }
      ui->PlotCalib->addFit(xx, yy, style_fit);
    }
  }

  std::set<double> chosen_peaks_chan;

  selection_model_.blockSignals(true);
  ui->tableMarkers->blockSignals(true);
  ui->tableMarkers->clearSelection();
  for (int i=0; i < peaks_.size(); ++i) {
    if (peaks_[i].selected) {
      ui->tableMarkers->selectRow(i);
      chosen_peaks_chan.insert(peaks_[i].center);
    }
  }
  ui->PlotCalib->set_selected_pts(chosen_peaks_chan);
  ui->tableMarkers->blockSignals(false);
  selection_model_.blockSignals(false);
}

void FormEnergyCalibration::update_peak_selection(std::set<double> pks) {
  for (int i=0; i < peaks_.size(); ++i)
    peaks_[i].selected = (pks.count(peaks_[i].center) > 0);
  toggle_push();
  replot_markers();
  if (isVisible())
    emit peaks_changed(peaks_, false);
}

void FormEnergyCalibration::selection_changed_in_plot() {
  std::set<double> chosen_peaks_chan = ui->PlotCalib->get_selected_pts();
  for (int i=0; i < peaks_.size(); ++i)
    peaks_[i].selected = (chosen_peaks_chan.count(peaks_[i].center) > 0);
  toggle_push();
  replot_markers();
  if (isVisible())
    emit peaks_changed(peaks_, false);
}

void FormEnergyCalibration::selection_changed_in_table(QItemSelection, QItemSelection) {
  for (auto &q : peaks_)
    q.selected = false;
  foreach (QModelIndex idx, ui->tableMarkers->selectionModel()->selectedRows())
    peaks_[idx.row()].selected = true;
  toggle_push();
  replot_markers();
  if (isVisible())
    emit peaks_changed(peaks_, false);
}

void FormEnergyCalibration::toggle_push() {
  ui->pushAllEnergies->setEnabled(false);
  ui->pushAllmarkers->setEnabled(false);

  if (!selection_model_.selectedIndexes().empty()) {
    if (selection_model_.selectedRows().size() == ui->isotopes->current_gammas().size())
      ui->pushAllEnergies->setEnabled(true);

    if (!ui->isotopes->current_isotope().isEmpty() && (ui->isotopes->current_gammas().empty()))
      ui->pushAllmarkers->setEnabled(true);
  }

  if (static_cast<int>(my_markers_.size()) >= 2) {
    ui->pushFit->setEnabled(true);
  } else {
    ui->pushFit->setEnabled(false);
  }
}

void FormEnergyCalibration::on_pushFit_clicked()
{  
  std::vector<double> x, y, coefs(ui->spinTerms->value()+1);
  x.resize(my_markers_.size());
  y.resize(my_markers_.size());
  int i = 0;
  for (auto &q : my_markers_) {
    x[i] = q.first;
    y[i] = q.second;
    i++;
  }

  Polynomial p = Polynomial(x, y, ui->spinTerms->value());

  if (p.coeffs_.size()) {
    new_calibration_.type_ = "Energy";
    new_calibration_.bits_ = bits_;
    new_calibration_.coefficients_ = p.coeffs_;
    new_calibration_.calib_date_ = boost::posix_time::microsec_clock::local_time();  //spectrum timestamp instead?
    new_calibration_.units_ = "keV";
    new_calibration_.model_ = Gamma::CalibrationModel::polynomial;
    Polynomial thispoly(new_calibration_.coefficients_);
    ui->PlotCalib->setFloatingText("E = " + QString::fromStdString(thispoly.to_UTF8(true)));
    ui->pushApplyCalib->setEnabled(new_calibration_ != old_calibration_);
  }
  else
    PL_INFO << "<Energy calibration> Gamma::Calibration failed";

  selection_model_.reset();
  marker_table_.update();
  toggle_push();
  replot_markers();
}

void FormEnergyCalibration::isotope_energies_chosen() {
  toggle_push();
}

void FormEnergyCalibration::on_pushApplyCalib_clicked()
{
  emit update_detector(ui->checkToSpectra->isChecked(), ui->checkToDB->isChecked());
}

void FormEnergyCalibration::on_pushFromDB_clicked()
{
  Gamma::Detector newdet = detectors_.get(detector_);
  new_calibration_ = newdet.energy_calibrations_.get(old_calibration_);
}

void FormEnergyCalibration::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(detectors_, data_directory_);
  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
  det_widget->exec();
}

void FormEnergyCalibration::on_pushAllmarkers_clicked()
{
  std::vector<double> gammas;
  if (!selection_model_.selectedIndexes().empty()) {
    foreach (QModelIndex idx, selection_model_.selectedRows()) {
      QModelIndex chan_ix = marker_table_.index(idx.row(), 1);
      double nrg = marker_table_.data(chan_ix, Qt::DisplayRole).toDouble();
      gammas.push_back(nrg);
    }
  }
  ui->isotopes->push_energies(gammas);
}

void FormEnergyCalibration::on_pushAllEnergies_clicked()
{
  std::vector<double> gammas = ui->isotopes->current_gammas();

  std::vector<double> to_change;
  if (!selection_model_.selectedIndexes().empty()) {
    foreach (QModelIndex idx, selection_model_.selectedRows()) {
      QModelIndex chan_ix = marker_table_.index(idx.row(), 0);
      double chan = marker_table_.data(chan_ix, Qt::DisplayRole).toDouble();
      to_change.push_back(chan);
    }
  }


  if (gammas.size() != to_change.size())
    return;

  std::sort(gammas.begin(), gammas.end());

  for (int i=0; i<gammas.size(); i++) {
    my_markers_[to_change[i]] = gammas[i];
  }

  marker_table_.update();
  replot_markers();
}

