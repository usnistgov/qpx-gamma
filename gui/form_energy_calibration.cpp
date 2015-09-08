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

FormEnergyCalibration::FormEnergyCalibration(QSettings &settings, XMLableDB<Gamma::Detector>& dets, Gamma::Fitter &fit, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormEnergyCalibration),
  settings_(settings),
  detectors_(dets),
  fit_data_(fit)
{
  ui->setupUi(this);

  loadSettings();

  style_fit.default_pen = QPen(Qt::blue, 0);
  style_pts.default_pen = QPen(Qt::darkBlue, 7);
  style_pts.themes["selected"] = QPen(Qt::cyan, 7);

  ui->PlotCalib->setLabels("channel", "energy");


  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(2);
  ui->tablePeaks->setHorizontalHeaderLabels({"chan", "energy"});
  ui->tablePeaks->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePeaks->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tablePeaks->horizontalHeader()->setStretchLastSection(true);
  ui->tablePeaks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tablePeaks->show();
  connect(ui->tablePeaks, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));

  connect(ui->PlotCalib, SIGNAL(selection_changed()), this, SLOT(selection_changed_in_plot()));

  ui->isotopes->show();
  connect(ui->isotopes, SIGNAL(energiesSelected()), this, SLOT(isotope_energies_chosen()));

  QShortcut* shortcut = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tablePeaks);
  connect(shortcut, SIGNAL(activated()), this, SLOT(on_pushMarkerRemove_clicked()));
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
  old_calibration_ = Gamma::Calibration();
  replot_markers();
  toggle_push();
  ui->PlotCalib->setFloatingText("");
  ui->PlotCalib->clearGraphs();
  ui->pushApplyCalib->setEnabled(false);
  ui->pushFromDB->setEnabled(false);
}

void FormEnergyCalibration::newSpectrum() {
  old_calibration_ = fit_data_.nrg_cali_;
  replot_markers();
  toggle_push();
}

void FormEnergyCalibration::update_peaks(bool contents_changed) {
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);

  if (contents_changed) {
    ui->tablePeaks->clearContents();
    ui->tablePeaks->setRowCount(fit_data_.peaks_.size());
    bool gray = false;
    QColor background_col;
    int i=0;
    for (auto &q : fit_data_.peaks_) {
      if (gray)
        background_col = Qt::lightGray;
      else
        background_col = Qt::white;
      add_peak_to_table(q.second, i, background_col);
      ++i;
      if (!q.second.intersects_R)
        gray = !gray;
    }
  }
  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);

  replot_markers();
  toggle_push();
}

void FormEnergyCalibration::replot_markers() {
  ui->PlotCalib->setFloatingText("");

  QVector<double> xx, yy;

  double xmin = std::numeric_limits<double>::max();
  double xmax = - std::numeric_limits<double>::max();
  std::set<double> chosen_peaks_chan;

  for (auto &q : fit_data_.peaks_) {
    double x = q.first;
    double y = q.second.energy;

    if (q.second.selected)
      chosen_peaks_chan.insert(q.second.center);

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
    ui->PlotCalib->set_selected_pts(chosen_peaks_chan);
    if (fit_data_.nrg_cali_.units_ != "channels") {

      double step = (xmax-xmin) / 50.0;
      xx.clear(); yy.clear();

      for (double i=xmin; i < xmax; i+=step) {
        xx.push_back(i);
        yy.push_back(fit_data_.nrg_cali_.transform(i));
      }
      ui->PlotCalib->addFit(xx, yy, style_fit);
      ui->PlotCalib->setFloatingText("E = " + QString::fromStdString(fit_data_.nrg_cali_.fancy_equation(3)));
    }
  }


  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);
  ui->tablePeaks->clearSelection();
  int i = 0;
  for (auto &q : fit_data_.peaks_) {
    if (q.second.selected) {
      ui->tablePeaks->selectRow(i);
    }
    ++i;
  }
  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}

void FormEnergyCalibration::selection_changed_in_plot() {
  std::set<double> chosen_peaks_chan = ui->PlotCalib->get_selected_pts();
  for (auto &q : fit_data_.peaks_)
    q.second.selected = (chosen_peaks_chan.count(q.second.center) > 0);
  replot_markers();
  if (isVisible())
    emit peaks_changed(false);
  toggle_push();
}

void FormEnergyCalibration::selection_changed_in_table() {
  for (auto &q : fit_data_.peaks_)
    q.second.selected = false;
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows()) {
    fit_data_.peaks_[ui->tablePeaks->item(i.row(), 0)->data(Qt::EditRole).toDouble()].selected = true;
  }
  replot_markers();

  if (isVisible())
    emit peaks_changed(false);
  toggle_push();
}

