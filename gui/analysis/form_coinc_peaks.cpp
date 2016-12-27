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
 *      FormCoincPeaks -
 *
 ******************************************************************************/

#include "form_coinc_peaks.h"
#include "widget_detectors.h"
#include "ui_form_coinc_peaks.h"
#include "fitter.h"
#include "qt_util.h"

FormCoincPeaks::FormCoincPeaks(QWidget *parent) :
  QWidget(parent),
  fit_data_(nullptr),
  ui(new Ui::FormCoincPeaks)
{
  ui->setupUi(this);

  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(3);
  ui->tablePeaks->setHorizontalHeaderLabels({"energy", "fwhm", "cps"});
  ui->tablePeaks->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePeaks->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tablePeaks->setEditTriggers(QTableView::NoEditTriggers);
  ui->tablePeaks->horizontalHeader()->setStretchLastSection(true);
  ui->tablePeaks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tablePeaks->show();
  connect(ui->tablePeaks, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));

//  connect(ui->plotPeaks, SIGNAL(range_changed(Range)), this, SLOT(change_range(Range)));
  connect(ui->plotPeaks, SIGNAL(peak_selection_changed(std::set<double>)), this, SLOT(update_selection(std::set<double>)));
  connect(ui->plotPeaks, SIGNAL(data_changed()), this, SLOT(update_peaks()));
  connect(ui->plotPeaks, SIGNAL(fitting_done()), this, SLOT(fitting_finished()));

//  QShortcut* shortcut = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tablePeaks);
//  connect(shortcut, SIGNAL(activated()), this, SLOT(remove_peak()));

  //QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), ui->mcaPlot);
  //connect(shortcut, SIGNAL(activated()), this, SLOT(on_pushResetScales_clicked()));
}

FormCoincPeaks::~FormCoincPeaks()
{
  delete ui;
}

void FormCoincPeaks::perform_fit()
{
  ui->plotPeaks->perform_fit();
}

//void FormCoincPeaks::remove_peak() {
//  if (!fit_data_)
//    return;

//  std::set<double> chosen_peaks;
//  double last_sel = -1;
//  for (auto &q : fit_data_->peaks())
//    if (q.second.selected) {
//      chosen_peaks.insert(q.second.center);
//      last_sel = q.first;
//    }

//  fit_data_->remove_peaks(chosen_peaks);

//  for (auto &q : fit_data_->peaks())
//    if (q.first > last_sel) {
//      q.second.selected = true;
//      break;
//    }

//  update_fit(true);
//  emit peaks_changed(true);
//}

void FormCoincPeaks::setFit(Qpx::Fitter* fit) {
  fit_data_ = fit;
  ui->plotPeaks->setFit(fit);
  ui->plotPeaks->update_spectrum();
}

void FormCoincPeaks::fitting_finished()
{
  emit fitting_done();
}

std::set<double> FormCoincPeaks::get_selected_peaks() {
  return ui->plotPeaks->get_selected_peaks();
}

void FormCoincPeaks::loadSettings(QSettings &settings_) {
  settings_.beginGroup("CoincPeaks");
  ui->plotPeaks->loadSettings(settings_);
  settings_.endGroup();
}

void FormCoincPeaks::saveSettings(QSettings &settings_) {
  settings_.beginGroup("CoincPeaks");
  ui->plotPeaks->saveSettings(settings_);
  settings_.endGroup();
}

void FormCoincPeaks::make_range(Coord marker) {
  ui->plotPeaks->make_range(marker);
}

//void FormCoincPeaks::change_range(RangeSelector range) {
//  emit range_changed(range);
//}

void FormCoincPeaks::update_spectrum() {
  ui->plotPeaks->update_spectrum();

  if (!fit_data_->peaks().empty())
    update_table(true);
}

void FormCoincPeaks::update_peaks() {
  update_table(true);
  emit peaks_changed();
}

void FormCoincPeaks::update_selection(std::set<double> selected_peaks) {
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (changed) {
    update_table(false);
    ui->plotPeaks->set_selected_peaks(selected_peaks_);
    emit peak_selection_changed(selected_peaks);
  }
}

void FormCoincPeaks::update_table(bool contents_changed) {
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);

  if (contents_changed) {
    ui->tablePeaks->clearContents();
    ui->tablePeaks->setRowCount(fit_data_->peaks().size());

    int i=0;
    for (auto &q : fit_data_->peaks()) {
      add_peak_to_table(q.second, i, Qt::white);
      ++i;
    }
  }

  ui->tablePeaks->clearSelection();
  int i = 0;
  for (auto &q : fit_data_->peaks()) {
    if (selected_peaks_.count(q.second.center().value()) > 0) {
      ui->tablePeaks->selectRow(i);
    }
    ++i;
  }

  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}


void FormCoincPeaks::add_peak_to_table(const Qpx::Peak &p, int row, QColor bckg) {
  QBrush background(bckg);

  add_to_table(ui->tablePeaks, row, 0, p.energy().to_string(),
               QVariant::fromValue(p.center().value()), background);
  add_to_table(ui->tablePeaks, row, 1, p.fwhm().to_string(), QVariant(), background);
  add_to_table(ui->tablePeaks, row, 2, p.cps_best().to_string(), QVariant(), background);
}

void FormCoincPeaks::selection_changed_in_table() {
  selected_peaks_.clear();
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows())
    selected_peaks_.insert(ui->tablePeaks->item(i.row(), 0)->data(Qt::EditRole).toDouble());
  ui->plotPeaks->set_selected_peaks(selected_peaks_);
  emit peak_selection_changed(selected_peaks_);
}


