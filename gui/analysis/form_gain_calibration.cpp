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

  ui->plotCalib->setAxisLabels("channel (" + QString::fromStdString(detector2_.name_) + ")", "channel (" + QString::fromStdString(detector1_.name_) + ")");

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
  ui->plotCalib->setAxisLabels("", "");
  ui->plotCalib->setTitle("");

  gain_match_cali_ = Qpx::Calibration("Gain", 0);
}


void FormGainCalibration::update_peaks() {
  plot_calib();
}

void FormGainCalibration::plot_calib()
{
  ui->plotCalib->clearGraphs();

  QVector<double> xx, yy;
  QVector<double> xx_sigma, yy_sigma;
  double xmin = std::numeric_limits<double>::max();
  double xmax = - std::numeric_limits<double>::max();
  for (auto &q: fit_data1_.peaks())
  {
    double nrg = q.second.energy().value();
    int multiple = 0;
    for (auto &p : fit_data1_.peaks())
      if (std::abs(p.second.energy().value() - nrg) < ui->doubleCullDelta->value())
      {
        xx.push_back(q.second.center().value());
        yy.push_back(p.second.center().value());
        xx_sigma.push_back(q.second.center().uncertainty());
        yy_sigma.push_back(p.second.center().uncertainty());
        xmin = std::min(xmin, q.second.center().value());
        xmax = std::max(xmax, q.second.center().value());
        multiple++;
      }
    if (multiple > 1)
      DBG << "Multiple peaks match c1=" << nrg;
  }

  ui->pushSaveCalib->setEnabled(gain_match_cali_.valid());
  ui->pushSymmetrize->setEnabled(gain_match_cali_.valid());

  bool good = (xx.size() && (xx.size() == yy.size()));

  double x_margin = (xmax - xmin) / 10;
  xmax += x_margin;
  xmin -= x_margin;

  ui->pushCalibGain->setEnabled(good);
  ui->spinPolyOrder->setEnabled(good);
  if (good)
  {
    ui->plotCalib->addPoints(style_pts, xx, yy, xx_sigma, yy_sigma);
    if ((gain_match_cali_.to_ == detector1_.name_) && gain_match_cali_.valid())
    {
      double step = (xmax-xmin) / 50.0;
      xx.clear(); yy.clear();

      for (double i=xmin; i < xmax; i+=step)
      {
        xx.push_back(i);
        yy.push_back(gain_match_cali_.transform(i));
      }
      ui->plotCalib->setFit(xx, yy, style_fit);
      ui->plotCalib->setTitle("E = " + QString::fromStdString(gain_match_cali_.fancy_equation(6, true)));
    }
  }
  ui->plotCalib->replotAll();


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
  std::vector<double> xx, yy;
  std::vector<double> xx_sigma, yy_sigma;
  for (auto &q: fit_data1_.peaks())
  {
    double nrg = q.second.energy().value();
    int multiples = 0;
    for (auto &p : fit_data1_.peaks())
      if (std::abs(p.second.energy().value() - nrg) < ui->doubleCullDelta->value())
      {
        xx.push_back(q.second.center().value());
        xx_sigma.push_back(q.second.center().uncertainty());
        yy.push_back(p.second.center().value());
        yy_sigma.push_back(p.second.center().uncertainty());
        multiples++;
      }
    if (multiples > 1)
      WARN << "Multiple peaks match c1=" << nrg;
  }

  bool good = (xx.size() && (xx.size() == yy.size()));

  if (!good)
  {
    DBG << "size mismatch";
    return;
  }

  PolyBounded p;
  p.add_coeff(0, -50, 50, 0);
  p.add_coeff(1,   0, 50, 1);
  for (int i=2; i <= ui->spinPolyOrder->value(); ++i)
    p.add_coeff(i, -5, 5, 0);

  DBG << "points " << xx.size() << " coefs " << p.coeffs().size();

//  yy_sigma.resize(yy.size(), 1);
  p.fit(xx, yy, xx_sigma, yy_sigma);

  if (p.coeffs_.size())
  {
    //    gain_match_cali_.type_ = ;
    gain_match_cali_.coefficients_ = p.coeffs();
    gain_match_cali_.to_ = detector1_.name_;
    gain_match_cali_.bits_ = fit_data2_.settings().bits_;
    gain_match_cali_.calib_date_ = boost::posix_time::microsec_clock::universal_time();  //spectrum timestamp instead?
    gain_match_cali_.model_ = Qpx::CalibrationModel::polynomial;
    ui->plotCalib->setTitle(QString::fromStdString(p.to_UTF8(3, true)));
    fit_data2_.detector_.gain_match_calibrations_.replace(gain_match_cali_);
  }
  else
    WARN << "<Gain match calibration> Calibration failed";

  plot_calib();
}

void FormGainCalibration::on_pushSymmetrize_clicked()
{
  emit symmetrize_requested();
}

void FormGainCalibration::on_doubleCullDelta_editingFinished()
{
  plot_calib();
}
