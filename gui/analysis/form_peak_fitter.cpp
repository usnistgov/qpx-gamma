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
 *      FormPeakFitter - 
 *
 ******************************************************************************/

#include "form_peak_fitter.h"
#include "widget_detectors.h"
#include "ui_form_peak_fitter.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormPeakFitter::FormPeakFitter(QSettings &settings, Gamma::Fitter &fit, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormPeakFitter),
  fit_data_(fit),
  settings_(settings)
{
  ui->setupUi(this);

  loadSettings();

  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(10);
  ui->tablePeaks->setHorizontalHeaderLabels({"chan", "energy", "fw(gaus)", "A(gaus)", "cps(gaus)", "fw(S4)", "A(S4)", "%err(S4)", "cps(S4)", "CQI"});
  ui->tablePeaks->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePeaks->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tablePeaks->setEditTriggers(QTableView::NoEditTriggers);
  ui->tablePeaks->horizontalHeader()->setStretchLastSection(true);
  ui->tablePeaks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tablePeaks->show();

  connect(ui->tablePeaks, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));
}

FormPeakFitter::~FormPeakFitter()
{
  delete ui;
}

bool FormPeakFitter::save_close() {
  saveSettings();
  return true;
}

void FormPeakFitter::loadSettings() {
  settings_.beginGroup("Program");
  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();

  settings_.beginGroup("peak_fitter");
  settings_.endGroup();

}

void FormPeakFitter::saveSettings() {

  settings_.beginGroup("peak_fitter");
  settings_.endGroup();
}

void FormPeakFitter::clear() {
  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(0);

  toggle_push();
}

void FormPeakFitter::update_data() {
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);

  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(fit_data_.peaks().size());
  int i=0;
  for (auto &q : fit_data_.peaks()) {
    add_peak_to_table(q.second, i, false);
    ++i;
  }

  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
  
  select_in_table();

  toggle_push();
}

void FormPeakFitter::add_peak_to_table(const Gamma::Peak &p, int row, bool gray) {
  QBrush background(gray ? Qt::lightGray : Qt::white);

  QTableWidgetItem *chn = new QTableWidgetItem(QString::number(p.center));
  chn->setData(Qt::EditRole, QVariant::fromValue(p.center));
  chn->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 0, chn);

  QTableWidgetItem *nrg = new QTableWidgetItem(QString::number(p.energy));
  nrg->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 1, nrg);

  QTableWidgetItem *fwhm = new QTableWidgetItem(QString::number(p.fwhm_hyp));
  fwhm->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 2, fwhm);

  QTableWidgetItem *area_hyp = new QTableWidgetItem(QString::number(p.area_hyp));
  area_hyp->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 3, area_hyp);

  QTableWidgetItem *cps_hyp = new QTableWidgetItem(QString::number(p.cps_hyp));
  cps_hyp->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 4, cps_hyp);

  QTableWidgetItem *fwhm4 = new QTableWidgetItem(QString::number(p.fwhm_sum4));
  fwhm4->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 5, fwhm4);

  QTableWidgetItem *area_net = new QTableWidgetItem(QString::number(p.area_sum4));
  area_net->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 6, area_net);

  QTableWidgetItem *err = new QTableWidgetItem(QString::number(p.sum4_.err));
  err->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 7, err);

  QTableWidgetItem *cps_net = new QTableWidgetItem(QString::number(p.cps_sum4));
  cps_net->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 8, cps_net);

  QTableWidgetItem *CQI = new QTableWidgetItem(QString::number(p.sum4_.currie_quality_indicator));
  CQI->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 9, CQI);

}

void FormPeakFitter::selection_changed_in_table() {
  selected_peaks_.clear();
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows())
    selected_peaks_.insert(ui->tablePeaks->item(i.row(), 0)->data(Qt::EditRole).toDouble());
  if (isVisible())
    emit selection_changed(selected_peaks_);
  toggle_push();
}

void FormPeakFitter::toggle_push() {
  ui->pushSaveReport->setEnabled(!fit_data_.peaks().empty());
}

void FormPeakFitter::update_selection(std::set<double> selected_peaks) {
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (changed)
    select_in_table();
}

void FormPeakFitter::select_in_table() {
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);
  ui->tablePeaks->clearSelection();
  int i = 0;
  for (auto &q : fit_data_.peaks()) {
    if (selected_peaks_.count(q.second.center) > 0) {
      ui->tablePeaks->selectRow(i);
    }
    ++i;
  }
  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}

void FormPeakFitter::on_pushSaveReport_clicked()
{
  emit save_peaks_request();
}
