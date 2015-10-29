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
 *      FormOnePeak -
 *
 ******************************************************************************/

#include "form_one_peak.h"
#include "ui_form_one_peak.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormOnePeak::FormOnePeak(QWidget *parent) :
  QWidget(parent),
  fit_data_(nullptr),
  ui(new Ui::FormOnePeak)
{
  ui->setupUi(this);

  qRegisterMetaType<Gaussian>("Gaussian");
  qRegisterMetaType<QVector<Gaussian>>("QVector<Gaussian>");

  connect(ui->plotPeaks, SIGNAL(peaks_changed(bool)), this, SLOT(peaks_changed_in_plot(bool)));
  connect(ui->plotPeaks, SIGNAL(range_changed(Range)), this, SLOT(change_range(Range)));

  ui->plotPeaks->set_visible_elements(ShowFitElements::gaussians | ShowFitElements::baselines | ShowFitElements::movavg | ShowFitElements::filtered, false);

  //QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), ui->mcaPlot);
  //connect(shortcut, SIGNAL(activated()), this, SLOT(on_pushResetScales_clicked()));
}

FormOnePeak::~FormOnePeak()
{
  delete ui;
}

void FormOnePeak::remove_peak() {
  if (!fit_data_)
    return;

  std::set<double> chosen_peaks;
  double last_sel = -1;
  for (auto &q : fit_data_->peaks_)
    if (q.second.selected) {
      chosen_peaks.insert(q.second.center);
      last_sel = q.first;
    }

  fit_data_->remove_peaks(chosen_peaks);

  for (auto &q : fit_data_->peaks_)
    if (q.first > last_sel) {
      q.second.selected = true;
      break;
    }

  update_fit(true);
  emit peaks_changed(true);
}

void FormOnePeak::setFit(Gamma::Fitter* fit) {
  fit_data_ = fit;
  ui->plotPeaks->setFit(fit);
  update_fit(true);
}

void FormOnePeak::loadSettings(QSettings &settings, QString name) {
  settings.beginGroup(name);
  ui->plotPeaks->loadSettings(settings);
  ui->plotPeaks->set_visible_elements(ShowFitElements::gaussians | ShowFitElements::baselines | ShowFitElements::edges
                                      /*| ShowFitElements::movavg*/ | ShowFitElements::filtered, false);
  settings.endGroup();
}

void FormOnePeak::saveSettings(QSettings &settings, QString name) {
  settings.beginGroup(name);
  ui->plotPeaks->saveSettings(settings);
  settings.endGroup();
}

void FormOnePeak::make_range(Marker marker) {
  ui->plotPeaks->make_range(marker);
}

void FormOnePeak::change_range(Range range) {
  emit range_changed(range);
}

void FormOnePeak::update_spectrum() {
  ui->plotPeaks->new_spectrum(" ");
  ui->plotPeaks->tighten();

  if (!fit_data_->peaks_.empty())
    update_fit(true);
  emit peaks_changed(true);
}

void FormOnePeak::update_fit(bool content_changed) {
  peak_info();

  ui->plotPeaks->update_fit(content_changed);
}

void FormOnePeak::peaks_changed_in_plot(bool content_changed) {
  if (!fit_data_->peaks_.empty()) {
    bool selected = false;
    double highest = fit_data_->peaks_.begin()->first;
//    PL_DBG << "will check " << fit_data_->peaks_.size() << " peaks";
    for (auto &q : fit_data_->peaks_) {
      if (q.second.selected)
        selected = true;
      if (q.second.height >= fit_data_->peaks_[highest].height)
        highest = q.first;
    }
    if (!selected) {
      fit_data_->peaks_[highest].selected = true;
      ui->plotPeaks->update_fit(false);
    }
  }

  peak_info();
  emit peaks_changed(content_changed);
}

void FormOnePeak::peak_info() {
  ui->textCentroid->clear();
  ui->textBackground->clear();
  ui->textBackgroundPerBin->clear();

  bool success = false;
//  PL_DBG << "info will check " << fit_data_->peaks_.size() << " peaks";
  for (auto &q : fit_data_->peaks_)
    if (q.second.selected) {
      ui->textCentroid->setText(
            QString::number(q.second.center) +
            " (" + QString::number(q.second.energy) + " " +
            QString::fromStdString(fit_data_->nrg_cali_.units_) + ")");

      ui->textBackground->setText(
            QString::number(q.second.sum4_.B_area) +
            " +/- " + QString::number(sqrt(q.second.sum4_.B_variance)));

      ui->textBackgroundPerBin->setText(
            QString::number(q.second.sum4_.B_area / q.second.sum4_.peak_width) +
            " +/- " + QString::number(sqrt(q.second.sum4_.B_variance) / q.second.sum4_.peak_width));
      success = true;
      break;
    }
  emit peak_available();
}