void FormEnergyCalibration::toggle_push() {
  int sel = 0;
  for (auto &q : fit_data_.peaks_)
    if (q.second.selected)
      sel++;

  ui->pushMarkerRemove->setEnabled(sel > 0);
  ui->pushAllEnergies->setEnabled((sel > 0) && (sel == ui->isotopes->current_gammas().size()));
  ui->pushAllmarkers->setEnabled((sel > 0) && (ui->isotopes->current_gammas().empty()));

  if (static_cast<int>(fit_data_.peaks_.size()) > 1) {
    ui->pushFit->setEnabled(true);
    ui->spinTerms->setEnabled(true);
  } else {
    ui->pushFit->setEnabled(false);
    ui->spinTerms->setEnabled(false);
  }

  if (detectors_.has_a(fit_data_.detector_) && detectors_.get(fit_data_.detector_).energy_calibrations_.has_a(old_calibration_))
    ui->pushFromDB->setEnabled(true);
  else
    ui->pushFromDB->setEnabled(false);

  ui->pushApplyCalib->setEnabled(fit_data_.nrg_cali_ != old_calibration_);
}

void FormEnergyCalibration::on_pushFit_clicked()
{  
  std::vector<double> x, y, coefs(ui->spinTerms->value()+1);
  x.resize(fit_data_.peaks_.size());
  y.resize(fit_data_.peaks_.size());
  int i = 0;
  for (auto &q : fit_data_.peaks_) {
    x[i] = q.first;
    y[i] = q.second.energy;
    i++;
  }

  Polynomial p = Polynomial(x, y, ui->spinTerms->value());

  if (p.coeffs_.size()) {
    fit_data_.nrg_cali_.type_ = "Energy";
    fit_data_.nrg_cali_.bits_ = fit_data_.metadata_.bits;
    fit_data_.nrg_cali_.coefficients_ = p.coeffs_;
    fit_data_.nrg_cali_.calib_date_ = boost::posix_time::microsec_clock::local_time();  //spectrum timestamp instead?
    fit_data_.nrg_cali_.units_ = "keV";
    fit_data_.nrg_cali_.model_ = Gamma::CalibrationModel::polynomial;
  }
  else
    PL_INFO << "<Energy calibration> Gamma::Calibration failed";

  replot_markers();
  toggle_push();
}

void FormEnergyCalibration::isotope_energies_chosen() {
  toggle_push();
}

void FormEnergyCalibration::on_pushApplyCalib_clicked()
{
  emit update_detector();
}

void FormEnergyCalibration::on_pushFromDB_clicked()
{
  Gamma::Detector newdet = detectors_.get(fit_data_.detector_);
  fit_data_.nrg_cali_ = newdet.energy_calibrations_.get(old_calibration_);
  replot_markers();
  toggle_push();
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
  for (auto &q : fit_data_.peaks_)
    if (q.second.selected)
      gammas.push_back(q.second.energy);
  ui->isotopes->push_energies(gammas);
}

void FormEnergyCalibration::on_pushAllEnergies_clicked()
{
  std::vector<double> gammas = ui->isotopes->current_gammas();

  std::vector<double> to_change;
  double last_sel = -1;
  for (auto &q : fit_data_.peaks_)
    if (q.second.selected) {
      to_change.push_back(q.first);
      last_sel = q.first;
      q.second.selected = false;
    }

  if (gammas.size() != to_change.size())
    return;

  std::sort(gammas.begin(), gammas.end());

  for (int i=0; i<gammas.size(); i++) {
    fit_data_.peaks_[to_change[i]].energy = gammas[i];
  }

  for (auto &q : fit_data_.peaks_)
    if (q.first > last_sel) {
      q.second.selected = true;
      break;
    }

  ui->isotopes->select_next_energy();

  update_peaks(true);
  emit peaks_changed(true);
}


void FormEnergyCalibration::on_pushMarkerRemove_clicked()
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

void FormEnergyCalibration::add_peak_to_table(const Gamma::Peak &p, int row, QColor bckg) {
  QBrush background(bckg);

  QTableWidgetItem *center = new QTableWidgetItem(QString::number(p.center));
  center->setData(Qt::EditRole, QVariant::fromValue(p.center));
  center->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 0, center);

  QTableWidgetItem *nrg = new QTableWidgetItem( QString::number(p.energy) );
  nrg->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 1, nrg);
}
