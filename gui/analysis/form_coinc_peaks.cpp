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

  qRegisterMetaType<Gaussian>("Gaussian");
  qRegisterMetaType<QVector<Gaussian>>("QVector<Gaussian>");

  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(6);
  ui->tablePeaks->setHorizontalHeaderLabels({"chan", "width(ch)", "energy", "fwhm", "area", "cps"});
  ui->tablePeaks->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePeaks->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tablePeaks->setEditTriggers(QTableView::NoEditTriggers);
  ui->tablePeaks->horizontalHeader()->setStretchLastSection(true);
  ui->tablePeaks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tablePeaks->show();
  connect(ui->tablePeaks, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));

  connect(ui->plotPeaks, SIGNAL(peaks_changed(bool)), this, SLOT(peaks_changed_in_plot(bool)));
  connect(ui->plotPeaks, SIGNAL(range_changed(Range)), this, SLOT(change_range(Range)));

  QShortcut* shortcut = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tablePeaks);
  connect(shortcut, SIGNAL(activated()), this, SLOT(remove_peak()));

  //QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), ui->mcaPlot);
  //connect(shortcut, SIGNAL(activated()), this, SLOT(on_pushResetScales_clicked()));
}

FormCoincPeaks::~FormCoincPeaks()
{
  delete ui;
}

void FormCoincPeaks::remove_peak() {
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

void FormCoincPeaks::setFit(Gamma::Fitter* fit) {
  fit_data_ = fit;
  ui->plotPeaks->setFit(fit);
  update_fit(true);
}

void FormCoincPeaks::loadSettings(QSettings &settings_) {
  settings_.beginGroup("CoincPeaks");
  ui->plotPeaks->loadSettings(settings_);
  ui->plotPeaks->set_visible_elements(ShowFitElements::gaussians | ShowFitElements::baselines, false);
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
  ui->plotPeaks->new_spectrum();

  if (!fit_data_->peaks_.empty())
    update_fit(true);
  emit peaks_changed(true);
}

void FormCoincPeaks::update_fit(bool content_changed) {

  ui->plotPeaks->update_fit(content_changed);
  update_table(content_changed);
}

void FormCoincPeaks::peaks_changed_in_plot(bool content_changed) {
  update_table(content_changed);
  emit peaks_changed(content_changed);
}

void FormCoincPeaks::update_table(bool contents_changed) {
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);

  if (contents_changed) {
    ui->tablePeaks->clearContents();
    ui->tablePeaks->setRowCount(fit_data_->peaks_.size());

    bool gray = false;
    QColor background_col;
    int i=0;
    for (auto &q : fit_data_->peaks_) {
      if (q.second.flagged && gray)
        background_col = Qt::darkGreen;
      else if (gray)
        background_col = Qt::lightGray;
      else if (q.second.flagged)
        background_col = Qt::green;
      else
        background_col = Qt::white;
      add_peak_to_table(q.second, i, background_col);
      ++i;
      if (!q.second.intersects_R)
        gray = !gray;
    }
  } else {
    ui->tablePeaks->clearSelection();
    int i = 0;
    for (auto &q : fit_data_->peaks_) {
      if (q.second.selected) {
        ui->tablePeaks->selectRow(i);
      }
      ++i;
    }
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

  QTableWidgetItem *edges = new QTableWidgetItem( QString::number(p.x_[p.x_.size() - 1] - p.x_[0]) );
  edges->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 1, edges);

  QTableWidgetItem *nrg = new QTableWidgetItem(QString::number(p.energy));
  nrg->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 2, nrg);

  QTableWidgetItem *fwhm = new QTableWidgetItem(QString::number(p.fwhm_gaussian));
  fwhm->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 3, fwhm);

  if (p.area_net_ != 0) {
    QTableWidgetItem *area_net = new QTableWidgetItem(QString::number(p.area_net_));
    area_net->setData(Qt::BackgroundRole, background);
    ui->tablePeaks->setItem(row, 4, area_net);

    QTableWidgetItem *cps_net = new QTableWidgetItem(QString::number(p.cts_per_sec_net_));
    cps_net->setData(Qt::BackgroundRole, background);
    ui->tablePeaks->setItem(row, 5, cps_net);
  } else {
    QTableWidgetItem *area_gauss = new QTableWidgetItem(QString::number(p.area_gauss_));
    area_gauss->setData(Qt::BackgroundRole, background);
    ui->tablePeaks->setItem(row, 4, area_gauss);

    QTableWidgetItem *cps_gauss = new QTableWidgetItem(QString::number(p.cts_per_sec_gauss_));
    cps_gauss->setData(Qt::BackgroundRole, background);
    ui->tablePeaks->setItem(row, 5, cps_gauss);
  }
}

void FormCoincPeaks::selection_changed_in_table() {
  for (auto &q : fit_data_->peaks_)
    q.second.selected = false;
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows()) {
    fit_data_->peaks_[ui->tablePeaks->item(i.row(), 0)->data(Qt::EditRole).toDouble()].selected = true;
  }
  ui->plotPeaks->update_fit(false);
  emit peaks_changed(false);
}


