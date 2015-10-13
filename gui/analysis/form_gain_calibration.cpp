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
 *      FormGainCalibration -
 *
 ******************************************************************************/

#include "form_gain_calibration.h"
#include "widget_detectors.h"
#include "ui_form_gain_calibration.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormGainCalibration::FormGainCalibration(QSettings &settings, XMLableDB<Gamma::Detector>& dets, Gamma::Fitter& fit1, Gamma::Fitter& fit2, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormGainCalibration),
  settings_(settings),
  fit_data1_(fit1),
  fit_data2_(fit2),
  detectors_(dets)
{
  ui->setupUi(this);

  loadSettings();

  style_pts.default_pen = QPen(Qt::darkBlue, 7);
  style_pts.themes["selected"] = QPen(Qt::cyan, 7);
  style_fit.default_pen = QPen(Qt::blue, 0);

  connect(ui->plotCalib, SIGNAL(selection_changed()), this, SLOT(selection_changed_in_plot()));

  //QShortcut* shortcut = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tableFWHM);
  //connect(shortcut, SIGNAL(activated()), this, SLOT(on_pushRemovePeak_clicked()));
}

void FormGainCalibration::newSpectrum() {
  int bits = fit_data1_.metadata_.bits;

  detector1_ = fit_data1_.detector_;
  detector2_ = fit_data2_.detector_;

  if (detector1_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits)))
    nrg_calibration1_ = detector1_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));

  if (detector2_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits)))
    nrg_calibration2_ = detector2_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));

  bool symmetrized = ((detector1_ != Gamma::Detector()) && (detector2_ != Gamma::Detector()) && (detector1_ == detector2_));

  ui->plotCalib->setLabels("channel (" + QString::fromStdString(detector2_.name_) + ")", "channel (" + QString::fromStdString(detector1_.name_) + ")");

  gain_match_cali_ = detector2_.get_gain_match(bits, detector1_.name_);

  plot_calib();
}


FormGainCalibration::~FormGainCalibration()
{
  delete ui;
}

void FormGainCalibration::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();


  settings_.beginGroup("Gain_calibration");
  ui->spinPolyOrder->setValue(settings_.value("fit_function_terms", 2).toInt());
  ui->doubleCullDelta->setValue(settings_.value("r_squared_goal", 3.00).toDouble());

  settings_.endGroup();

}

void FormGainCalibration::saveSettings() {

  settings_.beginGroup("Gain_calibration");
  settings_.setValue("fit_function_terms", ui->spinPolyOrder->value());
  settings_.setValue("r_squared_goal", ui->doubleCullDelta->value());
  settings_.endGroup();
}

void FormGainCalibration::clear() {
  ui->plotCalib->clearGraphs();
  ui->plotCalib->setLabels("", "");
  ui->plotCalib->setFloatingText("");

  gain_match_cali_ = Gamma::Calibration("Gain", 0);
}


void FormGainCalibration::update_peaks() {
  plot_calib();
}

void FormGainCalibration::plot_calib() {
  ui->plotCalib->clearGraphs();

  ui->pushCull->setEnabled(false);
  ui->doubleCullDelta->setEnabled(false);

  int total = fit_data1_.peaks_.size();
  if (total != fit_data2_.peaks_.size())
  {
    if (nrg_calibration1_.units_ == nrg_calibration2_.units_) {
      ui->pushCull->setEnabled(true);
      ui->doubleCullDelta->setEnabled(true);
    }
    //make invisible?
    return;
  }

  ui->pushSaveCalib->setEnabled(gain_match_cali_.valid());
  ui->pushSymmetrize->setEnabled(gain_match_cali_.valid());

  QVector<double> xx, yy;

  double xmin = std::numeric_limits<double>::max();
  double xmax = - std::numeric_limits<double>::max();

  for (auto &q : fit_data2_.peaks_) {
    xx.push_back(q.first);
    if (q.first < xmin)
      xmin = q.first;
    if (q.first > xmax)
      xmax = q.first;
  }

  for (auto &q : fit_data1_.peaks_)
    yy.push_back(q.first);

  double x_margin = (xmax - xmin) / 10;
  xmax += x_margin;
  xmin -= x_margin;

  ui->pushCalibGain->setEnabled(false);
  ui->spinPolyOrder->setEnabled(false);
  if (xx.size()) {
    ui->pushCalibGain->setEnabled(true);
    ui->spinPolyOrder->setEnabled(true);

    ui->plotCalib->addPoints(xx, yy, style_pts);
    if ((gain_match_cali_.to_ == detector1_.name_) && gain_match_cali_.valid()) {

      double step = (xmax-xmin) / 50.0;
      xx.clear(); yy.clear();

      for (double i=xmin; i < xmax; i+=step) {
        xx.push_back(i);
        yy.push_back(gain_match_cali_.transform(i));
      }
      ui->plotCalib->addFit(xx, yy, style_fit);
    }
  }
}

