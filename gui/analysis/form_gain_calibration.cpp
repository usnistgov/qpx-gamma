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
#include "fitter.h"
#include "qt_util.h"
#include <QSettings>


FormGainCalibration::FormGainCalibration(XMLableDB<Qpx::Detector>& dets, Qpx::Fitter& fit1, Qpx::Fitter& fit2, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormGainCalibration),
  fit_data1_(fit1),
  fit_data2_(fit2),
  detectors_(dets)
{
  ui->setupUi(this);

  loadSettings();

  QColor point_color;
  point_color.setHsv(180, 215, 150, 120);
  style_pts.default_pen = QPen(point_color, 9);
  QColor selected_color;
  selected_color.setHsv(225, 255, 230, 210);
  style_pts.themes["selected"] = QPen(selected_color, 9);
  style_fit.default_pen = QPen(Qt::darkCyan, 2);

//  connect(ui->plotCalib, SIGNAL(selection_changed()), this, SLOT(selection_changed_in_plot()));

  //QShortcut* shortcut = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tableFWHM);
  //connect(shortcut, SIGNAL(activated()), this, SLOT(on_pushRemovePeak_clicked()));
}

void FormGainCalibration::newSpectrum() {
  int bits = fit_data1_.settings().bits_;

  detector1_ = fit_data1_.detector_;
  detector2_ = fit_data2_.detector_;

  if (detector1_.energy_calibrations_.has_a(Qpx::Calibration("Energy", bits)))
    nrg_calibration1_ = detector1_.energy_calibrations_.get(Qpx::Calibration("Energy", bits));

  if (detector2_.energy_calibrations_.has_a(Qpx::Calibration("Energy", bits)))
    nrg_calibration2_ = detector2_.energy_calibrations_.get(Qpx::Calibration("Energy", bits));

  bool symmetrized = ((detector1_ != Qpx::Detector()) && (detector2_ != Qpx::Detector()) && (detector1_ == detector2_));

  ui->plotCalib->setLabels("channel (" + QString::fromStdString(detector2_.name_) + ")", "channel (" + QString::fromStdString(detector1_.name_) + ")");

  gain_match_cali_ = detector2_.get_gain_match(bits, detector1_.name_);

  plot_calib();
}


FormGainCalibration::~FormGainCalibration()
{
  delete ui;
}

void FormGainCalibration::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();


  settings_.beginGroup("Gain_calibration");
  ui->spinPolyOrder->setValue(settings_.value("fit_function_terms", 2).toInt());
  ui->doubleCullDelta->setValue(settings_.value("r_squared_goal", 3.00).toDouble());

  settings_.endGroup();

}

void FormGainCalibration::saveSettings() {
  QSettings settings_;

  settings_.beginGroup("Gain_calibration");
  settings_.setValue("fit_function_terms", ui->spinPolyOrder->value());
  settings_.setValue("r_squared_goal", ui->doubleCullDelta->value());
  settings_.endGroup();
}

void FormGainCalibration::clear() {
  ui->plotCalib->clearGraphs();
  ui->plotCalib->setLabels("", "");
  ui->plotCalib->setFloatingText("");

  gain_match_cali_ = Qpx::Calibration("Gain", 0);
}


void FormGainCalibration::update_peaks() {
  plot_calib();
}

void FormGainCalibration::plot_calib() {
  ui->plotCalib->clearGraphs();

  ui->doubleCullDelta->setEnabled(false);

  size_t total = fit_data1_.peaks().size();
  if (total != fit_data2_.peaks().size())
  {
    if (nrg_calibration1_.units_ == nrg_calibration2_.units_) {
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

  for (auto &q : fit_data2_.peaks()) {
    xx.push_back(q.first);
    if (q.first < xmin)
      xmin = q.first;
    if (q.first > xmax)
      xmax = q.first;
  }

  for (auto &q : fit_data1_.peaks())
    yy.push_back(q.first);

  double x_margin = (xmax - xmin) / 10;
  xmax += x_margin;
  xmin -= x_margin;

  ui->pushCalibGain->setEnabled(false);
  ui->spinPolyOrder->setEnabled(false);
  if (xx.size()) {
    ui->pushCalibGain->setEnabled(true);
    ui->spinPolyOrder->setEnabled(true);


    ui->plotCalib->addPoints(style_pts, xx, yy, QVector<double>(), QVector<double>());
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
  ui->plotCalib->redraw();
}

void FormGainCalibration::selection_changed_in_plot() {
  std::set<double> selected_peaks = ui->plotCalib->get_selected_pts();
  plot_calib();
  if (isVisible())
    emit selection_changed(selected_peaks);
}

void FormGainCalibration::on_pushSaveCalib_clicked()
{
  emit update_detector();
}

void FormGainCalibration::on_pushCalibGain_clicked()
{
  size_t total = fit_data1_.peaks().size();
  if (total != fit_data2_.peaks().size())
  {
    //make invisible?
    return;
  }

  std::vector<double> x, y;
  for (auto &q : fit_data2_.peaks())
    x.push_back(q.first);
  for (auto &q : fit_data1_.peaks())
    y.push_back(q.first);


  std::vector<double> sigmas(y.size(), 1);

  PolyBounded p;
  p.add_coeff(0, -50, 50, 1);
  for (int i=1; i <= ui->spinPolyOrder->value(); ++i)
    p.add_coeff(i, -5, 5, 1);

  p.fit_fityk(x,y,sigmas);

  if (p.coeffs_.size()) {
    gain_match_cali_.coefficients_ = p.coeffs();
    gain_match_cali_.to_ = detector1_.name_;
    gain_match_cali_.bits_ = fit_data2_.settings().bits_;
    gain_match_cali_.calib_date_ = boost::posix_time::microsec_clock::universal_time();  //spectrum timestamp instead?
    gain_match_cali_.model_ = Qpx::CalibrationModel::polynomial;
    ui->plotCalib->setFloatingText(QString::fromStdString(p.to_UTF8(3, true)));
    fit_data2_.detector_.gain_match_calibrations_.replace(gain_match_cali_);
  }
  else
    LINFO << "<Gain match calibration> Calibration failed";

  plot_calib();
}

//void FormGainCalibration::on_pushCull_clicked()
//{
//  //while (cull_mismatch()) {}

//  std::set<double> to_remove;

//  for (auto &q: fit_data1_.peaks()) {
//    double nrg = q.second.energy;
//    bool has = false;
//    for (auto &p : fit_data2_.peaks())
//      if (std::abs(p.second.energy - nrg) < ui->doubleCullDelta->value())
//        has = true;
//    if (!has)
//      to_remove.insert(q.first);
//  }
//  for (auto &q : to_remove)
//    fit_data1_.remove_peaks(to_remove);

//  for (auto &q: fit_data2_.peaks()) {
//    double nrg = q.second.energy;
//    bool has = false;
//    for (auto &p : fit_data1_.peaks())
//      if (std::abs(p.second.energy - nrg) < ui->doubleCullDelta->value())
//        has = true;
//    if (!has)
//      to_remove.insert(q.first);
//  }
//  for (auto &q : to_remove)
//    fit_data2_.remove_peaks(to_remove);

//  plot_calib();
//  emit peaks_changed(true);
//}

void FormGainCalibration::on_pushSymmetrize_clicked()
{
  emit symmetrize_requested();
}
