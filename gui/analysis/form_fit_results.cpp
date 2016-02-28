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
 *      FormFitResults -
 *
 ******************************************************************************/

#include "form_fit_results.h"
#include "widget_detectors.h"
#include "ui_form_fit_results.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormFitResults::FormFitResults(QSettings &settings, Qpx::Fitter &fit, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormFitResults),
  fit_data_(fit),
  settings_(settings)
{
  ui->setupUi(this);

  loadSettings();

  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(10);
  ui->tablePeaks->setHorizontalHeaderLabels({"chan", "energy", "fw(hyp)", "A(hyp)", "cps(hyp)", "fw(S4)", "A(S4)", "%err(S4)", "cps(S4)", "CQI"});
  ui->tablePeaks->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePeaks->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tablePeaks->setEditTriggers(QTableView::NoEditTriggers);
  ui->tablePeaks->horizontalHeader()->setStretchLastSection(true);
  ui->tablePeaks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tablePeaks->show();

  connect(ui->tablePeaks, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));
}

FormFitResults::~FormFitResults()
{
  delete ui;
}

bool FormFitResults::save_close() {
  saveSettings();
  return true;
}

void FormFitResults::loadSettings() {
  settings_.beginGroup("Program");
  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();

  settings_.beginGroup("peak_fitter");
  settings_.endGroup();

}

void FormFitResults::saveSettings() {

  settings_.beginGroup("peak_fitter");
  settings_.endGroup();
}

void FormFitResults::clear() {
  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(0);

  toggle_push();
}

void FormFitResults::update_data() {
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

void FormFitResults::add_peak_to_table(const Qpx::Peak &p, int row, bool gray) {
  QBrush background(gray ? Qt::lightGray : Qt::white);

  data_to_table(row, 0, p.center, background);
  data_to_table(row, 1, p.energy, background);
  data_to_table(row, 2, p.fwhm_hyp, background);
  data_to_table(row, 3, p.area_hyp.val, background);
  data_to_table(row, 4, p.cps_hyp, background);
  data_to_table(row, 5, p.fwhm_sum4, background);
  data_to_table(row, 6, p.area_sum4.val, background);
  data_to_table(row, 7, p.sum4_.peak_area.err(), background);
  data_to_table(row, 8, p.cps_sum4, background);
  data_to_table(row, 9, p.sum4_.currie_quality_indicator, background);

}

void FormFitResults::data_to_table(int row, int column, double value, QBrush background) {
  QTableWidgetItem *item = new QTableWidgetItem(QString::number(value));
  item->setData(Qt::EditRole, QVariant::fromValue(value));
  item->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, column, item);
}

void FormFitResults::selection_changed_in_table() {
  selected_peaks_.clear();
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows())
    selected_peaks_.insert(ui->tablePeaks->item(i.row(), 0)->data(Qt::EditRole).toDouble());
  if (isVisible())
    emit selection_changed(selected_peaks_);
  toggle_push();
}

void FormFitResults::toggle_push() {
  ui->pushSaveReport->setEnabled(!fit_data_.peaks().empty());
}

void FormFitResults::update_selection(std::set<double> selected_peaks) {
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (changed)
    select_in_table();
}

void FormFitResults::select_in_table() {
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);
  ui->tablePeaks->clearSelection();

  QItemSelectionModel *selectionModel = ui->tablePeaks->selectionModel();
  QItemSelection itemSelection = selectionModel->selection();

  for (int i=0; i < ui->tablePeaks->rowCount(); ++i)
    if (selected_peaks_.count(ui->tablePeaks->item(i, 0)->data(Qt::EditRole).toDouble())) {
      ui->tablePeaks->selectRow(i);
      itemSelection.merge(selectionModel->selection(), QItemSelectionModel::Select);
    }

  selectionModel->clearSelection();
  selectionModel->select(itemSelection,QItemSelectionModel::Select);

  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}

void FormFitResults::on_pushSaveReport_clicked()
{
  emit save_peaks_request();
}