void FormGainCalibration::selection_changed_in_plot() {
  std::set<double> chosen_peaks_nrg = ui->plotCalib->get_selected_pts();
  for (auto &q : fit_data1_.peaks_)
    q.second.selected = (chosen_peaks_nrg.count(q.second.energy) > 0);
  plot_calib();
  if (isVisible())
    emit peaks_changed(false);
}

void FormGainCalibration::on_pushSaveCalib_clicked()
{
  emit update_detector();
}

void FormGainCalibration::on_pushRemovePeak_clicked()
{
  std::set<double> chosen_peaks;
  double last_sel = -1;
  for (auto &q : fit_data1_.peaks_)
    if (q.second.selected) {
      chosen_peaks.insert(q.second.center);
      last_sel = q.first;
    }

  fit_data1_.remove_peaks(chosen_peaks);

  for (auto &q : fit_data1_.peaks_)
    if (q.first > last_sel) {
      q.second.selected = true;
      break;
    }

  update_peaks();
  emit peaks_changed(true);
}

void FormGainCalibration::on_pushCalibGain_clicked()
{
  int total = fit_data1_.peaks_.size();
  if (total != fit_data2_.peaks_.size())
  {
    //make invisible?
    return;
  }

  std::vector<double> x, y;
  for (auto &q : fit_data2_.peaks_)
    x.push_back(q.first);
  for (auto &q : fit_data1_.peaks_)
    y.push_back(q.first);

  Polynomial p = Polynomial(x, y, ui->spinPolyOrder->value());

  if (p.coeffs_.size()) {
    gain_match_cali_.coefficients_ = p.coeffs_;
    gain_match_cali_.to_ = detector1_.name_;
    gain_match_cali_.bits_ = fit_data2_.metadata_.bits;
    gain_match_cali_.calib_date_ = boost::posix_time::microsec_clock::local_time();  //spectrum timestamp instead?
    gain_match_cali_.model_ = Gamma::CalibrationModel::polynomial;
    ui->plotCalib->setFloatingText(QString::fromStdString(p.to_UTF8(3, true)));
    fit_data2_.detector_.gain_match_calibrations_.replace(gain_match_cali_);
  }
  else
    PL_INFO << "<Gain match calibration> Calibration failed";

  plot_calib();
}

void FormGainCalibration::on_pushCull_clicked()
{
  //while (cull_mismatch()) {}

  std::set<double> to_remove;

  for (auto &q: fit_data1_.peaks_) {
    double nrg = q.second.energy;
    bool has = false;
    for (auto &p : fit_data2_.peaks_)
      if (std::abs(p.second.energy - nrg) < ui->doubleCullDelta->value())
        has = true;
    if (!has)
      to_remove.insert(q.first);
  }
  for (auto &q : to_remove)
    fit_data1_.remove_peaks(to_remove);

  for (auto &q: fit_data2_.peaks_) {
    double nrg = q.second.energy;
    bool has = false;
    for (auto &p : fit_data1_.peaks_)
      if (std::abs(p.second.energy - nrg) < ui->doubleCullDelta->value())
        has = true;
    if (!has)
      to_remove.insert(q.first);
  }
  for (auto &q : to_remove)
    fit_data2_.remove_peaks(to_remove);

  plot_calib();
  emit peaks_changed(true);
}

void FormGainCalibration::on_pushSymmetrize_clicked()
{
  emit symmetrize_requested();
}
