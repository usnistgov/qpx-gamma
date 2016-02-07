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
#include "gamma_fitter.h"
#include "qt_util.h"

FormCoincPeaks::FormCoincPeaks(QWidget *parent) :
  QWidget(parent),
  fit_data_(nullptr),
  ui(new Ui::FormCoincPeaks)
{
  ui->setupUi(this);

  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(4);
  ui->tablePeaks->setHorizontalHeaderLabels({"bin", "energy", "fwhm", "area"});
  ui->tablePeaks->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePeaks->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tablePeaks->setEditTriggers(QTableView::NoEditTriggers);
  ui->tablePeaks->horizontalHeader()->setStretchLastSection(true);
  ui->tablePeaks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tablePeaks->show();
  connect(ui->tablePeaks, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));

  connect(ui->plotPeaks, SIGNAL(range_changed(Range)), this, SLOT(change_range(Range)));
  connect(ui->plotPeaks, SIGNAL(selection_changed(std::set<double>)), this, SLOT(update_selection(std::set<double>)));
  connect(ui->plotPeaks, SIGNAL(data_changed()), this, SLOT(update_data()));

//  QShortcut* shortcut = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tablePeaks);
//  connect(shortcut, SIGNAL(activated()), this, SLOT(remove_peak()));

  //QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), ui->mcaPlot);
  //connect(shortcut, SIGNAL(activated()), this, SLOT(on_pushResetScales_clicked()));
}

FormCoincPeaks::~FormCoincPeaks()
{
  delete ui;
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

void FormCoincPeaks::setFit(Gamma::Fitter* fit) {
  fit_data_ = fit;
  ui->plotPeaks->setFit(fit);
  update_data();
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

void FormCoincPeaks::make_range(Marker marker) {
  ui->plotPeaks->make_range(marker);
}

void FormCoincPeaks::change_range(Range range) {
  emit range_changed(range);
}

void FormCoincPeaks::update_spectrum() {
  ui->plotPeaks->update_spectrum();

  if (!fit_data_->peaks().empty())
    update_data();
  emit peaks_changed(true);
}

void FormCoincPeaks::update_data() {
  ui->plotPeaks->update_spectrum();
  update_table(true);
}

void FormCoincPeaks::update_selection(std::set<double> selected_peaks) {
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (changed) {
    update_table(false);
    emit peaks_changed(false);
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
    if (selected_peaks_.count(q.second.center) > 0) {
      ui->tablePeaks->selectRow(i);
    }
    ++i;
  }

  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}


void FormCoincPeaks::add_peak_to_table(const Gamma::Peak &p, int row, QColor bckg) {
  QBrush background(bckg);

  QTableWidgetItem *center = new QTableWidgetItem(QString::number(p.center));
  center->setData(Qt::EditRole, QVariant::fromValue(p.center));
  center->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 0, center);

//  QTableWidgetItem *edges = new QTableWidgetItem( QString::number(p.x_[p.x_.size() - 1] - p.x_[0]) );
//  edges->setData(Qt::BackgroundRole, background);
//  ui->tablePeaks->setItem(row, 1, edges);

  QTableWidgetItem *nrg = new QTableWidgetItem(QString::number(p.energy));
  nrg->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 1, nrg);

  QTableWidgetItem *fwhm = new QTableWidgetItem(QString::number(p.fwhm_hyp));
  fwhm->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 2, fwhm);

  if (p.area_sum4 != 0) {
    QTableWidgetItem *area_net = new QTableWidgetItem(QString::number(p.area_sum4));
    area_net->setData(Qt::BackgroundRole, background);
    ui->tablePeaks->setItem(row, 3, area_net);

//    QTableWidgetItem *cps_net = new QTableWidgetItem(QString::number(p.cts_per_sec_net_));
//    cps_net->setData(Qt::BackgroundRole, background);
//    ui->tablePeaks->setItem(row, 5, cps_net);
  } else {
    QTableWidgetItem *area_gauss = new QTableWidgetItem(QString::number(p.area_hyp));
    area_gauss->setData(Qt::BackgroundRole, background);
    ui->tablePeaks->setItem(row, 3, area_gauss);

//    QTableWidgetItem *cps_gauss = new QTableWidgetItem(QString::number(p.cts_per_sec_gauss_));
//    cps_gauss->setData(Qt::BackgroundRole, background);
//    ui->tablePeaks->setItem(row, 5, cps_gauss);
  }
}

void FormCoincPeaks::selection_changed_in_table() {
  selected_peaks_.clear();
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows())
    selected_peaks_.insert(ui->tablePeaks->item(i.row(), 0)->data(Qt::EditRole).toDouble());
  ui->plotPeaks->update_selection(selected_peaks_);
  emit peaks_changed(false);
}


